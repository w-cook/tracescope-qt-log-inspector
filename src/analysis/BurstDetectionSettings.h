#pragma once

#include <QtGlobal>

struct BurstDetectionSettings
{
    qint64 windowMilliseconds = 30 * 1000;

    int elevatedEventThreshold = 5;
    int errorCriticalThreshold = 3;

    qint64 mergeGapMilliseconds = 5 * 1000;

    bool isValid() const
    {
        return windowMilliseconds > 0
               && elevatedEventThreshold > 0
               && errorCriticalThreshold > 0
               && mergeGapMilliseconds >= 0;
    }
};