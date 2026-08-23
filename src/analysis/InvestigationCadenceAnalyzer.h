#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "InvestigationCadence.h"

class InvestigationCadenceAnalyzer
{
public:
    InvestigationCadence analyze(
        const QVector<InvestigationRecord> &records
        ) const;
};