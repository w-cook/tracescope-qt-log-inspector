#pragma once

#include "InvestigationComparisonSnapshot.h"
#include "WorkspacePersistenceState.h"

class InvestigationComparisonPersistence
{
public:
    static PersistedInvestigationComparison capture(
        const InvestigationComparisonSnapshot
            &snapshot,
        const InvestigationComparisonPresentationState
            &presentationState = {}
        );

    static InvestigationComparisonSnapshot restore(
        const PersistedInvestigationComparison
            &persistedComparison
        );
};