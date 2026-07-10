#pragma once

#include <QVector>

#include "../domain/TelemetryEvent.h"
#include "TelemetryFilterCriteria.h"

class TelemetryEventFilter
{
public:
    QVector<TelemetryEvent> apply(
        const QVector<TelemetryEvent> &events,
        const TelemetryFilterCriteria &criteria
        ) const;

private:
    bool matchesSearchText(
        const TelemetryEvent &event,
        const QString &searchText
        ) const;
};