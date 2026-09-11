#pragma once

#include <QString>
#include <QWidget>

class InvestigationSession;
class QLabel;
class LiveSessionFollowCoordinator;
class QToolButton;

class LiveFollowTabControl
    : public QWidget
{
    Q_OBJECT

public:
    explicit LiveFollowTabControl(
        InvestigationSession *session,
        QWidget *parent = nullptr
        );

private:
    void attachCoordinator(
        LiveSessionFollowCoordinator *coordinator
        );

    void handlePrimaryAction();
    void handleStop();

    void setError(
        const QString &errorMessage
        );

    void clearError();

    void refreshPresentation();

    void refreshTabGeometry();

    InvestigationSession *m_session =
        nullptr;

    LiveSessionFollowCoordinator
        *m_coordinator = nullptr;

    QLabel *m_statusBadge =
        nullptr;

    QToolButton *m_primaryButton =
        nullptr;

    QToolButton *m_stopButton =
        nullptr;

    QString m_errorMessage;
};