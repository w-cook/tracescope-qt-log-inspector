#include "InvestigationAnalyticsPanel.h"

#include <algorithm>
#include <cmath>

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScrollBar>

#include "../../analysis/BurstDetectionSettings.h"
#include "../../analysis/InvestigationCadence.h"
#include "../../analysis/InvestigationValueFrequency.h"
#include "../../domain/RecordSeverity.h"
#include "../../workspace/InvestigationSession.h"

namespace
{

constexpr int AnalyticsTopEntityCount = 10;

QString formatDurationMilliseconds(
    qint64 milliseconds
    )
{
    if (milliseconds < 1000) {
        return QStringLiteral("%1 ms")
        .arg(milliseconds);
    }

    if (milliseconds < 60 * 1000) {
        const double seconds =
            static_cast<double>(
                milliseconds
                )
            / 1000.0;

        return QStringLiteral("%1 s")
            .arg(
                seconds,
                0,
                'f',
                seconds < 10.0
                    ? 1
                    : 0
                );
    }

    if (milliseconds
        < 60 * 60 * 1000) {
        const double minutes =
            static_cast<double>(
                milliseconds
                )
            / static_cast<double>(
                60 * 1000
                );

        return QStringLiteral("%1 min")
            .arg(
                minutes,
                0,
                'f',
                minutes < 10.0
                    ? 1
                    : 0
                );
    }

    const double hours =
        static_cast<double>(
            milliseconds
            )
        / static_cast<double>(
            60 * 60 * 1000
            );

    return QStringLiteral("%1 h")
        .arg(
            hours,
            0,
            'f',
            hours < 10.0
                ? 1
                : 0
            );
}

InvestigationTablePresentationState
captureTablePresentationState(
    const QTableWidget *table
    )
{
    InvestigationTablePresentationState
        state;

    if (table == nullptr) {
        return state;
    }

    const QModelIndex currentIndex =
        table->currentIndex();

    if (currentIndex.isValid()) {
        state.currentRow =
            currentIndex.row();

        state.currentColumn =
            currentIndex.column();
    }

    if (const QScrollBar *horizontal =
        table->horizontalScrollBar();
        horizontal != nullptr) {
        state.scroll.horizontalValue =
            horizontal->value();
    }

    if (const QScrollBar *vertical =
        table->verticalScrollBar();
        vertical != nullptr) {
        state.scroll.verticalValue =
            vertical->value();
    }

    return state;
}

void restoreTablePresentationState(
    QTableWidget *table,
    const InvestigationTablePresentationState &state
    )
{
    if (table == nullptr) {
        return;
    }

    table->clearSelection();

    if (state.currentRow >= 0
        && state.currentRow < table->rowCount()
        && state.currentColumn >= 0
        && state.currentColumn
               < table->columnCount()) {
        table->setCurrentCell(
            state.currentRow,
            state.currentColumn
            );
    }

    if (QScrollBar *horizontal =
        table->horizontalScrollBar();
        horizontal != nullptr) {
        horizontal->setValue(
            std::clamp(
                state.scroll.horizontalValue,
                horizontal->minimum(),
                horizontal->maximum()
                )
            );
    }

    if (QScrollBar *vertical =
        table->verticalScrollBar();
        vertical != nullptr) {
        vertical->setValue(
            std::clamp(
                state.scroll.verticalValue,
                vertical->minimum(),
                vertical->maximum()
                )
            );
    }
}
}

