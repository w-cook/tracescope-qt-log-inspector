#include "InvestigationComparisonSnapshotBuilder.h"

#include <QUuid>

#include <stdexcept>
#include <utility>

#include "../analysis/InvestigationSessionComparisonAnalyzer.h"

InvestigationComparisonSnapshot
InvestigationComparisonSnapshotBuilder::build(
    const InvestigationSession &baselineSession,
    const InvestigationSession &comparisonSession,
    std::optional<BurstDetectionSettings>
        burstSettings,
    std::optional<InvestigationComparisonTimeRange>
        baselineTimeRange,
    std::optional<InvestigationComparisonTimeRange>
        comparisonTimeRange
    ) const
{
    const QVector<InvestigationRecord> &
        allBaselineRecords =
        baselineSession
            .investigationController()
            ->allRecords();

    const QVector<InvestigationRecord> &
        allComparisonRecords =
        comparisonSession
            .investigationController()
            ->allRecords();

    /*
     * Complete-session comparisons retain the
     * existing behavior and avoid unnecessary
     * copies of the imported record populations.
     */
    QVector<InvestigationRecord>
        selectedBaselineRecords;

    QVector<InvestigationRecord>
        selectedComparisonRecords;

    if (baselineTimeRange.has_value()) {
        if (!baselineTimeRange->isValid()) {
            throw std::invalid_argument(
                "Invalid baseline time range."
                );
        }

        selectedBaselineRecords =
            selectComparisonTimeRangeRecords(
                allBaselineRecords,
                *baselineTimeRange
                );

        if (selectedBaselineRecords.isEmpty()) {
            throw std::invalid_argument(
                "Baseline time range selects no records."
                );
        }
    }

    if (comparisonTimeRange.has_value()) {
        if (!comparisonTimeRange->isValid()) {
            throw std::invalid_argument(
                "Invalid comparison time range."
                );
        }

        selectedComparisonRecords =
            selectComparisonTimeRangeRecords(
                allComparisonRecords,
                *comparisonTimeRange
                );

        if (selectedComparisonRecords.isEmpty()) {
            throw std::invalid_argument(
                "Comparison time range selects no records."
                );
        }
    }

    const QVector<InvestigationRecord> &
        baselineRecords =
        baselineTimeRange.has_value()
            ? selectedBaselineRecords
            : allBaselineRecords;

    const QVector<InvestigationRecord> &
        comparisonRecords =
        comparisonTimeRange.has_value()
            ? selectedComparisonRecords
            : allComparisonRecords;

    InvestigationSessionComparisonAnalyzer analyzer;

    InvestigationSessionComparison analysis;

    if (burstSettings.has_value()) {
        analysis =
            analyzer.compare(
                baselineRecords,
                comparisonRecords,
                *burstSettings
                );
    } else {
        analysis =
            analyzer.compare(
                baselineRecords,
                comparisonRecords
                );
    }

    InvestigationComparisonSourceSnapshot
        baselineSource;

    baselineSource.sessionId =
        baselineSession.id();

    baselineSource.sourceMetadata =
        baselineSession.sourceMetadata();

    baselineSource.capturedTimeRange =
        std::move(baselineTimeRange);

    InvestigationComparisonSourceSnapshot
        comparisonSource;

    comparisonSource.sessionId =
        comparisonSession.id();

    comparisonSource.sourceMetadata =
        comparisonSession.sourceMetadata();

    comparisonSource.capturedTimeRange =
        std::move(comparisonTimeRange);

    return InvestigationComparisonSnapshot(
        QUuid::createUuid().toString(
            QUuid::WithoutBraces
            ),
        std::move(baselineSource),
        std::move(comparisonSource),
        std::move(burstSettings),
        std::move(analysis)
        );
}