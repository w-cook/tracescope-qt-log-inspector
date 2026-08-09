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

    resize(760, 620);

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

    mainLayout->addWidget(profileGroup);

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

    mainLayout->addWidget(mappingGroup);

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

    profileNameEdit->setText(
        workingProfile.name
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
}

void ImportConfigurationDialog::updateWorkingProfile()
{
    workingProfile.name =
        profileNameEdit->text();

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