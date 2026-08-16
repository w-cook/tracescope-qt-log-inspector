#pragma once

#include <QtTypes>
#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "ImportDiagnostic.h"

struct ImportResult
{
    QVector<InvestigationRecord> records;
    QVector<ImportDiagnostic> diagnostics;

    qint64 processedRecordCount = 0;

    bool sourceTruncated = false;
    bool cancelled = false;

    qint64 importedRecordCount() const;
    qint64 skippedRecordCount() const;

    bool hasWarnings() const;
    bool hasErrors() const;
};