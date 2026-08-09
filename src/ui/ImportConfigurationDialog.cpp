#include "ImportConfigurationDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSignalBlocker>
#include <QCheckBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QComboBox>
#include <QScrollArea>
#include <utility>

ImportConfigurationDialog::ImportConfigurationDialog(QWidget *parent)
    : QDialog(parent),
    filePathEdit(new QLineEdit(this)),
    browseButton(
        new QPushButton(
            tr("Browse..."),
            this
            )
        ),
    formatSuggestionLabel(
        new QLabel(this)
        ),
    profileNameEdit(
        new QLineEdit(this)
        ),
    preserveUnmappedCheckBox(
        new QCheckBox(
            tr("Preserve unmapped source fields"),
            this
            )
        ),
    timestampPathEdit(
        new QLineEdit(this)
        ),
    severityPathEdit(
        new QLineEdit(this)
        ),
    subsystemPathEdit(
        new QLineEdit(this)
        ),
    eventCodePathEdit(
        new QLineEdit(this)
        ),
    entityIdPathEdit(
        new QLineEdit(this)
        ),
    messagePathEdit(
        new QLineEdit(this)
        ),
    customFieldTable(
        new QTableWidget(
            0,
            2,
            this
            )
        ),
    addCustomFieldButton(
        new QPushButton(
            tr("Add Mapping"),
            this
            )
        ),
    removeCustomFieldButton(
        new QPushButton(
            tr("Remove Selected"),
            this
            )
        ),
    severityAliasTable(
        new QTableWidget(
            0,
            2,
            this
            )
        ),
    addSeverityAliasButton(
        new QPushButton(
            tr("Add Alias"),
            this
            )
        ),
    removeSeverityAliasButton(
        new QPushButton(
            tr("Remove Selected"),
            this
            )
        ),
    timestampRuleTable(
        new QTableWidget(
            0,
            2,
            this
            )
        ),
    addTimestampRuleButton(
        new QPushButton(
            tr("Add Rule"),
            this
            )
        ),
    removeTimestampRuleButton(
        new QPushButton(
            tr("Remove Selected"),
            this
            )
        ),
    validationLabel(
        new QLabel(this)
        ),
    buttonBox(
        new QDialogButtonBox(
            QDialogButtonBox::Cancel,
            this
            )
        ),
    importButton(
        new QPushButton(
            tr("Import"),
            this
            )
        )
{
    setWindowTitle(
        tr("Import Configuration")
        );

    setAcceptDrops(true);

    resize(820, 700);

    workingProfile.name =
        QStringLiteral("Default JSON Lines");

    buildLayout();
    populateProfileControls();

    updateSourceState();
    updateValidationState();
}

QString ImportConfigurationDialog::selectedFilePath() const
{
    return filePathEdit->text().trimmed();
}

ImportProfile ImportConfigurationDialog::configuredProfile() const
{
    return workingProfile;
}

void ImportConfigurationDialog::setSelectedFilePath(
    const QString &filePath
    )
{
    filePathEdit->setText(filePath);
}

