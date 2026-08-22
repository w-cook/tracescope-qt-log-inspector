#include "InvestigationCadenceAnalyzer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace
{
constexpr int MinimumPositiveGapCount = 10;
constexpr int ExpectedRecordsPerBurstWindow = 20;

constexpr qint64 FallbackBurstWindowMilliseconds =
    30 * 1000;

constexpr qint64 FallbackMergeGapMilliseconds =
    5 * 1000;

constexpr qint64 MillisecondsPerDay =
    24 * 60 * 60 * 1000;

/*
 * Human-friendly durations used for automatic
 * recommendations. Values above one day are
 * rounded upward to whole days.
 */
constexpr std::array<qint64, 25>
    PreferredDurationsMilliseconds = {
        1,
        2,
        5,
        10,
        20,
        50,
        100,
        200,
        500,
        1000,
        2000,
        5000,
        10000,
        15000,
        30000,
        60000,
        2 * 60000,
        5 * 60000,
        10 * 60000,
        15 * 60000,
        30 * 60000,
        60 * 60000,
        2 * 60 * 60000,
        6 * 60 * 60000,
        12 * 60 * 60000
};

qint64 roundDurationUp(
    qint64 durationMilliseconds
    )
{
    if (durationMilliseconds <= 1) {
        return 1;
    }

    for (const qint64 preferredDuration
         : PreferredDurationsMilliseconds) {
        if (durationMilliseconds
            <= preferredDuration) {
            return preferredDuration;
        }
    }

    if (durationMilliseconds
        <= MillisecondsPerDay) {
        return MillisecondsPerDay;
    }

    const qint64 wholeDays =
        durationMilliseconds
            / MillisecondsPerDay
        + (
            durationMilliseconds
                        % MillisecondsPerDay
                    == 0
                ? 0
                : 1
            );

    if (wholeDays
        > std::numeric_limits<qint64>::max()
              / MillisecondsPerDay) {
        return std::numeric_limits<qint64>::max();
    }

    return wholeDays
           * MillisecondsPerDay;
}

double medianOfSortedValues(
    const QVector<qint64> &values
    )
{
    if (values.isEmpty()) {
        return 0.0;
    }

    const int middle =
        values.size() / 2;

    if (values.size() % 2 != 0) {
        return static_cast<double>(
            values.at(middle)
            );
    }

    return (
               static_cast<double>(
                   values.at(middle - 1)
                   )
               + static_cast<double>(
                   values.at(middle)
                   )
               )
           / 2.0;
}

qint64 percentile90OfSortedValues(
    const QVector<qint64> &values
    )
{
    if (values.isEmpty()) {
        return 0;
    }

    /*
     * Nearest-rank percentile:
     *
     * rank = ceil(0.90 * N)
     *
     * Convert the one-based rank to a zero-based
     * QVector index.
     */
    const qsizetype rank =
        static_cast<qsizetype>(
            std::ceil(
                0.90
                * static_cast<double>(
                    values.size()
                    )
                )
            );

    const qsizetype index =
        std::clamp<qsizetype>(
            rank - 1,
            0,
            values.size() - 1
            );

    return values.at(index);
}

qint64 safeRecommendedWindow(
    double effectiveGapMilliseconds
    )
{
    const long double rawWindow =
        static_cast<long double>(
            effectiveGapMilliseconds
            )
        * ExpectedRecordsPerBurstWindow;

    if (rawWindow
        >= static_cast<long double>(
            std::numeric_limits<qint64>::max()
            )) {
        return std::numeric_limits<qint64>::max();
    }

    const qint64 rawMilliseconds =
        std::max<qint64>(
            1,
            static_cast<qint64>(
                std::ceil(rawWindow)
                )
            );

    return roundDurationUp(
        rawMilliseconds
        );
}
}

InvestigationCadence
InvestigationCadenceAnalyzer::analyze(
    const QVector<InvestigationRecord> &records
    ) const
{
    InvestigationCadence cadence;

    cadence.recommendedBurstWindowMilliseconds =
        FallbackBurstWindowMilliseconds;

    cadence.recommendedMergeGapMilliseconds =
        FallbackMergeGapMilliseconds;

    QVector<qint64> timestamps;

    timestamps.reserve(
        records.size()
        );

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        timestamps.append(
            record.timestamp
                ->toMSecsSinceEpoch()
            );
    }

    std::sort(
        timestamps.begin(),
        timestamps.end()
        );

    cadence.timestampCount =
        timestamps.size();

    if (timestamps.size() < 2) {
        return cadence;
    }

    QVector<qint64> positiveGaps;

    positiveGaps.reserve(
        timestamps.size() - 1
        );

    for (int index = 1;
         index < timestamps.size();
         ++index) {
        const qint64 gap =
            timestamps.at(index)
            - timestamps.at(index - 1);

        if (gap == 0) {
            ++cadence.zeroGapCount;
            continue;
        }

        /*
         * Timestamps were sorted first, so a
         * negative gap cannot occur here.
         */
        if (gap > 0) {
            positiveGaps.append(
                gap
                );
        }
    }

    cadence.positiveGapCount =
        positiveGaps.size();

    if (positiveGaps.isEmpty()) {
        return cadence;
    }

    /*
     * Gap order is already non-negative after
     * timestamp sorting, but gap magnitudes are
     * not necessarily sorted.
     */
    std::sort(
        positiveGaps.begin(),
        positiveGaps.end()
        );

    cadence.minimumPositiveGapMilliseconds =
        positiveGaps.first();

    cadence.maximumPositiveGapMilliseconds =
        positiveGaps.last();

    cadence.medianPositiveGapMilliseconds =
        medianOfSortedValues(
            positiveGaps
            );

    long double gapTotal = 0.0L;

    for (const qint64 gap
         : std::as_const(positiveGaps)) {
        gapTotal +=
            static_cast<long double>(
                gap
                );
    }

    cadence.meanPositiveGapMilliseconds =
        static_cast<double>(
            gapTotal
            / static_cast<long double>(
                positiveGaps.size()
                )
            );

    cadence.p90PositiveGapMilliseconds =
        percentile90OfSortedValues(
            positiveGaps
            );

    /*
     * Sparse investigations do not contain enough
     * timing information to justify an adaptive
     * recommendation. Keep the documented
     * fallback settings instead.
     */
    if (positiveGaps.size()
        < MinimumPositiveGapCount) {
        return cadence;
    }

    /*
     * Median establishes the normal cadence.
     * Mean can increase the representative gap
     * when variation is genuine, but is capped by
     * P90 so isolated idle periods cannot dominate
     * the recommendation.
     */
    const double effectiveGapMilliseconds =
        std::max(
            cadence.medianPositiveGapMilliseconds,
            std::min(
                cadence.meanPositiveGapMilliseconds,
                static_cast<double>(
                    cadence.p90PositiveGapMilliseconds
                    )
                )
            );

    cadence.recommendedBurstWindowMilliseconds =
        safeRecommendedWindow(
            effectiveGapMilliseconds
            );

    const qint64 rawMergeGapMilliseconds =
        std::max<qint64>(
            1,
            static_cast<qint64>(
                std::ceil(
                    static_cast<double>(
                        cadence
                            .recommendedBurstWindowMilliseconds
                        )
                    / 6.0
                    )
                )
            );

    cadence.recommendedMergeGapMilliseconds =
        roundDurationUp(
            rawMergeGapMilliseconds
            );

    cadence.usesFallbackRecommendation =
        false;

    return cadence;
}