#pragma once

#include <QByteArray>

#include "ILogImporter.h"
#include "ImportProfile.h"

class QIODevice;

class XmlImporter final
    : public ILogImporter
{
public:
    explicit XmlImporter(
        ImportProfile profile = {}
        );

    QString id() const override;
    QString displayName() const override;

    ImportResult importFile(
        const QString &filePath,
        qint64 maxProcessedRecords =
        UnlimitedRecordLimit,
        const ImportExecutionContext &executionContext = {}
        ) const override;

    ImportResult importContent(
        const QByteArray &xml,
        const QString &sourcePath =
        QString(),
        qint64 maxProcessedRecords =
        UnlimitedRecordLimit
        ) const;

private:
    ImportResult importDevice(
        QIODevice &device,
        const QString &sourcePath,
        qint64 maxProcessedRecords
        ) const;

    ImportProfile profile;
};