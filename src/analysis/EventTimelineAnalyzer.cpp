#include "EventTimelineAnalyzer.h"

#include <QDateTime>
#include <QMap>

QVector<EventCountBucket> EventTimelineAnalyzer::groupEventsByMinute(
    const QVector<TelemetryEvent> &events
    ) const
{
    QMap<QString, EventCountBucket> bucketsByMinute;

    for (const TelemetryEvent &event : events) {
        const QDateTime timestamp = QDateTime::fromString(
            event.timestamp,
            Qt::ISODateWithMs
            );

        if (!timestamp.isValid()) {
            continue;
        }

        const QString bucketLabel = timestamp.toUTC().toString("HH:mm");

        if (!bucketsByMinute.contains(bucketLabel)) {
            EventCountBucket bucket;
            bucket.label = bucketLabel;
            bucketsByMinute.insert(bucketLabel, bucket);
        }

        EventCountBucket bucket = bucketsByMinute.value(bucketLabel);

        if (event.level == "INFO") {
            ++bucket.infoCount;
        } else if (event.level == "WARN") {
            ++bucket.warningCount;
        } else if (event.level == "ERROR") {
            ++bucket.errorCount;
        }

        bucketsByMinute.insert(bucketLabel, bucket);
    }

    QVector<EventCountBucket> buckets;

    for (const EventCountBucket &bucket : bucketsByMinute) {
        buckets.push_back(bucket);
    }

    return buckets;
}