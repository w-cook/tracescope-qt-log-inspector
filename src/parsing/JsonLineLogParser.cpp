#include "JsonLineLogParser.h"

#include "../compatibility/TelemetryEventAdapter.h"

ImportResult JsonLineLogParser::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    return importer.importLines(
        lines,
        sourcePath
        );
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
    return toTelemetryEvents(
        importer.importLines(lines).records
        );
}

QVector<TelemetryEvent> JsonLineLogParser::parseFile(
    const QString &filePath
    ) const
{
    return toTelemetryEvents(
        importer.importFile(filePath).records
        );
}