void ImportConfigurationDialog::buildLayout()
{
    auto *mainLayout =
        new QVBoxLayout(this);

    auto *introLabel =
        new QLabel(
            tr(
                "Choose a source log file. "
                "Import mappings and preview options "
                "will be configured here before the "
                "records are loaded into TraceScope."
                ),
            this
            );

    introLabel->setWordWrap(true);

    mainLayout->addWidget(introLabel);

    auto *sourceGroup =
        new QGroupBox(
            tr("Source"),
            this
            );

    auto *sourceLayout =
        new QFormLayout(sourceGroup);

    auto *fileRow =
        new QHBoxLayout();

    formatSuggestionLabel->setWordWrap(true);

    sourceLayout->addRow(
        tr("Likely format:"),
        formatSuggestionLabel
        );

    filePathEdit->setPlaceholderText(
        tr("Select a log file...")
        );

    fileRow->addWidget(
        filePathEdit,
        1
        );

    fileRow->addWidget(
        browseButton
        );

    sourceLayout->addRow(
        tr("File:"),
        fileRow
        );

    mainLayout->addWidget(sourceGroup);

    auto *scrollArea =
        new QScrollArea(this);

    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *scrollContent =
        new QWidget(scrollArea);

    auto *scrollLayout =
        new QVBoxLayout(scrollContent);

    auto *profileGroup =
        new QGroupBox(
            tr("Import Profile"),
            this
            );

    auto *profileLayout =
        new QFormLayout(profileGroup);

    profileNameEdit->setPlaceholderText(
        tr("Profile name")
        );

    profileLayout->addRow(
        tr("Name:"),
        profileNameEdit
        );

    preserveUnmappedCheckBox->setToolTip(
        tr(
            "Keep source fields that are not mapped "
            "to canonical or explicit custom fields."
            )
        );

    profileLayout->addRow(
        preserveUnmappedCheckBox
        );

    scrollLayout->addWidget(profileGroup);

    auto *mappingGroup =
        new QGroupBox(
            tr("Canonical Field Mapping"),
            this
            );

    auto *mappingLayout =
        new QFormLayout(mappingGroup);

    timestampPathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    severityPathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    subsystemPathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    eventCodePathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    entityIdPathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    messagePathEdit->setPlaceholderText(
        tr("Source path or blank")
        );

    mappingLayout->addRow(
        tr("Timestamp:"),
        timestampPathEdit
        );

    mappingLayout->addRow(
        tr("Severity:"),
        severityPathEdit
        );

    mappingLayout->addRow(
        tr("Subsystem:"),
        subsystemPathEdit
        );

    mappingLayout->addRow(
        tr("Event code:"),
        eventCodePathEdit
        );

    mappingLayout->addRow(
        tr("Entity ID:"),
        entityIdPathEdit
        );

    mappingLayout->addRow(
        tr("Message:"),
        messagePathEdit
        );

    scrollLayout->addWidget(mappingGroup);

    auto *customFieldsGroup =
        new QGroupBox(
            tr("Custom Field Mappings"),
            this
            );

    auto *customFieldsLayout =
        new QVBoxLayout(customFieldsGroup);

    customFieldTable->setHorizontalHeaderLabels(
        {
            tr("Field Name"),
            tr("Source Path")
        }
        );

    customFieldTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );

    customFieldTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch
            );

    customFieldTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    customFieldTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    customFieldTable->setMinimumHeight(130);

    customFieldsLayout->addWidget(
        customFieldTable
        );

    auto *customFieldButtonLayout =
        new QHBoxLayout();

    customFieldButtonLayout->addWidget(
        addCustomFieldButton
        );

    customFieldButtonLayout->addWidget(
        removeCustomFieldButton
        );

    customFieldButtonLayout->addStretch();

    customFieldsLayout->addLayout(
        customFieldButtonLayout
        );

    scrollLayout->addWidget(
        customFieldsGroup
        );

    auto *severityAliasGroup =
        new QGroupBox(
            tr("Severity Aliases"),
            this
            );

    auto *severityAliasLayout =
        new QVBoxLayout(severityAliasGroup);

    severityAliasTable->setHorizontalHeaderLabels(
        {
            tr("Source Value"),
            tr("Maps To")
        }
        );

    severityAliasTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );

    severityAliasTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch
            );

    severityAliasTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    severityAliasTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    severityAliasTable->setMinimumHeight(120);

    severityAliasLayout->addWidget(
        severityAliasTable
        );

    auto *severityAliasButtonLayout =
        new QHBoxLayout();

    severityAliasButtonLayout->addWidget(
        addSeverityAliasButton
        );

    severityAliasButtonLayout->addWidget(
        removeSeverityAliasButton
        );

    severityAliasButtonLayout->addStretch();

    severityAliasLayout->addLayout(
        severityAliasButtonLayout
        );

    scrollLayout->addWidget(
        severityAliasGroup
        );

    auto *timestampRuleGroup =
        new QGroupBox(
            tr("Timestamp Rules"),
            this
            );

    auto *timestampRuleLayout =
        new QVBoxLayout(timestampRuleGroup);

    timestampRuleTable->setHorizontalHeaderLabels(
        {
            tr("Type"),
            tr("Format")
        }
        );

    timestampRuleTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents
            );

    timestampRuleTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch
            );

    timestampRuleTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    timestampRuleTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    timestampRuleTable->setMinimumHeight(120);

    timestampRuleLayout->addWidget(
        timestampRuleTable
        );

    auto *timestampRuleButtonLayout =
        new QHBoxLayout();

    timestampRuleButtonLayout->addWidget(
        addTimestampRuleButton
        );

    timestampRuleButtonLayout->addWidget(
        removeTimestampRuleButton
        );

    timestampRuleButtonLayout->addStretch();

    timestampRuleLayout->addLayout(
        timestampRuleButtonLayout
        );

    scrollLayout->addWidget(
        timestampRuleGroup
        );

    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(
        scrollArea,
        1
        );

    auto *validationGroup =
        new QGroupBox(
            tr("Validation"),
            this
            );

    auto *validationLayout =
        new QVBoxLayout(validationGroup);

    validationLabel->setWordWrap(true);
    validationLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        );

    validationLayout->addWidget(
        validationLabel
        );

    mainLayout->addWidget(validationGroup);

    buttonBox->addButton(
        importButton,
        QDialogButtonBox::AcceptRole
        );

    mainLayout->addWidget(buttonBox);

    connect(
        browseButton,
        &QPushButton::clicked,
        this,
        [this]() {
            browseForFile();
        }
        );

    connect(
        filePathEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            updateSourceState();
        }
        );

    connect(
        preserveUnmappedCheckBox,
        &QCheckBox::toggled,
        this,
        [this]() {
            updateWorkingProfile();
        }
        );

    connect(
        addCustomFieldButton,
        &QPushButton::clicked,
        this,
        [this]() {
            addCustomFieldMapping();
        }
        );

    connect(
        removeCustomFieldButton,
        &QPushButton::clicked,
        this,
        [this]() {
            removeSelectedCustomFieldMapping();
        }
        );

    connect(
        customFieldTable,
        &QTableWidget::cellChanged,
        this,
        [this](
            int,
            int
            ) {
            updateCustomFieldMappings();
        }
        );

    connect(
        addSeverityAliasButton,
        &QPushButton::clicked,
        this,
        [this]() {
            addSeverityAlias();
        }
        );

    connect(
        removeSeverityAliasButton,
        &QPushButton::clicked,
        this,
        [this]() {
            removeSelectedSeverityAlias();
        }
        );

    connect(
        severityAliasTable,
        &QTableWidget::cellChanged,
        this,
        [this](
            int,
            int
            ) {
            updateSeverityAliases();
        }
        );

    connect(
        addTimestampRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            addTimestampRule();
        }
        );

    connect(
        removeTimestampRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            removeSelectedTimestampRule();
        }
        );

    connect(
        timestampRuleTable,
        &QTableWidget::cellChanged,
        this,
        [this](
            int,
            int
            ) {
            updateTimestampRules();
        }
        );

    const QList<QLineEdit *> profileEdits {
        profileNameEdit,
        timestampPathEdit,
        severityPathEdit,
        subsystemPathEdit,
        eventCodePathEdit,
        entityIdPathEdit,
        messagePathEdit
    };

    for (QLineEdit *edit : profileEdits) {
        connect(
            edit,
            &QLineEdit::textChanged,
            this,
            [this]() {
                updateWorkingProfile();
            }
            );
    }

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &QDialog::accept
        );

    connect(
        buttonBox,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
        );
}

