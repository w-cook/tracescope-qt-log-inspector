#include "AnalysisTimeBucketRange.h"

#include <QTimeZone>

namespace
{
constexpr qint64 MillisecondsPerMinute =
    60 * 1000;

std::optional<qint64>
normalizedIntervalEpochMilliseconds(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds
    )
{
    if (!timestamp.isValid()
        || intervalMilliseconds <= 0) {
        return std::nullopt;
    }

    const qint64 epochMilliseconds =
        timestamp.toMSecsSinceEpoch();

    qint64 remainder =
        epochMilliseconds
        % intervalMilliseconds;

    /*
     * C++ remainder retains the sign of the
     * dividend. Normalize timestamps before the
     * Unix epoch to floor toward the preceding
     * interval boundary.
     */
    if (remainder < 0) {
        remainder +=
            intervalMilliseconds;
    }

    return epochMilliseconds
           - remainder;
}

QString labelForTimestamp(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds,
    bool spansMultipleDates
    )
{
    if (intervalMilliseconds < 1000) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "MM-dd HH:mm:ss.zzz"
                      )
                : QStringLiteral(
                      "HH:mm:ss.zzz"
                      )
            );
    }

    if (intervalMilliseconds
        < MillisecondsPerMinute) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "MM-dd HH:mm:ss"
                      )
                : QStringLiteral(
                      "HH:mm:ss"
                      )
            );
    }

    if (intervalMilliseconds
        < 24 * 60 * MillisecondsPerMinute) {
        return timestamp.toString(
            spansMultipleDates
                ? QStringLiteral(
                      "yyyy-MM-dd HH:mm"
                      )
                : QStringLiteral(
                      "HH:mm"
                      )
            );
    }

    return timestamp.toString(
        QStringLiteral(
            "yyyy-MM-dd"
            )
        );
}
}

AnalysisTimeBucketRange::
    AnalysisTimeBucketRange(
        qint64 firstBucketEpochMilliseconds,
        qint64 lastBucketEpochMilliseconds,
        qint64 intervalMilliseconds
        )
    : m_firstBucketEpochMilliseconds(
          firstBucketEpochMilliseconds
          ),
    m_lastBucketEpochMilliseconds(
        lastBucketEpochMilliseconds
        ),
    m_intervalMilliseconds(
        intervalMilliseconds
        )
{
}

std::optional<AnalysisTimeBucketRange>
AnalysisTimeBucketRange::create(
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp,
    qint64 intervalMilliseconds
    )
{
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp
        || intervalMilliseconds <= 0) {
        return std::nullopt;
    }

    const auto firstBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            firstTimestamp,
            intervalMilliseconds
            );

    const auto lastBucketEpoch =
        normalizedIntervalEpochMilliseconds(
            lastTimestamp,
            intervalMilliseconds
            );

    if (!firstBucketEpoch.has_value()
        || !lastBucketEpoch.has_value()
        || *firstBucketEpoch
               > *lastBucketEpoch) {
        return std::nullopt;
    }

    return AnalysisTimeBucketRange(
        *firstBucketEpoch,
        *lastBucketEpoch,
        intervalMilliseconds
        );
}

qint64 AnalysisTimeBucketRange::
    bucketCount() const
{
    return (
               m_lastBucketEpochMilliseconds
               - m_firstBucketEpochMilliseconds
               )
               / m_intervalMilliseconds
           + 1;
}

qint64 AnalysisTimeBucketRange::
    intervalMilliseconds() const
{
    return m_intervalMilliseconds;
}

QDateTime AnalysisTimeBucketRange::
    bucketTimestamp(
        qint64 bucketIndex
        ) const
{
    if (bucketIndex < 0
        || bucketIndex >= bucketCount()) {
        return {};
    }

    const qint64 bucketEpoch =
        m_firstBucketEpochMilliseconds
        + bucketIndex
              * m_intervalMilliseconds;

    return QDateTime::fromMSecsSinceEpoch(
        bucketEpoch,
        QTimeZone::UTC
        );
}

QString AnalysisTimeBucketRange::
    bucketLabel(
        qint64 bucketIndex
        ) const
{
    const QDateTime timestamp =
        bucketTimestamp(bucketIndex);

    if (!timestamp.isValid()) {
        return {};
    }

    const QDateTime firstTimestamp =
        QDateTime::fromMSecsSinceEpoch(
            m_firstBucketEpochMilliseconds,
            QTimeZone::UTC
            );

    const QDateTime lastTimestamp =
        QDateTime::fromMSecsSinceEpoch(
            m_lastBucketEpochMilliseconds,
            QTimeZone::UTC
            );

    const bool spansMultipleDates =
        firstTimestamp.date()
        != lastTimestamp.date();

    return labelForTimestamp(
        timestamp,
        m_intervalMilliseconds,
        spansMultipleDates
        );
}

std::optional<qint64>
    AnalysisTimeBucketRange::
    bucketIndexForTimestamp(
        const QDateTime &timestamp
        ) const
{
    const auto bucketEpoch =
        normalizedIntervalEpochMilliseconds(
            timestamp,
            m_intervalMilliseconds
            );

    if (!bucketEpoch.has_value()
        || *bucketEpoch
               < m_firstBucketEpochMilliseconds
        || *bucketEpoch
               > m_lastBucketEpochMilliseconds) {
        return std::nullopt;
    }

    return (
               *bucketEpoch
               - m_firstBucketEpochMilliseconds
               )
           / m_intervalMilliseconds;
}