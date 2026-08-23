#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>

#include "../domain/RecordSeverity.h"
#include "BurstDetectionSettings.h"

struct InvestigationBurst
{
    QDateTime startTimestamp;
    QDateTime endTimestamp;

    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;

    QVector<QString> recordIds;

    QMap<QString, int> subsystemCounts;
    QMap<QString, int> eventCodeCounts;
    QMap<QString, int> entityCounts;

    bool triggeredByElevatedThreshold = false;
    bool triggeredByErrorCriticalThreshold = false;

    BurstDetectionSettings settings;

    int totalElevatedCount() const
    {
        return warningCount
               + errorCount
               + criticalCount;
    }

    qint64 durationMilliseconds() const
    {
        if (!startTimestamp.isValid()
            || !endTimestamp.isValid()
            || endTimestamp < startTimestamp) {
            return 0;
        }

        return startTimestamp.msecsTo(
            endTimestamp
            );
    }

    RecordSeverity highestSeverity() const
    {
        if (criticalCount > 0) {
            return RecordSeverity::Critical;
        }

        if (errorCount > 0) {
            return RecordSeverity::Error;
        }

        return RecordSeverity::Warning;
    }
};