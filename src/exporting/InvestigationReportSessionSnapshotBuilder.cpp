#include "InvestigationReportSessionSnapshotBuilder.h"

#include <algorithm>

#include <QList>
#include <QSet>

#include "../analysis/AnalysisTimeBucketRange.h"
#include "../analysis/EventTimelineAnalyzer.h"
#include "../analysis/InvestigationAnalyticsAnalyzer.h"
#include "../analysis/InvestigationBurstAnalyzer.h"
#include "../analysis/InvestigationCadenceAnalyzer.h"
#include "../analysis/TelemetryIssueAnalyzer.h"

#include "../controllers/InvestigationController.h"
#include "../models/InvestigationFilterProxyModel.h"

#include "../workspace/InvestigationSession.h"
#include "../workspace/InvestigationStateStore.h"

namespace
{

constexpr qint64 MillisecondsPerSecond =
    1000;

constexpr qint64 MillisecondsPerMinute =
    60 * MillisecondsPerSecond;

constexpr qint64 MillisecondsPerHour =
    60 * MillisecondsPerMinute;

constexpr qint64 MillisecondsPerDay =
    24 * MillisecondsPerHour;

constexpr qint64 ReportMaximumTimelineBucketCount =
    60;

struct TimestampSummary
{
    qint64 timestampedRecordCount = 0;

    std::optional<QDateTime> firstTimestamp;
    std::optional<QDateTime> lastTimestamp;
};

TimestampSummary timestampSummary(
    const QVector<InvestigationRecord> &records
    )
{
    TimestampSummary summary;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        ++summary.timestampedRecordCount;

        if (!summary.firstTimestamp.has_value()
            || record.timestamp.value()
                   < summary.firstTimestamp.value()) {
            summary.firstTimestamp =
                record.timestamp;
        }

        if (!summary.lastTimestamp.has_value()
            || record.timestamp.value()
                   > summary.lastTimestamp.value()) {
            summary.lastTimestamp =
                record.timestamp;
        }
    }

    return summary;
}

InvestigationReportSeveritySummary
severitySummary(
    const QVector<InvestigationRecord> &records
    )
{
    InvestigationReportSeveritySummary summary;

    for (const InvestigationRecord &record
         : records) {
        if (!record.severity.has_value()) {
            ++summary.unspecifiedCount;
            continue;
        }

        switch (record.severity.value()) {
        case RecordSeverity::Trace:
            ++summary.traceCount;
            break;

        case RecordSeverity::Debug:
            ++summary.debugCount;
            break;

        case RecordSeverity::Info:
            ++summary.infoCount;
            break;

        case RecordSeverity::Warning:
            ++summary.warningCount;
            break;

        case RecordSeverity::Error:
            ++summary.errorCount;
            break;

        case RecordSeverity::Critical:
            ++summary.criticalCount;
            break;
        }
    }

    return summary;
}

qint64 reportTimelineIntervalMilliseconds(
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp
    )
{
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp) {
        return 0;
    }

    /*
     * Use the same human-friendly resolution ladder as
     * the interactive timeline, but choose solely from
     * the captured time span rather than widget width.
     *
     * The resulting report is therefore deterministic
     * across machines, window layouts, and detached
     * document states.
     */
    const QList<qint64> candidates {
        1,
        10,
        100,
        500,

        1 * MillisecondsPerSecond,
        5 * MillisecondsPerSecond,
        15 * MillisecondsPerSecond,
        30 * MillisecondsPerSecond,

        1 * MillisecondsPerMinute,
        5 * MillisecondsPerMinute,
        15 * MillisecondsPerMinute,
        30 * MillisecondsPerMinute,

        1 * MillisecondsPerHour,
        3 * MillisecondsPerHour,
        6 * MillisecondsPerHour,
        12 * MillisecondsPerHour,

        1 * MillisecondsPerDay,
        3 * MillisecondsPerDay,
        7 * MillisecondsPerDay
    };

    for (const qint64 candidate : candidates) {
        const auto range =
            AnalysisTimeBucketRange::create(
                firstTimestamp,
                lastTimestamp,
                candidate
                );

        if (range.has_value()
            && range->bucketCount()
                   <= ReportMaximumTimelineBucketCount) {
            return candidate;
        }
    }

    /*
     * Extremely long captures continue scaling in
     * deterministic whole-day units.
     */
    const qint64 spanMilliseconds =
        std::max<qint64>(
            1,
            firstTimestamp.msecsTo(
                lastTimestamp
                ) + 1
            );

    qint64 wholeDays =
        std::max<qint64>(
            1,
            (
                spanMilliseconds
                + (
                    ReportMaximumTimelineBucketCount
                    * MillisecondsPerDay
                    )
                - 1
                )
                / (
                    ReportMaximumTimelineBucketCount
                    * MillisecondsPerDay
                    )
            );

    while (true) {
        const qint64 intervalMilliseconds =
            wholeDays * MillisecondsPerDay;

        const auto range =
            AnalysisTimeBucketRange::create(
                firstTimestamp,
                lastTimestamp,
                intervalMilliseconds
                );

        if (range.has_value()
            && range->bucketCount()
                   <= ReportMaximumTimelineBucketCount) {
            return intervalMilliseconds;
        }

        ++wholeDays;
    }
}

