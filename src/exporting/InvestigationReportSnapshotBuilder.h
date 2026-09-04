#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

#include "InvestigationReportConfiguration.h"
#include "InvestigationReportSnapshot.h"

class InvestigationSession;
class InvestigationComparisonSnapshot;

struct InvestigationReportSessionInput
{
    const InvestigationSession *session = nullptr;
    QString documentTitle;
};

struct InvestigationReportComparisonInput
{
    const InvestigationComparisonSnapshot *snapshot = nullptr;
    QString documentTitle;
};

class InvestigationReportSnapshotBuilder
{
public:
    InvestigationReportSnapshot build(
        const InvestigationReportConfiguration &configuration,
        const QVector<InvestigationReportSessionInput>
            &availableSessions,
        const QVector<InvestigationReportComparisonInput>
            &availableComparisons,
        const QDateTime &generatedAtUtc
        ) const;
};