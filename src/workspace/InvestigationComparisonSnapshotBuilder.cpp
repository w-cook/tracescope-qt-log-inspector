#include "InvestigationComparisonSnapshotBuilder.h"

#include <QUuid>

#include <utility>

#include "../analysis/InvestigationSessionComparisonAnalyzer.h"

InvestigationComparisonSnapshot
InvestigationComparisonSnapshotBuilder::build(
    const InvestigationSession &baselineSession,
    const InvestigationSession &comparisonSession,
    std::optional<BurstDetectionSettings>
        burstSettings
    ) const
{
    const QVector<InvestigationRecord> &
        baselineRecords =
        baselineSession
            .investigationController()
            ->allRecords();

    const QVector<InvestigationRecord> &
        comparisonRecords =
        comparisonSession
            .investigationController()
            ->allRecords();

    InvestigationSessionComparisonAnalyzer analyzer;

    InvestigationSessionComparison analysis;

    if (burstSettings.has_value()) {
        analysis =
            analyzer.compare(
                baselineRecords,
                comparisonRecords,
                burstSettings.value()
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

    InvestigationComparisonSourceSnapshot
        comparisonSource;

    comparisonSource.sessionId =
        comparisonSession.id();

    comparisonSource.sourceMetadata =
        comparisonSession.sourceMetadata();

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