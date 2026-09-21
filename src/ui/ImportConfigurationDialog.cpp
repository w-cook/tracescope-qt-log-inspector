#include "ImportConfigurationDialog.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPromise>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <memory>
#include <optional>
#include <utility>

#include "../importing/BuiltInImporterRegistry.h"
#include "../importing/BuiltInImportProfilePresets.h"
#include "../importing/ILogImporter.h"
#include "../preferences/RotatedSourceSettingsStore.h"

#include "DevicePixelAlignedWidgetHost.h"
#include "RotatedSourceConfigurationDialog.h"

namespace
{
QString customFieldMappingKey(
    const CustomFieldMapping &mapping
    )
{
    return mapping.name
               .trimmed()
               .toCaseFolded()
           + QChar(0x1f)
           + mapping.sourcePath
                 .trimmed()
                 .toCaseFolded();
}

constexpr qint64
    AutomaticStructuredDocumentPreviewMaxBytes =
    16 * 1024 * 1024;

bool requiresManualStructuredDocumentPreview(
    const QFileInfo &fileInfo,
    const ImportProfile &profile
    )
{
    const bool isStructuredDocument =
        profile.importerId
            == QStringLiteral("structured-json")
        || profile.importerId
               == QStringLiteral("xml");

    return isStructuredDocument
           && fileInfo.size()
                  > AutomaticStructuredDocumentPreviewMaxBytes;
}

bool requiresManualStructuredDocumentPreview(
    const QStringList &filePaths,
    const ImportProfile &profile
    )
{
    for (const QString &filePath
         : filePaths) {
        if (requiresManualStructuredDocumentPreview(
                QFileInfo(filePath),
                profile
                )) {
            return true;
        }
    }

    return false;
}

QList<CustomFieldMapping> retainedCustomMappings(
    const ImportProfile &profile,
    const QSet<QString> &autoDetectedKeys
    )
{
    QList<CustomFieldMapping> retained;

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        if (autoDetectedKeys.contains(
                customFieldMappingKey(mapping)
                )) {
            continue;
        }

        retained.append(mapping);
    }

    return retained;
}
}

ImportConfigurationDialog::ImportConfigurationDialog(
    QWidget *parent,
    RecentItemsStore *recentItemsStore,
    RotatedSourceSettingsStore *rotatedSourceSettingsStore
    )
    : QDialog(parent),
    recentItemsStore(recentItemsStore),
    rotatedSourceSettingsStore(
        rotatedSourceSettingsStore
        ),
    filePathEdit(
        new QLineEdit(this)
        ),
    browseButton(
        new QPushButton(
            tr("Browse..."),
            this
            )
        ),
    formatSuggestionLabel(
        new QLabel(this)
        ),
    includeRotatedSourcesCheckBox(
        new QCheckBox(
            tr("Include rotations"),
            this
            )
        ),
    reviewRotatedSourcesButton(
        new QPushButton(
            tr("Review..."),
            this
            )
        ),
    previewSummaryLabel(
        new QLabel(this)
        ),
    previewTable(
        new QTableWidget(
            0,
            7,
            this
            )
        ),
    rawSourcePreview(
        new QPlainTextEdit(this)
        ),
    refreshPreviewButton(
        new QPushButton(
            tr("Refresh Preview"),
            this
            )
        ),
    previewRefreshTimer(
        new QTimer(this)
        ),
    profileNameEdit(
        new QLineEdit(this)
        ),
    importerComboBox(
        new QComboBox(this)
        ),
    recordPathEdit(
        new QLineEdit(this)
        ),
    regexPatternEdit(
        new QPlainTextEdit(this)
        ),
    preserveUnmappedCheckBox(
        new QCheckBox(
            tr("Preserve unmapped source fields"),
            this
            )
        ),
    newProfileFromSourceButton(
        new QPushButton(
            tr("New From Source"),
            this
            )
        ),
    loadProfileButton(
        new QPushButton(
            tr("Load Profile..."),
            this
            )
        ),
    saveProfileButton(
        new QPushButton(
            tr("Save Profile..."),
            this
            )
        ),
    recentProfilesButton(
        new QPushButton(
            tr("Recent Profiles"),
            this
            )
        ),
    recentProfilesMenu(
        new QMenu(this)
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
    resize(1100, 700);

    workingProfile.name =
        QStringLiteral(
            "Default JSON Lines"
            );

    previewRefreshTimer->setSingleShot(
        true
        );

    previewRefreshTimer->setInterval(
        250
        );

    connect(
        previewRefreshTimer,
        &QTimer::timeout,
        this,
        [this]() {
            if (workingProfile.importerId
                    == QStringLiteral(
                        "regex-text"
                        )
                && customFieldDetectionSourceKey
                       .isEmpty()) {
                detectCustomFieldMappings();
            }

            updatePreview();
        }
        );

    buildLayout();
    populateImporterOptions();
    populateProfileControls();

    updateSourceState();
    updateValidationState();
}

QString ImportConfigurationDialog::
    selectedFilePath() const
{
    return filePathEdit
        ->text()
        .trimmed();
}

void ImportConfigurationDialog::
    setSelectedFilePath(
        const QString &filePath
        )
{
    filePathEdit->setText(
        filePath
        );
}

ImportProfile ImportConfigurationDialog::
    configuredProfile() const
{
    return workingProfile;
}

bool ImportConfigurationDialog::
    includeRotatedSources() const
{
    return includeRotatedSourcesCheckBox
        ->isChecked();
}

const RotatedSourceRule &
ImportConfigurationDialog::
    rotatedSourceRule() const
{
    return rotatedSourceRuleState;
}

const QVector<RotatedSourceMatch> &
ImportConfigurationDialog::
    rotatedSources() const
{
    return rotatedSourceMatches;
}

