#pragma once

#include <QString>

struct TelemetryIssueGroup
{
    QString subsystem;
    int warningCount = 0;
    int errorCount = 0;

    int totalCount() const
    {
        return warningCount + errorCount;
    }
};