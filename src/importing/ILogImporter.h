#pragma once

#include <QtTypes>
#include <QString>

#include "ImportExecutionContext.h"
#include "ImportResult.h"

class ILogImporter
{
public:
    inline static constexpr qint64
        UnlimitedRecordLimit = 0;

    virtual ~ILogImporter() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    virtual ImportResult importFile(
        const QString &filePath,
        qint64 maxProcessedRecords =
        UnlimitedRecordLimit,
        const ImportExecutionContext &executionContext =
        {}
        ) const = 0;
};