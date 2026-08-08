#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "../domain/InvestigationRecord.h"

class InvestigationCsvExporter
{
public:
    bool exportToFile(
        const QVector<InvestigationRecord> &records,
        const QString &filePath
        ) const;

private:
    QStringList customAttributeKeys(
        const QVector<InvestigationRecord> &records
        ) const;

    QString variantToString(
        const QVariant &value
        ) const;

    QString escapeCsvField(
        const QString &value
        ) const;
};