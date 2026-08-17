#pragma once

#include <memory>
#include <vector>

#include <QObject>

#include "InvestigationSession.h"

class InvestigationWorkspace : public QObject
{
    Q_OBJECT

public:
    explicit InvestigationWorkspace(
        QObject *parent = nullptr
        );

    int sessionCount() const;

    int activeSessionIndex() const;

    InvestigationSession *activeSession();
    const InvestigationSession *activeSession() const;

    InvestigationSession *sessionAt(int index);
    const InvestigationSession *sessionAt(int index) const;

    int addSession(
        std::unique_ptr<InvestigationSession> session
        );

    bool setActiveSession(int index);

    bool closeSession(int index);

    int indexOfSession(
        const QString &sessionId
        ) const;

    bool reloadSession(
        const QString &sessionId,
        ImportResult result
        );

signals:
    void sessionAdded(int index);
    void activeSessionChanged(int index);
    void sessionClosed(int index);
    void sessionReloaded(int index);

private:
    std::vector<std::unique_ptr<InvestigationSession>>
        m_sessions;

    int m_activeSessionIndex = -1;
};