#include "JsonLineLogParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
TelemetryEvent createLegacyTelemetryEvent(
    const InvestigationRecord &record
    )
{
    const QJsonDocument document =
        QJsonDocument::fromJson(
            record.rawSource.trimmed().toUtf8()
            );

    const QJsonObject object = document.object();

    TelemetryEvent event;

    event.timestamp =
        object.value(QStringLiteral("timestamp")).toString();

    event.level =
        object.value(QStringLiteral("level")).toString();

    event.subsystem =
        object.value(QStringLiteral("subsystem")).toString();

    event.eventCode =
        object.value(QStringLiteral("eventCode")).toString();

    event.message =
        object.value(QStringLiteral("message")).toString();

    event.entityId =
        object.value(QStringLiteral("entityId")).toString();

    return event;
}

QVector<TelemetryEvent> createLegacyTelemetryEvents(
    const ImportResult &result
    )
{
    QVector<TelemetryEvent> events;
    events.reserve(result.records.size());

    for (const InvestigationRecord &record : result.records) {
        events.append(
            createLegacyTelemetryEvent(record)
            );
    }

    return events;
}
}

ImportResult JsonLineLogParser::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    return importer.importLines(lines, sourcePath);
}

ImportResult JsonLineLogParser::importFile(
    const QString &filePath
    ) const
{
    return importer.importFile(filePath);
}

QVector<TelemetryEvent> JsonLineLogParser::parseLines(
    const QStringList &lines
    ) const
{
    return createLegacyTelemetryEvents(
        importer.importLines(lines)
        );
}

QVector<TelemetryEvent> JsonLineLogParser::parseFile(
    const QString &filePath
    ) const
{
    return createLegacyTelemetryEvents(
        importer.importFile(filePath)
        );
}