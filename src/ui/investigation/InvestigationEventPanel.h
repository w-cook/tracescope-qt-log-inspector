#pragma once

#include <QGroupBox>
#include <QMetaObject>
#include <QString>

#include "../../workspace/InvestigationPresentationState.h"

class InvestigationRecord;
class InvestigationSession;
class QLabel;
class QPushButton;
class QTableView;

class InvestigationEventPanel
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit InvestigationEventPanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    const InvestigationRecord *
    selectedRecord() const;

    void clearSelection();

    void selectProxyRow(
        int proxyRow
        );

    void selectRecordId(
        const QString &recordId
        );

    void refreshNavigationState();

    void refreshPresentation();

    void focusTable();

    InvestigationEventTablePresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationEventTablePresentationState &state
        );

signals:
    void selectedRecordChanged();

    void customFieldFilterRequested(
        const QString &fieldName,
        const QString &value
        );

private:
    void connectSelectionModel();

    void navigateAdjacentEvent(
        int direction
        );

    void navigateAdjacentIssue(
        int direction
        );

    void updateRowHeaderWidth();

    void restoreColumnWidths();

    InvestigationSession *m_session =
        nullptr;

    QTableView *m_table =
        nullptr;

    QPushButton *m_previousEventButton =
        nullptr;

    QPushButton *m_nextEventButton =
        nullptr;

    QLabel *m_eventPositionLabel =
        nullptr;

    QPushButton *m_previousIssueButton =
        nullptr;

    QPushButton *m_nextIssueButton =
        nullptr;

    QMetaObject::Connection
        m_selectionConnection;
};