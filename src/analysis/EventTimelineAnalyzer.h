#pragma once

#include <QVector>
#include <QDateTime>

#include "../domain/InvestigationRecord.h"
#include "EventCountBucket.h"

class EventTimelineAnalyzer
{
public:
    QVector<EventCountBucket>
    groupRecordsByMinute(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<EventCountBucket>
    groupRecordsByMinute(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstMinute,
        const QDateTime &lastMinute
        ) const;
};