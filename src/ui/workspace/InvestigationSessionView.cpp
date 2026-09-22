#include "InvestigationSessionView.h"

#include <algorithm>
#include <utility>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringList>
#include <QTextOption>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "../InterfaceScale.h"
#include "../investigation/InvestigationAnalyticsPanel.h"
#include "../investigation/InvestigationEventDetailPanel.h"
#include "../investigation/InvestigationEventPanel.h"
#include "../investigation/InvestigationFilterPanel.h"
#include "../investigation/InvestigationFindingsPanel.h"
#include "../investigation/InvestigationIssueSummaryPanel.h"
#include "../investigation/InvestigationReviewPanel.h"
#include "../investigation/InvestigationSessionSummaryPanel.h"
#include "../investigation/InvestigationTimelinePanel.h"

#include "../../domain/InvestigationRecord.h"
#include "../../exporting/InvestigationCsvExporter.h"
#include "../../exporting/InvestigationFindingExportSnapshotBuilder.h"
#include "../../exporting/InvestigationFindingsCsvExporter.h"
#include "../../exporting/InvestigationRecordExportFormatter.h"
#include "../../models/InvestigationFilterProxyModel.h"
#include "../../preferences/FilterPresetStore.h"
#include "../../workspace/InvestigationSession.h"
#include "../../workspace/InvestigationStateStore.h"

#include "LiveFollowTabControl.h"

namespace
{
constexpr int
    CollapsedSectionFallbackLogicalHeight =
    28;

constexpr int
    TimelineMinimumUsefulLogicalHeight =
    150;

constexpr int
    EventMinimumUsefulLogicalHeight =
    180;

constexpr int
    LowerDetailsMinimumUsefulLogicalHeight =
    170;

constexpr int
    SectionCapacityHysteresisLogicalHeight =
    12;

QString documentIdFor(
    const InvestigationSession *session
    )
{
    return session != nullptr
               ? session->id()
               : QString();
}

void restoreSplitterSizes(
    QSplitter *splitter,
    const QVector<int> &sizes
    )
{
    if (splitter == nullptr
        || sizes.size() != splitter->count()) {
        return;
    }

    bool hasPositiveSize =
        false;

    for (const int size : sizes) {
        if (size < 0) {
            return;
        }

        if (size > 0) {
            hasPositiveSize =
                true;
        }
    }

    if (!hasPositiveSize) {
        return;
    }

    splitter->setSizes(
        sizes
        );
}

QString documentTitleFor(
    const InvestigationSession *session
    )
{
    if (session == nullptr) {
        return QString();
    }

    return session
        ->sourceMetadata()
        .sourceName;
}

QString findingsExportTitle(
    InvestigationFindingExportScope scope
    )
{
    switch (scope) {
    case InvestigationFindingExportScope::All:
        return QObject::tr(
            "Export All Findings"
            );

    case InvestigationFindingExportScope::Filtered:
        return QObject::tr(
            "Export Filtered Findings"
            );

    case InvestigationFindingExportScope::Bookmarked:
        return QObject::tr(
            "Export Bookmarked Findings"
            );
    }

    return QObject::tr(
        "Export Findings"
        );
}

QString findingsExportFileName(
    const InvestigationSession *session,
    InvestigationFindingExportScope scope
    )
{
    QString baseName;

    if (session != nullptr) {
        baseName =
            QFileInfo(
                session
                    ->sourceMetadata()
                    .sourceName
                )
                .completeBaseName();
    }

    if (baseName.trimmed().isEmpty()) {
        baseName =
            QStringLiteral("investigation");
    }

    QString suffix;

    switch (scope) {
    case InvestigationFindingExportScope::All:
        suffix =
            QStringLiteral("-findings.csv");
        break;

    case InvestigationFindingExportScope::Filtered:
        suffix =
            QStringLiteral(
                "-filtered-findings.csv"
                );
        break;

    case InvestigationFindingExportScope::Bookmarked:
        suffix =
            QStringLiteral(
                "-bookmarked-findings.csv"
                );
        break;
    }

    return baseName + suffix;
}

QString filteredResultsExportFileName(
    const InvestigationSession *session
    )
{
    QString baseName;

    if (session != nullptr) {
        baseName =
            QFileInfo(
                session
                    ->sourceMetadata()
                    .sourceName
                )
                .completeBaseName();
    }

    if (baseName.trimmed().isEmpty()) {
        baseName =
            QStringLiteral("investigation");
    }

    return baseName
           + QStringLiteral(
               "-filtered-records.csv"
               );
}
}

InvestigationSessionView::
    InvestigationSessionView(
        InvestigationSession *session,
        FilterPresetStore *filterPresetStore,
        QWidget *parent
        )
    : WorkspaceDocument(
          documentIdFor(session),
          documentTitleFor(session),
          parent
          ),
    m_session(session),
    m_filterPresetStore(filterPresetStore),
    m_summaryPanel(
        new InvestigationSessionSummaryPanel(
            session,
            this
            )
        ),
    m_filterPanel(
        new InvestigationFilterPanel(
            filterPresetStore,
            this
            )
        ),
    m_timelinePanel(
        new InvestigationTimelinePanel(
            this
            )
        ),
    m_eventPanel(
        new InvestigationEventPanel(
            this
            )
        ),
    m_reviewPanel(
        new InvestigationReviewPanel(
            this
            )
        ),
    m_eventDetailPanel(
        new InvestigationEventDetailPanel(
            this
            )
        ),
    m_bottomSplitter(
        new QSplitter(
            Qt::Horizontal,
            this
            )
        ),
    m_mainSplitter(
        new QSplitter(
            Qt::Vertical,
            this
            )
        ),
    m_liveRefreshTimer(
        new QTimer(this)
        )
{
    m_issueSummaryPanel =
        m_reviewPanel
            ->issueSummaryPanel();

    m_findingsPanel =
        m_reviewPanel
            ->findingsPanel();

    m_analyticsPanel =
        m_reviewPanel
            ->analyticsPanel();

    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        InterfaceScale::margins(
            6,
            4,
            6,
            6,
            this
            )
        );

    layout->setSpacing(
        InterfaceScale::pixels(
            4,
            this
            )
        );

    layout->addWidget(
        m_summaryPanel
        );

    layout->addWidget(
        m_filterPanel
        );

    /*
     * ---------------------------------------------------------
     * Review/detail splitter
     * ---------------------------------------------------------
     */
    m_bottomSplitter->addWidget(
        m_reviewPanel
        );

    m_bottomSplitter->addWidget(
        m_eventDetailPanel
        );

    m_bottomSplitter->setStretchFactor(
        0,
        0
        );

    m_bottomSplitter->setStretchFactor(
        1,
        1
        );

    m_bottomSplitter->setCollapsible(
        0,
        false
        );

    m_bottomSplitter->setCollapsible(
        1,
        false
        );

    m_bottomSplitter->setSizes({
        m_issueSummaryPanel
            ->preferredCompactWidth(),
        1000
    });

    /*
     * The lower investigation region has its own
     * responsive minimum-useful-height policy.
     *
     * Do not let content-derived minimumSizeHint()
     * values from the review/detail surfaces create a
     * second, larger hard vertical floor underneath it.
     *
     * Horizontal policies remain unchanged because the
     * nested horizontal splitter still owns width
     * allocation and responsive control wrapping.
     */
    QSizePolicy reviewSizePolicy =
        m_reviewPanel->sizePolicy();

    reviewSizePolicy.setVerticalPolicy(
        QSizePolicy::Ignored
        );

    m_reviewPanel->setSizePolicy(
        reviewSizePolicy
        );

    QSizePolicy detailSizePolicy =
        m_eventDetailPanel->sizePolicy();

    detailSizePolicy.setVerticalPolicy(
        QSizePolicy::Ignored
        );

    m_eventDetailPanel->setSizePolicy(
        detailSizePolicy
        );

    QSizePolicy bottomSplitterSizePolicy =
        m_bottomSplitter->sizePolicy();

    bottomSplitterSizePolicy.setVerticalPolicy(
        QSizePolicy::Ignored
        );

    m_bottomSplitter->setSizePolicy(
        bottomSplitterSizePolicy
        );

    /*
     * ---------------------------------------------------------
     * Event-section container
     * ---------------------------------------------------------
     *
     * Telemetry Events normally fills this container.
     *
     * The container exists separately from the Event panel
     * so a collapsed Event title strip can later be centered
     * in spare vertical space when all three investigation
     * sections are voluntarily collapsed.
     */
    m_eventSectionContainer =
        new QWidget(this);

    m_eventSectionLayout =
        new QVBoxLayout(
            m_eventSectionContainer
            );

    m_eventSectionLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    m_eventSectionLayout->setSpacing(
        0
        );

    m_eventPanel->setParent(
        m_eventSectionContainer
        );

    m_eventSectionLayout->addWidget(
        m_eventPanel
        );

    /*
     * ---------------------------------------------------------
     * Primary document splitter
     * ---------------------------------------------------------
     */
    m_mainSplitter->addWidget(
        m_timelinePanel
        );

    m_mainSplitter->addWidget(
        m_eventSectionContainer
        );

    m_mainSplitter->addWidget(
        m_bottomSplitter
        );

    m_mainSplitter->setStretchFactor(
        0,
        2
        );

    m_mainSplitter->setStretchFactor(
        1,
        5
        );

    m_mainSplitter->setStretchFactor(
        2,
        2
        );

    m_mainSplitter->setCollapsible(
        0,
        false
        );

    m_mainSplitter->setCollapsible(
        1,
        false
        );

    m_mainSplitter->setCollapsible(
        2,
        false
        );

    m_mainSplitter->setSizes(
        QList<int>{
            220,
            350,
            250
        }
        );

    QTimer::singleShot(
        0,
        this,
        [this]() {
            ensureSectionPreferredHeightsInitialized();

            updateMinimumConstrainedHeight();

            scheduleSectionCapacityUpdate();
        }
        );

    /*
     * The investigation splitter must never impose its
     * transient child-derived height requirements on the
     * outer workspace window.
     *
     * Section collapse/expansion changes child size hints
     * during an active native window resize. If those
     * hints are allowed to propagate upward, Windows can
     * temporarily treat the new layout requirement as the
     * resize boundary until the user releases the mouse.
     *
     * The surrounding document layout owns the available
     * vertical slot; the section-capacity coordinator
     * decides what can fit inside it.
     */
    m_mainSplitter->setMaximumHeight(
        QWIDGETSIZE_MAX
        );

    m_mainSplitter->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Ignored
        );

    updateLowerRegionMinimumHeight();

    layout->addWidget(
        m_mainSplitter,
        1
        );

    m_timelineCollapseButton =
        new QToolButton(
            m_timelinePanel
            );

    m_eventCollapseButton =
        new QToolButton(
            m_eventPanel
            );

    m_lowerRegionCollapseButton =
        new QToolButton(
            m_eventDetailPanel
            );

    for (
        QToolButton *button
        : {
            m_timelineCollapseButton,
            m_eventCollapseButton,
            m_lowerRegionCollapseButton
        }
        ) {
        button->setAutoRaise(
            true
            );

        button->setToolButtonStyle(
            Qt::ToolButtonIconOnly
            );

        button->setCursor(
            Qt::PointingHandCursor
            );

        button->raise();
    }

    QPalette sectionControlPalette =
        palette();

    sectionControlPalette.setColor(
        QPalette::ButtonText,
        QColor(128, 128, 128)
        );

    sectionControlPalette.setColor(
        QPalette::WindowText,
        QColor(128, 128, 128)
        );

    for (
        QToolButton *button
        : {
            m_timelineCollapseButton,
            m_eventCollapseButton,
            m_lowerRegionCollapseButton
        }
        ) {
        button->setPalette(
            sectionControlPalette
            );
    }

    connect(
        m_timelineCollapseButton,
        &QToolButton::clicked,
        this,
        [this]() {
            toggleSectionFromUser(
                InvestigationSection::Timeline
                );
        }
        );

    connect(
        m_eventCollapseButton,
        &QToolButton::clicked,
        this,
        [this]() {
            toggleSectionFromUser(
                InvestigationSection::Events
                );
        }
        );

    connect(
        m_lowerRegionCollapseButton,
        &QToolButton::clicked,
        this,
        [this]() {
            toggleSectionFromUser(
                InvestigationSection::LowerDetails
                );
        }
        );

    connect(
        m_mainSplitter,
        &QSplitter::splitterMoved,
        this,
        [this](
            int,
            int handleIndex
            ) {
            updateSectionCollapseControlGeometry();

            if (m_mainSplitter == nullptr) {
                return;
            }

            ensureSectionPreferredHeightsInitialized();

            const QList<int> sizes =
                m_mainSplitter->sizes();

            if (sizes.size() != 3) {
                return;
            }

            /*
             * -----------------------------------------------------
             * Special case: Events is collapsed
             * -----------------------------------------------------
             *
             * The middle section is fixed at its compact title
             * strip. Dragging either surrounding handle changes
             * one outer section intentionally and makes the other
             * outer section compensate mechanically.
             *
             * Only the intentionally resized outer section gets
             * a new preference.
             */
            if (m_eventCollapsed) {
                if (
                    handleIndex == 1
                    && !m_timelineCollapsed
                    ) {
                    m_timelineExpandedHeight =
                        sizes.at(0);

                    m_timelineManuallyResizedWhileEventCollapsed =
                        true;

                    m_eventPreCollapseSizes.clear();
                } else if (
                    handleIndex == 2
                    && !m_lowerRegionCollapsed
                    ) {
                    m_lowerRegionExpandedHeight =
                        sizes.at(2);

                    m_lowerRegionManuallyResizedWhileEventCollapsed =
                        true;

                    m_eventPreCollapseSizes.clear();
                }

                return;
            }

            /*
             * -----------------------------------------------------
             * Normal case: Events is open
             * -----------------------------------------------------
             *
             * A QSplitter handle directly resizes the two
             * neighboring sections:
             *
             *   handle 1:
             *       Timeline <-> Events
             *
             *   handle 2:
             *       Events <-> Lower Details
             *
             * Both visible neighboring sections represent direct
             * user intent and therefore both receive new preferred
             * heights.
             *
             * A collapsed neighboring section does not receive a
             * new preference.
             */
            if (handleIndex == 1) {
                if (!m_timelineCollapsed) {
                    m_timelineExpandedHeight =
                        sizes.at(0);
                }

                if (!m_eventCollapsed) {
                    m_eventExpandedHeight =
                        sizes.at(1);
                }
            } else if (handleIndex == 2) {
                if (!m_eventCollapsed) {
                    m_eventExpandedHeight =
                        sizes.at(1);
                }

                if (!m_lowerRegionCollapsed) {
                    m_lowerRegionExpandedHeight =
                        sizes.at(2);
                }
            }
        }
        );

    /*
     * ---------------------------------------------------------
     * Filter surface
     * ---------------------------------------------------------
     */
    connect(
        m_filterPanel,
        &InvestigationFilterPanel::
        filterChangeRequested,
        this,
        &InvestigationSessionView::
        applyFilters
        );

    /*
     * ---------------------------------------------------------
     * Event list
     * ---------------------------------------------------------
     */
    connect(
        m_eventPanel,
        &InvestigationEventPanel::
        selectedRecordChanged,
        this,
        &InvestigationSessionView::
        updateEventDetailFromSelection
        );

    connect(
        m_eventPanel,
        &InvestigationEventPanel::
        customFieldFilterRequested,
        this,
        [this](
            const QString &fieldName,
            const QString &value
            ) {
            if (
                m_filterPanel
                    ->addCustomFieldFilter(
                        fieldName,
                        value
                        )
                ) {
                applyFilters();
            }
        }
        );

    /*
     * ---------------------------------------------------------
     * Issue summary
     * ---------------------------------------------------------
     */
    connect(
        m_issueSummaryPanel,
        &InvestigationIssueSummaryPanel::
        drillDownRequested,
        this,
        &InvestigationSessionView::
        drillDownIssueSummary
        );

    /*
     * ---------------------------------------------------------
     * Findings
     * ---------------------------------------------------------
     */
    connect(
        m_findingsPanel,
        &InvestigationFindingsPanel::
        findingActivated,
        this,
        &InvestigationSessionView::
        navigateToFinding
        );

    connect(
        m_findingsPanel,
        &InvestigationFindingsPanel::
        exportRequested,
        this,
        &InvestigationSessionView::
        exportFindings
        );

    /*
     * ---------------------------------------------------------
     * Analytics
     * ---------------------------------------------------------
     */
    connect(
        m_analyticsPanel,
        &InvestigationAnalyticsPanel::
        burstDrillDownRequested,
        this,
        &InvestigationSessionView::
        drillDownBurst
        );

    connect(
        m_analyticsPanel,
        &InvestigationAnalyticsPanel::
        burstConfigurationChanged,
        this,
        &InvestigationSessionView::
        workspaceContentChanged
        );

    /*
     * ---------------------------------------------------------
     * Timeline
     * ---------------------------------------------------------
     */
    connect(
        m_timelinePanel,
        &InvestigationTimelinePanel::
        bucketDrillDownRequested,
        this,
        &InvestigationSessionView::
        applyTimelineDrillDown
        );

    /*
     * ---------------------------------------------------------
     * Event detail
     * ---------------------------------------------------------
     */
    connect(
        m_eventDetailPanel,
        &InvestigationEventDetailPanel::
        findingStatusChangeRequested,
        this,
        &InvestigationSessionView::
        updateSelectedEventFindingStatus
        );

    connect(
        m_eventDetailPanel,
        &InvestigationEventDetailPanel::
        noteEditRequested,
        this,
        &InvestigationSessionView::
        editSelectedEventNote
        );

    connect(
        m_eventDetailPanel,
        &InvestigationEventDetailPanel::
        bookmarkToggleRequested,
        this,
        &InvestigationSessionView::
        toggleSelectedEventBookmark
        );

    connect(
        m_eventDetailPanel,
        &InvestigationEventDetailPanel::
        copyStructuredJsonRequested,
        this,
        &InvestigationSessionView::
        copySelectedEventAsStructuredJson
        );

    connect(
        m_eventDetailPanel,
        &InvestigationEventDetailPanel::
        copyFormattedTextRequested,
        this,
        &InvestigationSessionView::
        copySelectedEventAsFormattedText
        );

    /*
     * The review container owns tab state.
     * This document owns only the surrounding
     * splitter proportions.
     */
    connect(
        m_reviewPanel,
        &InvestigationReviewPanel::
        currentTabChanged,
        this,
        &InvestigationSessionView::
        updateReviewSplitter
        );

    if (m_session != nullptr) {
        setToolTip(
            m_session
                ->sourceMetadata()
                .sourcePath
            );
    }

    m_liveRefreshTimer->setSingleShot(
        true
        );

    m_liveRefreshTimer->setInterval(
        750
        );

    connect(
        m_liveRefreshTimer,
        &QTimer::timeout,
        this,
        &InvestigationSessionView::
        refreshLiveSessionPresentation
        );

    updateSectionCollapseControls();

    refreshSession();
}

