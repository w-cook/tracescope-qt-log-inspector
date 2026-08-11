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
        ILogImporter::UnlimitedRecordLimit
        ) const override;

    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;

private:
    ImportProfile profile;
};