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

    return normalizedMinute(timestamp);
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
    const auto firstMinute =
        earliestMinute(events);

    const auto lastMinute =
        latestMinute(events);

    if (!firstMinute.has_value()
        || !lastMinute.has_value()) {
        return {};
    }

    return groupEventsByMinute(
        events,
        *firstMinute,
        *lastMinute
        );
}

QVector<EventCountBucket> EventTimelineAnalyzer::groupEventsByMinute(
    const QVector<TelemetryEvent> &events,
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
            == QStringLiteral("TRACE")) {
            ++bucket.traceCount;
        } else if (
            event.level
            == QStringLiteral("DEBUG")
            ) {
            ++bucket.debugCount;
        } else if (
            event.level
            == QStringLiteral("INFO")
            ) {
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
        } else if (
            event.level
            == QStringLiteral("CRITICAL")
            ) {
            ++bucket.criticalCount;
        } else {
            ++bucket.unspecifiedCount;
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