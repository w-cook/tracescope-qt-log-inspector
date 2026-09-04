#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "InvestigationFindingExport.h"

class InvestigationFindingsCsvExporter
{
public:
    bool exportToFile(
        const QVector<InvestigationFindingExport> &findings,
        const QString &filePath
        ) const;

private:
    QStringList customAttributeKeys(
        const QVector<InvestigationFindingExport> &findings
        ) const;

    QString findingStatusToString(
        FindingStatus status
        ) const;

    QString variantToString(
        const QVariant &value
        ) const;

    QString escapeCsvField(
        const QString &value
        ) const;
};