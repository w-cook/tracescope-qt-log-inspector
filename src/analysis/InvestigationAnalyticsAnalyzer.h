#pragma once

#include <QVector>
#include <QStringList>

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

    QVector<InvestigationValueFrequency>
    subsystemFrequencies(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<InvestigationValueTrendBucket>
    subsystemTrendsWindow(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds,
        qint64 startBucketIndex,
        int bucketCount
        ) const;

    int subsystemTrendScaleMaximum(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds,
        const QStringList &subsystems
        ) const;
};