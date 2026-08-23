#include "InvestigationAnalyticsAnalyzer.h"

#include <QMap>
#include <QHash>
#include <QSet>

#include <algorithm>
#include <limits>

#include "AnalysisTimeBucketRange.h"

namespace
{
template<typename ValueSelector>
QVector<InvestigationValueFrequency>
frequenciesFor(
    const QVector<InvestigationRecord> &records,
    ValueSelector selector
    )
{
    QMap<QString, int> counts;

    for (const InvestigationRecord &record
         : records) {
        const std::optional<QString> &value =
            selector(record);

        if (!value.has_value()
            || value->trimmed().isEmpty()) {
            continue;
        }

        ++counts[value.value()];
    }

    QVector<InvestigationValueFrequency>
        frequencies;

    frequencies.reserve(counts.size());

    for (auto iterator = counts.cbegin();
         iterator != counts.cend();
         ++iterator) {
        InvestigationValueFrequency frequency;
        frequency.value = iterator.key();
        frequency.count = iterator.value();

        frequencies.append(frequency);
    }

    std::sort(
        frequencies.begin(),
        frequencies.end(),
        [](
            const InvestigationValueFrequency &left,
            const InvestigationValueFrequency &right
            ) {
            if (left.count == right.count) {
                return QString::compare(
                           left.value,
                           right.value,
                           Qt::CaseSensitive
                           )
                       < 0;
            }

            return left.count > right.count;
        }
        );

    return frequencies;
}
}

QVector<InvestigationValueFrequency>
    InvestigationAnalyticsAnalyzer::
    eventCodeFrequencies(
        const QVector<InvestigationRecord> &records
        ) const
{
    return frequenciesFor(
        records,
        [](
            const InvestigationRecord &record
            ) -> const std::optional<QString> & {
            return record.eventCode;
        }
        );
}

QVector<InvestigationValueFrequency>
    InvestigationAnalyticsAnalyzer::
    entityFrequencies(
        const QVector<InvestigationRecord> &records
        ) const
{
    return frequenciesFor(
        records,
        [](
            const InvestigationRecord &record
            ) -> const std::optional<QString> & {
            return record.entityId;
        }
        );
}

QVector<InvestigationValueFrequency>
    InvestigationAnalyticsAnalyzer::
    subsystemFrequencies(
        const QVector<InvestigationRecord> &records
        ) const
{
    return frequenciesFor(
        records,
        [](
            const InvestigationRecord &record
            ) -> const std::optional<QString> & {
            return record.subsystem;
        }
        );
}

QVector<InvestigationValueTrendBucket>
    InvestigationAnalyticsAnalyzer::
    subsystemTrends(
        const QVector<InvestigationRecord> &records,
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

    if (!range.has_value()
        || range->bucketCount() <= 0
        || range->bucketCount()
               > std::numeric_limits<int>::max()) {
        return {};
    }

    return subsystemTrendsWindow(
        records,
        firstTimestamp,
        lastTimestamp,
        intervalMilliseconds,
        0,
        static_cast<int>(
            range->bucketCount()
            )
        );
}

QVector<InvestigationValueTrendBucket>
    InvestigationAnalyticsAnalyzer::
    subsystemTrendsWindow(
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

    QVector<InvestigationValueTrendBucket>
        buckets;

    buckets.reserve(
        actualBucketCount
        );

    for (int index = 0;
         index < actualBucketCount;
         ++index) {
        const qint64 absoluteBucketIndex =
            startBucketIndex
            + index;

        InvestigationValueTrendBucket bucket;

        bucket.startTimestamp =
            range->bucketTimestamp(
                absoluteBucketIndex
                );

        bucket.label =
            range->bucketLabel(
                absoluteBucketIndex
                );

        buckets.append(
            bucket
            );
    }

    /*
     * Only assign records that can contribute to
     * the currently materialized window.
     */
    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()
            || !record.subsystem.has_value()
            || record.subsystem
                   ->trimmed()
                   .isEmpty()) {
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

        ++buckets[
              static_cast<int>(
                  relativeBucketIndex
                  )
        ].countsByValue[
                  record.subsystem.value()
        ];
    }

    return buckets;
}

int InvestigationAnalyticsAnalyzer::
    subsystemTrendScaleMaximum(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds,
        const QStringList &subsystems
        ) const
{
    if (subsystems.isEmpty()) {
        return 0;
    }

    const auto range =
        AnalysisTimeBucketRange::create(
            firstTimestamp,
            lastTimestamp,
            intervalMilliseconds
            );

    if (!range.has_value()) {
        return 0;
    }

    QSet<QString> includedSubsystems;

    for (const QString &subsystem
         : subsystems) {
        if (!subsystem.trimmed().isEmpty()) {
            includedSubsystems.insert(
                subsystem
                );
        }
    }

    if (includedSubsystems.isEmpty()) {
        return 0;
    }

    /*
     * Store only occupied logical buckets.
     *
     * Fine-resolution investigations can have
     * millions of possible buckets, but no more
     * occupied buckets than contributing records.
     */
    QHash<
        qint64,
        QHash<QString, int>>
        occupiedBuckets;

    int maximum = 0;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()
            || !record.subsystem.has_value()) {
            continue;
        }

        const QString subsystem =
            record.subsystem.value();

        if (!includedSubsystems.contains(
                subsystem
                )) {
            continue;
        }

        const auto bucketIndex =
            range->bucketIndexForTimestamp(
                record.timestamp.value()
                );

        if (!bucketIndex.has_value()) {
            continue;
        }

        int &count =
            occupiedBuckets[
                *bucketIndex
        ][
                subsystem
        ];

        ++count;

        maximum =
            std::max(
                maximum,
                count
                );
    }

    return maximum;
}