InvestigationSession *
InvestigationSessionView::session() const
{
    return m_session;
}

QString InvestigationSessionView::
    tabToolTip() const
{
    if (m_session == nullptr) {
        return {};
    }

    QStringList lines;

    const InvestigationSessionBacking
        &backing =
        m_session->backing();

    switch (backing.mode()) {
    case InvestigationSessionBackingMode::
        SourceBacked: {
        lines.append(
            tr("Backing: SourceBacked")
            );

        lines.append(
            tr(
                "The external source is authoritative."
                )
            );

        const InvestigationExternalSourceBinding
            *source =
            backing.externalSource();

        if (source != nullptr
            && !source
                    ->sourcePath
                    .trimmed()
                    .isEmpty()) {
            const QFileInfo sourceInfo(
                source->sourcePath
                );

            lines.append(QString());

            lines.append(
                tr("Source:")
                );

            lines.append(
                source->sourcePath
                );

            lines.append(
                tr("Status: %1")
                    .arg(
                        sourceInfo.exists()
                                && sourceInfo.isFile()
                            ? tr("Available")
                            : tr("Unavailable")
                        )
                );
        }

        break;
    }

    case InvestigationSessionBackingMode::
        SnapshotBacked: {
        lines.append(
            tr("Backing: SnapshotBacked")
            );

        lines.append(
            tr(
                "Saved TraceScope evidence is "
                "authoritative."
                )
            );

        const InvestigationSnapshotBinding
            *snapshot =
            backing.snapshot();

        if (snapshot != nullptr
            && !snapshot
                    ->snapshotPath
                    .trimmed()
                    .isEmpty()) {
            lines.append(QString());

            lines.append(
                tr("Snapshot:")
                );

            lines.append(
                snapshot->snapshotPath
                );
        }

        const QString originalSourcePath =
            m_session
                ->sourceMetadata()
                .sourcePath;

        if (!originalSourcePath
                 .trimmed()
                 .isEmpty()) {
            lines.append(QString());

            lines.append(
                tr("Original source:")
                );

            lines.append(
                originalSourcePath
                );
        }

        break;
    }

    case InvestigationSessionBackingMode::
        Hybrid: {
        lines.append(
            tr("Backing: Hybrid")
            );

        lines.append(
            tr(
                "Saved evidence is preserved while "
                "the external source can contribute "
                "continuation records."
                )
            );

        const InvestigationSnapshotBinding
            *snapshot =
            backing.snapshot();

        if (snapshot != nullptr
            && !snapshot
                    ->snapshotPath
                    .trimmed()
                    .isEmpty()) {
            lines.append(QString());

            lines.append(
                tr("Snapshot:")
                );

            lines.append(
                snapshot->snapshotPath
                );
        }

        const InvestigationExternalSourceBinding
            *source =
            backing.externalSource();

        if (source != nullptr
            && !source
                    ->sourcePath
                    .trimmed()
                    .isEmpty()) {
            const QFileInfo sourceInfo(
                source->sourcePath
                );

            lines.append(QString());

            lines.append(
                tr("External source:")
                );

            lines.append(
                source->sourcePath
                );

            lines.append(
                tr("Status: %1")
                    .arg(
                        sourceInfo.exists()
                                && sourceInfo.isFile()
                            ? tr("Available")
                            : tr("Unavailable")
                        )
                );
        }

        break;
    }
    }

    return lines.join(
        QLatin1Char('\n')
        );
}

QColor InvestigationSessionView::
    tabTint() const
{
    if (m_session == nullptr) {
        return {};
    }

    switch (
        m_session->backing().mode()
        ) {
    case InvestigationSessionBackingMode::
        SourceBacked:
        /*
         * Blue.
         */
        return QColor(
            47,
            128,
            237
            );

    case InvestigationSessionBackingMode::
        SnapshotBacked:
        /*
         * Berry.
         */
        return QColor(
            214,
            70,
            160
            );

    case InvestigationSessionBackingMode::
        Hybrid:
        /*
         * Green.
         */
        return QColor(
            45,
            200,
            25
            );
    }

    return {};
}

InvestigationSessionSummaryPanel *
InvestigationSessionView::summaryPanel() const
{
    return m_summaryPanel;
}

void InvestigationSessionView::
    refreshSession()
{
    if (m_session == nullptr) {
        return;
    }

    if (m_liveRefreshTimer != nullptr) {
        m_liveRefreshTimer->stop();
    }

    m_filterPanel->setSession(
        m_session
        );

    m_eventPanel->setSession(
        m_session
        );

    m_timelinePanel->setSession(
        m_session
        );

    m_reviewPanel->setSession(
        m_session
        );

    syncInvestigationStatePresentation();

    m_reviewPanel
        ->setIssueSummaryAvailable(
            m_session->hasSeverityData()
            && m_session
                   ->hasSubsystemData()
            );

    m_reviewPanel->setVisible(
        true
        );

    m_reviewPanel
        ->restoreSelectedTab();

    applyFilters();

    m_eventPanel
        ->refreshNavigationState();

    emit documentTabPresentationChanged();
}

InvestigationSessionPresentationState
    InvestigationSessionView::
    capturePresentationState() const
{
    InvestigationSessionPresentationState
        state;

    if (m_eventPanel != nullptr) {
        state.eventTable =
            m_eventPanel
                ->capturePresentationState();
    }

    if (m_eventDetailPanel != nullptr) {
        state.eventDetailScroll =
            m_eventDetailPanel
                ->capturePresentationState();
    }

    if (m_timelinePanel != nullptr) {
        state.timeline =
            m_timelinePanel
                ->capturePresentationState();
    }

    if (m_reviewPanel != nullptr) {
        state.review =
            m_reviewPanel
                ->capturePresentationState();
    }

    if (m_mainSplitter != nullptr) {
        state.mainSplitterSizes =
            m_mainSplitter->sizes();
    }

    if (m_bottomSplitter != nullptr) {
        state.bottomSplitterSizes =
            m_bottomSplitter->sizes();
    }

    if (m_session != nullptr) {
        state.burstTimingMode =
            m_session->burstTimingMode();

        state.burstDetectionSettings =
            m_session->burstDetectionSettings();
    }

    return state;
}

void InvestigationSessionView::
    restorePresentationState(
        const InvestigationSessionPresentationState
            &state
        )
{
    if (m_session == nullptr) {
        return;
    }

    /*
     * Burst configuration affects the actual Analytics
     * data model, so restore it before attempting to
     * restore Analytics selection/presentation state.
     */
    m_session->setBurstTimingMode(
        state.burstTimingMode
        );

    m_session->setBurstDetectionSettings(
        state.burstDetectionSettings
        );

    if (m_analyticsPanel != nullptr) {
        m_analyticsPanel->updateRecords(
            m_session
                ->investigationController()
                ->recordsForAnalysis()
            );
    }

    /*
     * Event-table restoration first establishes the
     * saved sort and selected record.
     */
    if (m_eventPanel != nullptr) {
        m_eventPanel
            ->restorePresentationState(
                state.eventTable
                );
    }

    /*
     * Ensure Selected Event Details represents the
     * final restored selection before applying its
     * saved text viewport.
     */
    updateEventDetailFromSelection();

    if (m_eventDetailPanel != nullptr) {
        m_eventDetailPanel
            ->restorePresentationState(
                state.eventDetailScroll
                );
    }

    if (m_timelinePanel != nullptr) {
        m_timelinePanel
            ->restorePresentationState(
                state.timeline
                );
    }

    if (m_reviewPanel != nullptr) {
        m_reviewPanel
            ->restorePresentationState(
                state.review
                );
    }

    /*
     * Review-tab restoration deliberately updates the
     * adaptive Review/Detail layout. Apply the exact
     * saved splitter presentation only after that
     * capability-aware/tab-aware synchronization has
     * finished.
     */
    restoreSplitterSizes(
        m_bottomSplitter,
        state.bottomSplitterSizes
        );

    restoreSplitterSizes(
        m_mainSplitter,
        state.mainSplitterSizes
        );
}

void InvestigationSessionView::
    populateExportMenu(
        QMenu *menu
        )
{
    if (menu == nullptr
        || m_session == nullptr) {
        return;
    }

    WorkspaceDocument::populateExportMenu(
        menu
        );

    menu->addSeparator();

    menu->setToolTipsVisible(
        true
        );

    InvestigationController *controller =
        m_session
            ->investigationController();

    QAction *filteredResultsAction =
        menu->addAction(
            tr(
                "Export Filtered Results..."
                )
            );

    filteredResultsAction->setToolTip(
        tr(
            "Export the records matching the "
            "current investigation filters"
            )
        );

    filteredResultsAction->setEnabled(
        controller != nullptr
        && controller
                   ->proxyModel()
                   ->rowCount()
               > 0
        );

    connect(
        filteredResultsAction,
        &QAction::triggered,
        this,
        &InvestigationSessionView::
        exportFilteredResults
        );

    menu->addSeparator();

    /*
     * Findings remain available here as a second
     * document-level access path in addition to the
     * contextual Export control on the Findings tab.
     */
    QMenu *findingsMenu =
        menu->addMenu(
            tr("Findings")
            );

    findingsMenu->setToolTipsVisible(
        true
        );

    const InvestigationFindingExportSnapshotBuilder
        builder;

    const InvestigationFindingExportCounts counts =
        builder.counts(
            *m_session
            );

    QAction *allFindingsAction =
        findingsMenu->addAction(
            tr(
                "Export All Findings (%1)"
                )
                .arg(counts.all)
            );

    QAction *filteredFindingsAction =
        findingsMenu->addAction(
            tr(
                "Export Filtered Findings (%1)"
                )
                .arg(counts.filtered)
            );

    QAction *bookmarkedFindingsAction =
        findingsMenu->addAction(
            tr(
                "Export Bookmarked Findings (%1)"
                )
                .arg(counts.bookmarked)
            );

    allFindingsAction->setEnabled(
        counts.all > 0
        );

    filteredFindingsAction->setEnabled(
        counts.filtered > 0
        );

    bookmarkedFindingsAction->setEnabled(
        counts.bookmarked > 0
        );

    allFindingsAction->setToolTip(
        tr(
            "Export every explicitly classified "
            "finding in this investigation"
            )
        );

    filteredFindingsAction->setToolTip(
        tr(
            "Export findings whose source records "
            "match the current investigation filters"
            )
        );

    bookmarkedFindingsAction->setToolTip(
        tr(
            "Export bookmarked findings regardless "
            "of the current investigation filters"
            )
        );

    connect(
        allFindingsAction,
        &QAction::triggered,
        this,
        [this]() {
            exportFindings(
                InvestigationFindingExportScope::All
                );
        }
        );

    connect(
        filteredFindingsAction,
        &QAction::triggered,
        this,
        [this]() {
            exportFindings(
                InvestigationFindingExportScope::
                Filtered
                );
        }
        );

    connect(
        bookmarkedFindingsAction,
        &QAction::triggered,
        this,
        [this]() {
            exportFindings(
                InvestigationFindingExportScope::
                Bookmarked
                );
        }
        );
}

QWidget *InvestigationSessionView::
    createTabAccessoryWidget(
        QWidget *parent
        )
{
    if (m_session == nullptr
        || !m_session
                ->supportsLiveFollowing()) {
        return nullptr;
    }

    auto *control =
        new LiveFollowTabControl(
            m_session,
            parent
            );

    /*
     * Restore the document-owned Follow Newest state
     * whenever the tab accessory is created again.
     */
    control->setFollowNewestEnabled(
        m_followNewest
        );

    if (m_eventPanel != nullptr) {
        m_eventPanel
            ->setFollowNewestEnabled(
                m_followNewest
                );
    }

    connect(
        control,
        &LiveFollowTabControl::
        followNewestChanged,
        this,
        [this](
            bool enabled
            ) {
            m_followNewest =
                enabled;

            if (m_eventPanel != nullptr) {
                m_eventPanel
                    ->setFollowNewestEnabled(
                        enabled
                        );
            }
        }
        );

    connect(
        control,
        &LiveFollowTabControl::
        liveFollowStateChanged,
        this,
        &InvestigationSessionView::
        liveFollowStateChanged
        );

    connect(
        control,
        &LiveFollowTabControl::
        liveSessionUpdated,
        this,
        [this]() {
            /*
             * Newly admitted live evidence changes the
             * investigation snapshot that a workspace save
             * would persist.
             */
            emit workspaceContentChanged();

            scheduleLiveRefresh();
        }
        );

    return control;
}

