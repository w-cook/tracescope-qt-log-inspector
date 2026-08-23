#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"

#include "BurstDetectionSettings.h"
#include "InvestigationBurst.h"

class InvestigationBurstAnalyzer
{
public:
    QVector<InvestigationBurst>
    detectBursts(
        const QVector<InvestigationRecord> &records,
        const BurstDetectionSettings &settings
        ) const;
};