void ImportConfigurationDialog::
    buildLayout()
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

    formatSuggestionLabel->setWordWrap(
        true
        );

    auto *formatRow =
        new QHBoxLayout();

    formatRow->addWidget(
        formatSuggestionLabel,
        1
        );

    includeRotatedSourcesCheckBox
        ->setEnabled(false);

    formatRow->addWidget(
        includeRotatedSourcesCheckBox
        );

    reviewRotatedSourcesButton
        ->setEnabled(false);

    reviewRotatedSourcesButton
        ->setMinimumWidth(
            browseButton
                ->sizeHint()
                .width()
            );

    formatRow->addWidget(
        reviewRotatedSourcesButton
        );

    sourceLayout->addRow(
        tr("Likely format:"),
        formatRow
        );

    auto *fileRow =
        new QHBoxLayout();

    filePathEdit->setPlaceholderText(
        tr("Select a log file...")
        );

    auto *filePathHost =
        new DevicePixelAlignedWidgetHost(
            filePathEdit,
            Qt::RightEdge,
            this
            );

    filePathHost->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );

    fileRow->addWidget(
        filePathHost,
        1
        );

    fileRow->addWidget(
        browseButton
        );

    sourceLayout->addRow(
        tr("File:"),
        fileRow
        );

    mainLayout->addWidget(
        sourceGroup
        );

    auto *scrollArea =
        new QScrollArea(this);

    scrollArea->setWidgetResizable(
        true
        );

    scrollArea->setFrameShape(
        QFrame::NoFrame
        );

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

    profileLayout->addRow(
        tr("Format:"),
        importerComboBox
        );

    recordPathEdit->setPlaceholderText(
        tr(
            "Blank for document root, "
            "or a path such as session.events.event"
            )
        );

    recordPathEdit->setToolTip(
        tr(
            "For Structured JSON and Structured XML, "
            "identifies the object or element path "
            "containing the records to import. "
            "Use dot-separated paths such as "
            "'session.events.event'. Leave blank "
            "to use the document root."
            )
        );

    profileLayout->addRow(
        tr("Record path:"),
        recordPathEdit
        );

    regexPatternEdit->setPlaceholderText(
        tr(
            "Named-capture regular expression, "
            "for example: "
            "^(?<timestamp>...) (?<level>...) (?<message>.*)$"
            )
        );

    regexPatternEdit->setToolTip(
        tr(
            "For Regex Plain Text imports, named capture groups "
            "become source fields that can be mapped to canonical "
            "or custom fields."
            )
        );

    regexPatternEdit->setLineWrapMode(
        QPlainTextEdit::NoWrap
        );

    regexPatternEdit->setFixedHeight(
        70
        );

    profileLayout->addRow(
        tr("Regex pattern:"),
        regexPatternEdit
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

    auto *profileButtonLayout =
        new QHBoxLayout();

    profileButtonLayout->addWidget(
        newProfileFromSourceButton
        );

    profileButtonLayout->addWidget(
        loadProfileButton
        );

    recentProfilesButton->setMenu(
        recentProfilesMenu
        );

    recentProfilesMenu->setToolTipsVisible(
        true
        );

    connect(
        recentProfilesMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            refreshRecentProfilesMenu();
        }
        );

    profileButtonLayout->addWidget(
        recentProfilesButton
        );

    profileButtonLayout->addWidget(
        saveProfileButton
        );

    profileButtonLayout->addStretch();

    profileLayout->addRow(
        profileButtonLayout
        );

    scrollLayout->addWidget(
        profileGroup
        );

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

    scrollLayout->addWidget(
        mappingGroup
        );

    auto *customFieldsGroup =
        new QGroupBox(
            tr("Custom Field Mappings"),
            this
            );

    auto *customFieldsLayout =
        new QVBoxLayout(
            customFieldsGroup
            );

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

    customFieldTable->setMinimumHeight(
        130
        );

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
        new QVBoxLayout(
            severityAliasGroup
            );

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

    severityAliasTable->setMinimumHeight(
        120
        );

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
        new QVBoxLayout(
            timestampRuleGroup
            );

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

    timestampRuleTable->setMinimumHeight(
        120
        );

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

    scrollArea->setWidget(
        scrollContent
        );

    auto *previewGroup =
        new QGroupBox(
            tr("Source Record Preview"),
            this
            );

    auto *previewLayout =
        new QVBoxLayout(
            previewGroup
            );

    previewSummaryLabel->setWordWrap(
        true
        );

    auto *previewHeaderLayout =
        new QHBoxLayout();

    previewHeaderLayout->addWidget(
        previewSummaryLabel,
        1
        );

    previewHeaderLayout->addWidget(
        refreshPreviewButton
        );

    previewLayout->addLayout(
        previewHeaderLayout
        );

    refreshPreviewButton->setVisible(
        false
        );

    previewTable->setHorizontalHeaderLabels(
        {
            tr("Timestamp"),
            tr("Severity"),
            tr("Subsystem"),
            tr("Event Code"),
            tr("Entity ID"),
            tr("Message"),
            tr("Unmapped Custom Fields")
        }
        );

    previewTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    previewTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    previewTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    previewTable->setWordWrap(
        false
        );

    QHeaderView *previewHeader =
        previewTable->horizontalHeader();

    previewHeader->setSectionResizeMode(
        QHeaderView::Interactive
        );

    previewHeader->setMinimumSectionSize(
        70
        );

    previewTable->setColumnWidth(
        0,
        180
        );

    previewTable->setColumnWidth(
        1,
        80
        );

    previewTable->setColumnWidth(
        2,
        120
        );

    previewTable->setColumnWidth(
        3,
        140
        );

    previewTable->setColumnWidth(
        4,
        120
        );

    previewTable->setColumnWidth(
        5,
        300
        );

    previewTable->setColumnWidth(
        6,
        220
        );

    auto *previewSplitter =
        new QSplitter(
            Qt::Vertical,
            previewGroup
            );

    previewSplitter->setChildrenCollapsible(
        false
        );

    previewSplitter->setStretchFactor(
        0,
        1
        );

    previewSplitter->setStretchFactor(
        1,
        0
        );

    previewTable->setMinimumHeight(
        120
        );

    previewSplitter->addWidget(
        previewTable
        );

    auto *rawSourcePanel =
        new QWidget(
            previewSplitter
            );

    auto *rawSourceLayout =
        new QVBoxLayout(
            rawSourcePanel
            );

    rawSourceLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    rawSourceLayout->setSpacing(
        4
        );

    auto *rawSourceLabel =
        new QLabel(
            tr("Selected Raw Source:"),
            rawSourcePanel
            );

    rawSourceLayout->addWidget(
        rawSourceLabel
        );

    rawSourcePreview->setReadOnly(
        true
        );

    rawSourcePreview->setLineWrapMode(
        QPlainTextEdit::NoWrap
        );

    rawSourcePreview->setPlaceholderText(
        tr(
            "Select a preview record to inspect "
            "its original source."
            )
        );

    rawSourcePreview->setMinimumHeight(
        45
        );

    rawSourceLayout->addWidget(
        rawSourcePreview,
        1
        );

    previewSplitter->addWidget(
        rawSourcePanel
        );

    previewSplitter->setSizes(
        QList<int>{
            360,
            80
        }
        );

    previewLayout->addWidget(
        previewSplitter,
        1
        );

    auto *contentSplitter =
        new QSplitter(
            Qt::Horizontal,
            this
            );

    contentSplitter->addWidget(
        scrollArea
        );

    contentSplitter->addWidget(
        previewGroup
        );

    contentSplitter->setStretchFactor(
        0,
        1
        );

    contentSplitter->setStretchFactor(
        1,
        2
        );

    mainLayout->addWidget(
        contentSplitter,
        1
        );

    auto *validationGroup =
        new QGroupBox(
            tr("Validation"),
            this
            );

    auto *validationLayout =
        new QVBoxLayout(
            validationGroup
            );

    validationLabel->setWordWrap(
        true
        );

    validationLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        );

    validationLayout->addWidget(
        validationLabel
        );

    mainLayout->addWidget(
        validationGroup
        );

    buttonBox->addButton(
        importButton,
        QDialogButtonBox::AcceptRole
        );

    mainLayout->addWidget(
        buttonBox
        );

    connect(
        browseButton,
        &QPushButton::clicked,
        this,
        [this]() {
            browseForFile();
        }
        );

    connect(
        reviewRotatedSourcesButton,
        &QPushButton::clicked,
        this,
        [this]() {
            reviewRotatedSources();
        }
        );

    connect(
        includeRotatedSourcesCheckBox,
        &QCheckBox::toggled,
        this,
        [this]() {
            cancelManualPreview();
            customFieldDetectionSourceKey.clear();

            detectCustomFieldMappings();

            previewRefreshTimer->stop();
            updatePreview();
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
        importerComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            profileIsUserConfigured = true;

            workingProfile.importerId =
                importerComboBox
                    ->currentData()
                    .toString();

            workingProfile.customFields =
                retainedCustomMappings(
                    workingProfile,
                    autoDetectedCustomFieldKeys
                    );

            autoDetectedCustomFieldKeys.clear();
            customFieldDetectionSourceKey.clear();

            populateCustomFieldMappings();
            updateFormatSpecificControls();
            updateValidationState();
        }
        );

    connect(
        regexPatternEdit,
        &QPlainTextEdit::textChanged,
        this,
        [this]() {
            profileIsUserConfigured = true;

            workingProfile.regexPattern =
                regexPatternEdit
                    ->toPlainText();

            customFieldDetectionSourceKey.clear();

            updateValidationState();
        }
        );

    connect(
        preserveUnmappedCheckBox,
        &QCheckBox::toggled,
        this,
        [this]() {
            profileIsUserConfigured = true;
            updateWorkingProfile();
        }
        );

    connect(
        newProfileFromSourceButton,
        &QPushButton::clicked,
        this,
        [this]() {
            createProfileFromSource(true);
        }
        );

    connect(
        loadProfileButton,
        &QPushButton::clicked,
        this,
        [this]() {
            loadProfile();
        }
        );

    connect(
        saveProfileButton,
        &QPushButton::clicked,
        this,
        [this]() {
            saveProfile();
        }
        );

    connect(
        addCustomFieldButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            addCustomFieldMapping();
        }
        );

    connect(
        removeCustomFieldButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            removeSelectedCustomFieldMapping();
        }
        );

    connect(
        customFieldTable,
        &QTableWidget::cellChanged,
        this,
        [this](int, int) {
            profileIsUserConfigured = true;
            updateCustomFieldMappings();
        }
        );

    connect(
        addSeverityAliasButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            addSeverityAlias();
        }
        );

    connect(
        removeSeverityAliasButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            removeSelectedSeverityAlias();
        }
        );

    connect(
        severityAliasTable,
        &QTableWidget::cellChanged,
        this,
        [this](int, int) {
            profileIsUserConfigured = true;
            updateSeverityAliases();
        }
        );

    connect(
        addTimestampRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            addTimestampRule();
        }
        );

    connect(
        removeTimestampRuleButton,
        &QPushButton::clicked,
        this,
        [this]() {
            profileIsUserConfigured = true;
            removeSelectedTimestampRule();
        }
        );

    connect(
        timestampRuleTable,
        &QTableWidget::cellChanged,
        this,
        [this](int, int) {
            profileIsUserConfigured = true;
            updateTimestampRules();
        }
        );

    connect(
        previewTable,
        &QTableWidget::currentCellChanged,
        this,
        [this](
            int currentRow,
            int,
            int,
            int
            ) {
            updateRawSourcePreview(
                currentRow
                );
        }
        );

    connect(
        refreshPreviewButton,
        &QPushButton::clicked,
        this,
        [this]() {
            startManualPreview();
        }
        );

    connect(
        recordPathEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            profileIsUserConfigured = true;

            workingProfile.recordPath =
                recordPathEdit->text();

            customFieldDetectionSourceKey.clear();

            updateValidationState();
        }
        );

    connect(
        recordPathEdit,
        &QLineEdit::editingFinished,
        this,
        [this]() {
            if (workingProfile.importerId
                    != QStringLiteral(
                        "structured-json"
                        )
                && workingProfile.importerId
                       != QStringLiteral(
                           "xml"
                           )) {
                return;
            }

            detectCustomFieldMappings();

            previewRefreshTimer->stop();
            updatePreview();
        }
        );

    const QList<QLineEdit *>
        previewRelevantProfileEdits {
            timestampPathEdit,
            severityPathEdit,
            subsystemPathEdit,
            eventCodePathEdit,
            entityIdPathEdit,
            messagePathEdit
        };

    for (QLineEdit *edit
         : previewRelevantProfileEdits) {
        connect(
            edit,
            &QLineEdit::textChanged,
            this,
            [this]() {
                profileIsUserConfigured = true;
                updateWorkingProfile();
            }
            );
    }

    connect(
        profileNameEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            profileIsUserConfigured = true;

            workingProfile.name =
                profileNameEdit->text();

            updateValidationState(
                false
                );
        }
        );

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

    refreshRecentProfilesMenu();
}