void InvestigationSessionView::
    refreshInterfaceScale()
{
    if (layout() != nullptr) {
        layout()->setContentsMargins(
            InterfaceScale::margins(
                6,
                4,
                6,
                6,
                this
                )
            );

        layout()->setSpacing(
            InterfaceScale::pixels(
                4,
                this
                )
            );

        layout()->invalidate();
    }

    if (m_filterPanel != nullptr) {
        m_filterPanel
            ->refreshInterfaceScale();
    }

    if (m_timelinePanel != nullptr) {
        m_timelinePanel
            ->refreshInterfaceScale();
    }

    if (m_eventPanel != nullptr) {
        m_eventPanel
            ->refreshInterfaceScale();

        updateEventSectionPresentation();
    }

    if (m_reviewPanel != nullptr) {
        m_reviewPanel
            ->refreshInterfaceScale();
    }

    if (m_eventDetailPanel != nullptr) {
        m_eventDetailPanel
            ->refreshInterfaceScale();
    }

    /*
     * The remaining persistent panels will be added
     * here as their live-scale refresh methods are
     * implemented.
     */

    WorkspaceDocument::
        refreshInterfaceScale();

    /*
     * Interface scale changes can move the responsive
     * review/detail breakpoints even though the outer
     * window rectangle is intentionally preserved.
     *
     * Wait until Qt has processed the new font/style
     * and layout geometry before recalculating.
     */
    QTimer::singleShot(
        0,
        this,
        [this]() {
            if (
                m_reviewPanel == nullptr
                || m_bottomSplitter == nullptr
                ) {
                return;
            }

            updateReviewSplitter(
                m_reviewPanel
                    ->currentTab()
                );
        }
        );

    const int sectionButtonSize =
        InterfaceScale::pixels(
            18,
            this
            );

    for (
        QToolButton *button
        : {
            m_timelineCollapseButton,
            m_eventCollapseButton,
            m_lowerRegionCollapseButton
        }
        ) {
        if (button != nullptr) {
            button->setFixedSize(
                sectionButtonSize,
                sectionButtonSize
                );
        }
    }

    updateSectionCollapseControls();

    QTimer::singleShot(
        0,
        this,
        [this]() {
            updateSectionCollapseControlGeometry();
        }
        );

    QTimer::singleShot(
        0,
        this,
        [this]() {
            updateLowerRegionMinimumHeight();
            updateMinimumConstrainedHeight();
            scheduleSectionCapacityUpdate();
        }
        );
}

void InvestigationSessionView::
    applyFilters()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const QString selectedRecordId =
        m_session
            ->selectedRecordId();

    /*
     * This is the user-driven path. Only here do we
     * push the filter control state back into the
     * proxy model.
     */
    m_filterPanel->applyToSession();

    refreshDerivedViewsForCurrentFilter();

    const int selectedProxyRow =
        !selectedRecordId.isEmpty()
            ? controller
                  ->proxyRowForRecordId(
                      selectedRecordId
                      )
            : -1;

    if (selectedProxyRow >= 0) {
        m_eventPanel->selectProxyRow(
            selectedProxyRow
            );
    } else {
        m_eventPanel->clearSelection();

        clearEventDetail();
    }

    m_eventPanel
        ->refreshNavigationState();

    emit workspaceContentChanged();
}

void InvestigationSessionView::
    updateEventDetailFromSelection()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (record == nullptr) {
        clearEventDetail();

        return;
    }

    m_eventDetailPanel
        ->displayRecord(
            *record
            );

    updateInvestigationStateControls();
}

void InvestigationSessionView::
    clearEventDetail()
{
    m_eventDetailPanel
        ->clearRecord();

    updateInvestigationStateControls();
}

const InvestigationRecord *
    InvestigationSessionView::
    selectedEventRecord() const
{
    if (m_eventPanel == nullptr) {
        return nullptr;
    }

    return m_eventPanel
        ->selectedRecord();
}

void InvestigationSessionView::
    updateInvestigationStateControls()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (
        m_session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()
        ) {
        m_eventDetailPanel
            ->clearInvestigationState();

        return;
    }

    const InvestigationRecordState state =
        m_session
            ->investigationStateStore()
            ->stateForRecord(
                record->recordId
                );

    m_eventDetailPanel
        ->setInvestigationState(
            state
            );
}

void InvestigationSessionView::
    updateSelectedEventFindingStatus()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (
        m_session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()
        ) {
        return;
    }

    const FindingStatus status =
        m_eventDetailPanel
            ->selectedFindingStatus();

    InvestigationStateStore *stateStore =
        m_session
            ->investigationStateStore();

    const FindingStatus previousStatus =
        stateStore
            ->stateForRecord(
                record->recordId
                )
            .findingStatus;

    if (previousStatus != status) {
        stateStore->setFindingStatus(
            record->recordId,
            status
            );

        emit workspaceContentChanged();
    }

    syncInvestigationStatePresentation();

    updateFindingsPanel();

    if (
        m_filterPanel
            ->hasFindingStatusFilter()
        ) {
        applyFilters();
    } else {
        updateInvestigationStateControls();
    }
}

void InvestigationSessionView::
    toggleSelectedEventBookmark()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (
        m_session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()
        ) {
        return;
    }

    InvestigationStateStore *stateStore =
        m_session
            ->investigationStateStore();

    const bool currentlyBookmarked =
        stateStore
            ->stateForRecord(
                record->recordId
                )
            .bookmarked;

    stateStore->setBookmarked(
        record->recordId,
        !currentlyBookmarked
        );

    emit workspaceContentChanged();

    syncInvestigationStatePresentation();

    if (m_filterPanel->bookmarksOnly()) {
        applyFilters();
    } else {
        updateInvestigationStateControls();
    }
}

void InvestigationSessionView::
    copySelectedEventAsStructuredJson()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (record == nullptr) {
        return;
    }

    QClipboard *clipboard =
        QApplication::clipboard();

    if (clipboard == nullptr) {
        return;
    }

    const InvestigationRecordExportFormatter
        formatter;

    clipboard->setText(
        formatter.toStructuredJson(
            *record
            )
        );
}

void InvestigationSessionView::
    copySelectedEventAsFormattedText()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (record == nullptr) {
        return;
    }

    QClipboard *clipboard =
        QApplication::clipboard();

    if (clipboard == nullptr) {
        return;
    }

    const InvestigationRecordExportFormatter
        formatter;

    clipboard->setText(
        formatter.toFormattedText(
            *record
            )
        );
}

void InvestigationSessionView::
    syncInvestigationStatePresentation()
{
    if (m_session == nullptr) {
        return;
    }

    updateFindingsPanel();

    InvestigationStateStore *stateStore =
        m_session
            ->investigationStateStore();

    m_session
        ->investigationController()
        ->proxyModel()
        ->setInvestigationStateIndicators(
            stateStore
                ->bookmarkedRecordIds(),
            stateStore
                ->notedRecordIds(),
            stateStore
                ->findingStatuses()
            );

    updateFindingsExportState();
}

void InvestigationSessionView::
    updateFindingsPanel()
{
    m_findingsPanel->refresh();
}

void InvestigationSessionView::
    updateFindingsExportState()
{
    if (m_findingsPanel == nullptr) {
        return;
    }

    if (m_session == nullptr) {
        m_findingsPanel->setExportCounts(
            InvestigationFindingExportCounts{}
            );

        return;
    }

    const InvestigationFindingExportSnapshotBuilder
        builder;

    m_findingsPanel->setExportCounts(
        builder.counts(
            *m_session
            )
        );
}

void InvestigationSessionView::
    exportFindings(
        InvestigationFindingExportScope scope
        )
{
    if (m_session == nullptr) {
        return;
    }

    /*
     * Capture the immutable export state first.
     * The subsequent save dialog and filesystem
     * operation work only from this frozen snapshot.
     */
    const InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        findings =
        builder.build(
            *m_session,
            scope
            );

    if (findings.isEmpty()) {
        return;
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            findingsExportTitle(
                scope
                ),
            findingsExportFileName(
                m_session,
                scope
                ),
            tr(
                "CSV Files (*.csv);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    const InvestigationFindingsCsvExporter
        exporter;

    if (!exporter.exportToFile(
            findings,
            filePath
            )) {
        QMessageBox::warning(
            this,
            tr("Findings Export Failed"),
            tr(
                "TraceScope could not write "
                "the findings export to:\n\n%1"
                )
                .arg(filePath)
            );

        return;
    }

    QMessageBox::information(
        this,
        tr("Findings Exported"),
        tr(
            "Exported %1 findings to:\n\n%2"
            )
            .arg(findings.size())
            .arg(filePath)
        );
}

void InvestigationSessionView::
    editSelectedEventNote()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (
        record == nullptr
        || m_session == nullptr
        ) {
        return;
    }

    const QString recordId =
        record->recordId;

    const InvestigationRecordState state =
        m_session
            ->investigationStateStore()
            ->stateForRecord(
                recordId
                );

    auto *dialog =
        new QDialog(this);

    dialog->setAttribute(
        Qt::WA_DeleteOnClose
        );

    dialog->setWindowModality(
        Qt::NonModal
        );

    dialog->setModal(
        false
        );

    dialog->setWindowTitle(
        tr("Analyst Note")
        );

    dialog->resize(
        InterfaceScale::size(
            520,
            300,
            dialog
            )
        );

    auto *layout =
        new QVBoxLayout(
            dialog
            );

    QString recordDescription =
        tr("Source record #%1")
            .arg(
                record
                    ->source
                    .recordNumber
                );

    if (record->eventCode.has_value()) {
        recordDescription +=
            tr(" — %1")
                .arg(
                    record
                        ->eventCode
                        .value()
                    );
    }

    auto *recordLabel =
        new QLabel(
            recordDescription,
            dialog
            );

    layout->addWidget(
        recordLabel
        );

    auto *noteEdit =
        new QPlainTextEdit(
            dialog
            );

    noteEdit->setPlainText(
        state.note
        );

    noteEdit->setLineWrapMode(
        QPlainTextEdit::WidgetWidth
        );

    noteEdit->setWordWrapMode(
        QTextOption::
        WrapAtWordBoundaryOrAnywhere
        );

    noteEdit
        ->setHorizontalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff
            );

    layout->addWidget(
        noteEdit,
        1
        );

    auto *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Save
                | QDialogButtonBox::Cancel,
            dialog
            );

    layout->addWidget(
        buttonBox
        );

    connect(
        buttonBox,
        &QDialogButtonBox::rejected,
        dialog,
        &QDialog::close
        );

    connect(
        buttonBox,
        &QDialogButtonBox::accepted,
        this,
        [
            this,
            dialog,
            noteEdit,
            recordId
        ]() {
            if (m_session == nullptr) {
                dialog->close();

                return;
            }

            const QString note =
                noteEdit
                    ->toPlainText();

            InvestigationStateStore *stateStore =
                m_session
                    ->investigationStateStore();

            const QString normalizedNote =
                note.trimmed().isEmpty()
                    ? QString()
                    : note;

            const QString previousNote =
                stateStore
                    ->stateForRecord(
                        recordId
                        )
                    .note;

            if (previousNote != normalizedNote) {
                stateStore->setNote(
                    recordId,
                    normalizedNote
                    );

                emit workspaceContentChanged();
            }

            updateInvestigationStateControls();
            updateFindingsPanel();

            dialog->close();
        }
        );

    dialog->show();

    noteEdit->setFocus();
}

void InvestigationSessionView::
    drillDownIssueSummary(
        const QString &subsystem,
        InvestigationIssueDrillDownType type
        )
{
    if (
        m_session == nullptr
        || subsystem.isEmpty()
        ) {
        return;
    }

    QStringList targetSeverities;

    switch (type) {
    case InvestigationIssueDrillDownType::
        Warnings:
        targetSeverities = {
            QStringLiteral("WARN")
        };
        break;

    case InvestigationIssueDrillDownType::
        Errors:
        targetSeverities = {
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };
        break;

    case InvestigationIssueDrillDownType::
        AllElevated:
        targetSeverities = {
            QStringLiteral("WARN"),
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };
        break;
    }

    if (
        m_filterPanel
            ->configureIssueDrillDown(
            subsystem,
            targetSeverities
            )
        ) {
        applyFilters();
    }
}

void InvestigationSessionView::
    applyTimelineDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
        const QString &severity,
        const QString &subsystem
        )
{
    if (
        m_filterPanel
            ->configureTimelineDrillDown(
                startTimestamp,
                endTimestamp,
                severity,
                subsystem
                )
        ) {
        applyFilters();
    }
}

void InvestigationSessionView::
    drillDownBurst(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        )
{
    if (
        !m_filterPanel
             ->configureBurstDrillDown(
                 startTimestamp,
                 endTimestamp
                 )
        ) {
        return;
    }

    applyFilters();

    m_eventPanel->focusTable();
}

void InvestigationSessionView::
    navigateToFinding(
        const QString &recordId
        )
{
    if (
        m_session == nullptr
        || recordId.isEmpty()
        ) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const QVector<InvestigationRecord>
        &records =
        controller->allRecords();

    const InvestigationRecord *targetRecord =
        nullptr;

    for (
        const InvestigationRecord &record
        : records
        ) {
        if (record.recordId
            == recordId) {
            targetRecord =
                &record;

            break;
        }
    }

    if (targetRecord == nullptr) {
        return;
    }

    int proxyRow =
        controller
            ->proxyRowForRecordId(
                recordId
                );

    if (proxyRow >= 0) {
        m_eventPanel->selectProxyRow(
            proxyRow
            );

        m_eventPanel->focusTable();

        return;
    }

    revealFindingRecord(
        *targetRecord
        );

    proxyRow =
        controller
            ->proxyRowForRecordId(
                recordId
                );

    if (proxyRow < 0) {
        return;
    }

    m_eventPanel->selectProxyRow(
        proxyRow
        );

    m_eventPanel->focusTable();
}

void InvestigationSessionView::
    revealFindingRecord(
        const InvestigationRecord &record
        )
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    InvestigationFilterProxyModel *proxyModel =
        controller->proxyModel();

    const InvestigationFilterMatch match =
        proxyModel
            ->filterMatchForRecord(
                record
                );

    if (match.allMatch()) {
        return;
    }

    const InvestigationRecordState state =
        m_session
            ->investigationStateStore()
            ->stateForRecord(
                record.recordId
                );

    if (
        m_filterPanel->revealRecord(
            record,
            match,
            state
            )
        ) {
        applyFilters();
    }
}

void InvestigationSessionView::
    updateReviewSplitter(
        InvestigationReviewTab tab
        )
{
    if (
        m_bottomSplitter == nullptr
        || m_eventDetailPanel == nullptr
        ) {
        return;
    }

    /*
     * QSplitter::sizes() describes the widget areas
     * without the splitter handle itself, so base the
     * adaptive sizing on the same usable space.
     *
     * During initial session construction the review
     * tab can be restored before the splitter has been
     * assigned real geometry. Do not manufacture a
     * one-pixel layout in that transient state.
     */
    const int availableWidth =
        m_bottomSplitter->width()
        - m_bottomSplitter->handleWidth();

    if (availableWidth <= 0) {
        return;
    }

    /*
     * Selected Event Details has a content-derived
     * minimum width based on its actual controls.
     * Respect that constraint explicitly when
     * calculating adaptive splitter proportions.
     */
    const int detailMinimumWidth =
        m_eventDetailPanel->minimumWidth();

    if (availableWidth
        <= detailMinimumWidth) {
        return;
    }

    const int maximumReviewWidth =
        availableWidth
        - detailMinimumWidth;

    const bool wideReviewSelected =
        tab
            == InvestigationReviewTab::Findings
        || tab
               == InvestigationReviewTab::Analytics;

    const int narrowReviewBreakpoint =
        InterfaceScale::pixels(
            750,
            this
            );

    const int mediumReviewBreakpoint =
        InterfaceScale::pixels(
            1100,
            this
            );

    if (wideReviewSelected) {
        /*
         * Findings and Analytics carry more
         * investigation-oriented information than
         * Selected Event Details and need additional
         * room in narrow detached workspaces.
         */
        double reviewFraction =
            0.60;

        if (
            availableWidth
            < narrowReviewBreakpoint
            ) {
            reviewFraction =
                0.72;
        } else if (
            availableWidth
            < mediumReviewBreakpoint
            ) {
            reviewFraction =
                0.68;
        }

        const int desiredReviewWidth =
            static_cast<int>(
                availableWidth
                * reviewFraction
                );

        const int reviewWidth =
            std::clamp(
                desiredReviewWidth,
                1,
                maximumReviewWidth
                );

        m_bottomSplitter->setSizes({
            reviewWidth,
            availableWidth - reviewWidth
        });

        return;
    }

    /*
     * Issue Summary remains deliberately compact
     * because its table supports horizontal
     * scrolling when necessary.
     */
    const int desiredIssueWidth =
        std::max(
            m_issueSummaryPanel
                ->preferredCompactWidth(),
            static_cast<int>(
                availableWidth
                * 0.35
                )
            );

    const int issueWidth =
        std::clamp(
            desiredIssueWidth,
            1,
            maximumReviewWidth
            );

    m_bottomSplitter->setSizes({
        issueWidth,
        availableWidth - issueWidth
    });
}

