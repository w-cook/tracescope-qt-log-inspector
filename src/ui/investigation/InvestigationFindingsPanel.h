#pragma once

#include <QWidget>

#include "../../workspace/InvestigationPresentationState.h"

class InvestigationSession;
class QLabel;
class QTableWidget;

class InvestigationFindingsPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationFindingsPanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    void refresh();

    void clear();

    InvestigationTablePresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationTablePresentationState &state
        );

signals:
    void findingActivated(
        const QString &recordId
        );

private:
    void activateRow(
        int row
        );

    InvestigationSession *m_session =
        nullptr;

    QLabel *m_summaryLabel =
        nullptr;

    QTableWidget *m_table =
        nullptr;
};