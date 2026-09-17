#pragma once

#include <QStringList>
#include <QtTypes>

#include "ILogImporter.h"
#include "ImportExecutionContext.h"
#include "ImportResult.h"

enum class SourceFamilyRecordLimitMode
{
    Sequential,
    DistributedAcrossSources
};

struct SourceFamilyImportOptions
{
    qint64 maxProcessedRecords =
        ILogImporter::UnlimitedRecordLimit;

    SourceFamilyRecordLimitMode recordLimitMode =
        SourceFamilyRecordLimitMode::Sequential;
};

class SourceFamilyImportService
{
public:
    ImportResult importFiles(
        const QStringList &orderedFilePaths,
        const ILogImporter &importer,
        const SourceFamilyImportOptions &options = {},
        const ImportExecutionContext &executionContext = {}
        ) const;
};