InvestigationAnalyticsPanel::
    InvestigationAnalyticsPanel(
        QWidget *parent
        )
    : QWidget(parent),
    m_tabs(
        new QTabWidget(this)
        ),
    m_overviewPage(
        new QWidget(m_tabs)
        ),
    m_burstsPage(
        new QWidget(m_tabs)
        )
{
    auto *panelLayout =
        new QVBoxLayout(this);

    panelLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    panelLayout->setSpacing(
        0
        );

    m_tabs->setDocumentMode(
        true
        );

    /*
     * ---------------------------------------------------------
     * Overview
     * ---------------------------------------------------------
     */

    auto *overviewLayout =
        new QVBoxLayout(
            m_overviewPage
            );

    overviewLayout->setContentsMargins(
        4,
        2,
        4,
        4
        );

    overviewLayout->setSpacing(
        2
        );

    m_overviewEmptyLabel =
        new QLabel(
            tr(
                "Event-code and entity analytics are "
                "not available for this source."
                ),
            m_overviewPage
            );

    m_overviewEmptyLabel->setWordWrap(
        true
        );

    m_overviewEmptyLabel->setVisible(
        false
        );

    overviewLayout->addWidget(
        m_overviewEmptyLabel
        );

    m_overviewSplitter =
        new QSplitter(
            Qt::Horizontal,
            m_overviewPage
            );

    /*
     * Event-code frequencies.
     */

    m_eventCodeGroup =
        new QGroupBox(
            tr("Event Code Frequencies"),
            m_overviewSplitter
            );

    auto *eventCodeLayout =
        new QVBoxLayout(
            m_eventCodeGroup
            );

    m_eventCodeTable =
        new QTableWidget(
            0,
            2,
            m_eventCodeGroup
            );

    m_eventCodeTable
        ->setHorizontalHeaderLabels({
            tr("Event Code"),
            tr("Count")
        });

    m_eventCodeTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_eventCodeTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_eventCodeTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_eventCodeTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );

    m_eventCodeTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
            );

    m_eventCodeTable
        ->verticalHeader()
        ->setVisible(
            false
            );

    eventCodeLayout->addWidget(
        m_eventCodeTable
        );

    /*
     * Top entities.
     */

    m_entityGroup =
        new QGroupBox(
            tr("Top Entities"),
            m_overviewSplitter
            );

    auto *entityLayout =
        new QVBoxLayout(
            m_entityGroup
            );

    m_entityTable =
        new QTableWidget(
            0,
            2,
            m_entityGroup
            );

    m_entityTable
        ->setHorizontalHeaderLabels({
            tr("Entity"),
            tr("Count")
        });

    m_entityTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_entityTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_entityTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_entityTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );

    m_entityTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
            );

    m_entityTable
        ->verticalHeader()
        ->setVisible(
            false
            );

    entityLayout->addWidget(
        m_entityTable
        );

    m_overviewSplitter->addWidget(
        m_eventCodeGroup
        );

    m_overviewSplitter->addWidget(
        m_entityGroup
        );

    m_overviewSplitter->setStretchFactor(
        0,
        1
        );

    m_overviewSplitter->setStretchFactor(
        1,
        1
        );

    overviewLayout->addWidget(
        m_overviewSplitter
        );

    /*
     * ---------------------------------------------------------
     * Bursts
     * ---------------------------------------------------------
     */

    auto *burstsLayout =
        new QVBoxLayout(
            m_burstsPage
            );

    burstsLayout->setContentsMargins(
        4,
        2,
        4,
        4
        );

    burstsLayout->setSpacing(
        2
        );

    auto *burstToolbar =
        new QHBoxLayout();

    burstToolbar->setContentsMargins(
        0,
        0,
        0,
        0
        );

    burstToolbar->setSpacing(
        4
        );

    auto *burstHeading =
        new QLabel(
            tr(
                "Deterministic warning and error "
                "burst detection"
                ),
            m_burstsPage
            );

    m_burstSettingsButton =
        new QPushButton(
            tr("Burst Settings..."),
            m_burstsPage
            );

    connect(
        m_burstSettingsButton,
        &QPushButton::clicked,
        this,
        [this]() {
            showBurstSettingsDialog();
        }
        );

    burstToolbar->addWidget(
        burstHeading
        );

    burstToolbar->addStretch();

    burstToolbar->addWidget(
        m_burstSettingsButton
        );

    burstsLayout->addLayout(
        burstToolbar
        );

    m_burstSplitter =
        new QSplitter(
            Qt::Horizontal,
            m_burstsPage
            );

    /*
     * Detected burst list.
     */

    auto *burstListGroup =
        new QGroupBox(
            tr("Detected Bursts"),
            m_burstSplitter
            );

    auto *burstListLayout =
        new QVBoxLayout(
            burstListGroup
            );

    burstListLayout->setContentsMargins(
        4,
        4,
        4,
        4
        );

    burstListLayout->setSpacing(
        2
        );

    m_burstTable =
        new QTableWidget(
            0,
            4,
            burstListGroup
            );

    m_burstTable
        ->setHorizontalHeaderLabels({
            tr("Start"),
            tr("End"),
            tr("Elevated"),
            tr("Highest Severity")
        });

    m_burstTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_burstTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_burstTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_burstTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
            );

    m_burstTable
        ->horizontalHeader()
        ->setStretchLastSection(
            true
            );

    m_burstTable
        ->verticalHeader()
        ->setVisible(
            false
            );

    m_burstTable->setToolTip(
        tr(
            "Select a burst to review its explanation. "
            "Double-click to filter the investigation "
            "to its contributing elevated events."
            )
        );

    connect(
        m_burstTable,
        &QTableWidget::cellClicked,
        this,
        [this](
            int row,
            int
            ) {
            updateBurstDetail(
                row
                );
        }
        );

    connect(
        m_burstTable,
        &QTableWidget::cellDoubleClicked,
        this,
        [this](
            int row,
            int
            ) {
            requestBurstDrillDown(
                row
                );
        }
        );

    burstListLayout->addWidget(
        m_burstTable
        );

    /*
     * Burst explanation.
     */

    auto *burstDetailGroup =
        new QGroupBox(
            tr("Burst Explanation"),
            m_burstSplitter
            );

    auto *burstDetailLayout =
        new QVBoxLayout(
            burstDetailGroup
            );

    burstDetailLayout->setContentsMargins(
        4,
        4,
        4,
        4
        );

    burstDetailLayout->setSpacing(
        2
        );

    m_burstDetailText =
        new QPlainTextEdit(
            burstDetailGroup
            );

    m_burstDetailText->setReadOnly(
        true
        );

    m_burstDetailText->setPlaceholderText(
        tr(
            "Select a detected burst to review "
            "why it was identified."
            )
        );

    burstDetailLayout->addWidget(
        m_burstDetailText
        );

    m_burstSplitter->addWidget(
        burstListGroup
        );

    m_burstSplitter->addWidget(
        burstDetailGroup
        );

    m_burstSplitter->setStretchFactor(
        0,
        3
        );

    m_burstSplitter->setStretchFactor(
        1,
        2
        );

    burstsLayout->addWidget(
        m_burstSplitter
        );

    /*
     * ---------------------------------------------------------
     * Analytics tabs
     * ---------------------------------------------------------
     */

    m_tabs->addTab(
        m_overviewPage,
        tr("Overview")
        );

    m_tabs->addTab(
        m_burstsPage,
        tr("Bursts")
        );

    connect(
        m_tabs,
        &QTabWidget::currentChanged,
        this,
        [this](int index) {
            if (m_session == nullptr) {
                return;
            }

            QWidget *currentPage =
                m_tabs->widget(
                    index
                    );

            if (currentPage
                == m_overviewPage) {
                m_session->setAnalyticsTab(
                    InvestigationAnalyticsTab::
                    Overview
                    );
            } else if (
                currentPage
                == m_burstsPage
                ) {
                m_session->setAnalyticsTab(
                    InvestigationAnalyticsTab::
                    Bursts
                    );
            }
        }
        );

    panelLayout->addWidget(
        m_tabs
        );

    clear();
}

