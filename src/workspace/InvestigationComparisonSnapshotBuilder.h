#pragma once

#include <optional>

#include "../analysis/BurstDetectionSettings.h"
#include "InvestigationComparisonSnapshot.h"
#include "InvestigationSession.h"

class InvestigationComparisonSnapshotBuilder
{
public:
    InvestigationComparisonSnapshot build(
        const InvestigationSession &baselineSession,
        const InvestigationSession &comparisonSession,
        std::optional<BurstDetectionSettings>
            burstSettings = std::nullopt
        ) const;
};