void InvestigationSessionView::
    resizeEvent(
        QResizeEvent *event
        )
{
    QWidget::resizeEvent(
        event
        );

    /*
     * Force the outer layout to consume the new native
     * window geometry before calculating constrained
     * splitter sizes.
     *
     * Without this synchronous pass, QSplitter first
     * distributes the new height according to its
     * ordinary stretch factors and our queued recovery
     * pass corrects it one event-loop turn later. That
     * produces the visible grow/shrink oscillation.
     */
    if (layout() != nullptr) {
        layout()->activate();
    }

    const int currentAvailableHeight =
        availableMainSplitterHeight();

    const bool growing =
        m_lastAvailableMainSplitterHeight >= 0
        && currentAvailableHeight
               > m_lastAvailableMainSplitterHeight;

    /*
     * Synchronous recovery is only needed after capacity
     * has actually forced a section closed.
     *
     * That path prevents Qt from painting an intermediate
     * splitter distribution while a section is reopened.
     *
     * During an ordinary partial shrink/reverse, however,
     * no section is changing visibility. Let QSplitter
     * perform its normal gradual resize first; the queued
     * capacity-policy pass will only clamp auxiliary
     * sections when they reach their frozen preferences.
     */
    if (
        growing
        && m_hasConstrainedRestoreSnapshot
        && m_constrainedSectionCollapsedDuringCycle
        ) {
        applyConstrainedRecoveryLayout();
    }

    scheduleSectionCapacityUpdate();

    if (m_reviewPanel == nullptr) {
        return;
    }

    /*
     * QWidget receives its resize before the child
     * layout necessarily finishes assigning geometry
     * to the nested splitter.
     *
     * Defer the adaptive split until the current
     * event-loop turn has completed so calculations
     * use the splitter's settled width.
     */
    QTimer::singleShot(
        0,
        this,
        [this]() {
            if (
                m_reviewPanel != nullptr
                && m_bottomSplitter != nullptr
                && !m_lowerRegionCollapsed
                ) {
                updateReviewSplitter(
                    m_reviewPanel
                        ->currentTab()
                    );
            }

            updateSectionCollapseControlGeometry();
        }
        );
}

void InvestigationSessionView::
    exportFilteredResults()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    /*
     * Capture the current filtered record collection
     * before opening the save dialog. The export then
     * has the same immutable point-in-time semantics
     * as the Phase 14 findings exports.
     */
    const QVector<InvestigationRecord> records =
        controller->visibleRecords();

    if (records.isEmpty()) {
        QMessageBox::information(
            this,
            tr("No Records to Export"),
            tr(
                "There are no currently visible "
                "records to export."
                )
            );

        return;
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            tr("Export Filtered Records"),
            filteredResultsExportFileName(
                m_session
                ),
            tr(
                "CSV Files (*.csv);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    const InvestigationCsvExporter exporter;

    if (!exporter.exportToFile(
            records,
            filePath
            )) {
        QMessageBox::warning(
            this,
            tr("Export Failed"),
            tr(
                "TraceScope could not export "
                "the filtered records."
                )
            );

        return;
    }

    QMessageBox::information(
        this,
        tr("Export Complete"),
        tr(
            "Exported %1 records."
            )
            .arg(records.size())
        );
}

void InvestigationSessionView::
    refreshDerivedViewsForCurrentFilter()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const QVector<InvestigationRecord>
        visibleRecords =
        controller
            ->recordsForAnalysis();

    m_summaryPanel->refresh(
        visibleRecords
        );

    if (
        m_session->hasSeverityData()
        && m_session
               ->hasSubsystemData()
        ) {
        m_issueSummaryPanel
            ->updateRecords(
                visibleRecords
                );
    } else {
        m_issueSummaryPanel->clear();
    }

    m_analyticsPanel->updateRecords(
        visibleRecords
        );

    m_timelinePanel->updateRecords(
        visibleRecords
        );

    updateFindingsExportState();
}

void InvestigationSessionView::
    scheduleLiveRefresh()
{
    if (m_liveRefreshTimer == nullptr
        || m_liveRefreshTimer
               ->isActive()) {
        return;
    }

    /*
     * Throttle rather than debounce. Continuous live
     * input must still refresh the derived surfaces
     * periodically.
     */
    m_liveRefreshTimer->start();
}

void InvestigationSessionView::
    refreshLiveSessionPresentation()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    /*
     * The proxy model already handles newly inserted
     * source rows dynamically. Do not reapply the
     * user's filters here.
     */
    m_filterPanel
        ->refreshAvailableOptions();

    const bool issueSummaryAvailable =
        m_session->hasSeverityData()
        && m_session
               ->hasSubsystemData();

    m_reviewPanel
        ->setIssueSummaryAvailable(
            issueSummaryAvailable
            );

    refreshDerivedViewsForCurrentFilter();

    /*
     * Ordinary inserts preserve the current selection.
     * A rare dynamic-column schema expansion can reset
     * the table model, however, so restore the persisted
     * record only when the view actually lost it.
     *
     * Do not repeatedly reselect an intact row: that
     * would continually scroll the user back to it.
     */
    const QString selectedRecordId =
        m_session
            ->selectedRecordId();

    if (
        !selectedRecordId.isEmpty()
        && m_eventPanel
                   ->selectedRecord()
               == nullptr
        ) {
        const int selectedProxyRow =
            controller
                ->proxyRowForRecordId(
                    selectedRecordId
                    );

        if (selectedProxyRow >= 0) {
            m_eventPanel->selectProxyRow(
                selectedProxyRow
                );
        }
    }

    m_eventPanel
        ->refreshPresentation();
}

void InvestigationSessionView::
    updateSectionCollapseControls()
{
    const bool sectionExpansionAvailable =
        m_sectionCapacity
        != InvestigationSectionCapacity::None;

    if (m_timelineCollapseButton != nullptr) {
        m_timelineCollapseButton
            ->setArrowType(
                m_timelineCollapsed
                    ? Qt::LeftArrow
                    : Qt::DownArrow
                );

        m_timelineCollapseButton
            ->setToolTip(
                m_timelineCollapsed
                    ? tr("Expand timeline")
                    : tr("Collapse timeline")
                );

        m_timelineCollapseButton
            ->setAccessibleName(
                m_timelineCollapsed
                    ? tr("Expand timeline")
                    : tr("Collapse timeline")
                );

        m_timelineCollapseButton->setEnabled(
            sectionExpansionAvailable
            );
    }

    if (m_eventCollapseButton != nullptr) {
        m_eventCollapseButton
            ->setArrowType(
                m_eventCollapsed
                    ? Qt::LeftArrow
                    : Qt::DownArrow
                );

        m_eventCollapseButton
            ->setToolTip(
                m_eventCollapsed
                    ? tr("Expand telemetry events")
                    : tr("Collapse telemetry events")
                );

        m_eventCollapseButton
            ->setAccessibleName(
                m_eventCollapsed
                    ? tr("Expand telemetry events")
                    : tr("Collapse telemetry events")
                );

        m_eventCollapseButton->setEnabled(
            sectionExpansionAvailable
            );
    }

    if (m_lowerRegionCollapseButton != nullptr) {
        m_lowerRegionCollapseButton
            ->setArrowType(
                m_lowerRegionCollapsed
                    ? Qt::LeftArrow
                    : Qt::DownArrow
                );

        m_lowerRegionCollapseButton
            ->setToolTip(
                m_lowerRegionCollapsed
                    ? tr(
                          "Expand lower investigation panels"
                          )
                    : tr(
                          "Collapse lower investigation panels"
                          )
                );

        m_lowerRegionCollapseButton
            ->setAccessibleName(
                m_lowerRegionCollapsed
                    ? tr(
                          "Expand lower investigation panels"
                          )
                    : tr(
                          "Collapse lower investigation panels"
                          )
                );

        m_lowerRegionCollapseButton->setEnabled(
            sectionExpansionAvailable
            );
    }
}

void InvestigationSessionView::
    updateSectionCollapseControlGeometry()
{
    if (
        m_timelineCollapseButton == nullptr
        || m_eventCollapseButton == nullptr
        || m_lowerRegionCollapseButton == nullptr
        || m_timelinePanel == nullptr
        || m_eventPanel == nullptr
        || m_bottomSplitter == nullptr
        || m_reviewPanel == nullptr
        ) {
        return;
    }

    const int buttonSize =
        InterfaceScale::pixels(
            18,
            this
            );

    const int rightInset =
        InterfaceScale::pixels(
            8,
            this
            );

    /*
     * ---------------------------------------------------------
     * Timeline
     * ---------------------------------------------------------
     *
     * The button is a child of the Timeline panel, so
     * its coordinates are completely local. Moving the
     * splitter section automatically moves the button.
     */
    const int timelineTitleBandHeight =
        std::max(
            buttonSize,
            m_timelinePanel
                    ->fontMetrics()
                    .height()
                + InterfaceScale::pixels(
                    4,
                    m_timelinePanel
                    )
            );

    const int timelineButtonX =
        std::max(
            0,
            m_timelinePanel->width()
                - rightInset
                - buttonSize
            );

    const int timelineButtonY =
        std::max(
            0,
            (
                timelineTitleBandHeight
                - buttonSize
                )
                / 2
            )
        + (
            m_timelineCollapsed
                ? -InterfaceScale::pixels(
                      1,
                      this
                      )
                : InterfaceScale::pixels(
                      2,
                      this
                      )
            );

    /*
     * ---------------------------------------------------------
     * Telemetry Events
     * ---------------------------------------------------------
     *
     * Parenting directly to m_eventPanel also handles
     * the special all-collapsed centered presentation:
     * when the panel moves inside its flexible
     * container, its chevron moves with it.
     */
    const int eventTitleBandHeight =
        std::max(
            buttonSize,
            m_eventPanel
                    ->fontMetrics()
                    .height()
                + InterfaceScale::pixels(
                    4,
                    m_eventPanel
                    )
            );

    const int eventButtonX =
        std::max(
            0,
            m_eventPanel->width()
                - rightInset
                - buttonSize
            );

    const int eventButtonY =
        std::max(
            0,
            (
                eventTitleBandHeight
                - buttonSize
                )
                / 2
            )
        + (
            m_eventCollapsed
                ? -InterfaceScale::pixels(
                      1,
                      this
                      )
                : InterfaceScale::pixels(
                      2,
                      this
                      )
            );

    /*
     * ---------------------------------------------------------
     * Lower Details
     * ---------------------------------------------------------
     *
     * The lower control represents the complete
     * horizontal Review / Selected Event Details
     * region, so parent it to m_bottomSplitter rather
     * than either individual child.
     */
    const int lowerTitleBandHeight =
        std::max(
            buttonSize,
            m_eventDetailPanel
                    ->fontMetrics()
                    .height()
                + InterfaceScale::pixels(
                    4,
                    m_eventDetailPanel
                    )
            );

    const int lowerButtonX =
        std::max(
            0,
            m_eventDetailPanel->width()
                - rightInset
                - buttonSize
            );

    const int lowerButtonY =
        std::max(
            0,
            (
                lowerTitleBandHeight
                - buttonSize
                )
                / 2
            )
        + (
            m_lowerRegionCollapsed
                ? -InterfaceScale::pixels(
                      2,
                      this
                      )
                : InterfaceScale::pixels(
                      1,
                      this
                      )
            )
        + 1;

    m_timelineCollapseButton->setGeometry(
        timelineButtonX,
        timelineButtonY,
        buttonSize,
        buttonSize
        );

    m_eventCollapseButton->setGeometry(
        eventButtonX,
        eventButtonY,
        buttonSize,
        buttonSize
        );

    m_lowerRegionCollapseButton->setGeometry(
        lowerButtonX,
        lowerButtonY,
        buttonSize,
        buttonSize
        );

    m_timelineCollapseButton->raise();
    m_eventCollapseButton->raise();
    m_lowerRegionCollapseButton->raise();
}

void InvestigationSessionView::
    scheduleSectionCollapseControlGeometryUpdate()
{
    if (m_sectionCollapseGeometryUpdatePending) {
        return;
    }

    m_sectionCollapseGeometryUpdatePending =
        true;

    QTimer::singleShot(
        0,
        this,
        [this]() {
            m_sectionCollapseGeometryUpdatePending =
                false;

            /*
             * A section switch can change multiple
             * child minimum/maximum heights during one
             * operation.
             *
             * Make sure the outer document layout and
             * vertical splitter have consumed those
             * new constraints before mapping child
             * coordinates for the overlay chevrons.
             */
            if (layout() != nullptr) {
                layout()->activate();
            }

            if (m_mainSplitter != nullptr) {
                m_mainSplitter->updateGeometry();

                if (
                    m_mainSplitter
                            ->parentWidget() != nullptr
                    && m_mainSplitter
                               ->parentWidget()
                               ->layout() != nullptr
                    ) {
                    m_mainSplitter
                        ->parentWidget()
                        ->layout()
                        ->activate();
                }
            }

            updateSectionCollapseControlGeometry();
        }
        );
}

void InvestigationSessionView::
    updateEventSectionPresentation()
{
    if (
        m_eventSectionContainer == nullptr
        || m_eventSectionLayout == nullptr
        || m_eventPanel == nullptr
        ) {
        return;
    }

    const bool allSectionsCollapsed =
        m_timelineCollapsed
        && m_eventCollapsed
        && m_lowerRegionCollapsed;

    if (!m_eventCollapsed) {
        /*
         * Normal expanded behavior: Telemetry Events
         * fills the middle splitter section.
         */
        m_eventSectionContainer->setMinimumHeight(
            0
            );

        m_eventSectionContainer->setMaximumHeight(
            QWIDGETSIZE_MAX
            );

        m_eventSectionContainer->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Expanding
            );

        m_eventSectionLayout->setAlignment(
            m_eventPanel,
            Qt::Alignment()
            );

        m_eventPanel->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Expanding
            );

        m_eventSectionContainer
            ->updateGeometry();

        return;
    }

    const int eventCollapsedHeight =
        m_eventPanel->collapsedHeight();

    if (allSectionsCollapsed) {
        /*
         * This is the deliberately spacious manual
         * all-collapsed presentation.
         *
         * Timeline remains pinned to the top.
         * Lower Details remains pinned to the bottom.
         * The middle container absorbs spare space,
         * while the fixed-height Telemetry Events
         * title strip is centered inside it.
         *
         * A future severely-constrained layout tier
         * will override this and pack all three title
         * strips tightly instead.
         */
        m_eventSectionContainer->setMinimumHeight(
            eventCollapsedHeight
            );

        m_eventSectionContainer->setMaximumHeight(
            QWIDGETSIZE_MAX
            );

        m_eventSectionContainer->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Expanding
            );

        m_eventSectionLayout->setAlignment(
            m_eventPanel,
            Qt::AlignVCenter
            );
    } else {
        /*
         * Event is collapsed while at least one other
         * section remains open. Consume only the
         * native Telemetry Events title-strip height
         * so usable space remains available to the
         * expanded sections.
         */
        m_eventSectionContainer->setMinimumHeight(
            eventCollapsedHeight
            );

        m_eventSectionContainer->setMaximumHeight(
            eventCollapsedHeight
            );

        m_eventSectionContainer->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Fixed
            );

        m_eventSectionLayout->setAlignment(
            m_eventPanel,
            Qt::AlignVCenter
            );
    }

    m_eventPanel->setSizePolicy(
        QSizePolicy::Preferred,
        QSizePolicy::Fixed
        );

    m_eventSectionContainer
        ->updateGeometry();
}

