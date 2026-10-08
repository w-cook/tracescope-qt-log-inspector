#include "RotatedSourceConfigurationDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <utility>

#include "../preferences/RotatedSourceSettingsStore.h"

#include "InterfaceScale.h"
#include "ItemViewFocusDelegate.h"

namespace
{
bool rotatedSourceRulesEqual(
    const RotatedSourceRule &left,
    const RotatedSourceRule &right
    )
{
    return left.namingScheme
               == right.namingScheme
           && left.numericOrderDirection
                  == right.numericOrderDirection
           && left.customRegularExpression
                  == right.customRegularExpression
           && left.customOrderCaptureGroup
                  == right.customOrderCaptureGroup
           && left.customOrderValueType
                  == right.customOrderValueType
           && left.customOrderDirection
                  == right.customOrderDirection
           && left.customDateTimeFormat
                  == right.customDateTimeFormat;
}
}

RotatedSourceConfigurationDialog::
    RotatedSourceConfigurationDialog(
        const QString &activeSourcePath,
        RotatedSourceRule initialRule,
        bool includeRotatedSources,
        RotatedSourceSettingsStore
            *rotatedSourceSettingsStore,
        QWidget *parent
        )
    : QDialog(parent),
    m_activeSourcePath(
        QFileInfo(
            activeSourcePath
            ).absoluteFilePath()
        ),
    m_workingRule(
        std::move(initialRule)
        ),
    m_savedCustomRuleComboBox(
        new QComboBox(this)
        ),
    m_saveCustomRuleButton(
        new QPushButton(
            tr("Save As..."),
            this
            )
        ),
    m_deleteCustomRuleButton(
        new QPushButton(
            tr("Delete"),
            this
            )
        ),
    m_rotatedSourceSettingsStore(
        rotatedSourceSettingsStore
        ),
    m_namingSchemeComboBox(
        new QComboBox(this)
        ),
    m_numericOptionsWidget(
        new QWidget(this)
        ),
    m_numericOrderDirectionComboBox(
        new QComboBox(this)
        ),
    m_summaryLabel(
        new QLabel(this)
        ),
    m_sourceTable(
        new QTableWidget(
            0,
            2,
            this
            )
        ),
    m_customOptionsWidget(
        new QWidget(this)
        ),
    m_customRegexEdit(
        new QLineEdit(this)
        ),
    m_captureGroupSpinBox(
        new QSpinBox(this)
        ),
    m_orderValueTypeComboBox(
        new QComboBox(this)
        ),
    m_orderDirectionComboBox(
        new QComboBox(this)
        ),
    m_dateTimeFormatLabel(
        new QLabel(
            tr("Date/time format:"),
            this
            )
        ),
    m_dateTimeFormatEdit(
        new QLineEdit(this)
        ),
    m_includeRotatedSourcesCheckBox(
        new QCheckBox(
            tr("Include rotated source files"),
            this
            )
        ),
    m_buttonBox(
        new QDialogButtonBox(
            QDialogButtonBox::Cancel,
            this
            )
        ),
    m_useSourceFamilyButton(
        new QPushButton(
            tr("Use This Source Family"),
            this
            )
        )
{
    setWindowTitle(
        tr("Rotated Source Files")
        );

    resize(
        InterfaceScale::size(
            700,
            480,
            this
            )
        );

    buildLayout();
    populateControls();

    m_includeRotatedSourcesCheckBox
        ->setChecked(
            includeRotatedSources
            );

    updateCustomControlVisibility();
    refreshDiscovery();

    connect(
        m_namingSchemeComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateCustomControlVisibility();
            refreshDiscovery();
        }
        );

    connect(
        m_numericOrderDirectionComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            refreshDiscovery();
        }
        );

    connect(
        m_savedCustomRuleComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            applySelectedSavedCustomRule();
        }
        );

    connect(
        m_saveCustomRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            saveCustomRule();
        }
        );

    connect(
        m_deleteCustomRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            deleteSelectedCustomRule();
        }
        );

    connect(
        m_customRegexEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateSavedCustomRuleSelection();
            refreshDiscovery();
        }
        );

    connect(
        m_captureGroupSpinBox,
        &QSpinBox::valueChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateSavedCustomRuleSelection();
            refreshDiscovery();
        }
        );

    connect(
        m_orderValueTypeComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateCustomControlVisibility();
            updateSavedCustomRuleSelection();
            refreshDiscovery();
        }
        );

    connect(
        m_orderDirectionComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateSavedCustomRuleSelection();
            refreshDiscovery();
        }
        );

    connect(
        m_dateTimeFormatEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            updateWorkingRuleFromControls();
            updateSavedCustomRuleSelection();
            refreshDiscovery();
        }
        );

    connect(
        m_useSourceFamilyButton,
        &QPushButton::clicked,
        this,
        &QDialog::accept
        );

    connect(
        m_buttonBox,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
        );
}

