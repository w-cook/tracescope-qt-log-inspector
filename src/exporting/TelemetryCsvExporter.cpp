#include "TelemetryCsvExporter.h"

#include <QFile>
#include <QTextStream>

bool TelemetryCsvExporter::exportToFile(
    const QVector<TelemetryEvent> &events,
    const QString &filePath
    ) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);

    stream << "timestamp,level,subsystem,eventCode,entityId,message\n";

    for (const TelemetryEvent &event : events) {
        stream
            << escapeCsvField(event.timestamp) << ","
            << escapeCsvField(event.level) << ","
            << escapeCsvField(event.subsystem) << ","
            << escapeCsvField(event.eventCode) << ","
            << escapeCsvField(event.entityId) << ","
            << escapeCsvField(event.message) << "\n";
    }

    return true;
}

QString TelemetryCsvExporter::escapeCsvField(const QString &value) const
{
    QString escaped = value;
    escaped.replace("\"", "\"\"");

    if (escaped.contains(",")
        || escaped.contains("\"")
        || escaped.contains("\n")
        || escaped.contains("\r")) {
        return "\"" + escaped + "\"";
    }

    return escaped;
}