#pragma once

#include <QDateTime>
#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "EventCountBucket.h"

struct EventTimelineScale
{
    int maximumTotalCount = 0;
    int maximumSeriesCount = 0;
};

class EventTimelineAnalyzer
{
public:
    /*
     * Existing minute-based API retained for
     * compatibility with current callers/tests.
     */
    QVector<EventCountBucket>
    groupRecordsByMinute(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<EventCountBucket>
    groupRecordsByMinute(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp
        ) const;

    QVector<EventCountBucket>
    groupRecordsByInterval(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMinutes
        ) const;

    /*
     * Millisecond-resolution API used by the
     * responsive timeline UI.
     */
    qint64 intervalBucketCountMilliseconds(
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const;

    QVector<EventCountBucket>
    groupRecordsByIntervalMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const;

    QVector<EventCountBucket>
    groupRecordsByIntervalWindowMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds,
        qint64 startBucketIndex,
        int bucketCount
        ) const;

    EventTimelineScale
    scaleForIntervalMilliseconds(
        const QVector<InvestigationRecord> &records,
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const;
};