RotatedSourceRule
    RotatedSourceConfigurationDialog::
    configuredRule() const
{
    return m_workingRule;
}

const QVector<RotatedSourceMatch> &
    RotatedSourceConfigurationDialog::
    rotatedSources() const
{
    return m_rotatedSources;
}

bool RotatedSourceConfigurationDialog::
    includeRotatedSources() const
{
    return m_includeRotatedSourcesCheckBox
        ->isChecked();
}

void RotatedSourceConfigurationDialog::
    buildLayout()
{
    auto *mainLayout =
        new QVBoxLayout(this);

    auto *introLabel =
        new QLabel(
            tr(
                "Choose how TraceScope identifies "
                "physical files that belong to the "
                "same logical log source."
                ),
            this
            );

    introLabel->setWordWrap(true);

    mainLayout->addWidget(
        introLabel
        );

    auto *schemeLayout =
        new QFormLayout();

    schemeLayout->addRow(
        tr("Naming scheme:"),
        m_namingSchemeComboBox
        );

    mainLayout->addLayout(
        schemeLayout
        );

    auto *numericOptionsLayout =
        new QFormLayout(
            m_numericOptionsWidget
            );

    numericOptionsLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    numericOptionsLayout->addRow(
        tr("Numeric chronology:"),
        m_numericOrderDirectionComboBox
        );

    mainLayout->addWidget(
        m_numericOptionsWidget
        );

    auto *customGroup =
        new QGroupBox(
            tr("Custom Pattern"),
            m_customOptionsWidget
            );

    auto *customLayout =
        new QFormLayout(
            customGroup
            );

    auto *savedRuleRow =
        new QHBoxLayout();

    savedRuleRow->addWidget(
        m_savedCustomRuleComboBox,
        1
        );

    savedRuleRow->addWidget(
        m_saveCustomRuleButton
        );

    savedRuleRow->addWidget(
        m_deleteCustomRuleButton
        );

    customLayout->addRow(
        tr("Saved scheme:"),
        savedRuleRow
        );

    m_customRegexEdit->setPlaceholderText(
        tr(
            "^service\\.archive\\.(\\d+)\\.log$"
            )
        );

    customLayout->addRow(
        tr("Regular expression:"),
        m_customRegexEdit
        );

    m_captureGroupSpinBox->setMinimum(1);
    m_captureGroupSpinBox->setMaximum(99);

    customLayout->addRow(
        tr("Ordering capture group:"),
        m_captureGroupSpinBox
        );

    customLayout->addRow(
        tr("Ordering value:"),
        m_orderValueTypeComboBox
        );

    customLayout->addRow(
        tr("Direction:"),
        m_orderDirectionComboBox
        );

    m_dateTimeFormatEdit->setPlaceholderText(
        tr(
            "Example: yyyyMMdd-HHmmss"
            )
        );

    customLayout->addRow(
        m_dateTimeFormatLabel,
        m_dateTimeFormatEdit
        );

    auto *customOptionsLayout =
        new QVBoxLayout(
            m_customOptionsWidget
            );

    customOptionsLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    customOptionsLayout->addWidget(
        customGroup
        );

    mainLayout->addWidget(
        m_customOptionsWidget
        );

    m_summaryLabel->setWordWrap(true);

    mainLayout->addWidget(
        m_summaryLabel
        );

    m_sourceTable->setHorizontalHeaderLabels(
        {
            tr("File"),
            tr("Role")
        }
        );

    m_sourceTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_sourceTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_sourceTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_sourceTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );

    m_sourceTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
            );

    m_sourceTable->setItemDelegate(
        new ItemViewFocusDelegate(
            m_sourceTable
            )
        );

    mainLayout->addWidget(
        m_sourceTable,
        1
        );

    mainLayout->addWidget(
        m_includeRotatedSourcesCheckBox
        );

    m_buttonBox->addButton(
        m_useSourceFamilyButton,
        QDialogButtonBox::AcceptRole
        );

    mainLayout->addWidget(
        m_buttonBox
        );
}

