#include "EventTimelineAnalyzer.h"

#include <algorithm>
#include <limits>
#include <optional>

#include <QDateTime>
#include <QTimeZone>
#include <QHash>

#include "AnalysisTimeBucketRange.h"

namespace
{
constexpr qint64 MillisecondsPerMinute =
    60 * 1000;

std::optional<QDateTime>
earliestTimestamp(
    const QVector<InvestigationRecord> &records
    )
{
    std::optional<QDateTime> earliest;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        const QDateTime timestamp =
            record.timestamp->toUTC();

        if (!earliest.has_value()
            || timestamp < *earliest) {
            earliest =
                timestamp;
        }
    }

    return earliest;
}

std::optional<QDateTime>
latestTimestamp(
    const QVector<InvestigationRecord> &records
    )
{
    std::optional<QDateTime> latest;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        const QDateTime timestamp =
            record.timestamp->toUTC();

        if (!latest.has_value()
            || timestamp > *latest) {
            latest =
                timestamp;
        }
    }

    return latest;
}

void incrementBucket(
    EventCountBucket &bucket,
    const InvestigationRecord &record
    )
{
    if (!record.severity.has_value()) {
        ++bucket.unspecifiedCount;
        return;
    }

    switch (record.severity.value()) {
    case RecordSeverity::Trace:
        ++bucket.traceCount;
        break;

    case RecordSeverity::Debug:
        ++bucket.debugCount;
        break;

    case RecordSeverity::Info:
        ++bucket.infoCount;
        break;

    case RecordSeverity::Warning:
        ++bucket.warningCount;
        break;

    case RecordSeverity::Error:
        ++bucket.errorCount;
        break;

    case RecordSeverity::Critical:
        ++bucket.criticalCount;
        break;
    }
}
}

QVector<EventCountBucket>
EventTimelineAnalyzer::groupRecordsByMinute(
    const QVector<InvestigationRecord> &records
    ) const
{
    const auto firstTimestamp =
        earliestTimestamp(
            records
            );

    const auto lastTimestamp =
        latestTimestamp(
            records
            );

    if (!firstTimestamp.has_value()
        || !lastTimestamp.has_value()) {
        return {};
    }

    return groupRecordsByMinute(
        records,
        *firstTimestamp,
        *lastTimestamp
        );
}

QVector<EventCountBucket>
EventTimelineAnalyzer::groupRecordsByMinute(
    const QVector<InvestigationRecord> &records,
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp
    ) const
{
    return groupRecordsByIntervalMilliseconds(
        records,
        firstTimestamp,
        lastTimestamp,
        MillisecondsPerMinute
        );
}

QVector<EventCountBucket>
EventTimelineAnalyzer::groupRecordsByInterval(
    const QVector<InvestigationRecord> &records,
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp,
    qint64 intervalMinutes
    ) const
{
    if (intervalMinutes <= 0
        || intervalMinutes
               > std::numeric_limits<qint64>::max()
                     / MillisecondsPerMinute) {
        return {};
    }

    return groupRecordsByIntervalMilliseconds(
        records,
        firstTimestamp,
        lastTimestamp,
        intervalMinutes
            * MillisecondsPerMinute
        );
}

qint64
    EventTimelineAnalyzer::
    intervalBucketCountMilliseconds(
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const
{
    const auto range =
        AnalysisTimeBucketRange::create(
            firstTimestamp,
            lastTimestamp,
            intervalMilliseconds
            );

    return range.has_value()
               ? range->bucketCount()
               : 0;
}

QVector<EventCountBucket>
    EventTimelineAnalyzer::
    groupRecordsByIntervalMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const
{
    const qint64 bucketCount =
        intervalBucketCountMilliseconds(
            firstTimestamp,
            lastTimestamp,
            intervalMilliseconds
            );

    if (bucketCount <= 0
        || bucketCount
               > std::numeric_limits<int>::max()) {
        return {};
    }

    return groupRecordsByIntervalWindowMilliseconds(
        records,
        firstTimestamp,
        lastTimestamp,
        intervalMilliseconds,
        0,
        static_cast<int>(
            bucketCount
            )
        );
}

QVector<EventCountBucket>
    EventTimelineAnalyzer::
    groupRecordsByIntervalWindowMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds,
        qint64 startBucketIndex,
        int requestedBucketCount
        ) const
{
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp
        || intervalMilliseconds <= 0
        || startBucketIndex < 0
        || requestedBucketCount <= 0) {
        return {};
    }

    const auto range =
        AnalysisTimeBucketRange::create(
            firstTimestamp,
            lastTimestamp,
            intervalMilliseconds
            );

    if (!range.has_value()) {
        return {};
    }

    const qint64 totalBucketCount =
        range->bucketCount();

    if (startBucketIndex
        >= totalBucketCount) {
        return {};
    }

    const qint64 remainingBucketCount =
        totalBucketCount
        - startBucketIndex;

    const int actualBucketCount =
        static_cast<int>(
            std::min<qint64>(
                requestedBucketCount,
                remainingBucketCount
                )
            );

    QVector<EventCountBucket> buckets;

    buckets.reserve(
        actualBucketCount
        );

    for (int index = 0;
         index < actualBucketCount;
         ++index) {
        EventCountBucket bucket;

        bucket.label =
            range->bucketLabel(
                startBucketIndex
                + index
                );

        buckets.append(
            bucket
            );
    }

    /*
     * Only records that can affect the currently
     * materialized window are assigned to buckets.
     * The rest of the investigation does not cause
     * empty bucket objects to be allocated.
     */
    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        const auto bucketIndex =
            range->bucketIndexForTimestamp(
                record.timestamp.value()
                );

        if (!bucketIndex.has_value()) {
            continue;
        }

        const qint64 relativeBucketIndex =
            *bucketIndex
            - startBucketIndex;

        if (relativeBucketIndex < 0
            || relativeBucketIndex
                   >= actualBucketCount) {
            continue;
        }

        incrementBucket(
            buckets[
                static_cast<int>(
                    relativeBucketIndex
                    )
        ],
            record
            );
    }

    return buckets;
}

EventTimelineScale
    EventTimelineAnalyzer::
    scaleForIntervalMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const
{
    EventTimelineScale scale;

    const auto range =
        AnalysisTimeBucketRange::create(
            firstTimestamp,
            lastTimestamp,
            intervalMilliseconds
            );

    if (!range.has_value()) {
        return scale;
    }

    /*
     * Only occupied buckets are stored here.
     * A four-hour 1-ms investigation may contain
     * millions of possible buckets, but a file
     * containing 200,000 records can populate at
     * most 200,000 of them.
     */
    QHash<qint64, EventCountBucket>
        occupiedBuckets;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        const auto bucketIndex =
            range->bucketIndexForTimestamp(
                record.timestamp.value()
                );

        if (!bucketIndex.has_value()) {
            continue;
        }

        EventCountBucket &bucket =
            occupiedBuckets[
                *bucketIndex
        ];

        incrementBucket(
            bucket,
            record
            );

        scale.maximumTotalCount =
            std::max(
                scale.maximumTotalCount,
                bucket.totalCount()
                );

        scale.maximumSeriesCount =
            std::max({
                scale.maximumSeriesCount,
                bucket.traceCount,
                bucket.debugCount,
                bucket.infoCount,
                bucket.warningCount,
                bucket.errorCount,
                bucket.criticalCount,
                bucket.unspecifiedCount
            });
    }

    return scale;
}