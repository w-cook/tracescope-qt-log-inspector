#pragma once

#include <QByteArray>
#include <QString>

#include "ILogImporter.h"
#include "ImportProfile.h"
#include "StructuredJsonImportConfig.h"

class StructuredJsonImporter final : public ILogImporter
{
public:
    explicit StructuredJsonImporter(
        StructuredJsonImportConfig config = {},
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

    ImportResult importContent(
        const QByteArray &json,
        const QString &sourcePath = {},
        qint64 maxProcessedRecords =
        ILogImporter::UnlimitedRecordLimit
        ) const;

private:
    StructuredJsonImportConfig config;
    ImportProfile profile;
};