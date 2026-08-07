#pragma once

#include <QStringList>

#include "ILogImporter.h"
#include "JsonLinesImportConfig.h"

class JsonLinesImporter final : public ILogImporter
{
public:
    explicit JsonLinesImporter(
        JsonLinesImportConfig config = {}
        );

    QString id() const override;
    QString displayName() const override;

    ImportResult importFile(
        const QString &filePath
        ) const override;

    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;

private:
    JsonLinesImportConfig config;
};