#include "InvestigationSessionView.h"

#include <algorithm>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextOption>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QSizePolicy>

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
#include "../../models/InvestigationFilterProxyModel.h"
#include "../../preferences/FilterPresetStore.h"
#include "../../workspace/InvestigationSession.h"
#include "../../workspace/InvestigationStateStore.h"

namespace
{
QString documentIdFor(
    const InvestigationSession *session
    )
{
    return session != nullptr
               ? session->id()
               : QString();
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
        6,
        4,
        6,
        6
        );

    layout->setSpacing(
        4
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

    m_eventDetailPanel->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Preferred
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
        m_eventPanel
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

    layout->addWidget(
        m_mainSplitter,
        1
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

    refreshSession();
}

InvestigationSession *
InvestigationSessionView::session() const
{
    return m_session;
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

    m_filterPanel->applyToSession();

    const int selectedProxyRow =
        !selectedRecordId.isEmpty()
            ? controller
                  ->proxyRowForRecordId(
                      selectedRecordId
                      )
            : -1;

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

    m_session
        ->investigationStateStore()
        ->setFindingStatus(
            record->recordId,
            status
            );

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

    syncInvestigationStatePresentation();

    if (m_filterPanel->bookmarksOnly()) {
        applyFilters();
    } else {
        updateInvestigationStateControls();
    }
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
}

void InvestigationSessionView::
    updateFindingsPanel()
{
    m_findingsPanel->refresh();
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
        520,
        300
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

            m_session
                ->investigationStateStore()
                ->setNote(
                    recordId,
                    note.trimmed().isEmpty()
                        ? QString()
                        : note
                    );

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
    if (m_bottomSplitter == nullptr) {
        return;
    }

    const int totalWidth =
        std::max(
            1,
            m_bottomSplitter->width()
            );

    const bool wideReviewSelected =
        tab
            == InvestigationReviewTab::Findings
        || tab
               == InvestigationReviewTab::Analytics;

    if (wideReviewSelected) {
        /*
         * Findings and Analytics carry more
         * investigation-oriented information than
         * Selected Event Details and need additional
         * room in narrow detached workspaces.
         */
        double reviewFraction =
            0.60;

        if (totalWidth < 750) {
            reviewFraction =
                0.72;
        } else if (totalWidth < 1100) {
            reviewFraction =
                0.68;
        }

        const int reviewWidth =
            static_cast<int>(
                totalWidth
                * reviewFraction
                );

        m_bottomSplitter->setSizes({
            reviewWidth,
            std::max(
                1,
                totalWidth - reviewWidth
                )
        });

        return;
    }

    /*
     * Issue Summary remains deliberately compact
     * because its table supports horizontal
     * scrolling when necessary.
     */
    const int issueWidth =
        std::max(
            m_issueSummaryPanel
                ->preferredCompactWidth(),
            static_cast<int>(
                totalWidth * 0.35
                )
            );

    m_bottomSplitter->setSizes({
        issueWidth,
        std::max(
            1,
            totalWidth - issueWidth
            )
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

    if (m_reviewPanel == nullptr) {
        return;
    }

    updateReviewSplitter(
        m_reviewPanel->currentTab()
        );
}