#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "TelemetryIssueGroup.h"

class TelemetryIssueAnalyzer
{
public:
    QVector<TelemetryIssueGroup> groupWarningsAndErrorsBySubsystem(
        const QVector<InvestigationRecord> &records
        ) const;
};