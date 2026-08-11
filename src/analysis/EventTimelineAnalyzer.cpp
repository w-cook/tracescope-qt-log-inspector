#include "EventTimelineAnalyzer.h"

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QTime>

namespace
{
std::optional<QDateTime> eventMinute(
    const TelemetryEvent &event
    )
{
    QDateTime timestamp =
        QDateTime::fromString(
            event.timestamp,
            Qt::ISODateWithMs
            );

    if (!timestamp.isValid()) {
        timestamp =
            QDateTime::fromString(
                event.timestamp,
                Qt::ISODate
                );
    }

    if (!timestamp.isValid()) {
        return std::nullopt;
    }

    timestamp =
        timestamp.toUTC();

    timestamp.setTime(
        QTime(
            timestamp.time().hour(),
            timestamp.time().minute()
            )
        );

    return timestamp;
}

std::optional<QDateTime> earliestMinute(
    const QVector<TelemetryEvent> &events
    )
{
    std::optional<QDateTime> earliest;

    for (const TelemetryEvent &event : events) {
        const auto minute =
            eventMinute(event);

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
    const QVector<TelemetryEvent> &events
    )
{
    std::optional<QDateTime> latest;

    for (const TelemetryEvent &event : events) {
        const auto minute =
            eventMinute(event);

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

QVector<EventCountBucket> EventTimelineAnalyzer::groupEventsByMinute(
    const QVector<TelemetryEvent> &events
    ) const
{
    return groupEventsByMinute(
        events,
        events
        );
}

QVector<EventCountBucket> EventTimelineAnalyzer::groupEventsByMinute(
    const QVector<TelemetryEvent> &events,
    const QVector<TelemetryEvent> &rangeEvents
    ) const
{
    const auto firstMinute =
        earliestMinute(rangeEvents);

    const auto lastMinute =
        latestMinute(rangeEvents);

    if (!firstMinute.has_value()
        || !lastMinute.has_value()) {
        return {};
    }

    const bool spansMultipleDates =
        firstMinute->date()
        != lastMinute->date();

    QMap<QDateTime, EventCountBucket>
        bucketsByMinute;

    for (
        QDateTime minute = *firstMinute;
        minute <= *lastMinute;
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

    for (const TelemetryEvent &event : events) {
        const auto minute =
            eventMinute(event);

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

        if (event.level
            == QStringLiteral("INFO")) {
            ++bucket.infoCount;
        } else if (
            event.level
            == QStringLiteral("WARN")
            ) {
            ++bucket.warningCount;
        } else if (
            event.level
            == QStringLiteral("ERROR")
            ) {
            ++bucket.errorCount;
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