#pragma once

#include <QStringList>

#include "ILogImporter.h"

class JsonLinesImporter final : public ILogImporter
{
public:
    QString id() const override;
    QString displayName() const override;

    ImportResult importFile(
        const QString &filePath
        ) const override;

    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;
};