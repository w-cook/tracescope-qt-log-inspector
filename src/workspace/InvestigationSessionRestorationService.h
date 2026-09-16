#pragma once

#include <memory>

#include <QString>

#include "InvestigationSession.h"
#include "WorkspacePersistenceState.h"

struct InvestigationSessionRestorationResult
{
    bool succeeded = false;

    QString errorMessage;

    std::unique_ptr<InvestigationSession> session;
};

class InvestigationSessionRestorationService
{
public:
    InvestigationSessionRestorationResult restore(
        const PersistedInvestigationSession &persistedSession,
        const QString &workspaceFilePath
        ) const;
};