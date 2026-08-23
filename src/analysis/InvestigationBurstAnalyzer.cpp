#include "InvestigationBurstAnalyzer.h"

#include <algorithm>
#include <optional>

namespace
{
struct ElevatedRecord
{
    const InvestigationRecord *record = nullptr;
    qint64 epochMilliseconds = 0;
};

struct DetectedBurstRange
{
    qsizetype startIndex = 0;
    qsizetype endIndex = 0;

    bool triggeredByElevatedThreshold = false;
    bool triggeredByErrorCriticalThreshold = false;
};

bool isElevatedSeverity(
    RecordSeverity severity
    )
{
    return severity == RecordSeverity::Warning
           || severity == RecordSeverity::Error
           || severity == RecordSeverity::Critical;
}

bool isErrorOrCritical(
    RecordSeverity severity
    )
{
    return severity == RecordSeverity::Error
           || severity == RecordSeverity::Critical;
}

bool elevatedRecordLessThan(
    const ElevatedRecord &left,
    const ElevatedRecord &right
    )
{
    if (left.epochMilliseconds
        != right.epochMilliseconds) {
        return left.epochMilliseconds
               < right.epochMilliseconds;
    }

    const InvestigationRecord &leftRecord =
        *left.record;

    const InvestigationRecord &rightRecord =
        *right.record;

    if (leftRecord.source.sourcePath
        != rightRecord.source.sourcePath) {
        return leftRecord.source.sourcePath
               < rightRecord.source.sourcePath;
    }

    if (leftRecord.source.sourceName
        != rightRecord.source.sourceName) {
        return leftRecord.source.sourceName
               < rightRecord.source.sourceName;
    }

    if (leftRecord.source.recordNumber
        != rightRecord.source.recordNumber) {
        return leftRecord.source.recordNumber
               < rightRecord.source.recordNumber;
    }

    if (leftRecord.recordId
        != rightRecord.recordId) {
        return leftRecord.recordId
               < rightRecord.recordId;
    }

    return leftRecord.rawSource
           < rightRecord.rawSource;
}

bool rangesCanMerge(
    qint64 previousEndMilliseconds,
    qint64 nextStartMilliseconds,
    qint64 mergeGapMilliseconds
    )
{
    if (nextStartMilliseconds
        <= previousEndMilliseconds) {
        return true;
    }

    return nextStartMilliseconds
               - previousEndMilliseconds
           <= mergeGapMilliseconds;
}

void incrementAvailableValue(
    QMap<QString, int> &counts,
    const std::optional<QString> &value
    )
{
    if (!value.has_value()
        || value->trimmed().isEmpty()) {
        return;
    }

    ++counts[value.value()];
}

void addRecordToBurst(
    InvestigationBurst &burst,
    const InvestigationRecord &record
    )
{
    if (record.severity.has_value()) {
        switch (record.severity.value()) {
        case RecordSeverity::Warning:
            ++burst.warningCount;
            break;

        case RecordSeverity::Error:
            ++burst.errorCount;
            break;

        case RecordSeverity::Critical:
            ++burst.criticalCount;
            break;

        case RecordSeverity::Trace:
        case RecordSeverity::Debug:
        case RecordSeverity::Info:
            break;
        }
    }

    if (!record.recordId.isEmpty()) {
        burst.recordIds.append(
            record.recordId
            );
    }

    incrementAvailableValue(
        burst.subsystemCounts,
        record.subsystem
        );

    incrementAvailableValue(
        burst.eventCodeCounts,
        record.eventCode
        );

    incrementAvailableValue(
        burst.entityCounts,
        record.entityId
        );
}
}

