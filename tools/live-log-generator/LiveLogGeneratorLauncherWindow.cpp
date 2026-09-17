#include "LiveLogGeneratorLauncherWindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QWidget *pathRow(
    QLineEdit *lineEdit,
    QPushButton *browseButton
    )
{
    auto *widget = new QWidget;

    auto *layout =
        new QHBoxLayout(widget);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->addWidget(
        lineEdit,
        1
        );

    layout->addWidget(
        browseButton
        );

    return widget;
}
}

LiveLogGeneratorLauncherWindow::
    LiveLogGeneratorLauncherWindow(
        QWidget *parent
        )
    : QWidget(parent),
    process(new QProcess(this))
{
    setWindowTitle(
        QStringLiteral(
            "TraceScope Live-Log Generator"
            )
        );

    setMinimumWidth(720);

    scenarioPathEdit =
        new QLineEdit(this);

    outputPathEdit =
        new QLineEdit(this);

    auto *scenarioBrowseButton =
        new QPushButton(
            QStringLiteral("Browse..."),
            this
            );

    auto *outputBrowseButton =
        new QPushButton(
            QStringLiteral("Browse..."),
            this
            );

    formatComboBox =
        new QComboBox(this);

    formatComboBox->addItems({
        QStringLiteral("jsonl"),
        QStringLiteral("csv"),
        QStringLiteral("tsv"),
        QStringLiteral("logfmt"),
        QStringLiteral("regex-text"),
        QStringLiteral("iis-w3c"),
        QStringLiteral("apache-common"),
        QStringLiteral("apache-combined"),
        QStringLiteral("nginx-combined"),
        QStringLiteral("syslog-rfc5424"),
        QStringLiteral("syslog-rfc3164"),
        QStringLiteral("structured-json"),
        QStringLiteral("structured-xml"),
        QStringLiteral("windows-event-xml")
    });

    speedComboBox =
        new QComboBox(this);

    speedComboBox->addItem(
        QStringLiteral("0.5x"),
        QStringLiteral("0.5")
        );

    speedComboBox->addItem(
        QStringLiteral("1x"),
        QStringLiteral("1")
        );

    speedComboBox->addItem(
        QStringLiteral("2x"),
        QStringLiteral("2")
        );

    speedComboBox->addItem(
        QStringLiteral("4x"),
        QStringLiteral("4")
        );

    speedComboBox->addItem(
        QStringLiteral("10x"),
        QStringLiteral("10")
        );

    speedComboBox->addItem(
        QStringLiteral("25x"),
        QStringLiteral("25")
        );

    speedComboBox->addItem(
        QStringLiteral("50x"),
        QStringLiteral("50")
        );

    speedComboBox->setCurrentIndex(1);

    loopCheckBox =
        new QCheckBox(
            QStringLiteral(
                "Loop scenario"
                ),
            this
            );

    loopBehaviorComboBox =
        new QComboBox(this);

    loopBehaviorComboBox->addItem(
        QStringLiteral(
            "Append continuously"
            ),
        QStringLiteral("append")
        );

    loopBehaviorComboBox->addItem(
        QStringLiteral(
            "Restart output each iteration"
            ),
        QStringLiteral("restart")
        );

    loopBehaviorComboBox->setEnabled(
        false
        );

    playButton =
        new QPushButton(
            QStringLiteral("Play"),
            this
            );

    stopButton =
        new QPushButton(
            QStringLiteral("Stop"),
            this
            );

    stopButton->setEnabled(false);

    statusLabel =
        new QLabel(
            QStringLiteral("Ready."),
            this
            );

    outputTextEdit =
        new QPlainTextEdit(this);

    outputTextEdit->setReadOnly(true);

    outputTextEdit->setPlaceholderText(
        QStringLiteral(
            "Generator output will appear here."
            )
        );

    outputTextEdit->setMaximumBlockCount(
        500
        );

    auto *formLayout =
        new QFormLayout;

    formLayout->addRow(
        QStringLiteral("Scenario:"),
        pathRow(
            scenarioPathEdit,
            scenarioBrowseButton
            )
        );

    formLayout->addRow(
        QStringLiteral("Output:"),
        pathRow(
            outputPathEdit,
            outputBrowseButton
            )
        );

    formLayout->addRow(
        QStringLiteral("Format:"),
        formatComboBox
        );

    formLayout->addRow(
        QStringLiteral("Speed:"),
        speedComboBox
        );

    formLayout->addRow(
        QString(),
        loopCheckBox
        );

    formLayout->addRow(
        QString(),
        loopCheckBox
        );

    formLayout->addRow(
        QStringLiteral("Loop behavior:"),
        loopBehaviorComboBox
        );

    auto *buttonLayout =
        new QHBoxLayout;

    buttonLayout->addStretch();

    buttonLayout->addWidget(
        playButton
        );

    buttonLayout->addWidget(
        stopButton
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->addLayout(
        formLayout
        );

    layout->addLayout(
        buttonLayout
        );

    layout->addWidget(
        statusLabel
        );

    layout->addWidget(
        outputTextEdit
        );

    connect(
        scenarioBrowseButton,
        &QPushButton::clicked,
        this,
        &LiveLogGeneratorLauncherWindow::
        browseScenario
        );

    connect(
        outputBrowseButton,
        &QPushButton::clicked,
        this,
        &LiveLogGeneratorLauncherWindow::
        browseOutput
        );

    connect(
        loopCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            if (process->state()
                == QProcess::NotRunning) {
                loopBehaviorComboBox->setEnabled(
                    checked
                    );
            }
        }
        );

    connect(
        playButton,
        &QPushButton::clicked,
        this,
        &LiveLogGeneratorLauncherWindow::
        startPlayback
        );

    connect(
        stopButton,
        &QPushButton::clicked,
        this,
        &LiveLogGeneratorLauncherWindow::
        stopPlayback
        );

    connect(
        process,
        &QProcess::started,
        this,
        &LiveLogGeneratorLauncherWindow::
        handleProcessStarted
        );

    connect(
        process,
        &QProcess::readyReadStandardOutput,
        this,
        &LiveLogGeneratorLauncherWindow::
        appendStandardOutput
        );

    connect(
        process,
        &QProcess::readyReadStandardError,
        this,
        &LiveLogGeneratorLauncherWindow::
        appendStandardError
        );

    connect(
        process,
        &QProcess::finished,
        this,
        &LiveLogGeneratorLauncherWindow::
        handleProcessFinished
        );

    connect(
        process,
        &QProcess::errorOccurred,
        this,
        &LiveLogGeneratorLauncherWindow::
        handleProcessError
        );
}