void ImportConfigurationDialog::
    browseForFile()
{
    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            tr("Select Log File"),
            QString(),
            tr(
                "Supported Log Files "
                "(*.json *.jsonl *.ndjson *.xml *.csv *.tsv *.log *.txt);;"
                "Structured JSON (*.json);;"
                "Structured XML (*.xml);;"
                "JSON Lines (*.jsonl *.ndjson);;"
                "Delimited Text (*.csv *.tsv);;"
                "Log and Text Files (*.log *.txt);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    setSelectedFilePath(
        filePath
        );
}

void ImportConfigurationDialog::
    updateRotatedSourceState()
{
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        rotationConfigurationSourcePath.clear();
        rotatedSourceRuleState =
            RotatedSourceRule();
        rotatedSourceMatches.clear();

        const QSignalBlocker blocker(
            includeRotatedSourcesCheckBox
            );

        includeRotatedSourcesCheckBox
            ->setChecked(false);

        rotatedSourceRuleWasAutomaticallySuggested =
            false;

        updateRotatedSourceControls();
        return;
    }

    const QString absolutePath =
        fileInfo.absoluteFilePath();

    const bool sourceChanged =
        rotationConfigurationSourcePath
        != absolutePath;

    if (sourceChanged) {
        rotationConfigurationSourcePath =
            absolutePath;

        customFieldDetectionSourceKey.clear();
        previewSourceKey.clear();

        const QSignalBlocker blocker(
            includeRotatedSourcesCheckBox
            );

        includeRotatedSourcesCheckBox
            ->setChecked(false);

        rotatedSourceRuleState =
            RotatedSourceRule();

        rotatedSourceMatches.clear();

        rotatedSourceRuleWasAutomaticallySuggested =
            false;

        bool restoredRememberedConfiguration =
            false;

        if (rotatedSourceSettingsStore
            != nullptr) {
            const auto remembered =
                rotatedSourceSettingsStore
                    ->rememberedSourceConfiguration(
                        absolutePath
                        );

            if (remembered.has_value()) {
                rotatedSourceRuleState =
                    remembered->rule;

                includeRotatedSourcesCheckBox
                    ->setChecked(
                        remembered
                            ->includeRotatedSources
                        );

                restoredRememberedConfiguration =
                    true;
            }
        }

        if (!restoredRememberedConfiguration) {
            int bestMatchCount = 0;

            /*
             * Automatic source-family suggestions must be
             * derived from the active source itself.
             *
             * Built-in rules satisfy that requirement because
             * their filename patterns are constructed from the
             * selected active filename.
             *
             * Saved custom schemes are intentionally excluded
             * here. A custom regular expression may match files
             * elsewhere in the same directory without having
             * any relationship to the selected active source.
             * Saved schemes remain available explicitly through
             * the rotated-source configuration dialog.
             */
            const auto builtInSuggestions =
                RotatedSourceDiscoveryService::
                suggestRules(
                    absolutePath
                    );

            if (builtInSuggestions.succeeded) {
                for (const auto &suggestion
                     : builtInSuggestions
                           .suggestions) {
                    if (suggestion.matchCount
                        <= bestMatchCount) {
                        continue;
                    }

                    rotatedSourceRuleState =
                        suggestion.rule;

                    bestMatchCount =
                        suggestion.matchCount;

                    rotatedSourceRuleWasAutomaticallySuggested =
                        true;
                }
            }
        }
    }

    rotatedSourceMatches.clear();

    if (rotatedSourceRuleState.namingScheme
        != RotatedSourceNamingScheme::
            Disabled) {
        const auto discovery =
            RotatedSourceDiscoveryService::
                discover(
                    absolutePath,
                    rotatedSourceRuleState
                    );

        if (discovery.succeeded) {
            rotatedSourceMatches =
                discovery.rotatedSources;
        }
    }

    updateRotatedSourceControls();
}

void ImportConfigurationDialog::
    updateRotatedSourceControls()
{
    const QFileInfo sourceInfo(
        selectedFilePath()
        );

    const bool sourceIsValid =
        sourceInfo.exists()
        && sourceInfo.isFile();

    reviewRotatedSourcesButton
        ->setEnabled(
            sourceIsValid
            );

    const int count =
        rotatedSourceMatches.size();

    {
        const QSignalBlocker blocker(
            includeRotatedSourcesCheckBox
            );

        includeRotatedSourcesCheckBox
            ->setEnabled(
                sourceIsValid
                && count > 0
                );

        if (count <= 0) {
            includeRotatedSourcesCheckBox
                ->setChecked(false);
        }
    }

    if (count <= 0) {
        includeRotatedSourcesCheckBox
            ->setText(
                tr("Include rotations")
                );

        includeRotatedSourcesCheckBox
            ->setToolTip(
                sourceIsValid
                    ? tr(
                          "No related rotated files "
                          "are currently detected. "
                          "Choose Review to configure "
                          "a different naming rule."
                          )
                    : QString()
                );

        reviewRotatedSourcesButton
            ->setToolTip(
                sourceIsValid
                    ? tr(
                          "Review or configure how "
                          "rotated files belonging to "
                          "this source are identified."
                          )
                    : QString()
                );

        return;
    }

    includeRotatedSourcesCheckBox
        ->setText(
            count == 1
                ? tr("Include 1 rotation")
                : tr(
                      "Include %1 rotations"
                      )
                      .arg(count)
            );

    const QString toolTip =
        tr(
            "%1 rotated %2 detected.\n"
            "Oldest: %3\n"
            "Newest rotated: %4\n"
            "Active: %5"
            )
            .arg(
                QString::number(count),
                count == 1
                    ? tr("file")
                    : tr("files"),
                rotatedSourceMatches
                    .first()
                    .fileName,
                rotatedSourceMatches
                    .last()
                    .fileName,
                sourceInfo.fileName()
                );

    includeRotatedSourcesCheckBox
        ->setToolTip(
            toolTip
            );

    reviewRotatedSourcesButton
        ->setToolTip(
            toolTip
            );
}