void ImportConfigurationDialog::browseForFile()
{
    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            tr("Select Log File"),
            QString(),
            tr(
                "Log Files (*.jsonl *.ndjson *.log *.txt);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    setSelectedFilePath(filePath);
}

void ImportConfigurationDialog::updateSourceState()
{
    updateFormatSuggestion();
    updateImportAvailability();
}

void ImportConfigurationDialog::updateImportAvailability()
{
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    const bool sourceIsValid =
        fileInfo.exists()
        && fileInfo.isFile();

    const bool profileIsValid =
        profileValidator
            .validate(workingProfile)
            .isValid();

    importButton->setEnabled(
        sourceIsValid
        && profileIsValid
        );
}

void ImportConfigurationDialog::updateFormatSuggestion()
{
    const QString filePath =
        selectedFilePath();

    if (filePath.isEmpty()) {
        formatSuggestionLabel->setText(
            tr("Select a source file.")
            );
        return;
    }

    const ImportFormatSuggestion suggestion =
        formatSuggestionService
            .suggestForFile(filePath);

    if (!suggestion.hasSuggestion()) {
        formatSuggestionLabel->setText(
            tr(
                "No supported format could be "
                "suggested from the filename "
                "or sampled content."
                )
            );
        return;
    }

    formatSuggestionLabel->setText(
        tr("%1 — %2")
            .arg(
                suggestion.displayName,
                suggestion.reason
                )
        );
}

void ImportConfigurationDialog::dragEnterEvent(
    QDragEnterEvent *event
    )
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    event->acceptProposedAction();
}

