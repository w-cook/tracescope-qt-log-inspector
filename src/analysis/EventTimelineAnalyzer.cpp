#include "EventTimelineAnalyzer.h"

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QTime>

namespace
{
std::optional<QDateTime> normalizedMinute(
    QDateTime timestamp
    )
{
    if (!timestamp.isValid()) {
        return std::nullopt;
    }

    timestamp = timestamp.toUTC();

    timestamp.setTime(
        QTime(
            timestamp.time().hour(),
            timestamp.time().minute()
            )
        );

    return timestamp;
}

std::optional<QDateTime> recordMinute(
    const InvestigationRecord &record
    )
{
    if (!record.timestamp.has_value()) {
        return std::nullopt;
    }

    return normalizedMinute(
        record.timestamp.value()
        );
}

std::optional<QDateTime> earliestMinute(
    const QVector<InvestigationRecord> &records
    )
{
    std::optional<QDateTime> earliest;

    for (const InvestigationRecord &record
         : records) {
        const auto minute =
            recordMinute(record);

        if (!minute.has_value()) {
            continue;
        }

        if (!earliest.has_value()
            || *minute < *earliest) {
            earliest = minute;
        }
    }

    return earliest;
}

std::optional<QDateTime> latestMinute(
    const QVector<InvestigationRecord> &records
    )
{
    std::optional<QDateTime> latest;

    for (const InvestigationRecord &record
         : records) {
        const auto minute =
            recordMinute(record);

        if (!minute.has_value()) {
            continue;
        }

        if (!latest.has_value()
            || *minute > *latest) {
            latest = minute;
        }
    }

    return latest;
}
}

QVector<EventCountBucket> EventTimelineAnalyzer::groupRecordsByMinute(
    const QVector<InvestigationRecord> &records
    ) const
{
    const auto firstMinute =
        earliestMinute(records);

    const auto lastMinute =
        latestMinute(records);

    if (!firstMinute.has_value()
        || !lastMinute.has_value()) {
        return {};
    }

    return groupRecordsByMinute(
        records,
        *firstMinute,
        *lastMinute
        );
}

QVector<EventCountBucket> EventTimelineAnalyzer::groupRecordsByMinute(
    const QVector<InvestigationRecord> &records,
    const QDateTime &firstMinute,
    const QDateTime &lastMinute
    ) const
{
    const auto normalizedFirstMinute =
        normalizedMinute(firstMinute);

    const auto normalizedLastMinute =
        normalizedMinute(lastMinute);

    if (!normalizedFirstMinute.has_value()
        || !normalizedLastMinute.has_value()
        || *normalizedFirstMinute >
               *normalizedLastMinute) {
        return {};
    }

    const bool spansMultipleDates =
        normalizedFirstMinute->date()
        != normalizedLastMinute->date();

    QMap<QDateTime, EventCountBucket>
        bucketsByMinute;

    for (
        QDateTime minute =
        *normalizedFirstMinute;
        minute <= *normalizedLastMinute;
        minute = minute.addSecs(60)
        ) {
        EventCountBucket bucket;

        bucket.label =
            spansMultipleDates
                ? minute.toString(
                      QStringLiteral(
                          "yyyy-MM-dd HH:mm"
                          )
                      )
                : minute.toString(
                      QStringLiteral("HH:mm")
                      );

        bucketsByMinute.insert(
            minute,
            bucket
            );
    }

    for (const InvestigationRecord &record
         : records) {
        const auto minute =
            recordMinute(record);

        if (!minute.has_value()) {
            continue;
        }

        auto iterator =
            bucketsByMinute.find(
                *minute
                );

        if (iterator
            == bucketsByMinute.end()) {
            continue;
        }

        EventCountBucket &bucket =
            iterator.value();

        if (!record.severity.has_value()) {
            ++bucket.unspecifiedCount;
            continue;
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

    QVector<EventCountBucket> buckets;

    buckets.reserve(
        bucketsByMinute.size()
        );

    for (const EventCountBucket &bucket
         : bucketsByMinute) {
        buckets.append(bucket);
    }

    return buckets;
}