#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "InvestigationSessionComparison.h"

class InvestigationSessionComparisonAnalyzer
{
public:
    InvestigationSessionComparison compare(
        const QVector<InvestigationRecord> &baselineRecords,
        const QVector<InvestigationRecord> &comparisonRecords
        ) const;
};