void ImportConfigurationDialog::dropEvent(
    QDropEvent *event
    )
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    setSelectedFilePath(
        urls.first().toLocalFile()
        );

    event->acceptProposedAction();
}

void ImportConfigurationDialog::populateProfileControls()
{
    const QSignalBlocker profileNameBlocker(
        profileNameEdit
        );

    const QSignalBlocker timestampBlocker(
        timestampPathEdit
        );

    const QSignalBlocker severityBlocker(
        severityPathEdit
        );

    const QSignalBlocker subsystemBlocker(
        subsystemPathEdit
        );

    const QSignalBlocker eventCodeBlocker(
        eventCodePathEdit
        );

    const QSignalBlocker entityIdBlocker(
        entityIdPathEdit
        );

    const QSignalBlocker messageBlocker(
        messagePathEdit
        );

    const QSignalBlocker preserveUnmappedBlocker(
        preserveUnmappedCheckBox
        );

    profileNameEdit->setText(
        workingProfile.name
        );

    preserveUnmappedCheckBox->setChecked(
        workingProfile.preserveUnmappedFields
        );

    timestampPathEdit->setText(
        workingProfile
            .canonicalFields
            .timestampPath
        );

    severityPathEdit->setText(
        workingProfile
            .canonicalFields
            .severityPath
        );

    subsystemPathEdit->setText(
        workingProfile
            .canonicalFields
            .subsystemPath
        );

    eventCodePathEdit->setText(
        workingProfile
            .canonicalFields
            .eventCodePath
        );

    entityIdPathEdit->setText(
        workingProfile
            .canonicalFields
            .entityIdPath
        );

    messagePathEdit->setText(
        workingProfile
            .canonicalFields
            .messagePath
        );

    populateCustomFieldMappings();
    populateSeverityAliases();
    populateTimestampRules();
}

void ImportConfigurationDialog::populateCustomFieldMappings()
{
    const QSignalBlocker blocker(
        customFieldTable
        );

    customFieldTable->setRowCount(0);

    for (const CustomFieldMapping &mapping
         : std::as_const(
             workingProfile.customFields
             )) {
        const int row =
            customFieldTable->rowCount();

        customFieldTable->insertRow(row);

        customFieldTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                mapping.name
                )
            );

        customFieldTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                mapping.sourcePath
                )
            );
    }
}

void ImportConfigurationDialog::addCustomFieldMapping()
{
    QSignalBlocker blocker(
        customFieldTable
        );

    const int row =
        customFieldTable->rowCount();

    customFieldTable->insertRow(row);

    customFieldTable->setItem(
        row,
        0,
        new QTableWidgetItem()
        );

    customFieldTable->setItem(
        row,
        1,
        new QTableWidgetItem()
        );

    customFieldTable->setCurrentCell(
        row,
        0
        );

    blocker.unblock();

    updateCustomFieldMappings();

    customFieldTable->editItem(
        customFieldTable->item(
            row,
            0
            )
        );
}

void ImportConfigurationDialog::removeSelectedCustomFieldMapping()
{
    const int row =
        customFieldTable->currentRow();

    if (row < 0) {
        return;
    }

    {
        const QSignalBlocker blocker(
            customFieldTable
            );

        customFieldTable->removeRow(row);
    }

    updateCustomFieldMappings();
}

void ImportConfigurationDialog::updateCustomFieldMappings()
{
    QList<CustomFieldMapping> mappings;

    for (int row = 0;
         row < customFieldTable->rowCount();
         ++row) {
        QTableWidgetItem *nameItem =
            customFieldTable->item(
                row,
                0
                );

        QTableWidgetItem *pathItem =
            customFieldTable->item(
                row,
                1
                );

        CustomFieldMapping mapping;

        if (nameItem != nullptr) {
            mapping.name =
                nameItem->text();
        }

        if (pathItem != nullptr) {
            mapping.sourcePath =
                pathItem->text();
        }

        mappings.append(mapping);
    }

    workingProfile.customFields =
        std::move(mappings);

    updateValidationState();
}

