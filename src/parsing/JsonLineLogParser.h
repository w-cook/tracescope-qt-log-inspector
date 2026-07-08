#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "../domain/TelemetryEvent.h"

class JsonLineLogParser
{
public:
    QVector<TelemetryEvent> parseLines(const QStringList &lines) const;
    QVector<TelemetryEvent> parseFile(const QString &filePath) const;
};