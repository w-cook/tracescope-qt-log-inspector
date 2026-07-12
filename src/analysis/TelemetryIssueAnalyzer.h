#pragma once

#include <QVector>

#include "../domain/TelemetryEvent.h"
#include "TelemetryIssueGroup.h"

class TelemetryIssueAnalyzer
{
public:
    QVector<TelemetryIssueGroup> groupWarningsAndErrorsBySubsystem(
        const QVector<TelemetryEvent> &events
        ) const;
};