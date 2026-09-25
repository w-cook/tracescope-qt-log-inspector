#pragma once

#include <QDateTime>
#include <QGroupBox>
#include <QMetaObject>
#include <QSet>
#include <QString>

#include "../../workspace/InvestigationPresentationState.h"

class InvestigationRecord;
class InvestigationSession;
class QHBoxLayout;
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

    void setFollowNewestEnabled(
        bool enabled
        );

    void handleLiveSessionUpdated();

    InvestigationEventTablePresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationEventTablePresentationState &state
        );

    void refreshInterfaceScale();

    void setCollapsed(
        bool collapsed
        );

    bool isCollapsed() const;

    int collapsedHeight() const;

signals:
    void selectedRecordChanged();

    void customFieldFilterRequested(
        const QString &fieldName,
        const QString &value
        );

    void eventTableValueFilterToggleRequested(
        const QString &columnKey,
        const QString &value
        );

    void eventTableTimeBoundaryToggleRequested(
        const QDateTime &timestamp,
        bool startBoundary
        );

protected:
    bool eventFilter(
        QObject *watched,
        QEvent *event
        ) override;

private:
    void connectSelectionModel();

    void navigateAdjacentEvent(
        int direction
        );

    void navigateAdjacentIssue(
        int direction
        );

    void updateRowHeaderWidth();
    void refreshRowHeights();

    void restoreColumnWidths();

    void updateFollowNewestSelectionIntent();

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

    bool m_followNewest =
        false;

    bool m_followNewestSelectionIntent =
        false;

    bool m_suppressFollowNewestSelectionIntentUpdate =
        false;

    QHBoxLayout *m_navigationLayout =
        nullptr;

    qreal m_columnWidthScaleFactor =
        1.0;

    bool m_collapsed =
        false;

    bool m_headerMouseDown = false;
    QSet<int> m_manuallyResizedColumns;
};