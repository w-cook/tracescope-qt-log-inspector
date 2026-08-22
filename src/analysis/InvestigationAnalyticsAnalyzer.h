#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"

#include "InvestigationValueFrequency.h"
#include "InvestigationValueTrendBucket.h"

class InvestigationAnalyticsAnalyzer
{
public:
    QVector<InvestigationValueFrequency>
    eventCodeFrequencies(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<InvestigationValueFrequency>
    entityFrequencies(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<InvestigationValueTrendBucket>
    subsystemTrends(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const;
};