#pragma once

#include <QtTypes>

struct ImportProgress
{
    qint64 bytesProcessed = 0;
    qint64 totalBytes = 0;
    qint64 processedRecordCount = 0;
};