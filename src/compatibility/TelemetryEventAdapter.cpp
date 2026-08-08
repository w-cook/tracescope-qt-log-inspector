#include "TelemetryEventAdapter.h"

#include <utility>

#include "../domain/RecordSeverity.h"

TelemetryEvent toTelemetryEvent(
    const InvestigationRecord &record
    )
{
    TelemetryEvent event;

    if (record.timestamp.has_value()) {
        event.timestamp =
            record.timestamp->toString(
                Qt::ISODateWithMs
                );
    }

    if (record.severity.has_value()) {
        event.level =
            recordSeverityToString(
                record.severity.value()
                );
    }

    if (record.subsystem.has_value()) {
        event.subsystem =
            record.subsystem.value();
    }

    if (record.eventCode.has_value()) {
        event.eventCode =
            record.eventCode.value();
    }

    if (record.message.has_value()) {
        event.message =
            record.message.value();
    }

    if (record.entityId.has_value()) {
        event.entityId =
            record.entityId.value();
    }

    return event;
}

QVector<TelemetryEvent> toTelemetryEvents(
    const QVector<InvestigationRecord> &records
    )
{
    QVector<TelemetryEvent> events;
    events.reserve(records.size());

    for (
        const InvestigationRecord &record :
        std::as_const(records)
        ) {
        events.append(
            toTelemetryEvent(record)
            );
    }

    return events;
}