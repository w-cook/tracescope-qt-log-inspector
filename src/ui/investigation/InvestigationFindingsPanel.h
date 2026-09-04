#pragma once

#include <QWidget>

#include "../../exporting/InvestigationFindingExport.h"
#include "../../workspace/InvestigationPresentationState.h"

class InvestigationSession;
class QLabel;
class QTableWidget;
class QAction;
class QHBoxLayout;
class QMenu;
class QPushButton;

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

    void setExportCounts(
        const InvestigationFindingExportCounts &counts
        );

signals:
    void findingActivated(
        const QString &recordId
        );

    void exportRequested(
        InvestigationFindingExportScope scope
        );

private:
    void activateRow(
        int row
        );

    void updateMinimumUsableWidth();

    InvestigationSession *m_session =
        nullptr;

    QLabel *m_summaryLabel =
        nullptr;

    QHBoxLayout *m_headerLayout =
        nullptr;

    QPushButton *m_exportButton =
        nullptr;

    QMenu *m_exportMenu =
        nullptr;

    QAction *m_exportAllAction =
        nullptr;

    QAction *m_exportFilteredAction =
        nullptr;

    QAction *m_exportBookmarkedAction =
        nullptr;

    QTableWidget *m_table =
        nullptr;
};