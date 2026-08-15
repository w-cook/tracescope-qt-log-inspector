#include "EventTimelineAnalyzer.h"

#include <algorithm>
#include <limits>
#include <optional>

#include <QDateTime>
#include <QTimeZone>
#include <QHash>

namespace
{
constexpr qint64 MillisecondsPerMinute =
    60 * 1000;

std::optional<qint64>
normalizedIntervalEpochMilliseconds(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds
    )
{
    if (!timestamp.isValid()
        || intervalMilliseconds <= 0) {
        return std::nullopt;
    }

    const qint64 epochMilliseconds =
        timestamp.toMSecsSinceEpoch();

    qint64 remainder =
        epochMilliseconds
        % intervalMilliseconds;

    /*
     * C++ remainder retains the sign of the
     * dividend. Normalize timestamps before the
     * Unix epoch to the same floor behavior.
     */
    if (remainder < 0) {
        remainder +=
            intervalMilliseconds;
    }

    return epochMilliseconds
           - remainder;
}

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

QString bucketLabel(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds,
    bool spansMultipleDates
    )
{
    if (intervalMilliseconds < 1000) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "MM-dd HH:mm:ss.zzz"
                      )
                : QStringLiteral(
                      "HH:mm:ss.zzz"
                      )
            );
    }

    if (intervalMilliseconds
        < MillisecondsPerMinute) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "MM-dd HH:mm:ss"
                      )
                : QStringLiteral(
                      "HH:mm:ss"
                      )
            );
    }

    if (intervalMilliseconds
        < 24 * 60 * MillisecondsPerMinute) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "yyyy-MM-dd HH:mm"
                      )
                : QStringLiteral(
                      "HH:mm"
                      )
            );
    }

    return timestamp.toString(
        QStringLiteral(
            "yyyy-MM-dd"
            )
        );
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
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp
        || intervalMilliseconds <= 0) {
        return 0;
    }

    const auto firstBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            firstTimestamp,
            intervalMilliseconds
            );

    const auto lastBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            lastTimestamp,
            intervalMilliseconds
            );

    if (!firstBucketEpoch.has_value()
        || !lastBucketEpoch.has_value()
        || *firstBucketEpoch
               > *lastBucketEpoch) {
        return 0;
    }

    return (
               *lastBucketEpoch
               - *firstBucketEpoch
               )
               / intervalMilliseconds
           + 1;
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

    const auto firstBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            firstTimestamp,
            intervalMilliseconds
            );

    const auto lastBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            lastTimestamp,
            intervalMilliseconds
            );

    if (!firstBucketEpoch.has_value()
        || !lastBucketEpoch.has_value()
        || *firstBucketEpoch
               > *lastBucketEpoch) {
        return {};
    }

    const qint64 totalBucketCount =
        (
            *lastBucketEpoch
            - *firstBucketEpoch
            )
            / intervalMilliseconds
        + 1;

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

    const qint64 windowFirstEpoch =
        *firstBucketEpoch
        + startBucketIndex
              * intervalMilliseconds;

    const qint64 windowLastEpoch =
        windowFirstEpoch
        + static_cast<qint64>(
              actualBucketCount - 1
              )
              * intervalMilliseconds;

    const bool spansMultipleDates =
        QDateTime::fromMSecsSinceEpoch(
            *firstBucketEpoch,
            QTimeZone::UTC
            )
            .date()
        != QDateTime::fromMSecsSinceEpoch(
               *lastBucketEpoch,
               QTimeZone::UTC
               )
               .date();

    QVector<EventCountBucket> buckets;

    buckets.reserve(
        actualBucketCount
        );

    for (int index = 0;
         index < actualBucketCount;
         ++index) {
        const qint64 bucketEpoch =
            windowFirstEpoch
            + static_cast<qint64>(
                  index
                  )
                  * intervalMilliseconds;

        const QDateTime bucketTimestamp =
            QDateTime::fromMSecsSinceEpoch(
                bucketEpoch,
                QTimeZone::UTC
                );

        EventCountBucket bucket;

        bucket.label =
            bucketLabel(
                bucketTimestamp,
                intervalMilliseconds,
                spansMultipleDates
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

        const auto recordBucketEpoch =
            normalizedIntervalEpochMilliseconds(
                record.timestamp.value(),
                intervalMilliseconds
                );

        if (!recordBucketEpoch.has_value()
            || *recordBucketEpoch
                   < windowFirstEpoch
            || *recordBucketEpoch
                   > windowLastEpoch) {
            continue;
        }

        const qint64 relativeBucketIndex =
            (
                *recordBucketEpoch
                - windowFirstEpoch
                )
            / intervalMilliseconds;

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

    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp
        || intervalMilliseconds <= 0) {
        return scale;
    }

    const auto firstBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            firstTimestamp,
            intervalMilliseconds
            );

    const auto lastBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            lastTimestamp,
            intervalMilliseconds
            );

    if (!firstBucketEpoch.has_value()
        || !lastBucketEpoch.has_value()
        || *firstBucketEpoch
               > *lastBucketEpoch) {
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

        const auto bucketEpoch =
            normalizedIntervalEpochMilliseconds(
                record.timestamp.value(),
                intervalMilliseconds
                );

        if (!bucketEpoch.has_value()
            || *bucketEpoch < *firstBucketEpoch
            || *bucketEpoch > *lastBucketEpoch) {
            continue;
        }

        EventCountBucket &bucket =
            occupiedBuckets[
                *bucketEpoch
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