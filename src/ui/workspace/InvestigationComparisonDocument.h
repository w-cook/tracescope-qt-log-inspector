#pragma once

#include "WorkspaceDocument.h"

#include "../../workspace/InvestigationComparisonSnapshot.h"
#include "../../workspace/InvestigationPresentationState.h"

class QScrollArea;

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

    InvestigationComparisonPresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationComparisonPresentationState
            &state
        );

private:
    InvestigationComparisonSnapshot m_snapshot;

    QScrollArea *m_scrollArea = nullptr;
};