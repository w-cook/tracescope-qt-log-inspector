#pragma once

#include <QString>
#include <QVector>

#include "../domain/TelemetryEvent.h"

class TelemetryCsvExporter
{
public:
    bool exportToFile(
        const QVector<TelemetryEvent> &events,
        const QString &filePath
        ) const;

private:
    QString escapeCsvField(const QString &value) const;
};