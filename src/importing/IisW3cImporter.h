#pragma once

#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"

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

private:
    ImportProfile profile;
};