void InvestigationAnalyticsPanel::setSession(
    InvestigationSession *session
    )
{
    m_session =
        session;

    /*
     * Never retain filtered records from the
     * previously bound investigation.
     */
    m_records.clear();

    m_eventCodeTable->setRowCount(
        0
        );

    m_entityTable->setRowCount(
        0
        );

    m_burstTable->setRowCount(
        0
        );

    m_currentBursts.clear();

    m_burstDetailText->clear();

    if (m_session == nullptr) {
        m_eventCodeGroup->setVisible(
            false
            );

        m_entityGroup->setVisible(
            false
            );

        m_overviewEmptyLabel->setVisible(
            false
            );

        m_burstSettingsButton->setEnabled(
            false
            );

        return;
    }

    m_burstSettingsButton->setEnabled(
        true
        );

    restoreSelectedTab();
}

InvestigationSession *
InvestigationAnalyticsPanel::session() const
{
    return m_session;
}

void InvestigationAnalyticsPanel::updateRecords(
    const QVector<InvestigationRecord> &records
    )
{
    m_records =
        records;

    updateOverview();
    updateBursts();
}

void InvestigationAnalyticsPanel::clear()
{
    setSession(
        nullptr
        );
}

InvestigationAnalyticsPresentationState
    InvestigationAnalyticsPanel::
    capturePresentationState() const
{
    InvestigationAnalyticsPresentationState
        state;

    /*
     * Capture the actual visible Analytics page
     * rather than relying only on session state.
     */
    if (m_tabs != nullptr
        && m_tabs->currentWidget()
               == m_burstsPage) {
        state.selectedTab =
            InvestigationAnalyticsTab::Bursts;
    } else {
        state.selectedTab =
            InvestigationAnalyticsTab::Overview;
    }

    if (m_overviewSplitter != nullptr) {
        state.overviewSplitterSizes =
            m_overviewSplitter->sizes();
    }

    state.eventCodeTable =
        captureTablePresentationState(
            m_eventCodeTable
            );

    state.entityTable =
        captureTablePresentationState(
            m_entityTable
            );

    if (m_burstSplitter != nullptr) {
        state.burstSplitterSizes =
            m_burstSplitter->sizes();
    }

    state.burstTable =
        captureTablePresentationState(
            m_burstTable
            );

    /*
     * Preserve burst selection using its semantic
     * identity rather than relying only on a row
     * number.
     */
    if (m_burstTable != nullptr) {
        const int row =
            m_burstTable
                ->currentRow();

        if (row >= 0
            && row < m_currentBursts.size()) {
            const InvestigationBurst &burst =
                m_currentBursts.at(row);

            state.selectedBurstStartTimestamp =
                burst.startTimestamp;

            state.selectedBurstEndTimestamp =
                burst.endTimestamp;
        }
    }

    if (m_burstDetailText != nullptr) {
        if (const QScrollBar *horizontal =
            m_burstDetailText
                ->horizontalScrollBar();
            horizontal != nullptr) {
            state.burstDetailScroll
                .horizontalValue =
                horizontal->value();
        }

        if (const QScrollBar *vertical =
            m_burstDetailText
                ->verticalScrollBar();
            vertical != nullptr) {
            state.burstDetailScroll
                .verticalValue =
                vertical->value();
        }
    }

    return state;
}

