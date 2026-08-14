#pragma once

#include <QDate>
#include <QStringList>

#include "ILogImporter.h"
#include "ImportProfile.h"

class SyslogImporter final : public ILogImporter
{
public:
    explicit SyslogImporter(
        ImportProfile profile = {},
        QDate legacyReferenceDate = {}
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
    QDate legacyReferenceDate;
};