void InvestigationSessionView::
    toggleSectionFromUser(
        InvestigationSection section
        )
{
    ensureSectionPreferredHeightsInitialized();

    const bool currentlyCollapsed =
        isSectionCollapsed(
            section
            );

    /*
     * Manual collapse is always permitted.
     */
    if (!currentlyCollapsed) {
        setSectionPreferredCollapsed(
            section,
            true
            );

        setSectionCollapsed(
            section,
            true
            );

        if (m_hasConstrainedRestoreSnapshot) {
            rebuildConstrainedRestorePriorityFromCurrentState();
        }

        scheduleSectionCollapseControlGeometryUpdate();

        return;
    }

    /*
     * Capacity 0 deliberately forbids expansion.
     */
    const int maximumOpenSections =
        static_cast<int>(
            m_sectionCapacity
            );

    if (maximumOpenSections <= 0) {
        return;
    }

    /*
     * The user explicitly wants this section open.
     */
    setSectionPreferredCollapsed(
        section,
        false
        );

    /*
     * If the tier is already full, evict the
     * lowest-priority currently open section.
     *
     * This is effective-state eviction only. Its user
     * preference remains open, allowing it to return
     * when capacity later increases.
     */
    const bool wasApplyingPolicy =
        m_applyingSectionCapacityPolicy;

    m_applyingSectionCapacityPolicy =
        true;

    while (
        openSectionCount()
        >= maximumOpenSections
        ) {
        if (!m_timelineCollapsed) {
            setSectionCollapsed(
                InvestigationSection::Timeline,
                true
                );

            continue;
        }

        if (!m_lowerRegionCollapsed) {
            setSectionCollapsed(
                InvestigationSection::LowerDetails,
                true
                );

            continue;
        }

        if (!m_eventCollapsed) {
            setSectionCollapsed(
                InvestigationSection::Events,
                true
                );

            continue;
        }

        break;
    }

    m_applyingSectionCapacityPolicy =
        wasApplyingPolicy;

    setSectionCollapsed(
        section,
        false
        );

    if (
        m_sectionCapacity
        == InvestigationSectionCapacity::One
        ) {
        allocateSingleOpenSection(
            section
            );
    } else {
        scheduleSectionCollapseControlGeometryUpdate();
    }

    if (m_hasConstrainedRestoreSnapshot) {
        rebuildConstrainedRestorePriorityFromCurrentState();
    }
}

void InvestigationSessionView::
    setSectionPreferredCollapsed(
        InvestigationSection section,
        bool collapsed
        )
{
    switch (section) {
    case InvestigationSection::Timeline:
        m_timelinePreferredCollapsed =
            collapsed;
        break;

    case InvestigationSection::Events:
        m_eventPreferredCollapsed =
            collapsed;
        break;

    case InvestigationSection::LowerDetails:
        m_lowerRegionPreferredCollapsed =
            collapsed;
        break;
    }
}

bool InvestigationSessionView::
    isSectionPreferredCollapsed(
        InvestigationSection section
        ) const
{
    switch (section) {
    case InvestigationSection::Timeline:
        return m_timelinePreferredCollapsed;

    case InvestigationSection::Events:
        return m_eventPreferredCollapsed;

    case InvestigationSection::LowerDetails:
        return m_lowerRegionPreferredCollapsed;
    }

    return true;
}

bool InvestigationSessionView::
    isSectionCollapsed(
        InvestigationSection section
        ) const
{
    switch (section) {
    case InvestigationSection::Timeline:
        return m_timelineCollapsed;

    case InvestigationSection::Events:
        return m_eventCollapsed;

    case InvestigationSection::LowerDetails:
        return m_lowerRegionCollapsed;
    }

    return true;
}

void InvestigationSessionView::
    setSectionCollapsed(
        InvestigationSection section,
        bool collapsed
        )
{
    switch (section) {
    case InvestigationSection::Timeline:
        setTimelineCollapsed(
            collapsed
            );
        break;

    case InvestigationSection::Events:
        setEventSectionCollapsed(
            collapsed
            );
        break;

    case InvestigationSection::LowerDetails:
        setLowerRegionCollapsed(
            collapsed
            );
        break;
    }
}

void InvestigationSessionView::
    setTimelineCollapsed(
        bool collapsed
        )
{
    if (
        m_timelineCollapsed == collapsed
        || m_mainSplitter == nullptr
        || m_timelinePanel == nullptr
        ) {
        return;
    }

    if (m_eventCollapsed) {
        /*
         * The section configuration has changed since
         * Events was collapsed, so its exact pre-collapse
         * splitter snapshot is no longer authoritative.
         */
        m_eventPreCollapseSizes.clear();
    }

    /*
     * Capture the complete splitter allocation before
     * changing Timeline's height constraints.
     *
     * This lets both collapse and expansion preserve
     * the untouched lower auxiliary region exactly.
     */
    const QList<int> previousSizes =
        m_mainSplitter->sizes();

    m_timelineCollapsed =
        collapsed;

    m_timelinePanel->setCollapsed(
        collapsed
        );

    updateEventSectionPresentation();

    m_mainSplitter->updateGeometry();

    updateSectionCollapseControls();

    if (collapsed) {
        if (m_applyingSectionCapacityPolicy) {
            QTimer::singleShot(
                0,
                this,
                [this]() {
                    scheduleSectionCollapseControlGeometryUpdate();
                }
                );
        } else {
            QTimer::singleShot(
                0,
                this,
                [this, previousSizes]() {
                    if (!m_timelineCollapsed) {
                        return;
                    }

                    allocateCollapsedSectionSpaceToEvents(
                        0,
                        previousSizes
                        );

                    scheduleSectionCollapseControlGeometryUpdate();
                }
                );
        }
    } else {
        /*
         * Expansion is different: we already know both
         * the remembered expanded Timeline height and
         * the complete pre-expansion splitter state.
         *
         * Restore immediately so QSplitter never gets
         * an event-loop turn in which it can visibly
         * redistribute height into/out of the lower
         * auxiliary region.
         */
        restoreMainSplitterSectionHeight(
            0,
            m_timelineExpandedHeight,
            previousSizes
            );

        scheduleSectionCollapseControlGeometryUpdate();
    }
}

void InvestigationSessionView::
    setEventSectionCollapsed(
        bool collapsed
        )
{
    if (
        m_eventCollapsed == collapsed
        || m_mainSplitter == nullptr
        || m_eventPanel == nullptr
        || m_eventSectionContainer == nullptr
        ) {
        return;
    }

    const QList<int> previousSizes =
        m_mainSplitter->sizes();

    if (
        collapsed
        && previousSizes.size() == 3
        ) {
        if (m_applyingSectionCapacityPolicy) {
            m_eventPreCollapseSizes.clear();
        } else {
            m_eventPreCollapseSizes =
                previousSizes;
        }

        m_timelineManuallyResizedWhileEventCollapsed =
            false;

        m_lowerRegionManuallyResizedWhileEventCollapsed =
            false;
    }

    m_eventCollapsed =
        collapsed;

    m_eventPanel->setCollapsed(
        collapsed
        );

    updateEventSectionPresentation();

    m_mainSplitter->updateGeometry();

    updateSectionCollapseControls();

    if (collapsed) {
        if (!m_applyingSectionCapacityPolicy) {
            allocateCollapsedEventSpace(
                previousSizes
                );
        }
    } else {
        const bool hasManualResizePreference =
            m_timelineManuallyResizedWhileEventCollapsed
            || m_lowerRegionManuallyResizedWhileEventCollapsed;

        const bool timelineOpen =
            !m_timelineCollapsed;

        const bool lowerRegionOpen =
            !m_lowerRegionCollapsed;

        const bool exactlyOneAuxiliaryOpen =
            timelineOpen
            != lowerRegionOpen;

        if (
            hasManualResizePreference
            || exactlyOneAuxiliaryOpen
            ) {
            /*
         * There are two reasons to rebuild Events around
         * the current auxiliary preferences rather than
         * restore an old three-way splitter snapshot:
         *
         * 1. The user explicitly resized an auxiliary
         *    section while Events was closed.
         *
         * 2. Exactly one auxiliary section is open.
         *
         * In the second case, that section may currently
         * be oversized only because it absorbed the space
         * released by the other collapsed sections.
         *
         * Return it to its preferred height and let Events
         * consume the remaining available space.
         */
            restoreEventSectionUsingAuxiliaryPreferences(
                previousSizes
                );
        } else if (
            m_eventPreCollapseSizes.size() == 3
            ) {
            /*
             * No intervening state/preference change:
             * ordinary Events collapse -> reopen remains an
             * exact reversible transaction.
             */
            applyMainSplitterSizes(
                m_eventPreCollapseSizes
                );
        } else {
            restoreMainSplitterSectionHeight(
                1,
                m_eventExpandedHeight,
                previousSizes
                );
        }

        m_eventPreCollapseSizes.clear();

        m_timelineManuallyResizedWhileEventCollapsed =
            false;

        m_lowerRegionManuallyResizedWhileEventCollapsed =
            false;
    }

    QTimer::singleShot(
        0,
        this,
        [this]() {
            updateEventSectionPresentation();
            scheduleSectionCollapseControlGeometryUpdate();
        }
        );
}

void InvestigationSessionView::
    setLowerRegionCollapsed(
        bool collapsed
        )
{
    if (
        m_lowerRegionCollapsed == collapsed
        || m_mainSplitter == nullptr
        || m_reviewPanel == nullptr
        || m_eventDetailPanel == nullptr
        ) {
        return;
    }

    if (m_eventCollapsed) {
        m_eventPreCollapseSizes.clear();
    }

    /*
     * Preserve the exact pre-transition vertical
     * allocation before changing either lower panel's
     * collapsed height constraint.
     */
    const QList<int> previousSizes =
        m_mainSplitter->sizes();

    m_lowerRegionCollapsed =
        collapsed;

    m_reviewPanel->setCollapsed(
        collapsed
        );

    m_eventDetailPanel->setCollapsed(
        collapsed
        );

    updateLowerRegionMinimumHeight();

    updateEventSectionPresentation();

    m_bottomSplitter->updateGeometry();
    m_mainSplitter->updateGeometry();

    updateSectionCollapseControls();

    if (collapsed) {
        if (m_applyingSectionCapacityPolicy) {
            QTimer::singleShot(
                0,
                this,
                [this]() {
                    scheduleSectionCollapseControlGeometryUpdate();
                }
                );
        } else {
            QTimer::singleShot(
                0,
                this,
                [this, previousSizes]() {
                    if (!m_lowerRegionCollapsed) {
                        return;
                    }

                    allocateCollapsedSectionSpaceToEvents(
                        2,
                        previousSizes
                        );

                    scheduleSectionCollapseControlGeometryUpdate();
                }
                );
        }
    } else {
        /*
         * Ordinary manual expansion restores Lower Details
         * using the user's remembered splitter preference.
         *
         * During constrained automatic recovery, however,
         * the capacity coordinator owns the complete
         * vertical allocation. Running the ordinary restore
         * here can temporarily take space from Timeline
         * before applyConstrainedRecoveryLayout() immediately
         * corrects it, producing a visible one-frame jump.
         */
        if (!m_applyingSectionCapacityPolicy) {
            restoreMainSplitterSectionHeight(
                2,
                m_lowerRegionExpandedHeight,
                previousSizes
                );
        }

        QTimer::singleShot(
            0,
            this,
            [this]() {
                if (m_lowerRegionCollapsed) {
                    return;
                }

                updateReviewSplitter(
                    m_reviewPanel
                        ->currentTab()
                    );

                scheduleSectionCollapseControlGeometryUpdate();
            }
            );
    }
}

void InvestigationSessionView::
    allocateCollapsedSectionSpaceToEvents(
        int sectionIndex,
        const QList<int> &previousSizes
        )
{
    if (
        m_mainSplitter == nullptr
        || previousSizes.size() != 3
        || (
            sectionIndex != 0
            && sectionIndex != 2
            )
        ) {
        return;
    }

    /*
     * If this transition has produced the special
     * all-three-collapsed state, do not force the
     * released space into another compact section.
     *
     * The Event container owns it for the centered
     * Telemetry Events presentation.
     */
    if (
        m_timelineCollapsed
        && m_eventCollapsed
        && m_lowerRegionCollapsed
        ) {
        updateEventSectionPresentation();
        return;
    }

    const QList<int> settledSizes =
        m_mainSplitter->sizes();

    if (settledSizes.size() != 3) {
        return;
    }

    const int previousSectionHeight =
        previousSizes.at(
            sectionIndex
            );

    const int collapsedSectionHeight =
        std::min(
            previousSectionHeight,
            settledSizes.at(
                sectionIndex
                )
            );

    const int releasedHeight =
        std::max(
            0,
            previousSectionHeight
                - collapsedSectionHeight
            );

    QList<int> targetSizes =
        previousSizes;

    targetSizes[sectionIndex] =
        collapsedSectionHeight;

    /*
     * Global open-section priority:
     *
     *   Events
     *   Lower Details
     *   Timeline
     *
     * Therefore an auxiliary section normally donates
     * its released height to Events. If Events is
     * closed, use whichever remaining auxiliary
     * section is still open.
     */
    if (!m_eventCollapsed) {
        targetSizes[1] +=
            releasedHeight;
    } else if (
        sectionIndex != 2
        && !m_lowerRegionCollapsed
        ) {
        targetSizes[2] +=
            releasedHeight;
    } else if (
        sectionIndex != 0
        && !m_timelineCollapsed
        ) {
        targetSizes[0] +=
            releasedHeight;
    }

    applyMainSplitterSizes(
        targetSizes
        );
}

void InvestigationSessionView::
    allocateCollapsedEventSpace(
        const QList<int> &previousSizes
        )
{
    if (
        m_mainSplitter == nullptr
        || m_eventPanel == nullptr
        || previousSizes.size() != 3
        ) {
        return;
    }

    /*
     * When all three sections are collapsed, the
     * Event container deliberately owns the flexible
     * middle space so its title strip can be centered.
     */
    if (
        m_timelineCollapsed
        && m_eventCollapsed
        && m_lowerRegionCollapsed
        ) {
        updateEventSectionPresentation();
        return;
    }

    /*
     * Do not measure splitter index 1 here.
     *
     * It is now the Event section container and may
     * intentionally be larger than the collapsed
     * Event panel.
     *
     * The Event panel itself knows its true fixed
     * collapsed title-strip height.
     */
    const int collapsedEventHeight =
        m_eventPanel->collapsedHeight();

    if (collapsedEventHeight <= 0) {
        return;
    }

    const int previousEventHeight =
        previousSizes.at(1);

    const int releasedHeight =
        std::max(
            0,
            previousEventHeight
                - collapsedEventHeight
            );

    QList<int> targetSizes =
        previousSizes;

    targetSizes[1] =
        collapsedEventHeight;

    /*
     * Recipient priority when Events closes:
     *
     *   1. Lower Details
     *   2. Timeline
     *
     * Events itself is the section being closed.
     */
    if (!m_lowerRegionCollapsed) {
        targetSizes[2] +=
            releasedHeight;
    } else if (!m_timelineCollapsed) {
        targetSizes[0] +=
            releasedHeight;
    }

    applyMainSplitterSizes(
        targetSizes
        );
}

