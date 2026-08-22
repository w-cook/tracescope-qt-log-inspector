#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "InvestigationValueFrequency.h"

class InvestigationAnalyticsAnalyzer
{
public:
    QVector<InvestigationValueFrequency>
    eventCodeFrequencies(
        const QVector<InvestigationRecord> &records
        ) const;

    QVector<InvestigationValueFrequency>
    entityFrequencies(
        const QVector<InvestigationRecord> &records
        ) const;
};