void RotatedSourceConfigurationDialog::
    populateControls()
{
    m_namingSchemeComboBox->addItem(
        tr("None"),
        static_cast<int>(
            RotatedSourceNamingScheme::Disabled
            )
        );

    m_namingSchemeComboBox->addItem(
        tr("Numeric suffix (.1, .2, ...)"),
        static_cast<int>(
            RotatedSourceNamingScheme::
            NumericSuffix
            )
        );

    m_namingSchemeComboBox->addItem(
        tr("Numeric before extension (.1.log, .2.log, ...)"),
        static_cast<int>(
            RotatedSourceNamingScheme::
            NumericBeforeExtension
            )
        );

    m_namingSchemeComboBox->addItem(
        tr("Date / timestamp before extension"),
        static_cast<int>(
            RotatedSourceNamingScheme::
            IsoDateTimeBeforeExtension
            )
        );

    m_namingSchemeComboBox->addItem(
        tr("Custom pattern"),
        static_cast<int>(
            RotatedSourceNamingScheme::
            CustomRegex
            )
        );

    const int schemeIndex =
        m_namingSchemeComboBox->findData(
            static_cast<int>(
                m_workingRule.namingScheme
                )
            );

    if (schemeIndex >= 0) {
        m_namingSchemeComboBox
            ->setCurrentIndex(
                schemeIndex
                );
    }

    m_numericOrderDirectionComboBox->addItem(
        tr(
            "Higher numbers are older "
            "(.3 → .2 → .1 → active)"
            ),
        static_cast<int>(
            RotatedSourceOrderDirection::
            Descending
            )
        );

    m_numericOrderDirectionComboBox->addItem(
        tr(
            "Lower numbers are older "
            "(.1 → .2 → .3 → active)"
            ),
        static_cast<int>(
            RotatedSourceOrderDirection::
            Ascending
            )
        );

    const int numericDirectionIndex =
        m_numericOrderDirectionComboBox->findData(
            static_cast<int>(
                m_workingRule
                    .numericOrderDirection
                )
            );

    if (numericDirectionIndex >= 0) {
        m_numericOrderDirectionComboBox
            ->setCurrentIndex(
                numericDirectionIndex
                );
    }

    m_orderValueTypeComboBox->addItem(
        tr("Number"),
        static_cast<int>(
            RotatedSourceOrderValueType::
            Numeric
            )
        );

    m_orderValueTypeComboBox->addItem(
        tr("Text"),
        static_cast<int>(
            RotatedSourceOrderValueType::
            Lexicographic
            )
        );

    m_orderValueTypeComboBox->addItem(
        tr("Date / time"),
        static_cast<int>(
            RotatedSourceOrderValueType::
            DateTime
            )
        );

    const int valueTypeIndex =
        m_orderValueTypeComboBox->findData(
            static_cast<int>(
                m_workingRule
                    .customOrderValueType
                )
            );

    if (valueTypeIndex >= 0) {
        m_orderValueTypeComboBox
            ->setCurrentIndex(
                valueTypeIndex
                );
    }

    m_orderDirectionComboBox->addItem(
        tr("Oldest / lowest first"),
        static_cast<int>(
            RotatedSourceOrderDirection::
            Ascending
            )
        );

    m_orderDirectionComboBox->addItem(
        tr("Highest first"),
        static_cast<int>(
            RotatedSourceOrderDirection::
            Descending
            )
        );

    const int directionIndex =
        m_orderDirectionComboBox->findData(
            static_cast<int>(
                m_workingRule
                    .customOrderDirection
                )
            );

    if (directionIndex >= 0) {
        m_orderDirectionComboBox
            ->setCurrentIndex(
                directionIndex
                );
    }

    m_customRegexEdit->setText(
        m_workingRule
            .customRegularExpression
        );

    m_captureGroupSpinBox->setValue(
        m_workingRule
            .customOrderCaptureGroup
        );

    m_dateTimeFormatEdit->setText(
        m_workingRule
            .customDateTimeFormat
        );

    populateSavedCustomRules();
}

