#pragma once

#include <optional>

#include "../analysis/InvestigationSessionComparison.h"
#include "InvestigationComparisonTimeRange.h"

/*
 * Average across the analyst-selected time window,
 * including periods containing no records.
 *
 * This is distinct from the existing observed rate,
 * which uses the first/last selected event timestamps.
 */
inline std::optional<double>
comparisonWindowAverageRate(
    const std::optional<InvestigationComparisonTimeRange>
        &range,
    const InvestigationTimingSummary &timing
    )
{
    if (!range.has_value()
        || !range->isValid()
        || !range->startTime.has_value()
        || !range->endTime.has_value()) {
        return std::nullopt;
    }

    const qint64 duration =
        range->startTime->msecsTo(
            *range->endTime
            );

    if (duration <= 0) {
        return std::nullopt;
    }

    return static_cast<double>(
               timing.timestampedRecordCount
               )
           * 60000.0
           / static_cast<double>(duration);
}