void ImportConfigurationDialog::updateWorkingProfile()
{
    workingProfile.name =
        profileNameEdit->text();

    workingProfile.preserveUnmappedFields =
        preserveUnmappedCheckBox->isChecked();

    workingProfile
        .canonicalFields
        .timestampPath =
        timestampPathEdit->text();

    workingProfile
        .canonicalFields
        .severityPath =
        severityPathEdit->text();

    workingProfile
        .canonicalFields
        .subsystemPath =
        subsystemPathEdit->text();

    workingProfile
        .canonicalFields
        .eventCodePath =
        eventCodePathEdit->text();

    workingProfile
        .canonicalFields
        .entityIdPath =
        entityIdPathEdit->text();

    workingProfile
        .canonicalFields
        .messagePath =
        messagePathEdit->text();

    updateValidationState();
}

void ImportConfigurationDialog::updateValidationState()
{
    const ProfileValidationResult result =
        profileValidator.validate(
            workingProfile
            );

    if (result.issues.isEmpty()) {
        validationLabel->setText(
            tr("Profile configuration is valid.")
            );

        updateImportAvailability();
        return;
    }

    QStringList lines;

    for (const ProfileValidationIssue &issue
         : result.issues) {
        const QString prefix =
            issue.severity
                    == ProfileValidationSeverity::Error
                ? tr("Error")
                : tr("Warning");

        lines.append(
            QStringLiteral("%1: %2")
                .arg(
                    prefix,
                    issue.message
                    )
            );
    }

    validationLabel->setText(
        lines.join(
            QLatin1Char('\n')
            )
        );

    updateImportAvailability();
}

QComboBox *ImportConfigurationDialog::createSeverityCombo(
        RecordSeverity severity
        )
{
    auto *combo =
        new QComboBox(
            severityAliasTable
            );

    const QList<RecordSeverity> severities {
        RecordSeverity::Trace,
        RecordSeverity::Debug,
        RecordSeverity::Info,
        RecordSeverity::Warning,
        RecordSeverity::Error,
        RecordSeverity::Critical
    };

    for (const RecordSeverity value
         : severities) {
        combo->addItem(
            recordSeverityToString(value),
            static_cast<int>(value)
            );
    }

    const int index =
        combo->findData(
            static_cast<int>(severity)
            );

    if (index >= 0) {
        combo->setCurrentIndex(index);
    }

    connect(
        combo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateSeverityAliases();
        }
        );

    return combo;
}

void ImportConfigurationDialog::populateSeverityAliases()
{
    const QSignalBlocker blocker(
        severityAliasTable
        );

    severityAliasTable->setRowCount(0);

    for (auto iterator =
         workingProfile
             .severityAliases
             .constBegin();
         iterator !=
         workingProfile
             .severityAliases
             .constEnd();
         ++iterator) {
        const int row =
            severityAliasTable->rowCount();

        severityAliasTable->insertRow(row);

        severityAliasTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                iterator.key()
                )
            );

        severityAliasTable->setCellWidget(
            row,
            1,
            createSeverityCombo(
                iterator.value()
                )
            );
    }
}

void ImportConfigurationDialog::addSeverityAlias()
{
    const QSignalBlocker blocker(
        severityAliasTable
        );

    const int row =
        severityAliasTable->rowCount();

    severityAliasTable->insertRow(row);

    severityAliasTable->setItem(
        row,
        0,
        new QTableWidgetItem()
        );

    severityAliasTable->setCellWidget(
        row,
        1,
        createSeverityCombo(
            RecordSeverity::Info
            )
        );

    severityAliasTable->setCurrentCell(
        row,
        0
        );

    updateSeverityAliases();

    severityAliasTable->editItem(
        severityAliasTable->item(
            row,
            0
            )
        );
}