void RotatedSourceConfigurationDialog::
    updateWorkingRuleFromControls()
{
    m_workingRule.namingScheme =
        static_cast<
            RotatedSourceNamingScheme
            >(
            m_namingSchemeComboBox
                ->currentData()
                .toInt()
            );

    m_workingRule.customRegularExpression =
        m_customRegexEdit->text();

    m_workingRule.customOrderCaptureGroup =
        m_captureGroupSpinBox->value();

    m_workingRule.customOrderValueType =
        static_cast<
            RotatedSourceOrderValueType
            >(
            m_orderValueTypeComboBox
                ->currentData()
                .toInt()
            );

    m_workingRule.customOrderDirection =
        static_cast<
            RotatedSourceOrderDirection
            >(
            m_orderDirectionComboBox
                ->currentData()
                .toInt()
            );

    m_workingRule.customDateTimeFormat =
        m_dateTimeFormatEdit->text();

    m_workingRule.numericOrderDirection =
        static_cast<
            RotatedSourceOrderDirection
            >(
            m_numericOrderDirectionComboBox
                ->currentData()
                .toInt()
            );
}

void RotatedSourceConfigurationDialog::
    updateCustomControlVisibility()
{
    const bool custom =
        m_workingRule.namingScheme
        == RotatedSourceNamingScheme::
        CustomRegex;

    m_customOptionsWidget->setVisible(
        custom
        );

    const bool numeric =
        m_workingRule.namingScheme
            == RotatedSourceNamingScheme::
            NumericSuffix
        || m_workingRule.namingScheme
               == RotatedSourceNamingScheme::
               NumericBeforeExtension;

    m_numericOptionsWidget->setVisible(
        numeric
        );

    const bool dateTimeOrdering =
        custom
        && m_workingRule
                   .customOrderValueType
               == RotatedSourceOrderValueType::
               DateTime;

    m_dateTimeFormatLabel->setVisible(
        dateTimeOrdering
        );

    m_dateTimeFormatEdit->setVisible(
        dateTimeOrdering
        );

    m_savedCustomRuleComboBox
        ->setEnabled(
            custom
            && m_rotatedSourceSettingsStore
                   != nullptr
            );

    m_saveCustomRuleButton
        ->setEnabled(
            custom
            && m_rotatedSourceSettingsStore
                   != nullptr
            );

    m_deleteCustomRuleButton
        ->setEnabled(
            custom
            && m_rotatedSourceSettingsStore
                   != nullptr
            && m_savedCustomRuleComboBox
                       ->currentIndex()
                   > 0
            );
}

void RotatedSourceConfigurationDialog::
    refreshDiscovery()
{
    m_rotatedSources.clear();

    if (m_workingRule.namingScheme
        == RotatedSourceNamingScheme::Disabled) {
        m_summaryLabel->setText(
            tr(
                "No rotated-source naming rule "
                "is selected."
                )
            );

        m_includeRotatedSourcesCheckBox
            ->setChecked(false);

        m_includeRotatedSourcesCheckBox
            ->setEnabled(false);

        m_useSourceFamilyButton
            ->setEnabled(true);

        refreshSourceTable();
        return;
    }

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            m_activeSourcePath,
            m_workingRule
            );

    if (!result.succeeded) {
        m_summaryLabel->setText(
            tr(
                "Rotation rule error: %1"
                )
                .arg(
                    result.errorMessage
                    )
            );

        m_includeRotatedSourcesCheckBox
            ->setChecked(false);

        m_includeRotatedSourcesCheckBox
            ->setEnabled(false);

        m_useSourceFamilyButton
            ->setEnabled(false);

        refreshSourceTable();
        return;
    }

    m_rotatedSources =
        result.rotatedSources;

    const int count =
        m_rotatedSources.size();

    if (count == 0) {
        m_summaryLabel->setText(
            tr(
                "No rotated files match the "
                "selected naming rule."
                )
            );

        m_includeRotatedSourcesCheckBox
            ->setChecked(false);

        m_includeRotatedSourcesCheckBox
            ->setEnabled(false);
    } else {
        m_summaryLabel->setText(
            tr(
                "%1 rotated %2 detected. "
                "%3 physical %4 will form one "
                "investigation when rotation "
                "inclusion is enabled."
                )
                .arg(count)
                .arg(
                    count == 1
                        ? tr("file")
                        : tr("files")
                    )
                .arg(
                    count + 1
                    )
                .arg(
                    count + 1 == 1
                        ? tr("file")
                        : tr("files")
                    )
            );

        m_includeRotatedSourcesCheckBox
            ->setEnabled(true);
    }

    m_useSourceFamilyButton
        ->setEnabled(true);

    refreshSourceTable();
}

