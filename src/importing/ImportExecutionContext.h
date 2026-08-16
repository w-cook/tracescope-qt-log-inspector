#pragma once

#include <functional>

#include "ImportProgress.h"

struct ImportExecutionContext
{
    std::function<bool()> isCancellationRequested;
    std::function<void(const ImportProgress &)> reportProgress;

    bool cancellationRequested() const
    {
        return isCancellationRequested
               && isCancellationRequested();
    }

    void report(
        const ImportProgress &progress
        ) const
    {
        if (reportProgress) {
            reportProgress(progress);
        }
    }
};