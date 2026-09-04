#pragma once

#include <QString>

#include "InvestigationReportSnapshot.h"

class InvestigationSession;

class InvestigationReportSessionSnapshotBuilder
{
public:
    InvestigationReportSessionSnapshot build(
        const InvestigationSession &session,
        const QString &documentTitle,
        bool includeSupportingEvidence,
        bool includeTechnicalAppendix
        ) const;
};