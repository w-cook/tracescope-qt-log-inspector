#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "../domain/TelemetryEvent.h"
#include "../importing/ImportResult.h"
#include "../importing/JsonLinesImporter.h"

class JsonLineLogParser
{
public:
    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;

    ImportResult importFile(
        const QString &filePath
        ) const;

    QVector<TelemetryEvent> parseLines(
        const QStringList &lines
        ) const;

    QVector<TelemetryEvent> parseFile(
        const QString &filePath
        ) const;

private:
    JsonLinesImporter importer;
};