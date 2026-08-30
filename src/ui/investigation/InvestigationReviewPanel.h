#pragma once

#include <QWidget>

#include "../../workspace/InvestigationPresentationState.h"
#include "../../workspace/InvestigationSession.h"

class InvestigationAnalyticsPanel;
class InvestigationFindingsPanel;
class InvestigationIssueSummaryPanel;
class QTabWidget;

class InvestigationReviewPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationReviewPanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    InvestigationIssueSummaryPanel *
    issueSummaryPanel() const;

    InvestigationFindingsPanel *
    findingsPanel() const;

    InvestigationAnalyticsPanel *
    analyticsPanel() const;

    void setIssueSummaryAvailable(
        bool available
        );

    /*
     * Restore the active session's preferred
     * review tab after the caller has synchronized
     * source-dependent capability visibility.
     *
     * If the preferred tab is unavailable, fall
     * back deterministically to the first visible
     * review tab.
     */
    void restoreSelectedTab();

    InvestigationReviewTab currentTab() const;

    InvestigationReviewPresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationReviewPresentationState &state
        );

signals:
    /*
     * Emitted for both user-driven tab changes and
     * programmatic session-state restoration.
     *
     * The surrounding investigation layout can use
     * this signal for presentation concerns such as
     * splitter sizing without needing to know about
     * the internal QTabWidget.
     */
    void currentTabChanged(
        InvestigationReviewTab tab
        );

private:
    QWidget *pageForTab(
        InvestigationReviewTab tab
        ) const;

    int firstVisibleTabIndex() const;

    void handleCurrentChanged(
        int index
        );

    InvestigationSession *m_session =
        nullptr;

    QTabWidget *m_tabs =
        nullptr;

    InvestigationIssueSummaryPanel
        *m_issueSummaryPanel = nullptr;

    InvestigationFindingsPanel
        *m_findingsPanel = nullptr;

    InvestigationAnalyticsPanel
        *m_analyticsPanel = nullptr;
};