void LiveLogGeneratorLauncherWindow::
    closeEvent(
        QCloseEvent *event
        )
{
    if (process->state()
        != QProcess::NotRunning) {
        stopRequested = true;

        process->terminate();

        if (!process->waitForFinished(
                1000
                )) {
            process->kill();
            process->waitForFinished(
                1000
                );
        }
    }

    event->accept();
}

void LiveLogGeneratorLauncherWindow::
    browseScenario()
{
    const QString initialPath =
        scenarioPathEdit->text()
                .trimmed()
                .isEmpty()
            ? QDir::currentPath()
            : QFileInfo(
                  scenarioPathEdit->text()
                  )
                  .absolutePath();

    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            QStringLiteral(
                "Select Live-Log Scenario"
                ),
            initialPath,
            QStringLiteral(
                "JSON Files (*.json);;"
                "All Files (*)"
                )
            );

    if (!filePath.isEmpty()) {
        scenarioPathEdit->setText(
            QDir::toNativeSeparators(
                filePath
                )
            );
    }
}

void LiveLogGeneratorLauncherWindow::
    browseOutput()
{
    const QString initialPath =
        outputPathEdit->text()
                .trimmed()
                .isEmpty()
            ? QDir::currentPath()
            : QFileInfo(
                  outputPathEdit->text()
                  )
                  .absolutePath();

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            QStringLiteral(
                "Select Generated Log Output"
                ),
            initialPath,
            QStringLiteral(
                "All Files (*)"
                )
            );

    if (!filePath.isEmpty()) {
        outputPathEdit->setText(
            QDir::toNativeSeparators(
                filePath
                )
            );
    }
}

void LiveLogGeneratorLauncherWindow::
    startPlayback()
{
    if (process->state()
        != QProcess::NotRunning) {
        return;
    }

    const QString scenarioPath =
        scenarioPathEdit->text()
            .trimmed();

    const QString outputPath =
        outputPathEdit->text()
            .trimmed();

    if (scenarioPath.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Scenario Required"
                ),
            QStringLiteral(
                "Select a live-log scenario "
                "before starting playback."
                )
            );

        return;
    }

    if (outputPath.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Output Required"
                ),
            QStringLiteral(
                "Select an output file "
                "before starting playback."
                )
            );

        return;
    }

    if (!QFileInfo::exists(
            scenarioPath
            )) {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Scenario Not Found"
                ),
            QStringLiteral(
                "The selected scenario file "
                "does not exist."
                )
            );

        return;
    }

    const QString executablePath =
        generatorExecutablePath();

    if (!QFileInfo::exists(
            executablePath
            )) {
        QMessageBox::critical(
            this,
            QStringLiteral(
                "Generator Not Found"
                ),
            QStringLiteral(
                "Could not find the generator "
                "executable at:\n\n%1"
                )
                .arg(
                    QDir::toNativeSeparators(
                        executablePath
                        )
                    )
            );

        return;
    }

    stopRequested = false;

    outputTextEdit->clear();

    statusLabel->setText(
        QStringLiteral(
            "Starting playback..."
            )
        );

    setRunning(true);

    process->setProgram(
        executablePath
        );

    process->setArguments(
        buildArguments()
        );

    process->setProcessChannelMode(
        QProcess::SeparateChannels
        );

    process->start();
}

