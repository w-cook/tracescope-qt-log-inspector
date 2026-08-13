#pragma once

#include <QString>

struct EventCountBucket
{
    QString label;

    int traceCount = 0;
    int debugCount = 0;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;
    int unspecifiedCount = 0;

    int totalCount() const
    {
        return traceCount
               + debugCount
               + infoCount
               + warningCount
               + errorCount
               + criticalCount
               + unspecifiedCount;
    }
};