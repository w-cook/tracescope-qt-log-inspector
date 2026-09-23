#pragma once

#include <QDateTime>
#include <QList>

#include "WorkspaceDocument.h"

#include "../../workspace/InvestigationPresentationState.h"

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
class QResizeEvent;
class QTimer;
class QToolButton;
class QVBoxLayout;
class QWidget;

enum class InvestigationIssueDrillDownType;
enum class InvestigationReviewTab;
enum class InvestigationFindingExportScope;

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

    QString tabToolTip() const override;

    QColor tabTint() const override;

    InvestigationSessionSummaryPanel *
    summaryPanel() const;

    /*
     * Re-synchronize the complete document after
     * its existing InvestigationSession has been
     * reloaded in place.
     */
    void refreshSession();

    /*
     * Rebuild tab-level presentation derived from the
     * session backing without refreshing the complete
     * investigation UI.
     */
    void refreshTabPresentation();

    InvestigationSessionPresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationSessionPresentationState &state
        );

    void populateExportMenu(
        QMenu *menu
        ) override;

    QWidget *createTabAccessoryWidget(
        QWidget *parent
        ) override;

    void refreshInterfaceScale()
        override;

signals:
    void liveFollowStateChanged();

    /*
     * Emitted when substantive investigation state
     * captured by workspace persistence changes.
     *
     * Pure navigation/scrolling is intentionally not
     * treated as an unsaved-workspace edit.
     */
    void workspaceContentChanged();

protected:
    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    enum class InvestigationSection
    {
        Timeline,
        Events,
        LowerDetails
    };

    enum class InvestigationSectionCapacity
    {
        None = 0,
        One = 1,
        Two = 2,
        Three = 3
    };

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

    void copySelectedEventAsStructuredJson();

    void copySelectedEventAsFormattedText();

    void updateIssueSummary();

    void updateFindingsPanel();

    void updateFindingsExportState();

    void exportFindings(
        InvestigationFindingExportScope scope
        );

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

    void exportFilteredResults();

    void refreshDerivedViewsForCurrentFilter();

    void scheduleLiveRefresh();

    void refreshLiveSessionPresentation();

    void setTimelineCollapsed(
        bool collapsed
        );

    void setEventSectionCollapsed(
        bool collapsed
        );

    void setLowerRegionCollapsed(
        bool collapsed
        );

    void toggleSectionFromUser(
        InvestigationSection section
        );

    void setSectionPreferredCollapsed(
        InvestigationSection section,
        bool collapsed
        );

    bool isSectionPreferredCollapsed(
        InvestigationSection section
        ) const;

    bool isSectionCollapsed(
        InvestigationSection section
        ) const;

    void setSectionCollapsed(
        InvestigationSection section,
        bool collapsed
        );

    void updateEventSectionPresentation();

    void updateSectionCollapseControls();

    void updateSectionCollapseControlGeometry();

    void scheduleSectionCollapseControlGeometryUpdate();

    void allocateCollapsedSectionSpaceToEvents(
        int sectionIndex,
        const QList<int> &previousSizes
        );

    void allocateCollapsedEventSpace(
        const QList<int> &previousSizes
        );

    void allocateSingleOpenSection(
        InvestigationSection openSection
        );

    void restoreMainSplitterSectionHeight(
        int sectionIndex,
        int preferredHeight,
        const QList<int> &previousSizes
        );

    void restoreEventSectionUsingAuxiliaryPreferences(
        const QList<int> &previousSizes
        );

    void ensureSectionPreferredHeightsInitialized();

    void applyMainSplitterSizes(
        const QList<int> &sizes
        );

    InvestigationSectionCapacity
    sectionCapacityForAvailableHeight() const;

    int availableMainSplitterHeight() const;

    int sectionCompactHeight(
        InvestigationSection section
        ) const;

    int minimumUsefulExpandedHeight(
        InvestigationSection section
        ) const;

    int minimumCollapsedMainSplitterHeight() const;

    void updateMinimumConstrainedHeight();

    void updateTimelineMinimumHeight();

    void updateLowerRegionMinimumHeight();

    int constrainedPreferredHeight(
        InvestigationSection section
        ) const;

    int constrainedRecoveryMinimumHeight(
        InvestigationSection section
        ) const;

    int openSectionCount() const;

    void scheduleSectionCapacityUpdate();

    void applySectionCapacityPolicy();

    void captureConstrainedRestorePriority();

    void rebuildConstrainedRestorePriorityFromCurrentState();

    void updateConstrainedRecoveryCompletion();

    void updateConstrainedRecoveryStretchFactors();

    void resetConstrainedRestoreState();

    bool sectionHasRecoveredPreferredHeight(
        InvestigationSection section
        ) const;

    bool canRestoreConstrainedSection(
        InvestigationSection candidate
        ) const;

    void applyConstrainedRecoveryLayout();

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

    QWidget *m_eventSectionContainer =
        nullptr;

    QVBoxLayout *m_eventSectionLayout =
        nullptr;

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

    QWidget *m_lowerRegionContainer =
        nullptr;

    QSplitter *m_bottomSplitter =
        nullptr;

    QSplitter *m_mainSplitter =
        nullptr;

    QTimer *m_liveRefreshTimer =
        nullptr;

    bool m_followNewest =
        false;

    QToolButton *m_timelineCollapseButton =
        nullptr;

    QToolButton *m_eventCollapseButton =
        nullptr;

    QToolButton *m_lowerRegionCollapseButton =
        nullptr;

    bool m_timelineCollapsed =
        false;

    bool m_eventCollapsed =
        false;

    bool m_lowerRegionCollapsed =
        false;

    /*
     * User-preferred section state.
     *
     * These values describe what the user explicitly
     * requested with the section chevrons. Automatic
     * constrained-height behavior must never overwrite
     * them.
     */
    bool m_timelinePreferredCollapsed =
        false;

    bool m_eventPreferredCollapsed =
        false;

    bool m_lowerRegionPreferredCollapsed =
        false;

    int m_timelineExpandedHeight =
        0;

    int m_eventExpandedHeight =
        0;

    QList<int> m_eventPreCollapseSizes;

    int m_lowerRegionExpandedHeight =
        0;

    bool m_sectionPreferredHeightsInitialized =
        false;

    bool m_timelineManuallyResizedWhileEventCollapsed =
        false;

    bool m_lowerRegionManuallyResizedWhileEventCollapsed =
        false;

    InvestigationSectionCapacity m_sectionCapacity =
        InvestigationSectionCapacity::Three;

    bool m_sectionCapacityUpdatePending =
        false;

    bool m_applyingSectionCapacityPolicy =
        false;

    bool m_sectionCollapseGeometryUpdatePending =
        false;

    QList<InvestigationSection>
        m_constrainedRestorePriority;

    bool m_hasConstrainedRestoreSnapshot =
        false;

    bool m_constrainedRecoveryComplete =
        false;

    bool m_constrainedSectionCollapsedDuringCycle =
        false;

    int m_constrainedTimelinePreferredHeight =
        0;

    int m_constrainedEventPreferredHeight =
        0;

    int m_constrainedLowerPreferredHeight =
        0;

    bool m_constrainedLowerPreferredHeightRecovered =
        false;

    bool m_constrainedWindowResizeActive =
        false;

    QTimer *m_constrainedResizeSettleTimer =
        nullptr;

    int m_lastAvailableMainSplitterHeight =
        -1;
};