void InvestigationAnalyticsPanel::
    restorePresentationState(
        const InvestigationAnalyticsPresentationState
            &state
        )
{
    /*
     * Preserve the existing InvestigationSession
     * ownership of the preferred Analytics tab.
     */
    if (m_session != nullptr) {
        m_session->setAnalyticsTab(
            state.selectedTab
            );

        restoreSelectedTab();
    }

    /*
     * QSplitter may normalize requested sizes based
     * on its effective geometry. That is expected;
     * Save captures the resulting effective sizes.
     */
    if (m_overviewSplitter != nullptr
        && state.overviewSplitterSizes.size()
               == m_overviewSplitter->count()) {
        m_overviewSplitter->setSizes(
            state.overviewSplitterSizes
            );
    }

    restoreTablePresentationState(
        m_eventCodeTable,
        state.eventCodeTable
        );

    restoreTablePresentationState(
        m_entityTable,
        state.entityTable
        );

    if (m_burstSplitter != nullptr
        && state.burstSplitterSizes.size()
               == m_burstSplitter->count()) {
        m_burstSplitter->setSizes(
            state.burstSplitterSizes
            );
    }

    /*
     * Restore the generic burst-table state first.
     * If the saved burst still exists, semantic
     * identity takes precedence over its old row.
     */
    restoreTablePresentationState(
        m_burstTable,
        state.burstTable
        );

    int selectedBurstRow =
        -1;

    if (state.selectedBurstStartTimestamp
            .has_value()
        && state.selectedBurstEndTimestamp
               .has_value()) {
        for (int row = 0;
             row < m_currentBursts.size();
             ++row) {
            const InvestigationBurst &burst =
                m_currentBursts.at(row);

            if (burst.startTimestamp
                    == state
                           .selectedBurstStartTimestamp
                           .value()
                && burst.endTimestamp
                       == state
                              .selectedBurstEndTimestamp
                              .value()) {
                selectedBurstRow =
                    row;

                break;
            }
        }
    }

    if (selectedBurstRow >= 0) {
        m_burstTable->selectRow(
            selectedBurstRow
            );

        updateBurstDetail(
            selectedBurstRow
            );
    } else if (m_burstTable != nullptr) {
        const int currentRow =
            m_burstTable->currentRow();

        if (currentRow >= 0
            && currentRow
                   < m_currentBursts.size()) {
            updateBurstDetail(
                currentRow
                );
        }
    }

    /*
     * updateBurstDetail() replaces the detail text
     * and therefore resets its viewport. Restore the
     * saved text position only afterward.
     */
    if (m_burstDetailText != nullptr) {
        if (QScrollBar *horizontal =
            m_burstDetailText
                ->horizontalScrollBar();
            horizontal != nullptr) {
            horizontal->setValue(
                std::clamp(
                    state.burstDetailScroll
                        .horizontalValue,
                    horizontal->minimum(),
                    horizontal->maximum()
                    )
                );
        }

        if (QScrollBar *vertical =
            m_burstDetailText
                ->verticalScrollBar();
            vertical != nullptr) {
            vertical->setValue(
                std::clamp(
                    state.burstDetailScroll
                        .verticalValue,
                    vertical->minimum(),
                    vertical->maximum()
                    )
                );
        }
    }
}

void InvestigationAnalyticsPanel::
    restoreSelectedTab()
{
    if (m_session == nullptr) {
        return;
    }

    QWidget *preferredPage =
        nullptr;

    switch (m_session->analyticsTab()) {
    case InvestigationAnalyticsTab::Overview:
        preferredPage =
            m_overviewPage;
        break;

    case InvestigationAnalyticsTab::Bursts:
        preferredPage =
            m_burstsPage;
        break;
    }

    if (preferredPage == nullptr) {
        return;
    }

    const int index =
        m_tabs->indexOf(
            preferredPage
            );

    if (index < 0) {
        return;
    }

    /*
     * Restoring presentation state should not be
     * interpreted as a new analyst interaction.
     */
    const QSignalBlocker blocker(
        m_tabs
        );

    m_tabs->setCurrentIndex(
        index
        );
}

void InvestigationAnalyticsPanel::
    updateOverview()
{
    m_eventCodeTable->setRowCount(
        0
        );

    m_entityTable->setRowCount(
        0
        );

    if (m_session == nullptr) {
        m_eventCodeGroup->setVisible(
            false
            );

        m_entityGroup->setVisible(
            false
            );

        m_overviewEmptyLabel->setVisible(
            false
            );

        return;
    }

    const bool hasEventCodeData =
        m_session->hasEventCodeData();

    const bool hasEntityData =
        m_session->hasEntityData();

    m_eventCodeGroup->setVisible(
        hasEventCodeData
        );

    m_entityGroup->setVisible(
        hasEntityData
        );

    m_overviewEmptyLabel->setVisible(
        !hasEventCodeData
        && !hasEntityData
        );

    if (hasEventCodeData) {
        const QVector<InvestigationValueFrequency>
            frequencies =
            m_analyticsAnalyzer
                .eventCodeFrequencies(
                    m_records
                    );

        m_eventCodeTable->setRowCount(
            frequencies.size()
            );

        for (
            int row = 0;
            row < frequencies.size();
            ++row
            ) {
            const InvestigationValueFrequency
                &frequency =
                frequencies.at(row);

            auto *valueItem =
                new QTableWidgetItem(
                    frequency.value
                    );

            auto *countItem =
                new QTableWidgetItem(
                    QString::number(
                        frequency.count
                        )
                    );

            countItem->setTextAlignment(
                Qt::AlignRight
                | Qt::AlignVCenter
                );

            m_eventCodeTable->setItem(
                row,
                0,
                valueItem
                );

            m_eventCodeTable->setItem(
                row,
                1,
                countItem
                );
        }
    }

    if (hasEntityData) {
        const QVector<InvestigationValueFrequency>
            frequencies =
            m_analyticsAnalyzer
                .entityFrequencies(
                    m_records
                    );

        const int displayedCount =
            std::min(
                AnalyticsTopEntityCount,
                static_cast<int>(
                    frequencies.size()
                    )
                );

        m_entityTable->setRowCount(
            displayedCount
            );

        for (
            int row = 0;
            row < displayedCount;
            ++row
            ) {
            const InvestigationValueFrequency
                &frequency =
                frequencies.at(row);

            auto *valueItem =
                new QTableWidgetItem(
                    frequency.value
                    );

            auto *countItem =
                new QTableWidgetItem(
                    QString::number(
                        frequency.count
                        )
                    );

            countItem->setTextAlignment(
                Qt::AlignRight
                | Qt::AlignVCenter
                );

            m_entityTable->setItem(
                row,
                0,
                valueItem
                );

            m_entityTable->setItem(
                row,
                1,
                countItem
                );
        }
    }
}