void ImportConfigurationDialog::
    reviewRotatedSources()
{
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return;
    }

    RotatedSourceConfigurationDialog dialog(
        fileInfo.absoluteFilePath(),
        rotatedSourceRuleState,
        includeRotatedSourcesCheckBox
            ->isChecked(),
        rotatedSourceSettingsStore,
        this
        );

    if (dialog.exec()
        != QDialog::Accepted) {
        return;
    }

    rotatedSourceRuleState =
        dialog.configuredRule();

    rotatedSourceRuleWasAutomaticallySuggested =
        false;

    rotatedSourceMatches =
        dialog.rotatedSources();

    {
        const QSignalBlocker blocker(
            includeRotatedSourcesCheckBox
            );

        includeRotatedSourcesCheckBox
            ->setChecked(
                dialog.includeRotatedSources()
                );
    }

    updateRotatedSourceControls();

    customFieldDetectionSourceKey.clear();
    previewSourceKey.clear();

    detectCustomFieldMappings();

    previewRefreshTimer->stop();
    updatePreview();
}

void ImportConfigurationDialog::
    refineSuggestedNumericRotationDirectionFromTimestamps()
{
    if (!rotatedSourceRuleWasAutomaticallySuggested) {
        return;
    }

    const bool numericScheme =
        rotatedSourceRuleState.namingScheme
            == RotatedSourceNamingScheme::
            NumericSuffix
        || rotatedSourceRuleState.namingScheme
               == RotatedSourceNamingScheme::
               NumericBeforeExtension;

    if (!numericScheme
        || rotatedSourceMatches.size() < 2) {
        return;
    }

    if (workingProfile
            .canonicalFields
            .timestampPath
            .trimmed()
            .isEmpty()) {
        return;
    }

    if (!profileValidator
             .validate(
                 workingProfile
                 )
             .isValid()) {
        return;
    }

    const RotatedSourceMatch *lowestIndexSource =
        nullptr;

    const RotatedSourceMatch *highestIndexSource =
        nullptr;

    for (const RotatedSourceMatch &match
         : std::as_const(rotatedSourceMatches)) {
        if (match.rotationIndex <= 0) {
            continue;
        }

        if (lowestIndexSource == nullptr
            || match.rotationIndex
                   < lowestIndexSource
                         ->rotationIndex) {
            lowestIndexSource =
                &match;
        }

        if (highestIndexSource == nullptr
            || match.rotationIndex
                   > highestIndexSource
                         ->rotationIndex) {
            highestIndexSource =
                &match;
        }
    }

    if (lowestIndexSource == nullptr
        || highestIndexSource == nullptr
        || lowestIndexSource == highestIndexSource) {
        return;
    }

    const QStringList sampledPaths {
        lowestIndexSource->filePath,
        highestIndexSource->filePath
    };

    /*
     * Some structured formats require loading the
     * complete document before records can be sampled.
     * Do not turn automatic chronology detection into
     * an expensive or blocking import-dialog operation.
     */
    if (requiresManualStructuredDocumentPreview(
            sampledPaths,
            workingProfile
            )) {
        return;
    }

    const auto sampleTimestamp =
        [this](
            const QString &filePath
            ) -> std::optional<QDateTime> {
        constexpr qint64 SampleRecordLimit =
            10;

        const ImportPreviewResult preview =
            previewService.previewFile(
                filePath,
                workingProfile,
                SampleRecordLimit
                );

        if (!preview.canDisplayPreview()) {
            return std::nullopt;
        }

        std::optional<QDateTime>
            earliestTimestamp;

        for (const InvestigationRecord &record
             : preview.importResult.records) {
            if (!record.timestamp.has_value()
                || !record.timestamp
                        ->isValid()) {
                continue;
            }

            if (!earliestTimestamp.has_value()
                || record.timestamp.value()
                       < earliestTimestamp.value()) {
                earliestTimestamp =
                    record.timestamp.value();
            }
        }

        return earliestTimestamp;
    };

    const std::optional<QDateTime>
        lowestTimestamp =
        sampleTimestamp(
            lowestIndexSource->filePath
            );

    const std::optional<QDateTime>
        highestTimestamp =
        sampleTimestamp(
            highestIndexSource->filePath
            );

    if (!lowestTimestamp.has_value()
        || !highestTimestamp.has_value()
        || lowestTimestamp.value()
               == highestTimestamp.value()) {
        return;
    }

    /*
     * If lower numeric suffixes contain earlier
     * timestamps, numbering grows forward with time:
     *
     *     .1 -> .2 -> .3 -> active
     *
     * Otherwise use conventional rollover semantics:
     *
     *     .3 -> .2 -> .1 -> active
     */
    const RotatedSourceOrderDirection
        suggestedDirection =
        lowestTimestamp.value()
                < highestTimestamp.value()
            ? RotatedSourceOrderDirection::
            Ascending
            : RotatedSourceOrderDirection::
            Descending;

    if (rotatedSourceRuleState
            .numericOrderDirection
        == suggestedDirection) {
        return;
    }

    rotatedSourceRuleState
        .numericOrderDirection =
        suggestedDirection;

    /*
     * Re-run discovery so rotatedSourceMatches now
     * reflects the newly inferred chronological order.
     */
    const RotatedSourceDiscoveryResult discovery =
        RotatedSourceDiscoveryService::discover(
            selectedFilePath(),
            rotatedSourceRuleState
            );

    if (!discovery.succeeded) {
        return;
    }

    rotatedSourceMatches =
        discovery.rotatedSources;

    updateRotatedSourceControls();
}

bool ImportConfigurationDialog::
    previewIncludesRotations() const
{
    return includeRotatedSourcesCheckBox
               ->isChecked()
           && !rotatedSourceMatches
                   .isEmpty();
}

QStringList ImportConfigurationDialog::
    previewSourcePaths() const
{
    const QFileInfo activeInfo(
        selectedFilePath()
        );

    if (!activeInfo.exists()
        || !activeInfo.isFile()) {
        return {};
    }

    QStringList paths;

    if (previewIncludesRotations()) {
        for (const RotatedSourceMatch &match
             : rotatedSourceMatches) {
            paths.append(
                QFileInfo(
                    match.filePath
                    )
                    .absoluteFilePath()
                );
        }
    }

    paths.append(
        activeInfo.absoluteFilePath()
        );

    return paths;
}

QString ImportConfigurationDialog::
    previewSelectionKey() const
{
    const QStringList paths =
        previewSourcePaths();

    if (paths.isEmpty()) {
        return {};
    }

    return (
        previewIncludesRotations()
            ? QStringLiteral("family:")
            : QStringLiteral("single:")
        )
        + paths.join(
            QChar(0x1f)
            );
}

void ImportConfigurationDialog::
    updateSourceState()
{
    cancelManualPreview();

    updateFormatSuggestion();
    updateRotatedSourceState();

    const QFileInfo fileInfo(
        selectedFilePath()
        );

    const bool sourceIsValid =
        fileInfo.exists()
        && fileInfo.isFile();

    if (sourceIsValid
        && !profileIsUserConfigured
        && customFieldDetectionSourceKey
               != previewSelectionKey()) {
        createProfileFromSource(
            false
            );

        refineSuggestedNumericRotationDirectionFromTimestamps();

        updateImportAvailability();
        return;
    }

    refineSuggestedNumericRotationDirectionFromTimestamps();

    previewRefreshTimer->stop();

    updatePreview();
    updateImportAvailability();
}

void ImportConfigurationDialog::
    detectCustomFieldMappings()
{
    const QStringList sourcePaths =
        previewSourcePaths();

    if (sourcePaths.isEmpty()) {
        return;
    }

    const QString sourceKey =
        previewSelectionKey();

    if (requiresManualStructuredDocumentPreview(
            sourcePaths,
            workingProfile
            )) {
        return;
    }

    if (customFieldDetectionSourceKey
        == sourceKey) {
        return;
    }

    ImportProfile detectionProfile =
        workingProfile;

    detectionProfile.customFields =
        retainedCustomMappings(
            detectionProfile,
            autoDetectedCustomFieldKeys
            );

    detectionProfile.preserveUnmappedFields =
        true;

    const ImportPreviewResult preview =
        previewService.previewFiles(
            sourcePaths,
            detectionProfile
            );

    if (!preview.canDisplayPreview()) {
        return;
    }

    applyDetectedCustomFieldMappings(
        preview
        );

    customFieldDetectionSourceKey =
        sourceKey;
}