void InvestigationSessionView::
    allocateSingleOpenSection(
        InvestigationSection openSection
        )
{
    if (m_mainSplitter == nullptr) {
        return;
    }

    const QList<int> currentSizes =
        m_mainSplitter->sizes();

    if (currentSizes.size() != 3) {
        return;
    }

    /*
     * QSplitter::sizes() excludes the handles, so its
     * sum is exactly the child-space budget we need to
     * redistribute.
     *
     * This remains useful even if the current
     * proportions came from an intermediate
     * all-collapsed state.
     */
    const int availableChildHeight =
        currentSizes.at(0)
        + currentSizes.at(1)
        + currentSizes.at(2);

    QList<int> targetSizes{
        sectionCompactHeight(
            InvestigationSection::Timeline
            ),
        sectionCompactHeight(
            InvestigationSection::Events
            ),
        sectionCompactHeight(
            InvestigationSection::LowerDetails
            )
    };

    int openIndex =
        -1;

    switch (openSection) {
    case InvestigationSection::Timeline:
        openIndex = 0;
        break;

    case InvestigationSection::Events:
        openIndex = 1;
        break;

    case InvestigationSection::LowerDetails:
        openIndex = 2;
        break;
    }

    if (openIndex < 0) {
        return;
    }

    int collapsedHeightTotal = 0;

    for (
        int index = 0;
        index < targetSizes.size();
        ++index
        ) {
        if (index == openIndex) {
            continue;
        }

        collapsedHeightTotal +=
            targetSizes.at(index);
    }

    /*
     * At capacity 1 the open section deliberately owns
     * every remaining pixel.
     *
     * Preferred height is still retained separately;
     * this oversized presentation must never become a
     * new preference.
     */
    targetSizes[openIndex] =
        std::max(
            1,
            availableChildHeight
                - collapsedHeightTotal
            );

    applyMainSplitterSizes(
        targetSizes
        );

    updateEventSectionPresentation();

    scheduleSectionCollapseControlGeometryUpdate();
}

void InvestigationSessionView::
    restoreMainSplitterSectionHeight(
        int sectionIndex,
        int preferredHeight,
        const QList<int> &previousSizes
        )
{
    if (
        m_mainSplitter == nullptr
        || previousSizes.size() != 3
        || sectionIndex < 0
        || sectionIndex > 2
        || preferredHeight <= 0
        ) {
        return;
    }

    QList<int> targetSizes =
        previousSizes;

    /*
     * ---------------------------------------------------------
     * Section-state helpers
     * ---------------------------------------------------------
     */

    const auto isCollapsed =
        [this](
            int index
            ) {
            switch (index) {
            case 0:
                return m_timelineCollapsed;

            case 1:
                return m_eventCollapsed;

            case 2:
                return m_lowerRegionCollapsed;

            default:
                return true;
            }
        };

    const auto rememberedHeight =
        [this, &targetSizes](
            int index
            ) {
            int remembered = 0;

            switch (index) {
            case 0:
                remembered =
                    m_timelineExpandedHeight;
                break;

            case 1:
                remembered =
                    m_eventExpandedHeight;
                break;

            case 2:
                remembered =
                    m_lowerRegionExpandedHeight;
                break;

            default:
                break;
            }

            /*
             * If the user has never collapsed this
             * section, we have no explicit remembered
             * preference yet. Treat its current height
             * as its preference.
             */
            if (remembered <= 0) {
                remembered =
                    targetSizes.at(index);
            }

            return remembered;
        };

    /*
     * ---------------------------------------------------------
     * Recover spare space from the centered Event container
     * ---------------------------------------------------------
     *
     * When Events is collapsed and all sections were
     * previously collapsed, splitter index 1 may be
     * very large even though the visible Event strip
     * is only a few pixels high.
     *
     * Once another section is being opened, that
     * centering space becomes genuinely available.
     */
    int freeSpace = 0;

    if (
        sectionIndex != 1
        && m_eventCollapsed
        && m_eventPanel != nullptr
        ) {
        const int compactEventHeight =
            m_eventPanel->collapsedHeight();

        if (
            compactEventHeight > 0
            && targetSizes.at(1)
                   > compactEventHeight
            ) {
            freeSpace =
                targetSizes.at(1)
                - compactEventHeight;

            targetSizes[1] =
                compactEventHeight;
        }
    }

    /*
     * Do not shrink a target that already has more
     * than its remembered preferred height.
     */
    int remainingNeeded =
        std::max(
            0,
            preferredHeight
                - targetSizes.at(
                    sectionIndex
                    )
            );

    /*
     * First consume genuinely unowned spare space.
     */
    if (
        remainingNeeded > 0
        && freeSpace > 0
        ) {
        const int amount =
            std::min(
                remainingNeeded,
                freeSpace
                );

        targetSizes[sectionIndex] +=
            amount;

        remainingNeeded -=
            amount;

        freeSpace -=
            amount;
    }

    /*
     * ---------------------------------------------------------
     * Donor ordering
     * ---------------------------------------------------------
     *
     * When a section still needs room:
     *
     * Event expansion:
     *   Timeline, then Lower
     *
     * Lower expansion:
     *   Timeline, then Events
     *
     * Timeline expansion:
     *   Lower, then Events
     *
     * This is the priority ordering:
     *
     *   Events > Lower > Timeline
     */
    QList<int> donors;

    switch (sectionIndex) {
    case 0:
        donors = {
            2,
            1
        };
        break;

    case 1:
        donors = {
            0,
            2
        };
        break;

    case 2:
        donors = {
            0,
            1
        };
        break;

    default:
        return;
    }

    /*
     * ---------------------------------------------------------
     * Pass 1:
     * consume only surplus above remembered preferences
     * ---------------------------------------------------------
     *
     * This is the important behavior for the sequence:
     *
     * all collapsed
     *   -> Lower opens and becomes oversized
     *   -> Timeline opens
     *   -> Events opens
     *
     * Lower's surplus is consumed before Timeline is
     * pushed below its remembered size.
     */
    for (
        const int donorIndex
        : std::as_const(donors)
        ) {
        if (
            remainingNeeded <= 0
            || isCollapsed(
                donorIndex
                )
            ) {
            continue;
        }

        const int donorPreferred =
            rememberedHeight(
                donorIndex
                );

        const int donorSurplus =
            std::max(
                0,
                targetSizes.at(
                    donorIndex
                    )
                    - donorPreferred
                );

        const int amount =
            std::min(
                remainingNeeded,
                donorSurplus
                );

        targetSizes[donorIndex] -=
            amount;

        targetSizes[sectionIndex] +=
            amount;

        remainingNeeded -=
            amount;
    }

    /*
     * ---------------------------------------------------------
     * Pass 2:
     * compress below preference only from lower-priority
     * sections
     * ---------------------------------------------------------
     *
     * Priority:
     *
     *   Events
     *   Lower Details
     *   Timeline
     *
     * Surplus above preference is always available and was
     * already consumed in Pass 1.
     *
     * Below a section's preference, however, its space is
     * protected from lower-priority sections.
     *
     * Therefore:
     *
     * Opening Events may compress:
     *   Timeline, then Lower Details
     *
     * Opening Lower Details may compress:
     *   Timeline only
     *
     * Opening Timeline may compress:
     *   nobody
     *
     * If insufficient room remains, the newly opened
     * section simply receives less than its preferred
     * height. The future constrained-height coordinator
     * will handle that condition explicitly.
     */
    QList<int> belowPreferenceDonors;

    switch (sectionIndex) {
    case 0:
        /*
         * Timeline is lowest priority.
         *
         * It cannot push either Events or Lower Details
         * below their preferred heights.
         */
        break;

    case 1:
        /*
         * Events is highest priority.
         */
        belowPreferenceDonors = {
            0,
            2
        };
        break;

    case 2:
        /*
         * Lower Details outranks Timeline but not Events.
         */
        belowPreferenceDonors = {
            0
        };
        break;

    default:
        return;
    }

    for (
        const int donorIndex
        : std::as_const(belowPreferenceDonors)
        ) {
        if (
            remainingNeeded <= 0
            || isCollapsed(
                donorIndex
                )
            ) {
            continue;
        }

        QWidget *donorWidget =
            m_mainSplitter->widget(
                donorIndex
                );

        if (donorWidget == nullptr) {
            continue;
        }

        const int donorMinimum =
            std::max(
                1,
                std::max(
                    donorWidget->minimumHeight(),
                    donorWidget
                        ->minimumSizeHint()
                        .height()
                    )
                );

        const int available =
            std::max(
                0,
                targetSizes.at(
                    donorIndex
                    )
                    - donorMinimum
                );

        const int amount =
            std::min(
                remainingNeeded,
                available
                );

        targetSizes[donorIndex] -=
            amount;

        targetSizes[sectionIndex] +=
            amount;

        remainingNeeded -=
            amount;
    }

    /*
     * Any centering space left after restoring the
     * requested section still has to belong somewhere.
     *
     * Give it to the highest-priority currently-open
     * section:
     *
     *   Events
     *   Lower
     *   Timeline
     */
    if (freeSpace > 0) {
        int recipientIndex =
            -1;

        if (!m_eventCollapsed) {
            recipientIndex = 1;
        } else if (!m_lowerRegionCollapsed) {
            recipientIndex = 2;
        } else if (!m_timelineCollapsed) {
            recipientIndex = 0;
        }

        if (recipientIndex >= 0) {
            targetSizes[recipientIndex] +=
                freeSpace;
        }
    }

    applyMainSplitterSizes(
        targetSizes
        );
}

void InvestigationSessionView::
    restoreEventSectionUsingAuxiliaryPreferences(
        const QList<int> &previousSizes
        )
{
    if (
        m_mainSplitter == nullptr
        || m_eventPanel == nullptr
        || previousSizes.size() != 3
        ) {
        return;
    }

    QList<int> targetSizes =
        previousSizes;

    const int totalHeight =
        previousSizes.at(0)
        + previousSizes.at(1)
        + previousSizes.at(2);

    /*
     * ---------------------------------------------------------
     * Restore auxiliary preferences
     * ---------------------------------------------------------
     *
     * The manually changed section's remembered height
     * was updated by splitterMoved().
     *
     * The compensating section's remembered height was
     * deliberately left untouched.
     *
     * Therefore both values below now represent exactly
     * what we want after Events reopens.
     */
    if (
        !m_timelineCollapsed
        && m_timelineExpandedHeight > 0
        ) {
        targetSizes[0] =
            m_timelineExpandedHeight;
    }

    if (
        !m_lowerRegionCollapsed
        && m_lowerRegionExpandedHeight > 0
        ) {
        targetSizes[2] =
            m_lowerRegionExpandedHeight;
    }

    /*
     * Collapsed auxiliary sections retain their compact
     * current allocation.
     */

    int eventHeight =
        totalHeight
        - targetSizes.at(0)
        - targetSizes.at(2);

    /*
     * Events receives whatever remains after honoring
     * the current Timeline and Lower preferences.
     *
     * The user's newest manual size preference is more
     * important than restoring Events to the exact
     * height it had before it was collapsed.
     */
    const int eventMinimumHeight =
        std::max(
            1,
            m_eventPanel
                ->minimumSizeHint()
                .height()
            );

    if (eventHeight < eventMinimumHeight) {
        int deficit =
            eventMinimumHeight
            - eventHeight;

        /*
         * If ordinary geometry constraints make all
         * requested preferences impossible at once,
         * protect the manually resized section first.
         *
         * Global priority remains:
         *
         *   Events > Lower > Timeline
         *
         * but an explicit manual resize during this
         * transaction is treated as the freshest user
         * intent.
         */
        QList<int> donors;

        if (
            m_timelineManuallyResizedWhileEventCollapsed
            && !m_lowerRegionManuallyResizedWhileEventCollapsed
            ) {
            donors = {
                2,
                0
            };
        } else if (
            m_lowerRegionManuallyResizedWhileEventCollapsed
            && !m_timelineManuallyResizedWhileEventCollapsed
            ) {
            donors = {
                0,
                2
            };
        } else {
            /*
             * If both were manually adjusted, or we
             * somehow reach this path without a unique
             * manual target, compress the lower-priority
             * Timeline first.
             */
            donors = {
                0,
                2
            };
        }

        for (
            const int donorIndex
            : std::as_const(donors)
            ) {
            if (deficit <= 0) {
                break;
            }

            if (
                donorIndex == 0
                && m_timelineCollapsed
                ) {
                continue;
            }

            if (
                donorIndex == 2
                && m_lowerRegionCollapsed
                ) {
                continue;
            }

            QWidget *donor =
                m_mainSplitter->widget(
                    donorIndex
                    );

            if (donor == nullptr) {
                continue;
            }

            const int minimumHeight =
                std::max(
                    1,
                    std::max(
                        donor->minimumHeight(),
                        donor
                            ->minimumSizeHint()
                            .height()
                        )
                    );

            const int available =
                std::max(
                    0,
                    targetSizes.at(
                        donorIndex
                        )
                        - minimumHeight
                    );

            const int amount =
                std::min(
                    deficit,
                    available
                    );

            targetSizes[donorIndex] -=
                amount;

            deficit -=
                amount;
        }

        eventHeight =
            totalHeight
            - targetSizes.at(0)
            - targetSizes.at(2);
    }

    targetSizes[1] =
        std::max(
            1,
            eventHeight
            );

    applyMainSplitterSizes(
        targetSizes
        );
}

void InvestigationSessionView::
    ensureSectionPreferredHeightsInitialized()
{
    if (
        m_sectionPreferredHeightsInitialized
        || m_mainSplitter == nullptr
        ) {
        return;
    }

    const QList<int> sizes =
        m_mainSplitter->sizes();

    if (
        sizes.size() != 3
        || sizes.at(0) <= 0
        || sizes.at(1) <= 0
        || sizes.at(2) <= 0
        ) {
        return;
    }

    /*
     * These are the initial user-visible expanded
     * proportions. From this point onward they are
     * preferences, not merely current geometry.
     *
     * Automatic redistribution must not overwrite them.
     */
    m_timelineExpandedHeight =
        sizes.at(0);

    m_eventExpandedHeight =
        sizes.at(1);

    m_lowerRegionExpandedHeight =
        sizes.at(2);

    m_sectionPreferredHeightsInitialized =
        true;
}

void InvestigationSessionView::
    applyMainSplitterSizes(
        const QList<int> &sizes
        )
{
    if (
        m_mainSplitter == nullptr
        || sizes.size() != 3
        ) {
        return;
    }

    /*
     * Programmatic section redistribution must never
     * be mistaken for a user's splitter drag.
     */
    const QSignalBlocker blocker(
        m_mainSplitter
        );

    m_mainSplitter->setSizes(
        sizes
        );

    updateSectionCollapseControlGeometry();
}

int InvestigationSessionView::
    sectionCompactHeight(
        InvestigationSection section
        ) const
{
    const int fallback =
        InterfaceScale::pixels(
            CollapsedSectionFallbackLogicalHeight,
            this
            );

    switch (section) {
    case InvestigationSection::Timeline:
        if (
            m_timelineCollapsed
            && m_timelinePanel != nullptr
            ) {
            return std::max(
                fallback,
                m_timelinePanel->height()
                );
        }

        return fallback;

    case InvestigationSection::Events:
        if (
            m_eventCollapsed
            && m_eventPanel != nullptr
            ) {
            return std::max(
                fallback,
                m_eventPanel->collapsedHeight()
                );
        }

        return fallback;

    case InvestigationSection::LowerDetails:
    {
        int height =
            fallback;

        if (m_reviewPanel != nullptr) {
            height =
                std::max(
                    height,
                    m_reviewPanel->collapsedHeight()
                    );
        }

        if (
            m_lowerRegionCollapsed
            && m_eventDetailPanel != nullptr
            ) {
            height =
                std::max(
                    height,
                    m_eventDetailPanel
                        ->collapsedHeight()
                    );
        }

        return height;
    }
    }

    return fallback;
}

int InvestigationSessionView::
    minimumUsefulExpandedHeight(
        InvestigationSection section
        ) const
{
    int logicalHeight = 0;

    switch (section) {
    case InvestigationSection::Timeline:
        logicalHeight =
            TimelineMinimumUsefulLogicalHeight;
        break;

    case InvestigationSection::Events:
        logicalHeight =
            EventMinimumUsefulLogicalHeight;
        break;

    case InvestigationSection::LowerDetails:
        logicalHeight =
            LowerDetailsMinimumUsefulLogicalHeight;
        break;
    }

    return std::max(
        sectionCompactHeight(section) + 1,
        InterfaceScale::pixels(
            logicalHeight,
            this
            )
        );
}