void InvestigationAnalyticsPanel::
    updateBursts()
{
    m_burstTable->setRowCount(
        0
        );

    m_currentBursts.clear();

    m_burstDetailText->clear();

    if (m_session == nullptr) {
        return;
    }

    /*
     * Burst detection depends on the source having
     * both timestamp and severity capability.
     */
    const bool hasTimestampData =
        m_session
            ->firstTimestamp()
            .has_value()
        && m_session
               ->lastTimestamp()
               .has_value();

    if (!hasTimestampData
        || !m_session->hasSeverityData()) {
        m_burstDetailText->setPlainText(
            tr(
                "Burst detection requires valid "
                "timestamps and severity values."
                )
            );

        return;
    }

    const InvestigationCadence cadence =
        m_cadenceAnalyzer.analyze(
            m_records
            );

    BurstDetectionSettings settings =
        m_session
            ->burstDetectionSettings();

    const bool automaticTiming =
        m_session->burstTimingMode()
        == InvestigationBurstTimingMode::
        Auto;

    if (automaticTiming) {
        settings.windowMilliseconds =
            cadence
                .recommendedBurstWindowMilliseconds;

        settings.mergeGapMilliseconds =
            cadence
                .recommendedMergeGapMilliseconds;
    }

    if (!settings.isValid()) {
        m_burstDetailText->setPlainText(
            tr(
                "The current burst-detection "
                "settings are invalid."
                )
            );

        return;
    }

    m_currentBursts =
        m_burstAnalyzer.detectBursts(
            m_records,
            settings
            );

    /*
     * An empty result is meaningful analytical
     * information, not an error.
     */
    if (m_currentBursts.isEmpty()) {
        QStringList lines;

        lines.append(
            tr(
                "No warning/error bursts were "
                "detected with the current settings."
                )
            );

        lines.append(
            QString()
            );

        lines.append(
            tr("Timing mode: %1")
                .arg(
                    automaticTiming
                        ? tr("Auto")
                        : tr("Manual")
                    )
            );

        lines.append(
            tr("Window: %1")
                .arg(
                    formatDurationMilliseconds(
                        settings
                            .windowMilliseconds
                        )
                    )
            );

        lines.append(
            tr("Merge gap: %1")
                .arg(
                    formatDurationMilliseconds(
                        settings
                            .mergeGapMilliseconds
                        )
                    )
            );

        lines.append(
            tr(
                "WARN/ERROR/CRITICAL threshold: %1"
                )
                .arg(
                    settings
                        .elevatedEventThreshold
                    )
            );

        lines.append(
            tr(
                "ERROR/CRITICAL threshold: %1"
                )
                .arg(
                    settings
                        .errorCriticalThreshold
                    )
            );

        if (automaticTiming) {
            lines.append(
                QString()
                );

            if (
                cadence
                    .usesFallbackRecommendation
                ) {
                lines.append(
                    tr(
                        "Auto timing is using the "
                        "fallback recommendation "
                        "because this investigation "
                        "does not contain enough "
                        "positive timestamp gaps for "
                        "an adaptive recommendation."
                        )
                    );
            } else {
                lines.append(
                    tr(
                        "Auto timing was derived from "
                        "the timestamp cadence of the "
                        "currently filtered "
                        "investigation."
                        )
                    );
            }

            lines.append(
                tr("Valid timestamps: %1")
                    .arg(
                        cadence.timestampCount
                        )
                );

            lines.append(
                tr("Positive gaps: %1")
                    .arg(
                        cadence.positiveGapCount
                        )
                );

            lines.append(
                tr("Zero gaps: %1")
                    .arg(
                        cadence.zeroGapCount
                        )
                );

            if (cadence.positiveGapCount > 0) {
                lines.append(
                    tr(
                        "Median positive gap: %1"
                        )
                        .arg(
                            formatDurationMilliseconds(
                                static_cast<qint64>(
                                    std::llround(
                                        cadence
                                            .medianPositiveGapMilliseconds
                                        )
                                    )
                                )
                            )
                    );

                lines.append(
                    tr(
                        "P90 positive gap: %1"
                        )
                        .arg(
                            formatDurationMilliseconds(
                                cadence
                                    .p90PositiveGapMilliseconds
                                )
                            )
                    );
            }
        }

        m_burstDetailText->setPlainText(
            lines.join(
                QStringLiteral("\n")
                )
            );

        return;
    }

    const bool spansMultipleDates =
        m_session
            ->firstTimestamp()
            .has_value()
        && m_session
               ->lastTimestamp()
               .has_value()
        && m_session
                   ->firstTimestamp()
                   ->date()
               != m_session
                      ->lastTimestamp()
                      ->date();

    const QString timestampFormat =
        spansMultipleDates
            ? QStringLiteral(
                  "MM-dd HH:mm:ss.zzz"
                  )
            : QStringLiteral(
                  "HH:mm:ss.zzz"
                  );

    m_burstTable->setRowCount(
        m_currentBursts.size()
        );

    for (
        int row = 0;
        row < m_currentBursts.size();
        ++row
        ) {
        const InvestigationBurst &burst =
            m_currentBursts.at(
                row
                );

        auto *startItem =
            new QTableWidgetItem(
                burst
                    .startTimestamp
                    .toString(
                        timestampFormat
                        )
                );

        auto *endItem =
            new QTableWidgetItem(
                burst
                    .endTimestamp
                    .toString(
                        timestampFormat
                        )
                );

        startItem->setToolTip(
            burst
                .startTimestamp
                .toString(
                    Qt::ISODateWithMs
                    )
            );

        endItem->setToolTip(
            burst
                .endTimestamp
                .toString(
                    Qt::ISODateWithMs
                    )
            );

        auto *elevatedItem =
            new QTableWidgetItem(
                QString::number(
                    burst
                        .totalElevatedCount()
                    )
                );

        auto *severityItem =
            new QTableWidgetItem(
                recordSeverityToString(
                    burst
                        .highestSeverity()
                    )
                );

        elevatedItem->setTextAlignment(
            Qt::AlignRight
            | Qt::AlignVCenter
            );

        m_burstTable->setItem(
            row,
            0,
            startItem
            );

        m_burstTable->setItem(
            row,
            1,
            endItem
            );

        m_burstTable->setItem(
            row,
            2,
            elevatedItem
            );

        m_burstTable->setItem(
            row,
            3,
            severityItem
            );
    }

    m_burstTable->selectRow(
        0
        );

    updateBurstDetail(
        0
        );
}