void LiveLogGeneratorLauncherWindow::
    stopPlayback()
{
    if (process->state()
        == QProcess::NotRunning) {
        return;
    }

    stopRequested = true;

    statusLabel->setText(
        QStringLiteral(
            "Stopping playback..."
            )
        );

    stopButton->setEnabled(false);

    process->terminate();

    QTimer::singleShot(
        2000,
        this,
        [this]() {
            if (process->state()
                != QProcess::NotRunning) {
                process->kill();
            }
        }
        );
}

void LiveLogGeneratorLauncherWindow::
    appendStandardOutput()
{
    const QByteArray bytes =
        process->readAllStandardOutput();

    if (bytes.isEmpty()) {
        return;
    }

    outputTextEdit->moveCursor(
        QTextCursor::End
        );

    outputTextEdit->insertPlainText(
        QString::fromLocal8Bit(
            bytes
            )
        );

    outputTextEdit->moveCursor(
        QTextCursor::End
        );
}

void LiveLogGeneratorLauncherWindow::
    appendStandardError()
{
    const QByteArray bytes =
        process->readAllStandardError();

    if (bytes.isEmpty()) {
        return;
    }

    outputTextEdit->moveCursor(
        QTextCursor::End
        );

    outputTextEdit->insertPlainText(
        QString::fromLocal8Bit(
            bytes
            )
        );

    outputTextEdit->moveCursor(
        QTextCursor::End
        );
}

void LiveLogGeneratorLauncherWindow::
    handleProcessStarted()
{
    statusLabel->setText(
        QStringLiteral(
            "Playback running."
            )
        );
}

void LiveLogGeneratorLauncherWindow::
    handleProcessFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus
        )
{
    appendStandardOutput();
    appendStandardError();

    setRunning(false);

    if (stopRequested) {
        const QDateTime stoppedAt =
            QDateTime::currentDateTimeUtc();

        outputTextEdit->appendPlainText(
            QStringLiteral(
                "Playback stopped at %1"
                )
                .arg(
                    stoppedAt.toString(
                        Qt::ISODateWithMs
                        )
                    )
            );

        statusLabel->setText(
            QStringLiteral(
                "Playback stopped."
                )
            );

        stopRequested = false;

        return;
    }

    if (exitStatus
            == QProcess::NormalExit
        && exitCode == 0) {
        statusLabel->setText(
            QStringLiteral(
                "Playback completed."
                )
            );

        return;
    }

    statusLabel->setText(
        QStringLiteral(
            "Playback failed "
            "(exit code %1)."
            )
            .arg(exitCode)
        );
}

void LiveLogGeneratorLauncherWindow::
    handleProcessError(
        QProcess::ProcessError error
        )
{
    if (stopRequested
        && error == QProcess::Crashed) {
        return;
    }

    outputTextEdit->appendPlainText(
        QStringLiteral(
            "Process error: %1"
            )
            .arg(
                process->errorString()
                )
        );

    if (error
        == QProcess::FailedToStart) {
        setRunning(false);

        statusLabel->setText(
            QStringLiteral(
                "Playback could not be started."
                )
            );
    }
}

void LiveLogGeneratorLauncherWindow::
    setRunning(bool running)
{
    scenarioPathEdit->setEnabled(
        !running
        );

    outputPathEdit->setEnabled(
        !running
        );

    formatComboBox->setEnabled(
        !running
        );

    speedComboBox->setEnabled(
        !running
        );

    loopCheckBox->setEnabled(
        !running
        );

    loopBehaviorComboBox->setEnabled(
        !running
        && loopCheckBox->isChecked()
        );

    playButton->setEnabled(
        !running
        );

    stopButton->setEnabled(
        running
        );
}

QString LiveLogGeneratorLauncherWindow::
    generatorExecutablePath() const
{
    QString executableName =
        QStringLiteral(
            "TraceScopeLiveLogGenerator"
            );

#ifdef Q_OS_WIN
    executableName +=
        QStringLiteral(".exe");
#endif

    return QDir(
               QCoreApplication::
               applicationDirPath()
               )
        .filePath(
            executableName
            );
}

QStringList LiveLogGeneratorLauncherWindow::
    buildArguments() const
{
    QStringList arguments({
        QStringLiteral("--scenario"),
        scenarioPathEdit->text()
            .trimmed(),
        QStringLiteral("--output"),
        outputPathEdit->text()
            .trimmed(),
        QStringLiteral("--format"),
        formatComboBox->currentText(),
        QStringLiteral("--speed"),
        speedComboBox->currentData()
            .toString()
    });

    if (loopCheckBox->isChecked()) {
        arguments.append(
            QStringLiteral("--loop")
            );

        arguments.append(
            QStringLiteral("--loop-mode")
            );

        arguments.append(
            loopBehaviorComboBox
                ->currentData()
                .toString()
            );
    }

    return arguments;
}