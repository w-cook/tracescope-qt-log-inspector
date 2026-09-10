#pragma once

#include <QChar>
#include <QString>
#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"
#include "IncrementalImportInitializationResult.h"

enum class DelimitedTextHeaderStatus
{
    AwaitingHeader,
    Ready,
    Invalid
};

struct DelimitedTextImportState
{
    DelimitedTextHeaderStatus headerStatus =
        DelimitedTextHeaderStatus::AwaitingHeader;

    QStringList headers;

    void reset()
    {
        headerStatus =
            DelimitedTextHeaderStatus::AwaitingHeader;

        headers.clear();
    }
};

class DelimitedTextImporter final : public ILogImporter
{
public:
    DelimitedTextImporter(
        QString importerId,
        QString importerDisplayName,
        QChar delimiter,
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
        DelimitedTextImportState &state,
        const QString &sourcePath = {},
        qint64 firstPhysicalLineNumber = 1,
        quint64 sourceGeneration = 0
        ) const;

    IncrementalImportInitializationResult
    initializeIncrementalStateFromFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        DelimitedTextImportState &state
        ) const;

private:
    QString importerId;
    QString importerDisplayName;
    QChar delimiter;
    ImportProfile profile;
};