QVector<InvestigationBurst>
InvestigationBurstAnalyzer::detectBursts(
    const QVector<InvestigationRecord> &records,
    const BurstDetectionSettings &settings
    ) const
{
    if (!settings.isValid()) {
        return {};
    }

    QVector<ElevatedRecord> elevatedRecords;

    elevatedRecords.reserve(
        records.size()
        );

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()
            || !record.severity.has_value()
            || !isElevatedSeverity(
                record.severity.value()
                )) {
            continue;
        }

        ElevatedRecord elevatedRecord;

        elevatedRecord.record =
            &record;

        elevatedRecord.epochMilliseconds =
            record.timestamp
                ->toMSecsSinceEpoch();

        elevatedRecords.append(
            elevatedRecord
            );
    }

    if (elevatedRecords.isEmpty()) {
        return {};
    }

    /*
     * Detection must not depend on the current
     * table sort order or source-file ordering.
     */
    std::sort(
        elevatedRecords.begin(),
        elevatedRecords.end(),
        elevatedRecordLessThan
        );

    QVector<DetectedBurstRange> detectedRanges;

    qsizetype leftIndex = 0;

    int errorCriticalCount = 0;

    for (qsizetype rightIndex = 0;
         rightIndex < elevatedRecords.size();
         ++rightIndex) {
        const RecordSeverity rightSeverity =
            elevatedRecords.at(rightIndex)
                .record
                ->severity
                .value();

        if (isErrorOrCritical(
                rightSeverity
                )) {
            ++errorCriticalCount;
        }

        /*
         * The window is inclusive: events exactly
         * windowMilliseconds apart can qualify.
         */
        while (leftIndex <= rightIndex
               && elevatedRecords.at(
                    rightIndex
                    )
                    .epochMilliseconds
                        - elevatedRecords.at(
                            leftIndex
                            )
                            .epochMilliseconds
                    > settings.windowMilliseconds) {
            const RecordSeverity leftSeverity =
                elevatedRecords.at(leftIndex)
                    .record
                    ->severity
                    .value();

            if (isErrorOrCritical(
                    leftSeverity
                    )) {
                --errorCriticalCount;
            }

            ++leftIndex;
        }

        const qsizetype elevatedCount =
            rightIndex
            - leftIndex
            + 1;

        const bool meetsElevatedThreshold =
            elevatedCount
            >= static_cast<qsizetype>(
                settings.elevatedEventThreshold
                );

        const bool meetsErrorCriticalThreshold =
            errorCriticalCount
            >= settings.errorCriticalThreshold;

        if (!meetsElevatedThreshold
            && !meetsErrorCriticalThreshold) {
            continue;
        }

        DetectedBurstRange detectedRange;

        detectedRange.startIndex =
            leftIndex;

        detectedRange.endIndex =
            rightIndex;

        detectedRange
            .triggeredByElevatedThreshold =
            meetsElevatedThreshold;

        detectedRange
            .triggeredByErrorCriticalThreshold =
            meetsErrorCriticalThreshold;

        if (!detectedRanges.isEmpty()) {
            DetectedBurstRange &previousRange =
                detectedRanges.last();

            const qint64 previousEndMilliseconds =
                elevatedRecords.at(
                    previousRange.endIndex
                    )
                    .epochMilliseconds;

            const qint64 nextStartMilliseconds =
                elevatedRecords.at(
                    detectedRange.startIndex
                    )
                    .epochMilliseconds;

            if (rangesCanMerge(
                    previousEndMilliseconds,
                    nextStartMilliseconds,
                    settings.mergeGapMilliseconds
                    )) {
                previousRange.endIndex =
                    std::max(
                        previousRange.endIndex,
                        detectedRange.endIndex
                        );

                previousRange
                    .triggeredByElevatedThreshold =
                    previousRange
                        .triggeredByElevatedThreshold
                    || detectedRange
                           .triggeredByElevatedThreshold;

                previousRange
                    .triggeredByErrorCriticalThreshold =
                    previousRange
                        .triggeredByErrorCriticalThreshold
                    || detectedRange
                           .triggeredByErrorCriticalThreshold;

                continue;
            }
        }

        detectedRanges.append(
            detectedRange
            );
    }

    QVector<InvestigationBurst> bursts;

    bursts.reserve(
        detectedRanges.size()
        );

    for (const DetectedBurstRange &range
         : detectedRanges) {
        InvestigationBurst burst;

        burst.startTimestamp =
            elevatedRecords.at(
                range.startIndex
                )
                .record
                ->timestamp
                ->toUTC();

        burst.endTimestamp =
            elevatedRecords.at(
                range.endIndex
                )
                .record
                ->timestamp
                ->toUTC();

        burst.triggeredByElevatedThreshold =
            range.triggeredByElevatedThreshold;

        burst.triggeredByErrorCriticalThreshold =
            range.triggeredByErrorCriticalThreshold;

        burst.settings =
            settings;

        /*
         * Include every elevated record between
         * the merged episode boundaries, including
         * records that did not independently cause
         * a qualifying window. They are still part
         * of the diagnostic episode.
         */
        for (qsizetype recordIndex =
             range.startIndex;
             recordIndex <= range.endIndex;
             ++recordIndex) {
            addRecordToBurst(
                burst,
                *elevatedRecords.at(
                    recordIndex
                    )
                    .record
                );
        }

        bursts.append(
            burst
            );
    }

    return bursts;
}