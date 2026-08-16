#pragma once

#include <QtTypes>

#include <QString>

#include "ImportExecutionContext.h"
#include "ImportPreviewResult.h"
#include "ImportProfile.h"

class ImportPreviewService
{
public:
    inline static constexpr qint64
        DefaultMaxProcessedRecords = 50;

    ImportPreviewResult previewFile(
        const QString &filePath,
        const ImportProfile &profile,
        qint64 maxProcessedRecords =
        DefaultMaxProcessedRecords,
        const ImportExecutionContext &executionContext = {}
        ) const;
};