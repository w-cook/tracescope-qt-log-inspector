#pragma once

#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"

class RegexTextImporter final : public ILogImporter
{
public:
    explicit RegexTextImporter(
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
        const QString &sourcePath = {},
        qint64 firstPhysicalLineNumber = 1,
        quint64 sourceGeneration = 0
        ) const;

private:
    ImportProfile profile;
};