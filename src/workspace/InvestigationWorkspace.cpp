#include "InvestigationWorkspace.h"

#include <algorithm>
#include <utility>

#include "HybridInvestigationReconstructionService.h"

InvestigationWorkspace::InvestigationWorkspace(
    QObject *parent
    )
    : QObject(parent)
{
}

int InvestigationWorkspace::sessionCount() const
{
    return static_cast<int>(
        m_sessions.size()
        );
}

int InvestigationWorkspace::activeSessionIndex() const
{
    return m_activeSessionIndex;
}

InvestigationSession *
InvestigationWorkspace::activeSession()
{
    return sessionAt(
        m_activeSessionIndex
        );
}

const InvestigationSession *
InvestigationWorkspace::activeSession() const
{
    return sessionAt(
        m_activeSessionIndex
        );
}

InvestigationSession *
InvestigationWorkspace::sessionAt(int index)
{
    if (index < 0
        || index >= sessionCount()) {
        return nullptr;
    }

    return m_sessions[index].get();
}

const InvestigationSession *
InvestigationWorkspace::sessionAt(
    int index
    ) const
{
    if (index < 0
        || index >= sessionCount()) {
        return nullptr;
    }

    return m_sessions[index].get();
}

int InvestigationWorkspace::addSession(
    std::unique_ptr<InvestigationSession> session
    )
{
    if (!session) {
        return -1;
    }

    m_sessions.push_back(
        std::move(session)
        );

    const int newIndex =
        sessionCount() - 1;

    emit sessionAdded(newIndex);

    setActiveSession(newIndex);

    return newIndex;
}

bool InvestigationWorkspace::setActiveSession(
    int index
    )
{
    if (index < 0
        || index >= sessionCount()) {
        return false;
    }

    if (m_activeSessionIndex == index) {
        return true;
    }

    m_activeSessionIndex = index;

    emit activeSessionChanged(
        m_activeSessionIndex
        );

    return true;
}

bool InvestigationWorkspace::closeSession(
    int index
    )
{
    if (index < 0
        || index >= sessionCount()) {
        return false;
    }

    const bool closingActiveSession =
        index == m_activeSessionIndex;

    m_sessions.erase(
        m_sessions.begin() + index
        );

    emit sessionClosed(index);

    if (m_sessions.empty()) {
        if (m_activeSessionIndex != -1) {
            m_activeSessionIndex = -1;

            emit activeSessionChanged(-1);
        }

        return true;
    }

    if (index < m_activeSessionIndex) {
        --m_activeSessionIndex;

        return true;
    }

    if (!closingActiveSession) {
        return true;
    }

    /*
     * Prefer the session that moved into the
     * closed session's position. If the final
     * session was closed, activate the new last
     * session instead.
     */
    m_activeSessionIndex =
        std::min(
            index,
            sessionCount() - 1
            );

    emit activeSessionChanged(
        m_activeSessionIndex
        );

    return true;
}

int InvestigationWorkspace::indexOfSession(
    const QString &sessionId
    ) const
{
    for (
        int index = 0;
        index < sessionCount();
        ++index
        ) {
        const InvestigationSession *session =
            sessionAt(index);

        if (session != nullptr
            && session->id() == sessionId) {
            return index;
        }
    }

    return -1;
}

bool InvestigationWorkspace::reloadSession(
    const QString &sessionId,
    ImportResult result
    )
{
    const int index =
        indexOfSession(sessionId);

    if (index < 0) {
        return false;
    }

    InvestigationSession *session =
        sessionAt(index);

    if (session == nullptr) {
        return false;
    }

    session->reload(
        std::move(result)
        );

    emit sessionReloaded(index);

    return true;
}

bool InvestigationWorkspace::
    applySnapshotReload(
        const QString &sessionId,
        InvestigationSessionSnapshot snapshot
        )
{
    const int index =
        indexOfSession(
            sessionId
            );

    if (index < 0) {
        return false;
    }

    InvestigationSession *session =
        sessionAt(index);

    if (session == nullptr
        || !session->applySnapshotReload(
            std::move(snapshot)
            )) {
        return false;
    }

    emit sessionReloaded(index);

    return true;
}

InvestigationSessionHybridReloadResult
    InvestigationWorkspace::
    applyHybridReload(
        const QString &sessionId,
        InvestigationSessionSnapshot snapshot,
        HybridInvestigationReconstructionResult
            reconstruction
        )
{
    const int index =
        indexOfSession(
            sessionId
            );

    if (index < 0) {
        InvestigationSessionHybridReloadResult
            result;

        result.errorMessage =
            QStringLiteral(
                "The investigation session could "
                "not be found."
                );

        return result;
    }

    InvestigationSession *session =
        sessionAt(index);

    if (session == nullptr) {
        InvestigationSessionHybridReloadResult
            result;

        result.errorMessage =
            QStringLiteral(
                "The investigation session is no "
                "longer available."
                );

        return result;
    }

    InvestigationSessionHybridReloadResult result =
        session->applyHybridReload(
            std::move(snapshot),
            std::move(reconstruction)
            );

    if (result.succeeded) {
        emit sessionReloaded(index);
    }

    return result;
}

InvestigationSessionHybridReloadResult
    InvestigationWorkspace::
    applyHybridReconnect(
        const QString &sessionId,
        InvestigationSessionSnapshot snapshot,
        HybridInvestigationReconstructionResult
            reconstruction
        )
{
    const int index =
        indexOfSession(
            sessionId
            );

    if (index < 0) {
        InvestigationSessionHybridReloadResult
            result;

        result.errorMessage =
            QStringLiteral(
                "The investigation session could "
                "not be found."
                );

        return result;
    }

    InvestigationSession *session =
        sessionAt(
            index
            );

    if (session == nullptr) {
        InvestigationSessionHybridReloadResult
            result;

        result.errorMessage =
            QStringLiteral(
                "The investigation session is no "
                "longer available."
                );

        return result;
    }

    InvestigationSessionHybridReloadResult result =
        session->applyHybridReconnect(
            std::move(snapshot),
            std::move(reconstruction)
            );

    if (result.succeeded) {
        emit sessionReloaded(
            index
            );
    }

    return result;
}