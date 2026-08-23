#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

class AnalysisTimeBucketRange
{
public:
    static std::optional<AnalysisTimeBucketRange>
    create(
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        );

    qint64 bucketCount() const;

    qint64 intervalMilliseconds() const;

    QDateTime bucketTimestamp(
        qint64 bucketIndex
        ) const;

    QString bucketLabel(
        qint64 bucketIndex
        ) const;

    std::optional<qint64>
    bucketIndexForTimestamp(
        const QDateTime &timestamp
        ) const;

private:
    AnalysisTimeBucketRange(
        qint64 firstBucketEpochMilliseconds,
        qint64 lastBucketEpochMilliseconds,
        qint64 intervalMilliseconds
        );

    qint64 m_firstBucketEpochMilliseconds = 0;
    qint64 m_lastBucketEpochMilliseconds = 0;
    qint64 m_intervalMilliseconds = 0;
};