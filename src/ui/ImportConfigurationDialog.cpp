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
#include <QPlainTextEdit>
#include <QSplitter>
#include <QSet>
#include <QMessageBox>
#include <QSaveFile>
#include <QTimer>
#include <QFile>
#include <QtConcurrentRun>
#include <QPromise>

#include <memory>
#include <utility>

#include "../importing/BuiltInImporterRegistry.h"
#include "../importing/BuiltInImportProfilePresets.h"
#include "../importing/ILogImporter.h"

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
            == QStringLiteral(
                "structured-json"
                )
        || profile.importerId
               == QStringLiteral(
                   "xml"
                   );

    return isStructuredDocument
           && fileInfo.size()
                  > AutomaticStructuredDocumentPreviewMaxBytes;
}
}

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
    previewSummaryLabel(
        new QLabel(this)
        ),
    previewTable(
        new QTableWidget(
            0,
            8,
            this
            )
        ),
    previewRefreshTimer(
        new QTimer(this)
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
        QStringLiteral("Default JSON Lines");

    previewRefreshTimer->setSingleShot(true);
    previewRefreshTimer->setInterval(250);

    connect(
        previewRefreshTimer,
        &QTimer::timeout,
        this,
        [this]() {
            if (workingProfile.importerId
                    == QStringLiteral(
                        "regex-text"
                        )
                && customFieldDetectionSourcePath
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

    regexPatternEdit->setFixedHeight(70);

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

    profileButtonLayout->addWidget(
        saveProfileButton
        );

    profileButtonLayout->addStretch();

    profileLayout->addRow(
        profileButtonLayout
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

    auto *previewGroup =
        new QGroupBox(
            tr("Source Record Preview"),
            this
            );

    auto *previewLayout =
        new QVBoxLayout(previewGroup);

    previewSummaryLabel->setWordWrap(true);

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

    previewTable->setColumnCount(7);

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

    previewTable->setWordWrap(false);

    QHeaderView *previewHeader =
        previewTable->horizontalHeader();

    previewHeader->setSectionResizeMode(
        QHeaderView::Interactive
        );

    previewHeader->setMinimumSectionSize(70);

    previewTable->setColumnWidth(0, 180); // Timestamp
    previewTable->setColumnWidth(1, 80);  // Severity
    previewTable->setColumnWidth(2, 120); // Subsystem
    previewTable->setColumnWidth(3, 140); // Event Code
    previewTable->setColumnWidth(4, 120); // Entity ID
    previewTable->setColumnWidth(5, 300); // Message
    previewTable->setColumnWidth(6, 220); // Unmapped Custom Fields

    /*
 * Let the record table and selected raw-source
 * view share the available preview space through
 * a vertical splitter. The raw-source section
 * starts compact but can be expanded for
 * multiline JSON, XML, stack traces, and similar
 * records.
 */
    auto *previewSplitter =
        new QSplitter(
            Qt::Vertical,
            previewGroup
            );

    previewSplitter->setChildrenCollapsible(
        false
        );

    /*
 * The table should receive additional space when
 * the dialog grows. The raw-source panel keeps
 * its user-selected size unless the splitter is
 * moved explicitly.
 */
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

    /*
     * Keep the default appearance close to the
     * existing compact raw-source preview. The user
     * can drag the splitter upward whenever more
     * vertical space is useful.
     */
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
        importerComboBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            profileIsUserConfigured = true;

            workingProfile.importerId =
                importerComboBox
                    ->currentData()
                    .toString();

            /*
         * Automatically detected mappings belong
         * to the previous interpretation of the
         * source, so discard only those mappings.
         */
            QList<CustomFieldMapping>
                retainedMappings;

            for (const CustomFieldMapping &mapping
                 : std::as_const(
                     workingProfile.customFields
                     )) {
                if (autoDetectedCustomFieldKeys
                        .contains(
                            customFieldMappingKey(
                                mapping
                                )
                            )) {
                    continue;
                }

                retainedMappings.append(
                    mapping
                    );
            }

            workingProfile.customFields =
                std::move(
                    retainedMappings
                    );

            autoDetectedCustomFieldKeys.clear();
            customFieldDetectionSourcePath.clear();

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

            /*
         * A changed pattern defines a different
         * source-field set.
         */
            customFieldDetectionSourcePath.clear();

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
        [this](
            int,
            int
            ) {
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
        [this](
            int,
            int
            ) {
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
        [this](
            int,
            int
            ) {
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

            // A changed structured-document record path
            // selects a different logical record set,
            // so previously cached source detection
            // is no longer sufficient.
            customFieldDetectionSourcePath.clear();

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

    const QList<QLineEdit *> previewRelevantProfileEdits {
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

            updateValidationState(false);
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
}

void ImportConfigurationDialog::browseForFile()
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

    setSelectedFilePath(filePath);
}

void ImportConfigurationDialog::updateSourceState()
{
    cancelManualPreview();
    updateFormatSuggestion();

    const QString filePath =
        selectedFilePath();

    const QFileInfo fileInfo(filePath);

    const bool sourceIsValid =
        fileInfo.exists()
        && fileInfo.isFile();

    if (sourceIsValid
        && !profileIsUserConfigured
        && customFieldDetectionSourcePath
               != filePath) {
        createProfileFromSource(false);
        updateImportAvailability();
        return;
    }

    previewRefreshTimer->stop();

    updatePreview();
    updateImportAvailability();
}

void ImportConfigurationDialog::detectCustomFieldMappings()
{
    const QString filePath =
        selectedFilePath();

    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return;
    }

    if (requiresManualStructuredDocumentPreview(
            fileInfo,
            workingProfile
            )) {
        return;
    }

    if (customFieldDetectionSourcePath
        == filePath) {
        return;
    }

    /*
     * Build the detection profile without mappings
     * that TraceScope generated automatically during
     * the previous detection pass.
     *
     * Explicit/user-edited mappings remain in place.
     */
    ImportProfile detectionProfile =
        workingProfile;

    QList<CustomFieldMapping>
        retainedMappings;

    for (const CustomFieldMapping &mapping
         : std::as_const(
             detectionProfile.customFields
             )) {
        if (autoDetectedCustomFieldKeys.contains(
                customFieldMappingKey(
                    mapping
                    )
                )) {
            continue;
        }

        retainedMappings.append(
            mapping
            );
    }

    detectionProfile.customFields =
        retainedMappings;

    detectionProfile.preserveUnmappedFields =
        true;

    const ImportPreviewResult preview =
        previewService.previewFile(
            filePath,
            detectionProfile
            );

    /*
     * Do not modify the working profile unless the
     * new logical record set could actually be
     * previewed.
     */
    if (!preview.canDisplayPreview()) {
        return;
    }

    QSet<QString> explicitlyMappedNames;

    for (const CustomFieldMapping &mapping
         : std::as_const(
             retainedMappings
             )) {
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
                iterator.key().trimmed();

            if (fieldName.isEmpty()) {
                continue;
            }

            if (explicitlyMappedNames.contains(
                    fieldName.toCaseFolded()
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
         : std::as_const(
             sortedFields
             )) {
        bool alreadyMapped = false;

        for (const CustomFieldMapping &mapping
             : std::as_const(
                 updatedMappings
                 )) {
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

    customFieldDetectionSourcePath =
        filePath;

    populateCustomFieldMappings();
    updateValidationState(false);
}

void ImportConfigurationDialog::updateImportAvailability()
{
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    const bool sourceIsValid =
        fileInfo.exists()
        && fileInfo.isFile();

    newProfileFromSourceButton->setEnabled(
        sourceIsValid
        );

    const bool profileIsValid =
        profileValidator
            .validate(workingProfile)
            .isValid();

    saveProfileButton->setEnabled(
        profileIsValid
        );

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

void ImportConfigurationDialog::clearPreview(
    const QString &message
    )
{
    previewTable->setRowCount(0);

    previewSummaryLabel->setText(
        message
        );

    rawSourcePreview->clear();

    previewSourcePath.clear();
}

void ImportConfigurationDialog::updatePreview()
{
    const QString filePath =
        selectedFilePath();

    if (filePath.isEmpty()) {
        clearPreview(
            tr(
                "Select a source file to preview "
                "mapped records."
                )
            );

        return;
    }

    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        clearPreview(
            tr(
                "The selected source file does not "
                "exist or is not a file."
                )
            );

        return;
    }

    const ProfileValidationResult validation =
        profileValidator.validate(
            workingProfile
            );

    if (!validation.isValid()) {
        if (previewSourcePath == filePath
            && previewTable->rowCount() > 0) {
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
            fileInfo,
            workingProfile
            );

    refreshPreviewButton->setVisible(
        requiresManualPreview
        );

    if (requiresManualPreview) {
        clearPreview(
            tr(
                "Automatic preview is disabled for "
                "large structured documents to keep the "
                "Import Configuration interface responsive. "
                "Import configuration remains available. "
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
        previewService.previewFile(
            filePath,
            workingProfile
            );

    displayPreviewResult(
        filePath,
        preview
        );
}

void ImportConfigurationDialog::updateRawSourcePreview(int row)
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
                ).toString()
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

void ImportConfigurationDialog::updateValidationState(
    bool refreshPreview
    )
{
    const ProfileValidationResult result =
        profileValidator.validate(
            workingProfile
            );

    if (result.issues.isEmpty()) {
        validationLabel->setText(
            tr("Profile configuration is valid.")
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

    if (refreshPreview) {
        schedulePreviewRefresh();
    }
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

void ImportConfigurationDialog::saveProfile()
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

    QSaveFile file(filePath);

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

    if (bytesWritten !=
        serializedProfile.size()) {
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

    QMessageBox::information(
        this,
        tr("Profile Saved"),
        tr(
            "The import profile was saved successfully."
            )
        );
}

void ImportConfigurationDialog::loadProfile()
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

    QFile file(filePath);

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
                != ProfileValidationSeverity::Error) {
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

    profileIsUserConfigured = true;

    customFieldDetectionSourcePath =
        selectedFilePath();

    previewRefreshTimer->stop();

    populateProfileControls();

    updateValidationState(false);
    updatePreview();
    updateImportAvailability();
}

void ImportConfigurationDialog::schedulePreviewRefresh()
{
    cancelManualPreview();
    previewRefreshTimer->start();
}

void ImportConfigurationDialog::createProfileFromSource(
    bool userInitiated
    )
{
    const QString filePath =
        selectedFilePath();

    const QFileInfo fileInfo(filePath);

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
        const QMessageBox::StandardButton response =
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

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    workingProfile =
        ImportProfile();

    autoDetectedCustomFieldKeys.clear();

    const ImportFormatSuggestion suggestion =
        formatSuggestionService.suggestForFile(
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
                QStringLiteral("Default %1")
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

    profileIsUserConfigured = false;

    customFieldDetectionSourcePath.clear();

    populateProfileControls();

    if (!workingProfile.importerId.isEmpty()
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

    updateValidationState(false);
    updatePreview();

    if (userInitiated) {
        profileIsUserConfigured = true;
    }
}

void ImportConfigurationDialog::populateImporterOptions()
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

    const QVector<std::shared_ptr<ILogImporter>> importers =
        registry.importers();

    for (const std::shared_ptr<ILogImporter> &importer
         : importers) {
        importerComboBox->addItem(
            importer->displayName(),
            importer->id()
            );
    }
}

void ImportConfigurationDialog::updateFormatSpecificControls()
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

void ImportConfigurationDialog::displayPreviewResult(
    const QString &filePath,
    const ImportPreviewResult &preview)
{
    if (!preview.errorMessage.isEmpty()) {
        clearPreview(
            tr("Preview unavailable: %1")
                .arg(
                    preview.errorMessage
                    )
            );

        return;
    }

    const QVector<InvestigationRecord> &records =
        preview.importResult.records;

    previewSourcePath =
        filePath;

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

    const int canonicalColumnCount =
        headers.size();

    QSet<QString> mappedCustomFieldNames;

    for (const CustomFieldMapping &mapping
         : std::as_const(
             workingProfile.customFields
             )) {
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

    QHeaderView *previewHeader =
        previewTable->horizontalHeader();

    previewHeader->setStretchLastSection(false);

    for (
        int column = 0;
        column < headers.size();
        ++column
        ) {
        previewHeader->setSectionResizeMode(
            column,
            QHeaderView::Interactive
            );
    }

    if (previousColumnCount
        != headers.size()) {
        for (
            int column =
            canonicalColumnCount;
            column < headers.size() - 1;
            ++column
            ) {
            previewTable->setColumnWidth(
                column,
                140
                );
        }

        if (!headers.isEmpty()) {
            previewTable->setColumnWidth(
                headers.size() - 1,
                220
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

        const QString timestamp =
            record.timestamp.has_value()
                ? record.timestamp
                      ->toString(
                          Qt::ISODateWithMs
                          )
                : QString();

        const QString severity =
            record.severity.has_value()
                ? recordSeverityToString(
                      record.severity.value()
                      )
                : QString();

        const QString subsystem =
            record.subsystem.value_or(
                QString()
                );

        const QString eventCode =
            record.eventCode.value_or(
                QString()
                );

        const QString entityId =
            record.entityId.value_or(
                QString()
                );

        const QString message =
            record.message.value_or(
                QString()
                );

        QStringList mappedCustomValues;

        for (const CustomFieldMapping &mapping
             : std::as_const(
                 workingProfile.customFields
                 )) {
            const auto iterator =
                record.customAttributes.constFind(
                    mapping.name
                    );

            if (iterator ==
                record.customAttributes.constEnd()) {
                mappedCustomValues.append(
                    QString()
                    );

                continue;
            }

            mappedCustomValues.append(
                iterator.value().toString()
                );
        }

        QStringList unmappedFieldValues;

        QStringList attributeNames =
            record.customAttributes.keys();

        attributeNames.sort(
            Qt::CaseInsensitive
            );

        for (const QString &name
             : std::as_const(
                 attributeNames
                 )) {
            if (mappedCustomFieldNames.contains(
                    name
                    )) {
                continue;
            }

            unmappedFieldValues.append(
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

        const QString unmappedFields =
            unmappedFieldValues.join(
                QStringLiteral("; ")
                );

        QStringList values;

        if (showTimestamp) {
            values.append(
                timestamp
                );
        }

        if (showSeverity) {
            values.append(
                severity
                );
        }

        if (showSubsystem) {
            values.append(
                subsystem
                );
        }

        if (showEventCode) {
            values.append(
                eventCode
                );
        }

        if (showEntityId) {
            values.append(
                entityId
                );
        }

        if (showMessage) {
            values.append(
                message
                );
        }

        values.append(
            mappedCustomValues
            );

        values.append(
            unmappedFields
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

    if (!records.isEmpty()) {
        previewTable->selectRow(0);

        updateRawSourcePreview(0);
    } else {
        rawSourcePreview->clear();
    }

    if (!preview.importResult
             .diagnostics
             .isEmpty()) {
        summary +=
            tr(" %1 diagnostic(s).")
                .arg(
                    preview.importResult
                        .diagnostics
                        .size()
                    );
    }

    if (preview.sourceTruncated) {
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
        retainedMappings;

    for (const CustomFieldMapping &mapping
         : std::as_const(
             workingProfile.customFields
             )) {
        if (autoDetectedCustomFieldKeys
                .contains(
                    customFieldMappingKey(
                        mapping
                        )
                    )) {
            continue;
        }

        retainedMappings.append(
            mapping
            );
    }

    QSet<QString>
        explicitlyMappedNames;

    for (const CustomFieldMapping &mapping
         : std::as_const(
             retainedMappings
             )) {
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
                iterator.key().trimmed();

            if (fieldName.isEmpty()) {
                continue;
            }

            if (explicitlyMappedNames
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
         : std::as_const(
             sortedFields
             )) {
        bool alreadyMapped = false;

        for (const CustomFieldMapping &mapping
             : std::as_const(
                 updatedMappings
                 )) {
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

    updateValidationState(false);
}

void ImportConfigurationDialog::
    startManualPreview()
{
    if (previewWatcher != nullptr) {
        return;
    }

    const QString filePath =
        selectedFilePath();

    const QFileInfo fileInfo(
        filePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return;
    }

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
        customFieldDetectionSourcePath
        != filePath;

    if (detectCustomFields) {
        QList<CustomFieldMapping>
            retainedMappings;

        for (const CustomFieldMapping &mapping
             : std::as_const(
                 previewProfile.customFields
                 )) {
            if (autoDetectedCustomFieldKeys
                    .contains(
                        customFieldMappingKey(
                            mapping
                            )
                        )) {
                continue;
            }

            retainedMappings.append(
                mapping
                );
        }

        previewProfile.customFields =
            std::move(
                retainedMappings
                );

        /*
         * Detection needs visibility into source
         * fields that are not already mapped.
         */
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
            filePath,
            profileSnapshot,
            detectCustomFields
        ]() {
            const bool cancelled =
                watcher->isCanceled();

            if (previewWatcher == watcher) {
                previewWatcher = nullptr;
            }

            watcher->deleteLater();

            refreshPreviewButton->setEnabled(true);

            refreshPreviewButton->setText(
                tr("Refresh Preview")
                );

            if (cancelled) {
                updatePreview();
                return;
            }

            const ImportPreviewResult preview =
                watcher->result();

            /*
             * Do not display a preview that was
             * generated for a file or profile the
             * user has since changed.
             */
            if (selectedFilePath()
                    != filePath
                || profileSerializer.serialize(
                       workingProfile
                       )
                       != profileSnapshot) {
                updatePreview();
                return;
            }

            if (detectCustomFields
                && preview
                       .canDisplayPreview()) {
                applyDetectedCustomFieldMappings(
                    preview
                    );

                customFieldDetectionSourcePath =
                    filePath;
            }

            displayPreviewResult(
                filePath,
                preview
                );
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                filePath,
                previewProfile
            ](
                QPromise<ImportPreviewResult> &promise
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
                        .previewFile(
                            filePath,
                            previewProfile,
                            ImportPreviewService::
                            DefaultMaxProcessedRecords,
                            executionContext
                            );

                if (promise.isCanceled()
                    || preview.importResult.cancelled) {
                    return;
                }

                promise.addResult(
                    std::move(preview)
                    );
            }
            )
        );
}

void ImportConfigurationDialog::cancelManualPreview()
{
    if (previewWatcher == nullptr) {
        return;
    }

    previewWatcher->cancel();
}