int InvestigationSessionView::
    minimumCollapsedMainSplitterHeight() const
{
    if (m_mainSplitter == nullptr) {
        return 0;
    }

    const int timelineHeight =
        sectionCompactHeight(
            InvestigationSection::Timeline
            );

    const int eventHeight =
        sectionCompactHeight(
            InvestigationSection::Events
            );

    const int lowerHeight =
        sectionCompactHeight(
            InvestigationSection::LowerDetails
            );

    const int handleSpace =
        std::max(
            0,
            m_mainSplitter->count() - 1
            )
        * m_mainSplitter->handleWidth();

    return timelineHeight
           + eventHeight
           + lowerHeight
           + handleSpace;
}

void InvestigationSessionView::
    updateMinimumConstrainedHeight()
{
    if (
        m_mainSplitter == nullptr
        || layout() == nullptr
        ) {
        return;
    }

    const int minimumSplitterHeight =
        minimumCollapsedMainSplitterHeight();

    /*
     * The splitter remains vertically Ignored so its
     * changing expanded-content size hints cannot
     * create intermediate native resize barriers.
     *
     * But it does have one real minimum: enough room
     * to display all three collapsed section identities
     * and the splitter handles.
     */
    m_mainSplitter->setMinimumHeight(
        minimumSplitterHeight
        );

    /*
     * Also give the document itself an explicit
     * minimum derived from its non-splitter surfaces.
     *
     * This gives the containing workspace/window a
     * stable native floor rather than allowing the
     * collapsed section strips to be clipped.
     */
    const QMargins margins =
        layout()->contentsMargins();

    int minimumDocumentHeight =
        margins.top()
        + margins.bottom()
        + minimumSplitterHeight;

    int visibleItemCount = 1;

    const auto requiredHeight =
        [](
            const QWidget *widget
            ) {
            if (
                widget == nullptr
                || widget->isHidden()
                ) {
                return 0;
            }

            return std::max(
                0,
                std::max(
                    widget
                        ->minimumSizeHint()
                        .height(),
                    widget
                        ->sizeHint()
                        .height()
                    )
                );
        };

    if (
        m_summaryPanel != nullptr
        && !m_summaryPanel->isHidden()
        ) {
        minimumDocumentHeight +=
            requiredHeight(
                m_summaryPanel
                );

        ++visibleItemCount;
    }

    if (
        m_filterPanel != nullptr
        && !m_filterPanel->isHidden()
        ) {
        minimumDocumentHeight +=
            requiredHeight(
                m_filterPanel
                );

        ++visibleItemCount;
    }

    minimumDocumentHeight +=
        std::max(
            0,
            visibleItemCount - 1
            )
        * std::max(
            0,
            layout()->spacing()
            );

    setMinimumHeight(
        minimumDocumentHeight
        );
}

void InvestigationSessionView::
    updateLowerRegionMinimumHeight()
{
    if (
        m_bottomSplitter == nullptr
        || m_reviewPanel == nullptr
        ) {
        return;
    }

    /*
     * When collapsed, the two lower panels establish
     * their own compact fixed heights.
     *
     * Do not let the expanded usability minimum prevent
     * the lower region from reaching that collapsed
     * presentation.
     */
    if (m_lowerRegionCollapsed) {
        m_bottomSplitter->setMinimumHeight(
            0
            );

        return;
    }

    /*
     * Expanded hard floor:
     *
     * enough room for Analytics -> Bursts to expose
     * the Detected Bursts table header and first row.
     */
    const int minimumHeight =
        m_reviewPanel
            ->minimumUsefulExpandedHeight();

    m_bottomSplitter->setMinimumHeight(
        std::max(
            0,
            minimumHeight
            )
        );
}

int InvestigationSessionView::
    constrainedPreferredHeight(
        InvestigationSection section
        ) const
{
    switch (section) {
    case InvestigationSection::Timeline:
        return m_hasConstrainedRestoreSnapshot
                   ? m_constrainedTimelinePreferredHeight
                   : m_timelineExpandedHeight;

    case InvestigationSection::Events:
        return m_hasConstrainedRestoreSnapshot
                   ? m_constrainedEventPreferredHeight
                   : m_eventExpandedHeight;

    case InvestigationSection::LowerDetails:
        return m_hasConstrainedRestoreSnapshot
                   ? m_constrainedLowerPreferredHeight
                   : m_lowerRegionExpandedHeight;
    }

    return 0;
}

int InvestigationSessionView::
    constrainedRecoveryMinimumHeight(
        InvestigationSection section
        ) const
{
    const int policyMinimum =
        minimumUsefulExpandedHeight(
            section
            );

    const int preferredHeight =
        constrainedPreferredHeight(
            section
            );

    /*
     * The normal minimum-useful threshold describes
     * TraceScope's default responsive policy.
     *
     * But if the user manually established a smaller
     * valid expanded size before constrained resizing,
     * that explicit preference is allowed to override
     * the generic policy threshold during restoration.
     */
    int recoveryMinimum =
        preferredHeight > 0
            ? std::min(
                  policyMinimum,
                  preferredHeight
                  )
            : policyMinimum;

    /*
     * Lower Details additionally has a real content
     * floor: Analytics -> Bursts must retain enough
     * height for the Detected Bursts header and first
     * row.
     *
     * The user's preference can never go below that
     * hard floor.
     */
    if (
        section
            == InvestigationSection::LowerDetails
        && m_reviewPanel != nullptr
        ) {
        recoveryMinimum =
            std::max(
                recoveryMinimum,
                m_reviewPanel
                    ->minimumUsefulExpandedHeight()
                );
    }

    return recoveryMinimum;
}

InvestigationSessionView::
    InvestigationSectionCapacity
        InvestigationSessionView::
    sectionCapacityForAvailableHeight() const
{
    if (m_mainSplitter == nullptr) {
        return m_sectionCapacity;
    }

    const int availableHeight =
        availableMainSplitterHeight();

    if (availableHeight <= 0) {
        return m_sectionCapacity;
    }

    const int timelineCompact =
        sectionCompactHeight(
            InvestigationSection::Timeline
            );

    const int eventCompact =
        sectionCompactHeight(
            InvestigationSection::Events
            );

    const int lowerCompact =
        sectionCompactHeight(
            InvestigationSection::LowerDetails
            );

    const int handleSpace =
        std::max(
            0,
            m_mainSplitter->count() - 1
            )
        * m_mainSplitter->handleWidth();

    const int allCollapsedHeight =
        timelineCompact
        + eventCompact
        + lowerCompact
        + handleSpace;

    /*
     * Downward capacity thresholds.
     *
     * These remain the exact points at which sections
     * become too constrained to stay open.
     */
    const int oneSectionHeight =
        allCollapsedHeight
        + minimumUsefulExpandedHeight(
            InvestigationSection::Events
            )
        - eventCompact;

    const int twoSectionHeight =
        oneSectionHeight
        + minimumUsefulExpandedHeight(
            InvestigationSection::LowerDetails
            )
        - lowerCompact;

    const int threeSectionHeight =
        twoSectionHeight
        + minimumUsefulExpandedHeight(
            InvestigationSection::Timeline
            )
        - timelineCompact;

    /*
     * Growing requires a little more room than
     * shrinking.
     *
     * Without this dead zone, holding the mouse near a
     * threshold can alternate repeatedly between, for
     * example, capacity Two and Three as individual
     * resize/layout passes differ by only a pixel or two.
     */
    const int hysteresis =
        InterfaceScale::pixels(
            SectionCapacityHysteresisLogicalHeight,
            this
            );

    const int oneSectionReopenHeight =
        oneSectionHeight
        + hysteresis;

    const int twoSectionReopenHeight =
        twoSectionHeight
        + hysteresis;

    const int threeSectionReopenHeight =
        threeSectionHeight
        + hysteresis;

    switch (m_sectionCapacity) {
    case InvestigationSectionCapacity::Three:
        /*
         * Shrinking uses the normal thresholds
         * immediately.
         */
        if (availableHeight
            < oneSectionHeight) {
            return InvestigationSectionCapacity::None;
        }

        if (availableHeight
            < twoSectionHeight) {
            return InvestigationSectionCapacity::One;
        }

        if (availableHeight
            < threeSectionHeight) {
            return InvestigationSectionCapacity::Two;
        }

        return InvestigationSectionCapacity::Three;

    case InvestigationSectionCapacity::Two:
        /*
         * Continue shrinking normally.
         */
        if (availableHeight
            < oneSectionHeight) {
            return InvestigationSectionCapacity::None;
        }

        if (availableHeight
            < twoSectionHeight) {
            return InvestigationSectionCapacity::One;
        }

        /*
         * But do not reopen the third section until
         * there is a little breathing room beyond its
         * original threshold.
         */
        if (availableHeight
            >= threeSectionReopenHeight) {
            return InvestigationSectionCapacity::Three;
        }

        return InvestigationSectionCapacity::Two;

    case InvestigationSectionCapacity::One:
        if (availableHeight
            < oneSectionHeight) {
            return InvestigationSectionCapacity::None;
        }

        /*
         * A large resize event may cross more than one
         * boundary at once, so allow direct promotion.
         */
        if (availableHeight
            >= threeSectionReopenHeight) {
            return InvestigationSectionCapacity::Three;
        }

        if (availableHeight
            >= twoSectionReopenHeight) {
            return InvestigationSectionCapacity::Two;
        }

        return InvestigationSectionCapacity::One;

    case InvestigationSectionCapacity::None:
        if (availableHeight
            >= threeSectionReopenHeight) {
            return InvestigationSectionCapacity::Three;
        }

        if (availableHeight
            >= twoSectionReopenHeight) {
            return InvestigationSectionCapacity::Two;
        }

        if (availableHeight
            >= oneSectionReopenHeight) {
            return InvestigationSectionCapacity::One;
        }

        return InvestigationSectionCapacity::None;
    }

    return m_sectionCapacity;
}

int InvestigationSessionView::
    availableMainSplitterHeight() const
{
    if (layout() == nullptr) {
        return m_mainSplitter != nullptr
                   ? m_mainSplitter->height()
                   : 0;
    }

    const QMargins margins =
        layout()->contentsMargins();

    int availableHeight =
        contentsRect().height()
        - margins.top()
        - margins.bottom();

    int visibleItemCount = 1; // main splitter

    const auto preferredWidgetHeight =
        [](
            const QWidget *widget
            ) {
            if (
                widget == nullptr
                || widget->isHidden()
                ) {
                return 0;
            }

            const int sizeHintHeight =
                widget
                    ->sizeHint()
                    .height();

            const int minimumSizeHintHeight =
                widget
                    ->minimumSizeHint()
                    .height();

            /*
             * Use the widget's requested/preferred
             * vertical requirement, not its current
             * geometry.
             *
             * Current geometry may contain surplus
             * distributed by QVBoxLayout while the
             * investigation splitter is compact.
             */
            return std::max(
                0,
                std::max(
                    sizeHintHeight,
                    minimumSizeHintHeight
                    )
                );
        };

    if (
        m_summaryPanel != nullptr
        && !m_summaryPanel->isHidden()
        ) {
        availableHeight -=
            preferredWidgetHeight(
                m_summaryPanel
                );

        ++visibleItemCount;
    }

    if (
        m_filterPanel != nullptr
        && !m_filterPanel->isHidden()
        ) {
        availableHeight -=
            preferredWidgetHeight(
                m_filterPanel
                );

        ++visibleItemCount;
    }

    const int spacing =
        std::max(
            0,
            layout()->spacing()
            );

    availableHeight -=
        std::max(
            0,
            visibleItemCount - 1
            )
        * spacing;

    return std::max(
        0,
        availableHeight
        );
}

int InvestigationSessionView::
    openSectionCount() const
{
    int count = 0;

    if (!m_timelineCollapsed) {
        ++count;
    }

    if (!m_eventCollapsed) {
        ++count;
    }

    if (!m_lowerRegionCollapsed) {
        ++count;
    }

    return count;
}

void InvestigationSessionView::
    scheduleSectionCapacityUpdate()
{
    if (m_sectionCapacityUpdatePending) {
        return;
    }

    m_sectionCapacityUpdatePending =
        true;

    QTimer::singleShot(
        0,
        this,
        [this]() {
            m_sectionCapacityUpdatePending =
                false;

            applySectionCapacityPolicy();
        }
        );
}

void InvestigationSessionView::
    applySectionCapacityPolicy()
{
    if (m_mainSplitter == nullptr) {
        return;
    }

    const InvestigationSectionCapacity
        previousCapacity =
        m_sectionCapacity;

    const InvestigationSectionCapacity
        newCapacity =
        sectionCapacityForAvailableHeight();

    const int currentAvailableHeight =
        availableMainSplitterHeight();

    const int previousAvailableHeight =
        m_lastAvailableMainSplitterHeight;

    const bool heightIncreasing =
        previousAvailableHeight >= 0
        && currentAvailableHeight
               > previousAvailableHeight;

    const bool heightDecreasing =
        previousAvailableHeight >= 0
        && currentAvailableHeight
               < previousAvailableHeight;

    /*
     * Store this before making any section-state changes.
     *
     * A queued settling pass at the same document height
     * must not be mistaken for additional user growth.
     */
    m_lastAvailableMainSplitterHeight =
        currentAvailableHeight;

    /*
     * A constrained resize cycle begins as soon as the
     * available investigation height starts decreasing.
     *
     * Do not wait for a capacity boundary to be crossed.
     * The user may reverse direction before any section
     * needs to collapse, and in that case recovery still
     * needs the pre-resize preferred section heights.
     */
    if (heightDecreasing) {
        /*
         * A completely recovered previous cycle is no
         * longer relevant once a new downward resize
         * begins. Start a fresh cycle from the user's
         * current preferred arrangement.
         */
        if (m_constrainedRecoveryComplete) {
            resetConstrainedRestoreState();
        }

        if (!m_hasConstrainedRestoreSnapshot) {
            captureConstrainedRestorePriority();
        }
    }

    const bool capacityChanged =
        newCapacity
        != previousCapacity;

    const int initialOpenSectionCount =
        openSectionCount();

    m_sectionCapacity =
        newCapacity;

    const int maximumOpenSections =
        static_cast<int>(
            newCapacity
            );

    const bool wasApplyingPolicy =
        m_applyingSectionCapacityPolicy;

    m_applyingSectionCapacityPolicy =
        true;

    /*
     * Once capacity actually forces a section closed, this
     * becomes a full constrained recovery cycle rather than
     * a simple partial resize/reverse.
     */
    if (
        openSectionCount()
        > maximumOpenSections
        ) {
        m_constrainedSectionCollapsedDuringCycle =
            true;
    }

    /*
     * ---------------------------------------------------------
     * Resize downward
     * ---------------------------------------------------------
     *
     * Collapse lowest retention priority first:
     *
     *   Timeline
     *   Lower Details
     *   Events
     *
     * Preferred/manual state is deliberately untouched.
     */
    while (
        openSectionCount()
        > maximumOpenSections
        ) {
        if (!m_timelineCollapsed) {
            setSectionCollapsed(
                InvestigationSection::Timeline,
                true
                );

            continue;
        }

        if (!m_lowerRegionCollapsed) {
            setSectionCollapsed(
                InvestigationSection::LowerDetails,
                true
                );

            continue;
        }

        if (!m_eventCollapsed) {
            setSectionCollapsed(
                InvestigationSection::Events,
                true
                );

            continue;
        }

        break;
    }

    /*
     * ---------------------------------------------------------
     * Resize upward
     * ---------------------------------------------------------
     */
    if (
        heightIncreasing
        && m_hasConstrainedRestoreSnapshot
        ) {
        /*
         * First redistribute the sections that are already
         * open according to the constrained recovery rules.
         */
        applyConstrainedRecoveryLayout();

        while (
            openSectionCount()
            < maximumOpenSections
            ) {
            InvestigationSection candidate =
                InvestigationSection::Timeline;

            bool foundCandidate =
                false;

            for (
                const InvestigationSection section
                : std::as_const(
                    m_constrainedRestorePriority
                    )
                ) {
                if (isSectionCollapsed(section)) {
                    candidate =
                        section;

                    foundCandidate =
                        true;

                    break;
                }
            }

            if (!foundCandidate) {
                break;
            }

            /*
             * Capacity alone is not sufficient.
             *
             * There must be enough actual room for the
             * candidate's useful minimum while preserving
             * every already-restored auxiliary section at
             * its frozen preferred height.
             */
            if (
                !canRestoreConstrainedSection(
                    candidate
                    )
                ) {
                break;
            }

            /*
             * Opening a constrained section changes its
             * geometry constraints before the recovery
             * allocator can impose the final splitter sizes.
             *
             * Keep that intermediate redistribution from
             * being painted.
             */
            m_mainSplitter->setUpdatesEnabled(
                false
                );

            setSectionCollapsed(
                candidate,
                false
                );

            applyConstrainedRecoveryLayout();

            m_mainSplitter->setUpdatesEnabled(
                true
                );

            m_mainSplitter->update();
        }

        /*
         * Even when nothing new opened, continued window
         * growth must flow to the correct recovering
         * section.
         */
        applyConstrainedRecoveryLayout();
    }

    if (
        maximumOpenSections == 1
        && openSectionCount() == 1
        ) {
        if (!m_eventCollapsed) {
            allocateSingleOpenSection(
                InvestigationSection::Events
                );
        } else if (!m_lowerRegionCollapsed) {
            allocateSingleOpenSection(
                InvestigationSection::LowerDetails
                );
        } else if (!m_timelineCollapsed) {
            allocateSingleOpenSection(
                InvestigationSection::Timeline
                );
        }
    }

    updateConstrainedRecoveryCompletion();

    const bool sectionStateChanged =
        openSectionCount()
        != initialOpenSectionCount;

    const bool needsSettledCapacityRecheck =
        capacityChanged
        || sectionStateChanged;

    m_applyingSectionCapacityPolicy =
        wasApplyingPolicy;

    updateEventSectionPresentation();
    updateSectionCollapseControls();

    QTimer::singleShot(
        0,
        this,
        [this, needsSettledCapacityRecheck]() {
            updateEventSectionPresentation();

            updateMinimumConstrainedHeight();

            updateSectionCollapseControlGeometry();

            if (needsSettledCapacityRecheck) {
                scheduleSectionCapacityUpdate();
            }
        }
        );
}

