#pragma once

#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"
#include "IncrementalImportInitializationResult.h"

struct IisW3cImportState
{
    QStringList activeFields;

    void reset()
    {
        activeFields.clear();
    }
};

class IisW3cImporter final : public ILogImporter
{
public:
    explicit IisW3cImporter(
        ImportProfile profile = {}
        );

    QString id() const override;
    QString displayName() const override;

    ImportResult importFile(
        const QString &filePath,
        qint64 maxProcessedRecords =
        ILogImporter::UnlimitedRecordLimit,
        const ImportExecutionContext &executionContext = {}
        ) const override;

    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;

    ImportResult importIncrementalLines(
        const QStringList &lines,
        IisW3cImportState &state,
        const QString &sourcePath = {},
        qint64 firstPhysicalLineNumber = 1,
        quint64 sourceGeneration = 0
        ) const;

    IncrementalImportInitializationResult
    initializeIncrementalStateFromFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        IisW3cImportState &state
        ) const;

private:
    ImportProfile profile;
};