void ImportConfigurationDialog::
    updateImportAvailability()
{
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    const bool sourceIsValid =
        fileInfo.exists()
        && fileInfo.isFile();

    newProfileFromSourceButton
        ->setEnabled(
            sourceIsValid
            );

    const bool profileIsValid =
        profileValidator
            .validate(
                workingProfile
                )
            .isValid();

    saveProfileButton->setEnabled(
        profileIsValid
        );

    importButton->setEnabled(
        sourceIsValid
        && profileIsValid
        );
}

void ImportConfigurationDialog::
    updateFormatSuggestion()
{
    const QString filePath =
        selectedFilePath();

    if (filePath.isEmpty()) {
        formatSuggestionLabel
            ->setText(
                tr(
                    "Select a source file."
                    )
                );

        return;
    }

    const ImportFormatSuggestion suggestion =
        formatSuggestionService
            .suggestForFile(
                filePath
                );

    if (!suggestion.hasSuggestion()) {
        formatSuggestionLabel
            ->setText(
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

void ImportConfigurationDialog::
    clearPreview(
        const QString &message
        )
{
    previewTable->setRowCount(
        0
        );

    /*
     * Never leave a stale family-only Source column
     * behind after rotations are turned off.
     */
    if (!previewIncludesRotations()
        && previewTable->columnCount()
               > 0) {
        QTableWidgetItem *firstHeader =
            previewTable
                ->horizontalHeaderItem(
                    0
                    );

        if (firstHeader != nullptr
            && firstHeader->text()
                   == tr("Source")) {
            previewTable->removeColumn(
                0
                );
        }
    }

    previewSummaryLabel->setText(
        message
        );

    rawSourcePreview->clear();

    previewSourceKey.clear();
}

void ImportConfigurationDialog::
    updatePreview()
{
    const QFileInfo activeInfo(
        selectedFilePath()
        );

    if (selectedFilePath().isEmpty()) {
        clearPreview(
            tr(
                "Select a source file to preview "
                "mapped records."
                )
            );

        return;
    }

    if (!activeInfo.exists()
        || !activeInfo.isFile()) {
        clearPreview(
            tr(
                "The selected source file does not "
                "exist or is not a file."
                )
            );

        return;
    }

    const QStringList sourcePaths =
        previewSourcePaths();

    const QString sourceKey =
        previewSelectionKey();

    const bool showSourceColumn =
        previewIncludesRotations();

    const int physicalSourceCount =
        sourcePaths.size();

    const ProfileValidationResult validation =
        profileValidator.validate(
            workingProfile
            );

    if (!validation.isValid()) {
        if (previewSourceKey
                == sourceKey
            && previewTable->rowCount()
                   > 0) {
            previewSummaryLabel->setText(
                tr(
                    "Profile configuration is currently "
                    "invalid. The preview below shows the "
                    "last valid configuration and will "
                    "refresh when the profile is valid again."
                    )
                );
        } else {
            clearPreview(
                tr(
                    "Fix the profile validation errors "
                    "to preview mapped records."
                    )
                );
        }

        return;
    }

    const bool requiresManualPreview =
        requiresManualStructuredDocumentPreview(
            sourcePaths,
            workingProfile
            );

    refreshPreviewButton->setVisible(
        requiresManualPreview
        );

    if (requiresManualPreview) {
        clearPreview(
            showSourceColumn
                ? tr(
                      "Automatic preview is disabled because "
                      "at least one selected structured source "
                      "is large. Click Refresh Preview to "
                      "generate up to %1 processed records "
                      "across the selected source family."
                      )
                      .arg(
                          ImportPreviewService::
                              DefaultMaxProcessedRecords
                          )
                : tr(
                      "Automatic preview is disabled for "
                      "large structured documents to keep the "
                      "Import Configuration interface responsive. "
                      "Click Refresh Preview to generate the "
                      "first %1 processed records in the background."
                      )
                      .arg(
                          ImportPreviewService::
                              DefaultMaxProcessedRecords
                          )
            );

        return;
    }

    const ImportPreviewResult preview =
        previewService.previewFiles(
            sourcePaths,
            workingProfile
            );

    displayPreviewResult(
        sourceKey,
        preview,
        showSourceColumn,
        physicalSourceCount
        );
}

void ImportConfigurationDialog::
    updateRawSourcePreview(
        int row
        )
{
    if (row < 0
        || row >= previewTable->rowCount()) {
        rawSourcePreview->clear();
        return;
    }

    QTableWidgetItem *item =
        previewTable->item(
            row,
            0
            );

    if (item == nullptr) {
        rawSourcePreview->clear();
        return;
    }

    rawSourcePreview->setPlainText(
        item->data(
                Qt::UserRole
                )
            .toString()
        );
}

void ImportConfigurationDialog::
    dragEnterEvent(
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

void ImportConfigurationDialog::
    dropEvent(
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

void ImportConfigurationDialog::
    populateProfileControls()
{
    const QSignalBlocker profileNameBlocker(
        profileNameEdit
        );

    const QSignalBlocker recordPathBlocker(
        recordPathEdit
        );

    const QSignalBlocker importerBlocker(
        importerComboBox
        );

    const QSignalBlocker regexPatternBlocker(
        regexPatternEdit
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

    const int importerIndex =
        importerComboBox->findData(
            workingProfile.importerId
            );

    importerComboBox->setCurrentIndex(
        importerIndex >= 0
            ? importerIndex
            : 0
        );

    regexPatternEdit->setPlainText(
        workingProfile.regexPattern
        );

    recordPathEdit->setText(
        workingProfile.recordPath
        );

    updateFormatSpecificControls();

    preserveUnmappedCheckBox->setChecked(
        workingProfile
            .preserveUnmappedFields
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

void ImportConfigurationDialog::
    populateCustomFieldMappings()
{
    const QSignalBlocker blocker(
        customFieldTable
        );

    customFieldTable->setRowCount(
        0
        );

    for (const CustomFieldMapping &mapping
         : std::as_const(workingProfile.customFields)) {
        const int row =
            customFieldTable->rowCount();

        customFieldTable->insertRow(
            row
            );

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

void ImportConfigurationDialog::
    addCustomFieldMapping()
{
    QSignalBlocker blocker(
        customFieldTable
        );

    const int row =
        customFieldTable->rowCount();

    customFieldTable->insertRow(
        row
        );

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

void ImportConfigurationDialog::
    removeSelectedCustomFieldMapping()
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

        customFieldTable->removeRow(
            row
            );
    }

    updateCustomFieldMappings();
}

void ImportConfigurationDialog::
    updateCustomFieldMappings()
{
    QList<CustomFieldMapping> mappings;

    for (int row = 0;
         row < customFieldTable->rowCount();
         ++row) {
        CustomFieldMapping mapping;

        if (QTableWidgetItem *item =
                customFieldTable->item(
                    row,
                    0
                    )) {
            mapping.name =
                item->text();
        }

        if (QTableWidgetItem *item =
                customFieldTable->item(
                    row,
                    1
                    )) {
            mapping.sourcePath =
                item->text();
        }

        mappings.append(
            std::move(mapping)
            );
    }

    workingProfile.customFields =
        std::move(mappings);

    updateValidationState();
}

void ImportConfigurationDialog::
    updateWorkingProfile()
{
    workingProfile.name =
        profileNameEdit->text();

    workingProfile.importerId =
        importerComboBox
            ->currentData()
            .toString();

    workingProfile.regexPattern =
        regexPatternEdit
            ->toPlainText();

    workingProfile.recordPath =
        recordPathEdit->text();

    workingProfile.preserveUnmappedFields =
        preserveUnmappedCheckBox
            ->isChecked();

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

void ImportConfigurationDialog::
    updateValidationState(
        bool refreshPreview
        )
{
    const ProfileValidationResult result =
        profileValidator.validate(
            workingProfile
            );

    if (result.issues.isEmpty()) {
        validationLabel->setText(
            tr(
                "Profile configuration is valid."
                )
            );

        if (refreshPreview) {
            schedulePreviewRefresh();
        }

        updateImportAvailability();
        return;
    }

    QStringList lines;

    for (const ProfileValidationIssue &issue
         : result.issues) {
        const QString prefix =
            issue.severity
                    == ProfileValidationSeverity::
                        Error
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

    if (refreshPreview) {
        schedulePreviewRefresh();
    }

    updateImportAvailability();
}

QComboBox *
ImportConfigurationDialog::
    createSeverityCombo(
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
            recordSeverityToString(
                value
                ),
            static_cast<int>(
                value
                )
            );
    }

    const int index =
        combo->findData(
            static_cast<int>(
                severity
                )
            );

    if (index >= 0) {
        combo->setCurrentIndex(
            index
            );
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

void ImportConfigurationDialog::
    populateSeverityAliases()
{
    const QSignalBlocker blocker(
        severityAliasTable
        );

    severityAliasTable->setRowCount(
        0
        );

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

        severityAliasTable->insertRow(
            row
            );

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

void ImportConfigurationDialog::
    addSeverityAlias()
{
    const QSignalBlocker blocker(
        severityAliasTable
        );

    const int row =
        severityAliasTable->rowCount();

    severityAliasTable->insertRow(
        row
        );

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

void ImportConfigurationDialog::
    removeSelectedSeverityAlias()
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

        severityAliasTable->removeRow(
            row
            );
    }

    updateSeverityAliases();
}

void ImportConfigurationDialog::
    updateSeverityAliases()
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

QComboBox *
ImportConfigurationDialog::
    createTimestampRuleTypeCombo(
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
            static_cast<int>(
                type
                )
            );

    if (index >= 0) {
        combo->setCurrentIndex(
            index
            );
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

void ImportConfigurationDialog::
    populateTimestampRules()
{
    const QSignalBlocker blocker(
        timestampRuleTable
        );

    timestampRuleTable->setRowCount(
        0
        );

    for (const TimestampRule &rule
         : std::as_const(workingProfile.timestampRules)) {
        const int row =
            timestampRuleTable->rowCount();

        timestampRuleTable->insertRow(
            row
            );

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

void ImportConfigurationDialog::
    addTimestampRule()
{
    const QSignalBlocker blocker(
        timestampRuleTable
        );

    const int row =
        timestampRuleTable->rowCount();

    timestampRuleTable->insertRow(
        row
        );

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

void ImportConfigurationDialog::
    removeSelectedTimestampRule()
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

        timestampRuleTable->removeRow(
            row
            );
    }

    updateTimestampRules();
}

void ImportConfigurationDialog::
    updateTimestampRules()
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

void ImportConfigurationDialog::
    saveProfile()
{
    const ProfileValidationResult validation =
        profileValidator.validate(
            workingProfile
            );

    if (!validation.isValid()) {
        QMessageBox::warning(
            this,
            tr("Cannot Save Profile"),
            tr(
                "Fix the profile validation errors "
                "before saving."
                )
            );

        return;
    }

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            tr("Save Import Profile"),
            QStringLiteral(
                "import-profile.json"
                ),
            tr(
                "TraceScope Import Profiles (*.json);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    if (QFileInfo(filePath)
            .suffix()
            .isEmpty()) {
        filePath +=
            QStringLiteral(".json");
    }

    const QByteArray serializedProfile =
        profileSerializer.serialize(
            workingProfile
            );

    QSaveFile file(
        filePath
        );

    if (!file.open(
            QIODevice::WriteOnly
            )) {
        QMessageBox::critical(
            this,
            tr("Save Profile Failed"),
            tr(
                "The import profile could not be "
                "opened for writing:\n%1"
                )
                .arg(
                    file.errorString()
                    )
            );

        return;
    }

    const qint64 bytesWritten =
        file.write(
            serializedProfile
            );

    if (bytesWritten
        != serializedProfile.size()) {
        file.cancelWriting();

        QMessageBox::critical(
            this,
            tr("Save Profile Failed"),
            tr(
                "The complete import profile could "
                "not be written:\n%1"
                )
                .arg(
                    file.errorString()
                    )
            );

        return;
    }

    if (!file.commit()) {
        QMessageBox::critical(
            this,
            tr("Save Profile Failed"),
            tr(
                "The import profile could not be "
                "committed to disk:\n%1"
                )
                .arg(
                    file.errorString()
                    )
            );

        return;
    }

    if (recentItemsStore != nullptr) {
        recentItemsStore->addRecentProfile(
            filePath
            );

        refreshRecentProfilesMenu();
    }

    QMessageBox::information(
        this,
        tr("Profile Saved"),
        tr(
            "The import profile was saved successfully."
            )
        );
}

void ImportConfigurationDialog::
    loadProfile()
{
    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            tr("Load Import Profile"),
            QString(),
            tr(
                "TraceScope Import Profiles (*.json);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    loadProfileFromPath(
        filePath
        );
}

void ImportConfigurationDialog::
    loadProfileFromPath(
        const QString &filePath
        )
{
    QFile file(
        filePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        QMessageBox::critical(
            this,
            tr("Load Profile Failed"),
            tr(
                "The import profile could not be "
                "opened:\n%1"
                )
                .arg(
                    file.errorString()
                    )
            );

        return;
    }

    const QByteArray json =
        file.readAll();

    if (file.error()
        != QFileDevice::NoError) {
        QMessageBox::critical(
            this,
            tr("Load Profile Failed"),
            tr(
                "The import profile could not be "
                "read completely:\n%1"
                )
                .arg(
                    file.errorString()
                    )
            );

        return;
    }

    const ProfileDeserializationResult result =
        profileSerializer.deserialize(
            json
            );

    if (!result.isSuccess()) {
        QMessageBox::critical(
            this,
            tr("Invalid Import Profile"),
            tr(
                "The selected file is not a valid "
                "TraceScope import profile.\n\n%1"
                )
                .arg(
                    result.errorMessage
                    )
            );

        return;
    }

    const ImportProfile loadedProfile =
        result.profile.value();

    const ProfileValidationResult validation =
        profileValidator.validate(
            loadedProfile
            );

    if (!validation.isValid()) {
        QStringList messages;

        for (const ProfileValidationIssue &issue
             : validation.issues) {
            if (issue.severity
                != ProfileValidationSeverity::
                    Error) {
                continue;
            }

            messages.append(
                QStringLiteral("• %1")
                    .arg(
                        issue.message
                        )
                );
        }

        QMessageBox::critical(
            this,
            tr("Invalid Import Profile"),
            tr(
                "The selected profile contains "
                "configuration errors:\n\n%1"
                )
                .arg(
                    messages.join(
                        QLatin1Char('\n')
                        )
                    )
            );

        return;
    }

    const ImporterRegistry registry =
        createBuiltInImporterRegistry(
            loadedProfile
            );

    if (!registry.importerById(
            loadedProfile.importerId
            )) {
        QMessageBox::critical(
            this,
            tr("Unsupported Import Profile"),
            tr(
                "This version of TraceScope does not "
                "support the importer '%1'."
                )
                .arg(
                    loadedProfile.importerId
                    )
            );

        return;
    }

    cancelManualPreview();

    workingProfile =
        loadedProfile;

    autoDetectedCustomFieldKeys.clear();

    profileIsUserConfigured =
        true;

    customFieldDetectionSourceKey =
        previewSelectionKey();

    previewRefreshTimer->stop();

    populateProfileControls();

    updateValidationState(
        false
        );

    updatePreview();
    updateImportAvailability();

    if (recentItemsStore != nullptr) {
        recentItemsStore->addRecentProfile(
            filePath
            );

        refreshRecentProfilesMenu();
    }
}

void ImportConfigurationDialog::
    schedulePreviewRefresh()
{
    cancelManualPreview();
    previewRefreshTimer->start();
}

void ImportConfigurationDialog::
    createProfileFromSource(
        bool userInitiated
        )
{
    const QString filePath =
        selectedFilePath();

    const QFileInfo fileInfo(
        filePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        if (userInitiated) {
            QMessageBox::warning(
                this,
                tr("No Source File"),
                tr(
                    "Select a valid source file before "
                    "creating a profile from it."
                    )
                );
        }

        return;
    }

    if (userInitiated
        && profileIsUserConfigured) {
        const auto response =
            QMessageBox::question(
                this,
                tr("Create New Profile"),
                tr(
                    "Creating a new profile from the "
                    "selected source will replace the "
                    "current profile configuration.\n\n"
                    "Continue?"
                    ),
                QMessageBox::Yes
                    | QMessageBox::No,
                QMessageBox::No
                );

        if (response
            != QMessageBox::Yes) {
            return;
        }
    }

    workingProfile =
        ImportProfile();

    autoDetectedCustomFieldKeys.clear();

    const ImportFormatSuggestion suggestion =
        formatSuggestionService
            .suggestForFile(
                filePath
                );

    if (suggestion.hasSuggestion()) {
        const std::optional<ImportProfile>
            preset =
                builtInImportProfilePreset(
                    suggestion.profilePresetId
                    );

        if (preset.has_value()) {
            workingProfile =
                preset.value();
        } else {
            workingProfile.importerId =
                suggestion.importerId;

            workingProfile.name =
                QStringLiteral(
                    "Default %1"
                    )
                    .arg(
                        suggestion.displayName
                        );
        }
    } else {
        workingProfile.importerId.clear();

        workingProfile.name =
            QStringLiteral(
                "New Import Profile"
                );
    }

    profileIsUserConfigured =
        false;

    customFieldDetectionSourceKey.clear();

    populateProfileControls();

    if (!workingProfile.importerId
             .isEmpty()
        && workingProfile.importerId
               != QStringLiteral(
                   "structured-json"
                   )
        && workingProfile.importerId
               != QStringLiteral(
                   "xml"
                   )
        && workingProfile.importerId
               != QStringLiteral(
                   "regex-text"
                   )) {
        detectCustomFieldMappings();
    }

    previewRefreshTimer->stop();

    updateValidationState(
        false
        );

    updatePreview();

    if (userInitiated) {
        profileIsUserConfigured =
            true;
    }
}

void ImportConfigurationDialog::
    populateImporterOptions()
{
    const QSignalBlocker blocker(
        importerComboBox
        );

    importerComboBox->clear();

    importerComboBox->addItem(
        tr("Select format..."),
        QString()
        );

    const ImporterRegistry registry =
        createBuiltInImporterRegistry(
            workingProfile
            );

    const QVector<
        std::shared_ptr<ILogImporter>
        > importers =
        registry.importers();

    for (
        const std::shared_ptr<ILogImporter>
            &importer
        : importers
        ) {
        importerComboBox->addItem(
            importer->displayName(),
            importer->id()
            );
    }
}

void ImportConfigurationDialog::
    updateFormatSpecificControls()
{
    const QString importerId =
        importerComboBox
            ->currentData()
            .toString();

    recordPathEdit->setEnabled(
        importerId
            == QStringLiteral(
                "structured-json"
                )
        || importerId
               == QStringLiteral(
                   "xml"
                   )
        );

    regexPatternEdit->setEnabled(
        importerId
        == QStringLiteral(
            "regex-text"
            )
        );
}

void ImportConfigurationDialog::
    displayPreviewResult(
        const QString &sourceKey,
        const ImportPreviewResult &preview,
        bool showSourceColumn,
        int physicalSourceCount
        )
{
    if (!preview.errorMessage.isEmpty()) {
        clearPreview(
            tr(
                "Preview unavailable: %1"
                )
                .arg(
                    preview.errorMessage
                    )
            );

        return;
    }

    const auto &records =
        preview.importResult.records;

    previewSourceKey =
        sourceKey;

    const bool showTimestamp =
        !workingProfile
             .canonicalFields
             .timestampPath
             .trimmed()
             .isEmpty();

    const bool showSeverity =
        !workingProfile
             .canonicalFields
             .severityPath
             .trimmed()
             .isEmpty();

    const bool showSubsystem =
        !workingProfile
             .canonicalFields
             .subsystemPath
             .trimmed()
             .isEmpty();

    const bool showEventCode =
        !workingProfile
             .canonicalFields
             .eventCodePath
             .trimmed()
             .isEmpty();

    const bool showEntityId =
        !workingProfile
             .canonicalFields
             .entityIdPath
             .trimmed()
             .isEmpty();

    const bool showMessage =
        !workingProfile
             .canonicalFields
             .messagePath
             .trimmed()
             .isEmpty();

    QStringList headers;

    if (showSourceColumn) {
        headers.append(
            tr("Source")
            );
    }

    if (showTimestamp) {
        headers.append(
            tr("Timestamp")
            );
    }

    if (showSeverity) {
        headers.append(
            tr("Severity")
            );
    }

    if (showSubsystem) {
        headers.append(
            tr("Subsystem")
            );
    }

    if (showEventCode) {
        headers.append(
            tr("Event Code")
            );
    }

    if (showEntityId) {
        headers.append(
            tr("Entity ID")
            );
    }

    if (showMessage) {
        headers.append(
            tr("Message")
            );
    }

    QSet<QString>
        mappedCustomFieldNames;

    for (const CustomFieldMapping &mapping
         : std::as_const(workingProfile.customFields)) {
        headers.append(
            mapping.name
            );

        mappedCustomFieldNames.insert(
            mapping.name
            );
    }

    headers.append(
        tr("Unmapped Custom Fields")
        );

    const int previousColumnCount =
        previewTable->columnCount();

    previewTable->clearContents();

    previewTable->setColumnCount(
        headers.size()
        );

    previewTable->setHorizontalHeaderLabels(
        headers
        );

    QHeaderView *header =
        previewTable->horizontalHeader();

    header->setStretchLastSection(
        false
        );

    for (int column = 0;
         column < headers.size();
         ++column) {
        header->setSectionResizeMode(
            column,
            QHeaderView::Interactive
            );
    }

    if (previousColumnCount
        != headers.size()) {
        for (int column = 0;
             column < headers.size();
             ++column) {
            const QString label =
                headers.at(column);

            int width = 140;

            if (label == tr("Source")) {
                width = 180;
            } else if (
                label == tr("Timestamp")
                ) {
                width = 180;
            } else if (
                label == tr("Severity")
                ) {
                width = 80;
            } else if (
                label == tr("Subsystem")
                ) {
                width = 120;
            } else if (
                label == tr("Event Code")
                ) {
                width = 140;
            } else if (
                label == tr("Entity ID")
                ) {
                width = 120;
            } else if (
                label == tr("Message")
                ) {
                width = 300;
            } else if (
                label
                == tr(
                    "Unmapped Custom Fields"
                    )
                ) {
                width = 220;
            }

            previewTable->setColumnWidth(
                column,
                width
                );
        }
    }

    previewTable->setRowCount(
        records.size()
        );

    for (int row = 0;
         row < records.size();
         ++row) {
        const InvestigationRecord &record =
            records.at(row);

        QStringList values;

        if (showSourceColumn) {
            values.append(
                record.source.sourceName
                );
        }

        if (showTimestamp) {
            values.append(
                record.timestamp.has_value()
                    ? record.timestamp
                          ->toString(
                              Qt::ISODateWithMs
                              )
                    : QString()
                );
        }

        if (showSeverity) {
            values.append(
                record.severity.has_value()
                    ? recordSeverityToString(
                          record.severity.value()
                          )
                    : QString()
                );
        }

        if (showSubsystem) {
            values.append(
                record.subsystem.value_or(
                    QString()
                    )
                );
        }

        if (showEventCode) {
            values.append(
                record.eventCode.value_or(
                    QString()
                    )
                );
        }

        if (showEntityId) {
            values.append(
                record.entityId.value_or(
                    QString()
                    )
                );
        }

        if (showMessage) {
            values.append(
                record.message.value_or(
                    QString()
                    )
                );
        }

        for (const CustomFieldMapping &mapping
             : std::as_const(workingProfile.customFields)) {
            const auto iterator =
                record.customAttributes
                    .constFind(
                        mapping.name
                        );

            values.append(
                iterator
                        != record
                               .customAttributes
                               .constEnd()
                    ? iterator.value()
                          .toString()
                    : QString()
                );
        }

        QStringList unmappedValues;
        QStringList attributeNames =
            record.customAttributes.keys();

        attributeNames.sort(
            Qt::CaseInsensitive
            );

        for (const QString &name
             : std::as_const(attributeNames)) {
            if (mappedCustomFieldNames
                    .contains(name)) {
                continue;
            }

            unmappedValues.append(
                QStringLiteral("%1=%2")
                    .arg(
                        name,
                        record
                            .customAttributes
                            .value(name)
                            .toString()
                        )
                );
        }

        values.append(
            unmappedValues.join(
                QStringLiteral("; ")
                )
            );

        for (int column = 0;
             column < values.size();
             ++column) {
            auto *item =
                new QTableWidgetItem(
                    values.at(column)
                    );

            if (column == 0) {
                item->setData(
                    Qt::UserRole,
                    record.rawSource
                    );
            }

            previewTable->setItem(
                row,
                column,
                item
                );
        }
    }

    QString summary =
        tr(
            "Preview: %1 imported of %2 processed "
            "record(s); %3 skipped."
            )
            .arg(
                preview.importResult
                    .importedRecordCount()
                )
            .arg(
                preview.importResult
                    .processedRecordCount
                )
            .arg(
                preview.importResult
                    .skippedRecordCount()
                );

    if (showSourceColumn) {
        summary =
            tr(
                "Source family: %1 physical files. "
                )
                .arg(
                    physicalSourceCount
                    )
            + summary;
    }

    if (!records.isEmpty()) {
        previewTable->selectRow(
            0
            );

        updateRawSourcePreview(
            0
            );
    } else {
        rawSourcePreview->clear();
    }

    if (!preview.importResult
             .diagnostics
             .isEmpty()) {
        summary +=
            tr(
                " %1 diagnostic(s)."
                )
                .arg(
                    preview
                        .importResult
                        .diagnostics
                        .size()
                    );
    }

    if (preview.sourceTruncated) {
        if (showSourceColumn) {
            summary +=
                tr(
                    " Preview is limited to %1 "
                    "processed records distributed "
                    "across the selected sources."
                    )
                    .arg(
                        ImportPreviewService::
                            DefaultMaxProcessedRecords
                        );
        } else {
            summary +=
                tr(
                    " Preview is limited to the first "
                    "%1 processed records."
                    )
                    .arg(
                        ImportPreviewService::
                            DefaultMaxProcessedRecords
                        );
        }
    }

    previewSummaryLabel->setText(
        summary
        );
}

void ImportConfigurationDialog::
    applyDetectedCustomFieldMappings(
        const ImportPreviewResult &preview
        )
{
    if (!preview.canDisplayPreview()) {
        return;
    }

    QList<CustomFieldMapping>
        retainedMappings =
            retainedCustomMappings(
                workingProfile,
                autoDetectedCustomFieldKeys
                );

    QSet<QString>
        explicitlyMappedNames;

    for (const CustomFieldMapping &mapping
         : std::as_const(retainedMappings)) {
        explicitlyMappedNames.insert(
            mapping.name
                .trimmed()
                .toCaseFolded()
            );
    }

    QSet<QString> detectedFields;

    for (const InvestigationRecord &record
         : preview.importResult.records) {
        for (auto iterator =
                 record.customAttributes
                     .constBegin();
             iterator !=
                 record.customAttributes
                     .constEnd();
             ++iterator) {
            const QString fieldName =
                iterator.key()
                    .trimmed();

            if (fieldName.isEmpty()
                || explicitlyMappedNames
                       .contains(
                           fieldName
                               .toCaseFolded()
                           )) {
                continue;
            }

            detectedFields.insert(
                fieldName
                );
        }
    }

    QStringList sortedFields =
        detectedFields.values();

    sortedFields.sort(
        Qt::CaseInsensitive
        );

    QList<CustomFieldMapping>
        updatedMappings =
            retainedMappings;

    QSet<QString>
        updatedAutoDetectedKeys;

    for (const QString &fieldName
         : std::as_const(sortedFields)) {
        bool alreadyMapped = false;

        for (const CustomFieldMapping &mapping
             : updatedMappings) {
            if (mapping.sourcePath
                    .trimmed()
                    .compare(
                        fieldName,
                        Qt::CaseInsensitive
                        )
                == 0) {
                alreadyMapped = true;
                break;
            }
        }

        if (alreadyMapped) {
            continue;
        }

        const CustomFieldMapping mapping {
            fieldName,
            fieldName
        };

        updatedMappings.append(
            mapping
            );

        updatedAutoDetectedKeys.insert(
            customFieldMappingKey(
                mapping
                )
            );
    }

    workingProfile.customFields =
        std::move(
            updatedMappings
            );

    autoDetectedCustomFieldKeys =
        std::move(
            updatedAutoDetectedKeys
            );

    populateCustomFieldMappings();
    updateValidationState(
        false
        );
}

void ImportConfigurationDialog::
    startManualPreview()
{
    if (previewWatcher != nullptr) {
        return;
    }

    const QStringList sourcePaths =
        previewSourcePaths();

    if (sourcePaths.isEmpty()) {
        return;
    }

    const QString sourceKey =
        previewSelectionKey();

    const bool showSourceColumn =
        previewIncludesRotations();

    const int physicalSourceCount =
        sourcePaths.size();

    const ProfileValidationResult validation =
        profileValidator.validate(
            workingProfile
            );

    if (!validation.isValid()) {
        return;
    }

    ImportProfile previewProfile =
        workingProfile;

    const bool detectCustomFields =
        customFieldDetectionSourceKey
        != sourceKey;

    if (detectCustomFields) {
        previewProfile.customFields =
            retainedCustomMappings(
                previewProfile,
                autoDetectedCustomFieldKeys
                );

        previewProfile.preserveUnmappedFields =
            true;
    }

    const QByteArray profileSnapshot =
        profileSerializer.serialize(
            workingProfile
            );

    previewRefreshTimer->stop();

    auto *watcher =
        new QFutureWatcher<
            ImportPreviewResult
            >(
            this
            );

    previewWatcher =
        watcher;

    refreshPreviewButton->setEnabled(
        false
        );

    refreshPreviewButton->setText(
        tr("Generating...")
        );

    previewSummaryLabel->setText(
        tr(
            "Generating preview in the background. "
            "The import configuration remains usable."
            )
        );

    connect(
        watcher,
        &QFutureWatcher<
            ImportPreviewResult
            >::finished,
        this,
        [
            this,
            watcher,
            sourceKey,
            showSourceColumn,
            physicalSourceCount,
            profileSnapshot,
            detectCustomFields
        ]() {
            const bool cancelled =
                watcher->isCanceled();

            if (previewWatcher
                == watcher) {
                previewWatcher =
                    nullptr;
            }

            watcher->deleteLater();

            refreshPreviewButton
                ->setEnabled(
                    true
                    );

            refreshPreviewButton->setText(
                tr("Refresh Preview")
                );

            if (cancelled) {
                updatePreview();
                return;
            }

            const ImportPreviewResult preview =
                watcher->result();

            if (previewSelectionKey()
                    != sourceKey
                || profileSerializer.serialize(
                       workingProfile
                       )
                       != profileSnapshot) {
                updatePreview();
                return;
            }

            if (detectCustomFields
                && preview.canDisplayPreview()) {
                applyDetectedCustomFieldMappings(
                    preview
                    );

                customFieldDetectionSourceKey =
                    sourceKey;
            }

            displayPreviewResult(
                sourceKey,
                preview,
                showSourceColumn,
                physicalSourceCount
                );
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                sourcePaths,
                previewProfile
            ](
                QPromise<ImportPreviewResult>
                    &promise
                ) {
                ImportExecutionContext
                    executionContext;

                executionContext
                    .isCancellationRequested =
                    [&promise]() {
                        return promise.isCanceled();
                    };

                ImportPreviewResult preview =
                    ImportPreviewService()
                        .previewFiles(
                            sourcePaths,
                            previewProfile,
                            ImportPreviewService::
                                DefaultMaxProcessedRecords,
                            executionContext
                            );

                if (promise.isCanceled()
                    || preview
                           .importResult
                           .cancelled) {
                    return;
                }

                promise.addResult(
                    std::move(preview)
                    );
            }
            )
        );
}

void ImportConfigurationDialog::
    cancelManualPreview()
{
    if (previewWatcher == nullptr) {
        return;
    }

    previewWatcher->cancel();
}

void ImportConfigurationDialog::
    refreshRecentProfilesMenu()
{
    recentProfilesMenu->clear();

    if (recentItemsStore == nullptr) {
        recentProfilesButton->setEnabled(
            false
            );

        return;
    }

    const QStringList profiles =
        recentItemsStore
            ->recentProfiles();

    int validItemCount = 0;

    for (const QString &filePath
         : profiles) {
        const QFileInfo fileInfo(
            filePath
            );

        if (!fileInfo.exists()
            || !fileInfo.isFile()) {
            recentItemsStore
                ->removeRecentProfile(
                    filePath
                    );

            continue;
        }

        QAction *action =
            recentProfilesMenu
                ->addAction(
                    fileInfo.fileName()
                    );

        action->setToolTip(
            filePath
            );

        connect(
            action,
            &QAction::triggered,
            this,
            [this, filePath]() {
                loadProfileFromPath(
                    filePath
                    );
            }
            );

        ++validItemCount;
    }

    recentProfilesButton->setEnabled(
        validItemCount > 0
        );
}