InvestigationReportBurstTimingMode
reportBurstTimingMode(
    InvestigationBurstTimingMode timingMode
    )
{
    switch (timingMode) {
    case InvestigationBurstTimingMode::Auto:
        return InvestigationReportBurstTimingMode::Auto;

    case InvestigationBurstTimingMode::Manual:
        return InvestigationReportBurstTimingMode::Manual;
    }

    return InvestigationReportBurstTimingMode::Auto;
}

QSet<QString> burstRecordIds(
    const QVector<InvestigationBurst> &bursts
    )
{
    QSet<QString> recordIds;

    for (const InvestigationBurst &burst
         : bursts) {
        for (const QString &recordId
             : burst.recordIds) {
            if (!recordId.isEmpty()) {
                recordIds.insert(
                    recordId
                    );
            }
        }
    }

    return recordIds;
}

}

InvestigationReportSessionSnapshot
InvestigationReportSessionSnapshotBuilder::build(
    const InvestigationSession &session,
    const QString &documentTitle,
    bool includeSupportingEvidence,
    bool includeTechnicalAppendix
    ) const
{
    InvestigationReportSessionSnapshot snapshot;

    /*
     * -----------------------------------------------------
     * Source/import context
     * -----------------------------------------------------
     */

    const InvestigationSessionSourceMetadata
        &sourceMetadata =
        session.sourceMetadata();

    const ImportProfile &importProfile =
        session.importProfile();

    snapshot.source.sessionId =
        session.id();

    snapshot.source.documentTitle =
        documentTitle;

    snapshot.source.sourceName =
        sourceMetadata.sourceName;

    snapshot.source.sourceSizeBytes =
        sourceMetadata.sourceSizeBytes;

    snapshot.source.sourceLastModified =
        sourceMetadata.sourceLastModified;

    snapshot.source.importedAtUtc =
        sourceMetadata.importedAtUtc;

    snapshot.source.importProfileName =
        importProfile.name;

    snapshot.source.importerId =
        importProfile.importerId;

    snapshot.source.processedRecordCount =
        session.processedRecordCount();

    snapshot.source.importedRecordCount =
        session.importedRecordCount();

    snapshot.source.skippedRecordCount =
        session.skippedRecordCount();

    snapshot.source.sourceTruncated =
        session.sourceTruncated();

    snapshot.source.diagnostics =
        session.diagnostics();

    if (includeTechnicalAppendix) {
        snapshot.source.technicalImportProfile =
            importProfile;
    }

    /*
     * -----------------------------------------------------
     * Controller/filter state
     * -----------------------------------------------------
     */

    const InvestigationController *controller =
        session.investigationController();

    if (controller == nullptr) {
        return snapshot;
    }

    const InvestigationFilterProxyModel *proxy =
        controller->proxyModel();

    if (proxy != nullptr) {
        snapshot.filters.severities =
            proxy->severityFilters();

        snapshot.filters.subsystems =
            proxy->subsystemFilters();

        snapshot.filters.searchText =
            proxy->searchText();

        snapshot.filters.eventCodes =
            proxy->eventCodeFilters();

        snapshot.filters.entityIds =
            proxy->entityFilters();

        snapshot.filters.startTime =
            proxy->timeRangeStart();

        snapshot.filters.endTime =
            proxy->timeRangeEnd();

        snapshot.filters.customFieldFilters =
            proxy->customFieldFilters();

        snapshot.filters.findingStatuses =
            proxy->findingStatusFilters();

        snapshot.filters.bookmarkedOnly =
            proxy->bookmarkedOnly();
    }

    const QVector<InvestigationRecord>
        visibleRecords =
        controller->visibleRecords();

    const QVector<InvestigationRecord>
        analysisRecords =
        controller->recordsForAnalysis();

    snapshot.recordContext.totalRecordCount =
        controller->totalRecordCount();

    snapshot.recordContext.visibleRecordCount =
        visibleRecords.size();

    snapshot.recordContext.analysisRecordCount =
        analysisRecords.size();

    const TimestampSummary visibleTimestamps =
        timestampSummary(
            visibleRecords
            );

    snapshot.recordContext
        .visibleTimestampedRecordCount =
        visibleTimestamps.timestampedRecordCount;

    snapshot.recordContext.visibleFirstTimestamp =
        visibleTimestamps.firstTimestamp;

    snapshot.recordContext.visibleLastTimestamp =
        visibleTimestamps.lastTimestamp;

    const TimestampSummary analysisTimestamps =
        timestampSummary(
            analysisRecords
            );

    snapshot.recordContext
        .analysisTimestampedRecordCount =
        analysisTimestamps.timestampedRecordCount;

    snapshot.recordContext.analysisFirstTimestamp =
        analysisTimestamps.firstTimestamp;

    snapshot.recordContext.analysisLastTimestamp =
        analysisTimestamps.lastTimestamp;

    /*
     * -----------------------------------------------------
     * Capabilities
     * -----------------------------------------------------
     */

    snapshot.capabilities.hasTimestampData =
        session.firstTimestamp().has_value()
        && session.lastTimestamp().has_value();

    snapshot.capabilities.hasSeverityData =
        session.hasSeverityData();

    snapshot.capabilities.hasSubsystemData =
        session.hasSubsystemData();

    snapshot.capabilities.hasEventCodeData =
        session.hasEventCodeData();

    snapshot.capabilities.hasEntityData =
        session.hasEntityData();

    snapshot.capabilities.hasCustomFieldData =
        session.hasCustomFieldData();

    /*
     * -----------------------------------------------------
     * Summaries and deterministic analytics
     * -----------------------------------------------------
     */

    snapshot.visibleSeveritySummary =
        severitySummary(
            visibleRecords
            );

    snapshot.analysisSeveritySummary =
        severitySummary(
            analysisRecords
            );

    TelemetryIssueAnalyzer issueAnalyzer;

    if (snapshot.capabilities.hasSeverityData
        && snapshot.capabilities.hasSubsystemData) {
        snapshot.elevatedIssueGroups =
            issueAnalyzer
                .groupWarningsAndErrorsBySubsystem(
                    analysisRecords
                    );
    }

    InvestigationAnalyticsAnalyzer analyticsAnalyzer;

    if (snapshot.capabilities.hasEventCodeData) {
        snapshot.eventCodeFrequencies =
            analyticsAnalyzer.eventCodeFrequencies(
                analysisRecords
                );
    }

    if (snapshot.capabilities.hasEntityData) {
        /*
         * Unlike the current screen-space-constrained UI,
         * retain the complete frequency collection. The
         * HTML renderer can show a compact Top-N view and
         * expose the remainder through progressive
         * disclosure.
         */
        snapshot.entityFrequencies =
            analyticsAnalyzer.entityFrequencies(
                analysisRecords
                );
    }

    if (snapshot.capabilities.hasSubsystemData) {
        snapshot.subsystemFrequencies =
            analyticsAnalyzer.subsystemFrequencies(
                analysisRecords
                );
    }

    /*
     * -----------------------------------------------------
     * Timeline/trends
     * -----------------------------------------------------
     */

    if (analysisTimestamps.firstTimestamp.has_value()
        && analysisTimestamps.lastTimestamp.has_value()) {
        const qint64 intervalMilliseconds =
            reportTimelineIntervalMilliseconds(
                analysisTimestamps
                    .firstTimestamp
                    .value(),
                analysisTimestamps
                    .lastTimestamp
                    .value()
                );

        if (intervalMilliseconds > 0) {
            EventTimelineAnalyzer timelineAnalyzer;

            snapshot.timeline.available =
                true;

            snapshot.timeline.intervalMilliseconds =
                intervalMilliseconds;

            snapshot.timeline.buckets =
                timelineAnalyzer
                    .groupRecordsByIntervalMilliseconds(
                        analysisRecords,
                        analysisTimestamps
                            .firstTimestamp
                            .value(),
                        analysisTimestamps
                            .lastTimestamp
                            .value(),
                        intervalMilliseconds
                        );

            if (snapshot.capabilities
                    .hasSubsystemData) {
                snapshot.subsystemTrends.available =
                    true;

                snapshot.subsystemTrends
                    .intervalMilliseconds =
                    intervalMilliseconds;

                snapshot.subsystemTrends.buckets =
                    analyticsAnalyzer.subsystemTrends(
                        analysisRecords,
                        analysisTimestamps
                            .firstTimestamp
                            .value(),
                        analysisTimestamps
                            .lastTimestamp
                            .value(),
                        intervalMilliseconds
                        );
            }
        }
    }

    /*
     * -----------------------------------------------------
     * Burst analysis
     * -----------------------------------------------------
     */

    InvestigationCadenceAnalyzer cadenceAnalyzer;

    snapshot.burstAnalysis.cadence =
        cadenceAnalyzer.analyze(
            analysisRecords
            );

    snapshot.burstAnalysis.timingMode =
        reportBurstTimingMode(
            session.burstTimingMode()
            );

    snapshot.burstAnalysis.configuredSettings =
        session.burstDetectionSettings();

    snapshot.burstAnalysis.effectiveSettings =
        snapshot.burstAnalysis.configuredSettings;

    if (session.burstTimingMode()
        == InvestigationBurstTimingMode::Auto) {
        snapshot.burstAnalysis
            .effectiveSettings
            .windowMilliseconds =
            snapshot.burstAnalysis
                .cadence
                .recommendedBurstWindowMilliseconds;

        snapshot.burstAnalysis
            .effectiveSettings
            .mergeGapMilliseconds =
            snapshot.burstAnalysis
                .cadence
                .recommendedMergeGapMilliseconds;
    }

    snapshot.burstAnalysis.available =
        snapshot.capabilities.hasTimestampData
        && snapshot.capabilities.hasSeverityData
        && snapshot.burstAnalysis
               .effectiveSettings
               .isValid();

    if (snapshot.burstAnalysis.available) {
        InvestigationBurstAnalyzer burstAnalyzer;

        snapshot.burstAnalysis.bursts =
            burstAnalyzer.detectBursts(
                analysisRecords,
                snapshot.burstAnalysis
                    .effectiveSettings
                );
    }

    /*
     * -----------------------------------------------------
     * Investigator state / supporting evidence
     * -----------------------------------------------------
     */

    const InvestigationStateStore *stateStore =
        session.investigationStateStore();

    const QSet<QString> burstEvidenceIds =
        burstRecordIds(
            snapshot.burstAnalysis.bursts
            );

    /*
     * Iterate source records rather than hash/set IDs so
     * evidence remains deterministic in source order.
     */
    for (const InvestigationRecord &record
         : controller->allRecords()) {
        const bool hasInvestigatorState =
            stateStore != nullptr
            && stateStore->hasStateForRecord(
                record.recordId
                );

        const bool contributesToBurst =
            burstEvidenceIds.contains(
                record.recordId
                );

        const bool includeRecord =
            hasInvestigatorState
            || (
                includeSupportingEvidence
                && contributesToBurst
                );

        if (!includeRecord) {
            continue;
        }

        InvestigationReportEvidenceRecord evidence;

        evidence.record =
            record;

        /*
         * Report evidence is intended to be shareable.
         * Preserve the source filename and record number
         * needed for provenance, but do not retain the
         * originating workstation's local filesystem path.
         */
        evidence.record.source.sourcePath.clear();

        if (hasInvestigatorState) {
            evidence.state =
                stateStore->stateForRecord(
                    record.recordId
                    );
        }

        evidence.burstEvidence =
            contributesToBurst;

        snapshot.evidenceRecords.append(
            std::move(evidence)
            );
    }

    return snapshot;
}