void InvestigationAnalyticsPanel::
    updateBurstDetail(
        int row
        )
{
    if (row < 0
        || row >= m_currentBursts.size()) {
        return;
    }

    const InvestigationBurst &burst =
        m_currentBursts.at(
            row
            );

    QStringList lines;

    lines.append(
        tr("Detected burst")
        );

    lines.append(
        QString()
        );

    lines.append(
        tr("Start: %1")
            .arg(
                burst
                    .startTimestamp
                    .toString(
                        Qt::ISODateWithMs
                        )
                )
        );

    lines.append(
        tr("End: %1")
            .arg(
                burst
                    .endTimestamp
                    .toString(
                        Qt::ISODateWithMs
                        )
                )
        );

    lines.append(
        tr("Duration: %1")
            .arg(
                formatDurationMilliseconds(
                    burst
                        .durationMilliseconds()
                    )
                )
        );

    lines.append(
        QString()
        );

    lines.append(
        tr("Elevated events: %1")
            .arg(
                burst
                    .totalElevatedCount()
                )
        );

    lines.append(
        tr("WARN: %1")
            .arg(
                burst.warningCount
                )
        );

    lines.append(
        tr("ERROR: %1")
            .arg(
                burst.errorCount
                )
        );

    lines.append(
        tr("CRITICAL: %1")
            .arg(
                burst.criticalCount
                )
        );

    lines.append(
        tr("Highest severity: %1")
            .arg(
                recordSeverityToString(
                    burst
                        .highestSeverity()
                    )
                )
        );

    lines.append(
        QString()
        );

    if (m_session != nullptr) {
        lines.append(
            tr("Timing mode: %1")
                .arg(
                    m_session
                                ->burstTimingMode()
                            == InvestigationBurstTimingMode::
                            Auto
                        ? tr("Auto")
                        : tr("Manual")
                    )
            );

        lines.append(
            QString()
            );
    }

    lines.append(
        tr("Why this was detected:")
        );

    if (
        burst
            .triggeredByElevatedThreshold
        ) {
        lines.append(
            tr(
                "• At least %1 WARN/ERROR/CRITICAL "
                "events occurred within a %2 window."
                )
                .arg(
                    burst
                        .settings
                        .elevatedEventThreshold
                    )
                .arg(
                    formatDurationMilliseconds(
                        burst
                            .settings
                            .windowMilliseconds
                        )
                    )
            );
    }

    if (
        burst
            .triggeredByErrorCriticalThreshold
        ) {
        lines.append(
            tr(
                "• At least %1 ERROR/CRITICAL events "
                "occurred within a %2 window."
                )
                .arg(
                    burst
                        .settings
                        .errorCriticalThreshold
                    )
                .arg(
                    formatDurationMilliseconds(
                        burst
                            .settings
                            .windowMilliseconds
                        )
                    )
            );
    }

    lines.append(
        QString()
        );

    lines.append(
        tr("Merge gap: %1")
            .arg(
                formatDurationMilliseconds(
                    burst
                        .settings
                        .mergeGapMilliseconds
                    )
                )
        );

    lines.append(
        tr(
            "Contributing elevated records: %1"
            )
            .arg(
                burst
                    .recordIds
                    .size()
                )
        );

    auto appendCounts =
        [&lines](
            const QString &heading,
            const QMap<QString, int> &counts
            ) {
            if (counts.isEmpty()) {
                return;
            }

            lines.append(
                QString()
                );

            lines.append(
                heading
                );

            for (
                auto iterator =
                counts.cbegin();
                iterator != counts.cend();
                ++iterator
                ) {
                lines.append(
                    QStringLiteral(
                        "• %1: %2"
                        )
                        .arg(
                            iterator.key()
                            )
                        .arg(
                            iterator.value()
                            )
                    );
            }
        };

    appendCounts(
        tr("Subsystems:"),
        burst.subsystemCounts
        );

    appendCounts(
        tr("Event codes:"),
        burst.eventCodeCounts
        );

    appendCounts(
        tr("Entities:"),
        burst.entityCounts
        );

    m_burstDetailText->setPlainText(
        lines.join(
            QStringLiteral("\n")
            )
        );
}

