#pragma once

#include "InvestigationSession.h"
#include "WorkspacePersistenceState.h"

class InvestigationSessionPersistence
{
public:
    static PersistedInvestigationSession capture(
        const InvestigationSession &session,
        const InvestigationSessionPresentationState
            &presentationState = {}
        );

    static void restoreState(
        const PersistedInvestigationSession
            &persistedSession,
        InvestigationSession &session
        );
};