#pragma once

#include <QChar>
#include <QString>
#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"

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
        ILogImporter::UnlimitedRecordLimit
        ) const override;

    ImportResult importLines(
        const QStringList &lines,
        const QString &sourcePath = {}
        ) const;

private:
    QString importerId;
    QString importerDisplayName;
    QChar delimiter;
    ImportProfile profile;
};