void ImportConfigurationDialog::removeSelectedSeverityAlias()
{
    const int row =
        severityAliasTable->currentRow();

    if (row < 0) {
        return;
    }

    {
        const QSignalBlocker blocker(
            severityAliasTable
            );

        severityAliasTable->removeRow(row);
    }

    updateSeverityAliases();
}

void ImportConfigurationDialog::updateSeverityAliases()
{
    QMap<QString, RecordSeverity> aliases;

    for (int row = 0;
         row < severityAliasTable->rowCount();
         ++row) {
        QTableWidgetItem *aliasItem =
            severityAliasTable->item(
                row,
                0
                );

        auto *severityCombo =
            qobject_cast<QComboBox *>(
                severityAliasTable
                    ->cellWidget(
                        row,
                        1
                        )
                );

        const QString alias =
            aliasItem != nullptr
                ? aliasItem->text()
                : QString();

        const RecordSeverity severity =
            severityCombo != nullptr
                ? static_cast<RecordSeverity>(
                      severityCombo
                          ->currentData()
                          .toInt()
                      )
                : RecordSeverity::Info;

        aliases.insert(
            alias,
            severity
            );
    }

    workingProfile.severityAliases =
        std::move(aliases);

    updateValidationState();
}

QComboBox *ImportConfigurationDialog::createTimestampRuleTypeCombo(
        TimestampRuleType type
        )
{
    auto *combo =
        new QComboBox(
            timestampRuleTable
            );

    combo->addItem(
        tr("ISO 8601"),
        static_cast<int>(
            TimestampRuleType::Iso8601
            )
        );

    combo->addItem(
        tr("Qt Format"),
        static_cast<int>(
            TimestampRuleType::QtFormat
            )
        );

    const int index =
        combo->findData(
            static_cast<int>(type)
            );

    if (index >= 0) {
        combo->setCurrentIndex(index);
    }

    connect(
        combo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateTimestampRules();
        }
        );

    return combo;
}

void ImportConfigurationDialog::populateTimestampRules()
{
    const QSignalBlocker blocker(
        timestampRuleTable
        );

    timestampRuleTable->setRowCount(0);

    for (const TimestampRule &rule
         : std::as_const(
             workingProfile.timestampRules
             )) {
        const int row =
            timestampRuleTable->rowCount();

        timestampRuleTable->insertRow(row);

        timestampRuleTable->setCellWidget(
            row,
            0,
            createTimestampRuleTypeCombo(
                rule.type
                )
            );

        timestampRuleTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                rule.format
                )
            );
    }
}

void ImportConfigurationDialog::addTimestampRule()
{
    const QSignalBlocker blocker(
        timestampRuleTable
        );

    const int row =
        timestampRuleTable->rowCount();

    timestampRuleTable->insertRow(row);

    timestampRuleTable->setCellWidget(
        row,
        0,
        createTimestampRuleTypeCombo(
            TimestampRuleType::Iso8601
            )
        );

    timestampRuleTable->setItem(
        row,
        1,
        new QTableWidgetItem()
        );

    timestampRuleTable->setCurrentCell(
        row,
        1
        );

    updateTimestampRules();
}

void ImportConfigurationDialog::removeSelectedTimestampRule()
{
    const int row =
        timestampRuleTable->currentRow();

    if (row < 0) {
        return;
    }

    {
        const QSignalBlocker blocker(
            timestampRuleTable
            );

        timestampRuleTable->removeRow(row);
    }

    updateTimestampRules();
}

void ImportConfigurationDialog::updateTimestampRules()
{
    QList<TimestampRule> rules;

    for (int row = 0;
         row < timestampRuleTable->rowCount();
         ++row) {
        auto *typeCombo =
            qobject_cast<QComboBox *>(
                timestampRuleTable
                    ->cellWidget(
                        row,
                        0
                        )
                );

        QTableWidgetItem *formatItem =
            timestampRuleTable->item(
                row,
                1
                );

        TimestampRule rule;

        if (typeCombo != nullptr) {
            rule.type =
                static_cast<TimestampRuleType>(
                    typeCombo
                        ->currentData()
                        .toInt()
                    );
        }

        if (formatItem != nullptr) {
            rule.format =
                formatItem->text();
        }

        rules.append(
            std::move(rule)
            );
    }

    workingProfile.timestampRules =
        std::move(rules);

    updateValidationState();
}