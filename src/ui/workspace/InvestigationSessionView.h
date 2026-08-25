#pragma once

#include <QDateTime>

#include "WorkspaceDocument.h"

class FilterPresetStore;
class InvestigationAnalyticsPanel;
class InvestigationEventDetailPanel;
class InvestigationEventPanel;
class InvestigationFilterPanel;
class InvestigationFindingsPanel;
class InvestigationIssueSummaryPanel;
class InvestigationRecord;
class InvestigationReviewPanel;
class InvestigationSession;
class InvestigationSessionSummaryPanel;
class InvestigationTimelinePanel;
class QSplitter;

enum class InvestigationIssueDrillDownType;
enum class InvestigationReviewTab;

class InvestigationSessionView
    : public WorkspaceDocument
{
    Q_OBJECT

public:
    explicit InvestigationSessionView(
        InvestigationSession *session,
        FilterPresetStore *filterPresetStore,
        QWidget *parent = nullptr
        );

    InvestigationSession *session() const;

    InvestigationSessionSummaryPanel *
    summaryPanel() const;

    /*
     * Re-synchronize the complete document after
     * its existing InvestigationSession has been
     * reloaded in place.
     */
    void refreshSession();

private:
    void applyFilters();

    void updateEventDetailFromSelection();
    void clearEventDetail();

    const InvestigationRecord *
    selectedEventRecord() const;

    void updateInvestigationStateControls();

    void updateSelectedEventFindingStatus();

    void editSelectedEventNote();

    void syncInvestigationStatePresentation();

    void toggleSelectedEventBookmark();

    void updateIssueSummary();

    void updateFindingsPanel();

    void drillDownIssueSummary(
        const QString &subsystem,
        InvestigationIssueDrillDownType type
        );

    void applyTimelineDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
        const QString &severity,
        const QString &subsystem
        );

    void navigateToFinding(
        const QString &recordId
        );

    void revealFindingRecord(
        const InvestigationRecord &record
        );

    void drillDownBurst(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        );

    void updateReviewSplitter(
        InvestigationReviewTab tab
        );

    InvestigationSession *m_session =
        nullptr;

    FilterPresetStore *m_filterPresetStore =
        nullptr;

    InvestigationSessionSummaryPanel
        *m_summaryPanel = nullptr;

    InvestigationFilterPanel
        *m_filterPanel = nullptr;

    InvestigationTimelinePanel
        *m_timelinePanel = nullptr;

    InvestigationEventPanel
        *m_eventPanel = nullptr;

    InvestigationReviewPanel
        *m_reviewPanel = nullptr;

    InvestigationIssueSummaryPanel
        *m_issueSummaryPanel = nullptr;

    InvestigationFindingsPanel
        *m_findingsPanel = nullptr;

    InvestigationAnalyticsPanel
        *m_analyticsPanel = nullptr;

    InvestigationEventDetailPanel
        *m_eventDetailPanel = nullptr;

    QSplitter *m_bottomSplitter =
        nullptr;

    QSplitter *m_mainSplitter =
        nullptr;
};