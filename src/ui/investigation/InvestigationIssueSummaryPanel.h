#pragma once

#include <QGroupBox>
#include <QVector>

#include "../../analysis/TelemetryIssueAnalyzer.h"
#include "../../domain/InvestigationRecord.h"
#include "../../workspace/InvestigationPresentationState.h"

class QTableWidget;

enum class InvestigationIssueDrillDownType
{
    AllElevated,
    Warnings,
    Errors
};

class InvestigationIssueSummaryPanel
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit InvestigationIssueSummaryPanel(
        QWidget *parent = nullptr
        );

    void updateRecords(
        const QVector<InvestigationRecord> &records
        );

    void clear();

    int preferredCompactWidth() const;

    InvestigationTablePresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationTablePresentationState &state
        );

signals:
    void drillDownRequested(
        const QString &subsystem,
        InvestigationIssueDrillDownType type
        );

private:
    void handleCellDoubleClicked(
        int row,
        int column
        );

    void updateMinimumTableWidth();

    TelemetryIssueAnalyzer m_analyzer;

    QTableWidget *m_table =
        nullptr;
};