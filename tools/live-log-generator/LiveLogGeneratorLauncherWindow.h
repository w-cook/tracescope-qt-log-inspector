#pragma once

#include <QProcess>
#include <QWidget>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class LiveLogGeneratorLauncherWindow final
    : public QWidget
{
    Q_OBJECT

public:
    explicit LiveLogGeneratorLauncherWindow(
        QWidget *parent = nullptr
        );

protected:
    void closeEvent(
        QCloseEvent *event
        ) override;

private:
    void browseScenario();
    void browseOutput();
    void startPlayback();
    void stopPlayback();

    void appendStandardOutput();
    void appendStandardError();

    void handleProcessStarted();

    void handleProcessFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus
        );

    void handleProcessError(
        QProcess::ProcessError error
        );

    void setRunning(bool running);

    QString generatorExecutablePath() const;
    QStringList buildArguments() const;

    QLineEdit *scenarioPathEdit = nullptr;
    QLineEdit *outputPathEdit = nullptr;

    QComboBox *formatComboBox = nullptr;
    QComboBox *speedComboBox = nullptr;

    QCheckBox *loopCheckBox = nullptr;

    QPushButton *playButton = nullptr;
    QPushButton *stopButton = nullptr;

    QLabel *statusLabel = nullptr;
    QPlainTextEdit *outputTextEdit = nullptr;

    QProcess *process = nullptr;

    bool stopRequested = false;
};