void RotatedSourceConfigurationDialog::
    refreshSourceTable()
{
    m_sourceTable->setRowCount(
        m_rotatedSources.size() + 1
        );

    int row = 0;

    for (const RotatedSourceMatch &source
         : std::as_const(
             m_rotatedSources
             )) {
        m_sourceTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                source.fileName
                )
            );

        m_sourceTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                tr("Rotated")
                )
            );

        ++row;
    }

    m_sourceTable->setItem(
        row,
        0,
        new QTableWidgetItem(
            QFileInfo(
                m_activeSourcePath
                ).fileName()
            )
        );

    m_sourceTable->setItem(
        row,
        1,
        new QTableWidgetItem(
            tr("Active source")
            )
        );
}

void RotatedSourceConfigurationDialog::
    populateSavedCustomRules()
{
    const QSignalBlocker blocker(
        m_savedCustomRuleComboBox
        );

    m_savedCustomRuleComboBox->clear();

    m_savedCustomRuleComboBox->addItem(
        tr("Unsaved custom rule"),
        QString()
        );

    int matchingIndex = 0;

    if (m_rotatedSourceSettingsStore
        != nullptr) {
        const auto savedRules =
            m_rotatedSourceSettingsStore
                ->savedCustomRules();

        for (const SavedRotatedSourceRule
                 &savedRule
             : savedRules) {
            m_savedCustomRuleComboBox
                ->addItem(
                    savedRule.name,
                    savedRule.name
                    );

            if (rotatedSourceRulesEqual(
                    savedRule.rule,
                    m_workingRule
                    )) {
                matchingIndex =
                    m_savedCustomRuleComboBox
                        ->count()
                    - 1;
            }
        }
    }

    m_savedCustomRuleComboBox
        ->setCurrentIndex(
            matchingIndex
            );

    m_deleteCustomRuleButton
        ->setEnabled(
            matchingIndex > 0
            );

    const bool storeAvailable =
        m_rotatedSourceSettingsStore
        != nullptr;

    m_savedCustomRuleComboBox
        ->setEnabled(
            storeAvailable
            );

    m_saveCustomRuleButton
        ->setEnabled(
            storeAvailable
            );
}

void RotatedSourceConfigurationDialog::
    applySelectedSavedCustomRule()
{
    if (m_rotatedSourceSettingsStore
        == nullptr) {
        return;
    }

    const QString name =
        m_savedCustomRuleComboBox
            ->currentData()
            .toString();

    m_deleteCustomRuleButton
        ->setEnabled(
            !name.isEmpty()
            );

    if (name.isEmpty()) {
        return;
    }

    const auto savedRules =
        m_rotatedSourceSettingsStore
            ->savedCustomRules();

    for (const SavedRotatedSourceRule
             &savedRule
         : savedRules) {
        if (savedRule.name.compare(
                name,
                Qt::CaseInsensitive
                )
            != 0) {
            continue;
        }

        m_workingRule =
            savedRule.rule;

        const QSignalBlocker
            regexBlocker(
                m_customRegexEdit
                );

        const QSignalBlocker
            captureBlocker(
                m_captureGroupSpinBox
                );

        const QSignalBlocker
            valueTypeBlocker(
                m_orderValueTypeComboBox
                );

        const QSignalBlocker
            directionBlocker(
                m_orderDirectionComboBox
                );

        const QSignalBlocker
            dateBlocker(
                m_dateTimeFormatEdit
                );

        m_customRegexEdit->setText(
            m_workingRule
                .customRegularExpression
            );

        m_captureGroupSpinBox->setValue(
            m_workingRule
                .customOrderCaptureGroup
            );

        const int valueTypeIndex =
            m_orderValueTypeComboBox
                ->findData(
                    static_cast<int>(
                        m_workingRule
                            .customOrderValueType
                        )
                    );

        if (valueTypeIndex >= 0) {
            m_orderValueTypeComboBox
                ->setCurrentIndex(
                    valueTypeIndex
                    );
        }

        const int directionIndex =
            m_orderDirectionComboBox
                ->findData(
                    static_cast<int>(
                        m_workingRule
                            .customOrderDirection
                        )
                    );

        if (directionIndex >= 0) {
            m_orderDirectionComboBox
                ->setCurrentIndex(
                    directionIndex
                    );
        }

        m_dateTimeFormatEdit->setText(
            m_workingRule
                .customDateTimeFormat
            );

        updateCustomControlVisibility();
        refreshDiscovery();

        return;
    }
}

