#pragma once

#include "WorkspaceDocument.h"

#include "../../workspace/InvestigationComparisonSnapshot.h"

class InvestigationComparisonDocument
    : public WorkspaceDocument
{
    Q_OBJECT

public:
    explicit InvestigationComparisonDocument(
        InvestigationComparisonSnapshot snapshot,
        QWidget *parent = nullptr
        );

    const InvestigationComparisonSnapshot &
    snapshot() const;

private:
    InvestigationComparisonSnapshot m_snapshot;
};