#pragma once

#include <QVector>
#include <QDateTime>

#include "../domain/TelemetryEvent.h"
#include "EventCountBucket.h"

class EventTimelineAnalyzer
{
public:
    QVector<EventCountBucket> groupEventsByMinute(
        const QVector<TelemetryEvent> &events
        ) const;

    QVector<EventCountBucket> groupEventsByMinute(
        const QVector<TelemetryEvent> &events,
        const QDateTime &firstMinute,
        const QDateTime &lastMinute
        ) const;
};