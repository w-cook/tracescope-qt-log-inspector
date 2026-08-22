#include "InvestigationAnalyticsAnalyzer.h"

#include <QMap>

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
        || range->bucketCount() >
               std::numeric_limits<int>::max()) {
        return {};
    }

    QVector<InvestigationValueTrendBucket>
        buckets;

    buckets.reserve(
        static_cast<int>(
            range->bucketCount()
            )
        );

    for (qint64 bucketIndex = 0;
         bucketIndex < range->bucketCount();
         ++bucketIndex) {
        InvestigationValueTrendBucket bucket;

        bucket.startTimestamp =
            range->bucketTimestamp(
                bucketIndex
                );

        bucket.label =
            range->bucketLabel(
                bucketIndex
                );

        buckets.append(
            bucket
            );
    }

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

        ++buckets[
              static_cast<int>(
                  *bucketIndex
                  )
        ].countsByValue[
                  record.subsystem.value()
        ];
    }

    return buckets;
}