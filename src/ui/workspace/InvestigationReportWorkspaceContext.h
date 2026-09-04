#pragma once

#include <QString>
#include <QVector>

#include "../../exporting/InvestigationReportSelectionModel.h"
#include "../../exporting/InvestigationReportSnapshotBuilder.h"

class WorkspaceDocumentHost;

struct InvestigationReportWorkspaceContext
{
    QVector<InvestigationReportSessionSelection>
        sessionSelections;

    QVector<InvestigationReportComparisonSelection>
        comparisonSelections;

    QVector<InvestigationReportSessionInput>
        sessionInputs;

    QVector<InvestigationReportComparisonInput>
        comparisonInputs;

    InvestigationReportSelectionOrigin origin;

    QString suggestedTitle;

    bool hasReportDocuments() const
    {
        return !sessionSelections.isEmpty()
        || !comparisonSelections.isEmpty();
    }
};

class InvestigationReportWorkspaceContextBuilder
{
public:
    InvestigationReportWorkspaceContext build(
        const WorkspaceDocumentHost &host,
        const QString &originDocumentId
        ) const;
};