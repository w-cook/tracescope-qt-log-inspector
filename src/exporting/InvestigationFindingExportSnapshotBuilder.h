#pragma once

#include <QVector>

#include "InvestigationFindingExport.h"

class InvestigationSession;

class InvestigationFindingExportSnapshotBuilder
{
public:
    QVector<InvestigationFindingExport> build(
        const InvestigationSession &session
        ) const;
};