void RotatedSourceConfigurationDialog::
    updateSavedCustomRuleSelection()
{
    if (m_rotatedSourceSettingsStore
        == nullptr) {
        return;
    }

    const auto savedRules =
        m_rotatedSourceSettingsStore
            ->savedCustomRules();

    int matchingIndex = 0;

    for (const SavedRotatedSourceRule
             &savedRule
         : savedRules) {
        if (!rotatedSourceRulesEqual(
                savedRule.rule,
                m_workingRule
                )) {
            continue;
        }

        matchingIndex =
            m_savedCustomRuleComboBox
                ->findData(
                    savedRule.name
                    );

        break;
    }

    const QSignalBlocker blocker(
        m_savedCustomRuleComboBox
        );

    m_savedCustomRuleComboBox
        ->setCurrentIndex(
            std::max(
                0,
                matchingIndex
                )
            );

    m_deleteCustomRuleButton
        ->setEnabled(
            matchingIndex > 0
            );
}

void RotatedSourceConfigurationDialog::
    saveCustomRule()
{
    if (m_rotatedSourceSettingsStore
            == nullptr
        || m_workingRule.namingScheme
               != RotatedSourceNamingScheme::
               CustomRegex) {
        return;
    }

    updateWorkingRuleFromControls();

    const auto discovery =
        RotatedSourceDiscoveryService::
        discover(
            m_activeSourcePath,
            m_workingRule
            );

    if (!discovery.succeeded) {
        QMessageBox::warning(
            this,
            tr("Invalid Rotation Rule"),
            discovery.errorMessage
            );

        return;
    }

    bool accepted = false;

    const QString name =
        QInputDialog::getText(
            this,
            tr("Save Rotation Scheme"),
            tr("Scheme name:"),
            QLineEdit::Normal,
            QString(),
            &accepted
            )
            .trimmed();

    if (!accepted
        || name.isEmpty()) {
        return;
    }

    const auto existingRules =
        m_rotatedSourceSettingsStore
            ->savedCustomRules();

    for (const auto &existing
         : existingRules) {
        if (existing.name.compare(
                name,
                Qt::CaseInsensitive
                )
            != 0) {
            continue;
        }

        const auto answer =
            QMessageBox::question(
                this,
                tr("Replace Saved Scheme"),
                tr(
                    "A rotation scheme named "
                    "\"%1\" already exists. "
                    "Replace it?"
                    )
                    .arg(name)
                );

        if (answer
            != QMessageBox::Yes) {
            return;
        }

        break;
    }

    SavedRotatedSourceRule savedRule;

    savedRule.name =
        name;

    savedRule.rule =
        m_workingRule;

    if (!m_rotatedSourceSettingsStore
             ->saveCustomRule(
                 std::move(
                     savedRule
                     )
                 )) {
        QMessageBox::warning(
            this,
            tr("Save Rotation Scheme"),
            tr(
                "The rotation scheme could "
                "not be saved."
                )
            );

        return;
    }

    populateSavedCustomRules();

    const int savedIndex =
        m_savedCustomRuleComboBox
            ->findData(name);

    if (savedIndex >= 0) {
        m_savedCustomRuleComboBox
            ->setCurrentIndex(
                savedIndex
                );
    }
}

void RotatedSourceConfigurationDialog::
    deleteSelectedCustomRule()
{
    if (m_rotatedSourceSettingsStore
        == nullptr) {
        return;
    }

    const QString name =
        m_savedCustomRuleComboBox
            ->currentData()
            .toString();

    if (name.isEmpty()) {
        return;
    }

    const auto answer =
        QMessageBox::question(
            this,
            tr("Delete Rotation Scheme"),
            tr(
                "Delete the saved rotation "
                "scheme \"%1\"?"
                )
                .arg(name)
            );

    if (answer
        != QMessageBox::Yes) {
        return;
    }

    if (!m_rotatedSourceSettingsStore
             ->removeCustomRule(
                 name
                 )) {
        return;
    }

    /*
     * Removing a reusable preset does not discard
     * the currently configured rule. It simply
     * becomes an unsaved custom configuration.
     */
    populateSavedCustomRules();
}