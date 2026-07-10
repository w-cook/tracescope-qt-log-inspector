#include "TelemetryEventFilter.h"

QVector<TelemetryEvent> TelemetryEventFilter::apply(
    const QVector<TelemetryEvent> &events,
    const TelemetryFilterCriteria &criteria
    ) const
{
    QVector<TelemetryEvent> filtered;

    const QString normalizedSearchText = criteria.searchText.trimmed();

    for (const TelemetryEvent &event : events) {
        if (!criteria.level.isEmpty() && event.level != criteria.level) {
            continue;
        }

        if (!criteria.subsystem.isEmpty() && event.subsystem != criteria.subsystem) {
            continue;
        }

        if (!normalizedSearchText.isEmpty()
            && !matchesSearchText(event, normalizedSearchText)) {
            continue;
        }

        filtered.push_back(event);
    }

    return filtered;
}

bool TelemetryEventFilter::matchesSearchText(
    const TelemetryEvent &event,
    const QString &searchText
    ) const
{
    return event.timestamp.contains(searchText, Qt::CaseInsensitive)
        || event.level.contains(searchText, Qt::CaseInsensitive)
        || event.subsystem.contains(searchText, Qt::CaseInsensitive)
        || event.eventCode.contains(searchText, Qt::CaseInsensitive)
        || event.message.contains(searchText, Qt::CaseInsensitive)
        || event.entityId.contains(searchText, Qt::CaseInsensitive);
}