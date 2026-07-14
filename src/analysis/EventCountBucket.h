#pragma once

#include <QString>

struct EventCountBucket
{
    QString label;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;

    int totalCount() const
    {
        return infoCount + warningCount + errorCount;
    }
};