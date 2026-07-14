#pragma once

#include <QVector>

#include "../domain/TelemetryEvent.h"
#include "EventCountBucket.h"

class EventTimelineAnalyzer
{
public:
    QVector<EventCountBucket> groupEventsByMinute(
        const QVector<TelemetryEvent> &events
        ) const;
};