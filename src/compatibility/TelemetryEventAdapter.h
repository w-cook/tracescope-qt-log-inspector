#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "../domain/TelemetryEvent.h"

TelemetryEvent toTelemetryEvent(
    const InvestigationRecord &record
    );

QVector<TelemetryEvent> toTelemetryEvents(
    const QVector<InvestigationRecord> &records
    );