void InvestigationSessionView::
    rebuildConstrainedRestorePriorityFromCurrentState()
{
    /*
     * Normal restoration priority:
     *
     *   Events
     *   Lower Details
     *   Timeline
     *
     * The user's currently open sections move ahead of
     * currently closed sections, while preserving this
     * normal priority inside each group.
     *
     * Examples:
     *
     *   Timeline only:
     *     Timeline, Events, Lower
     *
     *   Lower + Timeline:
     *     Lower, Timeline, Events
     *
     *   Events + Timeline:
     *     Events, Timeline, Lower
     *
     *   all open / all closed:
     *     Events, Lower, Timeline
     */
    const QList<InvestigationSection>
        normalPriority{
            InvestigationSection::Events,
            InvestigationSection::LowerDetails,
            InvestigationSection::Timeline
        };

    QList<InvestigationSection>
        restorePriority;

    /*
     * Previously/currently open sections first.
     */
    for (
        const InvestigationSection section
        : normalPriority
        ) {
        if (!isSectionCollapsed(section)) {
            restorePriority.append(
                section
                );
        }
    }

    /*
     * Then sections that were already closed.
     */
    for (
        const InvestigationSection section
        : normalPriority
        ) {
        if (isSectionCollapsed(section)) {
            restorePriority.append(
                section
                );
        }
    }

    m_constrainedRestorePriority =
        restorePriority;
}

void InvestigationSessionView::
    captureConstrainedRestorePriority()
{
    if (m_hasConstrainedRestoreSnapshot) {
        return;
    }

    ensureSectionPreferredHeightsInitialized();

    rebuildConstrainedRestorePriorityFromCurrentState();

    /*
     * Freeze the user's preferred section heights at
     * the beginning of this constrained resize cycle.
     *
     * The ordinary preference fields remain responsible
     * for normal manual splitter behavior. These frozen
     * values exist only so mechanical redistribution
     * during native window resizing cannot move the
     * recovery targets for the active constrained cycle.
     */
    m_constrainedTimelinePreferredHeight =
        m_timelineExpandedHeight;

    m_constrainedEventPreferredHeight =
        m_eventExpandedHeight;

    m_constrainedLowerPreferredHeight =
        m_lowerRegionExpandedHeight;

    m_hasConstrainedRestoreSnapshot =
        true;

    m_constrainedSectionCollapsedDuringCycle =
        false;
}

void InvestigationSessionView::
    updateConstrainedRecoveryCompletion()
{
    if (!m_hasConstrainedRestoreSnapshot) {
        return;
    }

    if (
        m_sectionCapacity
            != InvestigationSectionCapacity::Three
        || openSectionCount() != 3
        ) {
        return;
    }

    /*
     * Do not destroy the recovery snapshot merely
     * because the original preferred layout has been
     * recovered.
     *
     * We continue using the constrained allocator
     * while the window keeps growing so Timeline and
     * Lower Details stay at their preferred heights
     * and Events remains the elastic section.
     */
    if (
        sectionHasRecoveredPreferredHeight(
            InvestigationSection::Timeline
            )
        && sectionHasRecoveredPreferredHeight(
            InvestigationSection::Events
            )
        && sectionHasRecoveredPreferredHeight(
            InvestigationSection::LowerDetails
            )
        ) {
        m_constrainedRecoveryComplete =
            true;
    }
}

void InvestigationSessionView::
    resetConstrainedRestoreState()
{
    m_constrainedRestorePriority.clear();

    m_constrainedTimelinePreferredHeight =
        0;

    m_constrainedEventPreferredHeight =
        0;

    m_constrainedLowerPreferredHeight =
        0;

    m_hasConstrainedRestoreSnapshot =
        false;

    m_constrainedRecoveryComplete =
        false;

    m_constrainedSectionCollapsedDuringCycle =
        false;
}

bool InvestigationSessionView::
    sectionHasRecoveredPreferredHeight(
        InvestigationSection section
        ) const
{
    if (
        m_mainSplitter == nullptr
        || isSectionCollapsed(section)
        ) {
        return true;
    }

    const QList<int> sizes =
        m_mainSplitter->sizes();

    if (sizes.size() != 3) {
        return true;
    }

    int sectionIndex = -1;
    int preferredHeight = 0;

    switch (section) {
    case InvestigationSection::Timeline:
        sectionIndex = 0;

        preferredHeight =
            m_hasConstrainedRestoreSnapshot
                ? m_constrainedTimelinePreferredHeight
                : m_timelineExpandedHeight;
        break;

    case InvestigationSection::Events:
        sectionIndex = 1;

        preferredHeight =
            m_hasConstrainedRestoreSnapshot
                ? m_constrainedEventPreferredHeight
                : m_eventExpandedHeight;
        break;

    case InvestigationSection::LowerDetails:
        sectionIndex = 2;

        preferredHeight =
            m_hasConstrainedRestoreSnapshot
                ? m_constrainedLowerPreferredHeight
                : m_lowerRegionExpandedHeight;
        break;
    }

    if (
        sectionIndex < 0
        || preferredHeight <= 0
        ) {
        return true;
    }

    const int tolerance =
        std::max(
            1,
            InterfaceScale::pixels(
                2,
                this
                )
            );

    return sizes.at(sectionIndex)
               + tolerance
           >= preferredHeight;
}

bool InvestigationSessionView::
    canRestoreConstrainedSection(
        InvestigationSection candidate
        ) const
{
    if (
        m_mainSplitter == nullptr
        || !m_hasConstrainedRestoreSnapshot
        ) {
        return false;
    }

    const int handleSpace =
        std::max(
            0,
            m_mainSplitter->count() - 1
            )
        * m_mainSplitter->handleWidth();

    int requiredHeight =
        handleSpace;

    for (
        const InvestigationSection section
        : {
            InvestigationSection::Timeline,
            InvestigationSection::Events,
            InvestigationSection::LowerDetails
        }
        ) {
        /*
         * The section we are considering opening only
         * needs its useful minimum initially.
         */
        if (section == candidate) {
            requiredHeight +=
                constrainedRecoveryMinimumHeight(
                    section
                    );

            continue;
        }

        if (isSectionCollapsed(section)) {
            requiredHeight +=
                sectionCompactHeight(
                    section
                    );

            continue;
        }

        if (section == InvestigationSection::Events) {
            /*
             * Events is the elastic section.
             *
             * Opening another section may reduce Events
             * toward its useful minimum, but must never
             * force a restored auxiliary section below
             * its frozen preferred height.
             */
            requiredHeight +=
                constrainedRecoveryMinimumHeight(
                    InvestigationSection::Events
                    );

            continue;
        }

        int preferredHeight = 0;

        switch (section) {
        case InvestigationSection::Timeline:
            preferredHeight =
                m_constrainedTimelinePreferredHeight;
            break;

        case InvestigationSection::LowerDetails:
            preferredHeight =
                m_constrainedLowerPreferredHeight;
            break;

        case InvestigationSection::Events:
            break;
        }

        requiredHeight +=
            std::max(
                constrainedRecoveryMinimumHeight(
                    section
                    ),
                preferredHeight
                );
    }

    return m_mainSplitter->height()
           >= requiredHeight;
}

void InvestigationSessionView::
    applyConstrainedRecoveryLayout()
{
    if (
        m_mainSplitter == nullptr
        || !m_hasConstrainedRestoreSnapshot
        ) {
        return;
    }

    /*
     * ---------------------------------------------------------
     * Partial resize/reverse
     * ---------------------------------------------------------
     *
     * If no section has actually been collapsed during this
     * resize cycle, do not reconstruct the preferred layout.
     *
     * QSplitter already provides the correct gradual growth
     * as the window becomes larger again. We only need to
     * stop the auxiliary sections once they reach their
     * frozen preferred heights.
     *
     * Any pixels Qt attempts to give Timeline or Lower
     * Details beyond those preferences are redirected to
     * Events, which remains the elastic section.
     */
    if (
        !m_constrainedSectionCollapsedDuringCycle
        && !m_timelineCollapsed
        && !m_eventCollapsed
        && !m_lowerRegionCollapsed
        ) {
        QList<int> targetSizes =
            m_mainSplitter->sizes();

        if (targetSizes.size() != 3) {
            return;
        }

        int excessHeight = 0;

        if (
            m_constrainedTimelinePreferredHeight > 0
            && targetSizes.at(0)
                   > m_constrainedTimelinePreferredHeight
            ) {
            excessHeight +=
                targetSizes.at(0)
                - m_constrainedTimelinePreferredHeight;

            targetSizes[0] =
                m_constrainedTimelinePreferredHeight;
        }

        if (
            m_constrainedLowerPreferredHeight > 0
            && targetSizes.at(2)
                   > m_constrainedLowerPreferredHeight
            ) {
            excessHeight +=
                targetSizes.at(2)
                - m_constrainedLowerPreferredHeight;

            targetSizes[2] =
                m_constrainedLowerPreferredHeight;
        }

        /*
         * If neither auxiliary has exceeded its preferred
         * height yet, leave Qt's current splitter geometry
         * completely alone. This is what gives recovery its
         * gradual visual behavior.
         */
        if (excessHeight <= 0) {
            return;
        }

        targetSizes[1] +=
            excessHeight;

        applyMainSplitterSizes(
            targetSizes
            );

        return;
    }

    const int handleSpace =
        std::max(
            0,
            m_mainSplitter->count() - 1
            )
        * m_mainSplitter->handleWidth();

    const int childBudget =
        std::max(
            0,
            m_mainSplitter->height()
                - handleSpace
            );

    QList<int> targetSizes{
        0,
        0,
        0
    };

    const auto indexForSection =
        [](
            InvestigationSection section
            ) {
            switch (section) {
            case InvestigationSection::Timeline:
                return 0;

            case InvestigationSection::Events:
                return 1;

            case InvestigationSection::LowerDetails:
                return 2;
            }

            return -1;
        };

    /*
     * ---------------------------------------------------------
     * Base allocation
     * ---------------------------------------------------------
     *
     * Every collapsed section gets compact chrome.
     * Every open section gets its useful minimum.
     */
    for (
        const InvestigationSection section
        : {
            InvestigationSection::Timeline,
            InvestigationSection::Events,
            InvestigationSection::LowerDetails
        }
        ) {
        const int index =
            indexForSection(section);

        targetSizes[index] =
            isSectionCollapsed(section)
                ? sectionCompactHeight(section)
                : constrainedRecoveryMinimumHeight(
                      section
                      );
    }

    int usedHeight =
        targetSizes.at(0)
        + targetSizes.at(1)
        + targetSizes.at(2);

    if (usedHeight > childBudget) {
        /*
         * The capacity coordinator owns the collapse
         * decision. Do not invent another compression
         * scheme here.
         */
        return;
    }

    int remainingHeight =
        childBudget
        - usedHeight;

    /*
     * ---------------------------------------------------------
     * Restore auxiliary preferred heights
     * ---------------------------------------------------------
     *
     * Timeline and Lower Details consume growth toward
     * their frozen preferred heights in the captured
     * restoration order.
     *
     * Events deliberately does NOT participate here.
     * It is the elastic remainder section.
     */
    for (
        const InvestigationSection section
        : std::as_const(
            m_constrainedRestorePriority
            )
        ) {
        if (
            remainingHeight <= 0
            || isSectionCollapsed(section)
            || section
                   == InvestigationSection::Events
            ) {
            continue;
        }

        const int index =
            indexForSection(section);

        int preferredHeight = 0;

        switch (section) {
        case InvestigationSection::Timeline:
            preferredHeight =
                m_constrainedTimelinePreferredHeight;
            break;

        case InvestigationSection::LowerDetails:
            preferredHeight =
                m_constrainedLowerPreferredHeight;
            break;

        case InvestigationSection::Events:
            break;
        }

        const int desiredHeight =
            std::max(
                targetSizes.at(index),
                preferredHeight
                );

        const int amount =
            std::min(
                remainingHeight,
                desiredHeight
                    - targetSizes.at(index)
                );

        targetSizes[index] +=
            amount;

        remainingHeight -=
            amount;
    }

    /*
     * ---------------------------------------------------------
     * Events owns all remaining growth
     * ---------------------------------------------------------
     */
    if (
        remainingHeight > 0
        && !m_eventCollapsed
        ) {
        targetSizes[1] +=
            remainingHeight;

        remainingHeight = 0;
    }

    if (remainingHeight > 0) {
        /*
         * Events is still closed, so some temporary
         * surplus has to live in an already-open section
         * until the next section becomes admissible.
         *
         * Give that surplus to the highest-priority
         * currently open section in the captured restore
         * order.
         *
         * Example:
         *
         *   restore order:
         *     Details, Timeline, Events
         *
         * Once Details and Timeline have both recovered
         * their preferred heights, additional space stays
         * with Details until Events can open.
         *
         * The surplus is temporary. When Events becomes
         * admissible, the recovery allocator returns the
         * auxiliary sections to their frozen preferred
         * heights and gives Events the remaining space.
         */
        for (
            const InvestigationSection section
            : std::as_const(
                m_constrainedRestorePriority
                )
            ) {
            if (!isSectionCollapsed(section)) {
                const int index =
                    indexForSection(section);

                targetSizes[index] +=
                    remainingHeight;

                break;
            }
        }
    }

    applyMainSplitterSizes(
        targetSizes
        );
}