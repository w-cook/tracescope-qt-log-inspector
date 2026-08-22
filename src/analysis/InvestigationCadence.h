#pragma once

#include <QtGlobal>

struct InvestigationCadence
{
    int timestampCount = 0;
    int positiveGapCount = 0;
    int zeroGapCount = 0;

    qint64 minimumPositiveGapMilliseconds = 0;
    double medianPositiveGapMilliseconds = 0.0;
    double meanPositiveGapMilliseconds = 0.0;
    qint64 p90PositiveGapMilliseconds = 0;
    qint64 maximumPositiveGapMilliseconds = 0;

    bool usesFallbackRecommendation = true;

    qint64 recommendedBurstWindowMilliseconds =
        30 * 1000;

    qint64 recommendedMergeGapMilliseconds =
        5 * 1000;
};