void InvestigationAnalyticsPanel::
    showBurstSettingsDialog()
{
    if (m_session == nullptr) {
        return;
    }

    const InvestigationCadence cadence =
        m_cadenceAnalyzer.analyze(
            m_records
            );

    const BurstDetectionSettings currentSettings =
        m_session
            ->burstDetectionSettings();

    QDialog dialog(this);

    dialog.setWindowTitle(
        tr("Burst Detection Settings")
        );

    auto *layout =
        new QVBoxLayout(
            &dialog
            );

    layout->setSpacing(
        8
        );

    /*
     * Timing.
     */

    auto *timingGroup =
        new QGroupBox(
            tr("Timing"),
            &dialog
            );

    auto *timingLayout =
        new QVBoxLayout(
            timingGroup
            );

    auto *autoRadio =
        new QRadioButton(
            tr("Auto"),
            timingGroup
            );

    auto *manualRadio =
        new QRadioButton(
            tr("Manual"),
            timingGroup
            );

    const bool automatic =
        m_session->burstTimingMode()
        == InvestigationBurstTimingMode::
        Auto;

    autoRadio->setChecked(
        automatic
        );

    manualRadio->setChecked(
        !automatic
        );

    timingLayout->addWidget(
        autoRadio
        );

    auto *autoDescription =
        new QLabel(
            timingGroup
            );

    autoDescription->setWordWrap(
        true
        );

    QString autoText =
        tr(
            "Recommended window: %1\n"
            "Recommended merge gap: %2\n"
            "Valid timestamps: %3    "
            "Positive gaps: %4    "
            "Zero gaps: %5"
            )
            .arg(
                formatDurationMilliseconds(
                    cadence
                        .recommendedBurstWindowMilliseconds
                    ),
                formatDurationMilliseconds(
                    cadence
                        .recommendedMergeGapMilliseconds
                    ),
                QString::number(
                    cadence.timestampCount
                    ),
                QString::number(
                    cadence.positiveGapCount
                    ),
                QString::number(
                    cadence.zeroGapCount
                    )
                );

    if (cadence.positiveGapCount > 0) {
        autoText +=
            tr(
                "\nMedian gap: %1    "
                "Mean gap: %2    "
                "P90 gap: %3"
                )
                .arg(
                    formatDurationMilliseconds(
                        static_cast<qint64>(
                            std::llround(
                                cadence
                                    .medianPositiveGapMilliseconds
                                )
                            )
                        ),
                    formatDurationMilliseconds(
                        static_cast<qint64>(
                            std::llround(
                                cadence
                                    .meanPositiveGapMilliseconds
                                )
                            )
                        ),
                    formatDurationMilliseconds(
                        cadence
                            .p90PositiveGapMilliseconds
                        )
                    );
    }

    if (
        cadence
            .usesFallbackRecommendation
        ) {
        autoText +=
            tr(
                "\n\nFallback timing is being used "
                "because this investigation does not "
                "contain enough positive timestamp gaps "
                "for an adaptive recommendation."
                );
    } else {
        autoText +=
            tr(
                "\n\nAuto derives timing from the "
                "timestamp cadence of the currently "
                "filtered investigation."
                );
    }

    autoDescription->setText(
        autoText
        );

    timingLayout->addWidget(
        autoDescription
        );

    timingLayout->addWidget(
        manualRadio
        );

    /*
     * Manual timing.
     */

    auto *manualWidget =
        new QWidget(
            timingGroup
            );

    auto *manualLayout =
        new QFormLayout(
            manualWidget
            );

    manualLayout->setContentsMargins(
        20,
        0,
        0,
        0
        );

    auto *windowSpin =
        new QDoubleSpinBox(
            manualWidget
            );

    windowSpin->setDecimals(
        3
        );

    windowSpin->setRange(
        0.001,
        7.0 * 24.0 * 60.0 * 60.0
        );

    windowSpin->setSuffix(
        tr(" s")
        );

    windowSpin->setValue(
        static_cast<double>(
            currentSettings
                .windowMilliseconds
            )
        / 1000.0
        );

    auto *mergeGapSpin =
        new QDoubleSpinBox(
            manualWidget
            );

    mergeGapSpin->setDecimals(
        3
        );

    mergeGapSpin->setRange(
        0.0,
        7.0 * 24.0 * 60.0 * 60.0
        );

    mergeGapSpin->setSuffix(
        tr(" s")
        );

    mergeGapSpin->setValue(
        static_cast<double>(
            currentSettings
                .mergeGapMilliseconds
            )
        / 1000.0
        );

    manualLayout->addRow(
        tr("Window:"),
        windowSpin
        );

    manualLayout->addRow(
        tr("Merge gap:"),
        mergeGapSpin
        );

    manualWidget->setEnabled(
        !automatic
        );

    timingLayout->addWidget(
        manualWidget
        );

    layout->addWidget(
        timingGroup
        );

    /*
     * Detection thresholds.
     */

    auto *thresholdGroup =
        new QGroupBox(
            tr("Detection Thresholds"),
            &dialog
            );

    auto *thresholdLayout =
        new QFormLayout(
            thresholdGroup
            );

    auto *elevatedSpin =
        new QSpinBox(
            thresholdGroup
            );

    elevatedSpin->setRange(
        1,
        1000000
        );

    elevatedSpin->setValue(
        currentSettings
            .elevatedEventThreshold
        );

    auto *errorCriticalSpin =
        new QSpinBox(
            thresholdGroup
            );

    errorCriticalSpin->setRange(
        1,
        1000000
        );

    errorCriticalSpin->setValue(
        currentSettings
            .errorCriticalThreshold
        );

    thresholdLayout->addRow(
        tr(
            "WARN/ERROR/CRITICAL events:"
            ),
        elevatedSpin
        );

    thresholdLayout->addRow(
        tr(
            "ERROR/CRITICAL events:"
            ),
        errorCriticalSpin
        );

    auto *thresholdDescription =
        new QLabel(
            tr(
                "A burst is detected when either "
                "threshold is met within the selected "
                "time window."
                ),
            thresholdGroup
            );

    thresholdDescription->setWordWrap(
        true
        );

    thresholdLayout->addRow(
        thresholdDescription
        );

    layout->addWidget(
        thresholdGroup
        );

    connect(
        autoRadio,
        &QRadioButton::toggled,
        &dialog,
        [manualWidget](bool checked) {
            manualWidget->setEnabled(
                !checked
                );
        }
        );

    auto *buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel,
            &dialog
            );

    connect(
        buttons,
        &QDialogButtonBox::accepted,
        &dialog,
        &QDialog::accept
        );

    connect(
        buttons,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject
        );

    layout->addWidget(
        buttons
        );

    if (
        dialog.exec()
        != QDialog::Accepted
        ) {
        return;
    }

    BurstDetectionSettings newSettings =
        currentSettings;

    newSettings.elevatedEventThreshold =
        elevatedSpin->value();

    newSettings.errorCriticalThreshold =
        errorCriticalSpin->value();

    if (manualRadio->isChecked()) {
        newSettings.windowMilliseconds =
            std::max<qint64>(
                1,
                static_cast<qint64>(
                    std::llround(
                        windowSpin->value()
                        * 1000.0
                        )
                    )
                );

        newSettings.mergeGapMilliseconds =
            std::max<qint64>(
                0,
                static_cast<qint64>(
                    std::llround(
                        mergeGapSpin->value()
                        * 1000.0
                        )
                    )
                );

        m_session->setBurstTimingMode(
            InvestigationBurstTimingMode::
            Manual
            );
    } else {
        /*
         * Preserve the last manual timing values.
         * Auto replaces them only for analysis.
         */
        m_session->setBurstTimingMode(
            InvestigationBurstTimingMode::
            Auto
            );
    }

    m_session->setBurstDetectionSettings(
        newSettings
        );

    updateBursts();
}

void InvestigationAnalyticsPanel::
    requestBurstDrillDown(
        int row
        )
{
    if (row < 0
        || row >= m_currentBursts.size()) {
        return;
    }

    const InvestigationBurst &burst =
        m_currentBursts.at(
            row
            );

    if (!burst.startTimestamp.isValid()
        || !burst.endTimestamp.isValid()) {
        return;
    }

    emit burstDrillDownRequested(
        burst.startTimestamp,
        burst.endTimestamp
        );
}