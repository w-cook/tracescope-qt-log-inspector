#include "JsonLineLogParser.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>

QVector<TelemetryEvent> JsonLineLogParser::parseLines(const QStringList &lines) const
{
    QVector<TelemetryEvent> events;

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(
            trimmed.toUtf8(),
            &parseError
            );

        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            continue;
        }

        const QJsonObject object = document.object();

        TelemetryEvent event;
        event.timestamp = object.value("timestamp").toString();
        event.level = object.value("level").toString();
        event.subsystem = object.value("subsystem").toString();
        event.eventCode = object.value("eventCode").toString();
        event.message = object.value("message").toString();
        event.entityId = object.value("entityId").toString();

        events.push_back(event);
    }

    return events;
}

QVector<TelemetryEvent> JsonLineLogParser::parseFile(const QString &filePath) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    QStringList lines;

    while (!stream.atEnd()) {
        lines.append(stream.readLine());
    }

    return parseLines(lines);
}