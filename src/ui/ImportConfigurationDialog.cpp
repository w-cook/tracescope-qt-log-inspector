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

    resize(720, 420);

    buildLayout();
    updateSourceState();
}

QString ImportConfigurationDialog::selectedFilePath() const
{
    return filePathEdit->text().trimmed();
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

    auto *configurationPlaceholder =
        new QGroupBox(
            tr("Import Profile"),
            this
            );

    auto *placeholderLayout =
        new QVBoxLayout(
            configurationPlaceholder
            );

    auto *placeholderLabel =
        new QLabel(
            tr(
                "Field mappings, validation, "
                "profile controls, and import preview "
                "will appear here."
                ),
            configurationPlaceholder
            );

    placeholderLabel->setWordWrap(true);

    placeholderLayout->addWidget(
        placeholderLabel
        );

    mainLayout->addWidget(
        configurationPlaceholder,
        1
        );

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
    const QFileInfo fileInfo(
        selectedFilePath()
        );

    importButton->setEnabled(
        fileInfo.exists()
        && fileInfo.isFile()
        );

    updateFormatSuggestion();
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