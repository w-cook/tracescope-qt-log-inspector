#include "InvestigationTimelinePanel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QGraphicsLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimeZone>
#include <QVariant>
#include <QVBoxLayout>
#include <QColor>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QValueAxis>

#include "../../models/InvestigationFilterProxyModel.h"
#include "../../workspace/InvestigationSession.h"

namespace
{

constexpr int TimelineAutoTargetBuckets =
    20;

constexpr int TimelineVisibleBucketCount =
    20;

constexpr int TimelineScaledScrollMaximum =
    1'000'000;

constexpr qint64 MillisecondsPerSecond =
    1'000;

constexpr qint64 MillisecondsPerMinute =
    60 * MillisecondsPerSecond;

constexpr qint64 MillisecondsPerHour =
    60 * MillisecondsPerMinute;

constexpr qint64 MillisecondsPerDay =
    24 * MillisecondsPerHour;

qint64 automaticTimelineIntervalMilliseconds(
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp
    )
{
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp) {
        return MillisecondsPerMinute;
    }

    const qint64 spanMilliseconds =
        std::max<qint64>(
            1,
            firstTimestamp.msecsTo(
                lastTimestamp
                )
                + 1
            );

    const QList<qint64> candidates {
        1,
        10,
        100,
        500,

        1 * MillisecondsPerSecond,
        5 * MillisecondsPerSecond,
        15 * MillisecondsPerSecond,
        30 * MillisecondsPerSecond,

        1 * MillisecondsPerMinute,
        5 * MillisecondsPerMinute,
        15 * MillisecondsPerMinute,
        30 * MillisecondsPerMinute,

        1 * MillisecondsPerHour,
        3 * MillisecondsPerHour,
        6 * MillisecondsPerHour,
        12 * MillisecondsPerHour,

        1 * MillisecondsPerDay,
        3 * MillisecondsPerDay,
        7 * MillisecondsPerDay
    };

    for (const qint64 intervalMilliseconds
         : candidates) {
        const qint64 bucketCount =
            (
                spanMilliseconds
                + intervalMilliseconds
                - 1
                )
            / intervalMilliseconds;

        if (bucketCount
            <= TimelineAutoTargetBuckets) {
            return intervalMilliseconds;
        }
    }

    /*
     * For unusually long investigations,
     * continue scaling in whole-day units.
     */
    const qint64 targetMilliseconds =
        (
            spanMilliseconds
            + TimelineAutoTargetBuckets
            - 1
            )
        / TimelineAutoTargetBuckets;

    const qint64 wholeDays =
        std::max<qint64>(
            1,
            (
                targetMilliseconds
                + MillisecondsPerDay
                - 1
                )
                / MillisecondsPerDay
            );

    return wholeDays
           * MillisecondsPerDay;
}

void configureEventCountAxis(
    QValueAxis *axis,
    int maxCount
    )
{
    const int effectiveMax =
        std::max(
            1,
            maxCount
            );

    axis->setLabelFormat(
        QStringLiteral("%d")
        );

    axis->setTruncateLabels(
        false
        );

    /*
     * Keep the Y range tied directly to the
     * observed maximum instead of allowing Qt to
     * expand it with applyNiceNumbers().
     */
    axis->setRange(
        0,
        effectiveMax
        );

    axis->setTickType(
        QValueAxis::TicksDynamic
        );

    axis->setTickAnchor(
        0
        );

    if (effectiveMax <= 10) {
        axis->setTickInterval(
            1
            );

        return;
    }

    const int tickInterval =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    static_cast<double>(
                        effectiveMax
                        )
                    / 5.0
                    )
                )
            );

    axis->setTickInterval(
        tickInterval
        );
}

int timelineScrollMaximum(
    qint64 totalBucketCount
    )
{
    const qint64 maximumStartBucketIndex =
        std::max<qint64>(
            0,
            totalBucketCount
                - TimelineVisibleBucketCount
            );

    if (maximumStartBucketIndex
        <= std::numeric_limits<int>::max()) {
        return static_cast<int>(
            maximumStartBucketIndex
            );
    }

    return TimelineScaledScrollMaximum;
}

qint64 timelineStartBucketIndex(
    qint64 totalBucketCount,
    int scrollValue
    )
{
    const qint64 maximumStartBucketIndex =
        std::max<qint64>(
            0,
            totalBucketCount
                - TimelineVisibleBucketCount
            );

    if (maximumStartBucketIndex <= 0) {
        return 0;
    }

    if (maximumStartBucketIndex
        <= std::numeric_limits<int>::max()) {
        return std::clamp<qint64>(
            scrollValue,
            0,
            maximumStartBucketIndex
            );
    }

    const int clampedScrollValue =
        std::clamp(
            scrollValue,
            0,
            TimelineScaledScrollMaximum
            );

    const long double fraction =
        static_cast<long double>(
            clampedScrollValue
            )
        / static_cast<long double>(
            TimelineScaledScrollMaximum
            );

    return std::clamp<qint64>(
        static_cast<qint64>(
            std::llround(
                fraction
                * static_cast<long double>(
                    maximumStartBucketIndex
                    )
                )
            ),
        0,
        maximumStartBucketIndex
        );
}

qint64 normalizedTimelineBucketEpoch(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds
    )
{
    const qint64 epochMilliseconds =
        timestamp.toMSecsSinceEpoch();

    qint64 remainder =
        epochMilliseconds
        % intervalMilliseconds;

    if (remainder < 0) {
        remainder +=
            intervalMilliseconds;
    }

    return epochMilliseconds
           - remainder;
}

QString timelineDisplayLabel(
    const QString &canonicalLabel,
    qint64 intervalMilliseconds
    )
{
    /*
     * Fine-resolution windows contain at most
     * twenty adjacent buckets, so the surrounding
     * visible-range label supplies full date/time
     * context.
     */
    if (intervalMilliseconds < 1000) {
        return canonicalLabel.right(
            6
            );
    }

    return canonicalLabel;
}

}

InvestigationTimelinePanel::
    InvestigationTimelinePanel(
        QWidget *parent
        )
    : QGroupBox(
          tr("Event Counts Over Time"),
          parent
          ),
    m_chartView(
        new QChartView(this)
        ),
    m_intervalCombo(
        new QComboBox(this)
        ),
    m_breakdownWidget(
        new QWidget(this)
        ),
    m_breakdownCombo(
        new QComboBox(
            m_breakdownWidget
            )
        ),
    m_subsystemShowWidget(
        new QWidget(this)
        ),
    m_subsystemLimitCombo(
        new QComboBox(
            m_subsystemShowWidget
            )
        ),
    m_scrollBar(
        new QScrollBar(
            Qt::Horizontal,
            this
            )
        ),
    m_rangeLabel(
        new QLabel(
            tr("Visible: —"),
            this
            )
        )
{
    auto *timelineLayout =
        new QVBoxLayout(this);

    timelineLayout->setContentsMargins(
        4,
        2,
        4,
        2
        );

    timelineLayout->setSpacing(
        2
        );

    auto *controlsLayout =
        new QHBoxLayout();

    controlsLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    controlsLayout->setSpacing(
        6
        );

    /*
     * ---------------------------------------------------------
     * Bucket size
     * ---------------------------------------------------------
     */
    auto *intervalLabel =
        new QLabel(
            tr("Bucket size:"),
            this
            );

    m_intervalCombo->addItem(
        tr("Auto"),
        QVariant::fromValue<qint64>(
            0
            )
        );

    m_intervalCombo->addItem(
        tr("1 ms"),
        QVariant::fromValue<qint64>(
            1
            )
        );

    m_intervalCombo->addItem(
        tr("10 ms"),
        QVariant::fromValue<qint64>(
            10
            )
        );

    m_intervalCombo->addItem(
        tr("100 ms"),
        QVariant::fromValue<qint64>(
            100
            )
        );

    m_intervalCombo->addItem(
        tr("500 ms"),
        QVariant::fromValue<qint64>(
            500
            )
        );

    m_intervalCombo->addItem(
        tr("1 second"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerSecond
            )
        );

    m_intervalCombo->addItem(
        tr("5 seconds"),
        QVariant::fromValue<qint64>(
            5 * MillisecondsPerSecond
            )
        );

    m_intervalCombo->addItem(
        tr("15 seconds"),
        QVariant::fromValue<qint64>(
            15 * MillisecondsPerSecond
            )
        );

    m_intervalCombo->addItem(
        tr("30 seconds"),
        QVariant::fromValue<qint64>(
            30 * MillisecondsPerSecond
            )
        );

    m_intervalCombo->addItem(
        tr("1 minute"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerMinute
            )
        );

    m_intervalCombo->addItem(
        tr("5 minutes"),
        QVariant::fromValue<qint64>(
            5 * MillisecondsPerMinute
            )
        );

    m_intervalCombo->addItem(
        tr("15 minutes"),
        QVariant::fromValue<qint64>(
            15 * MillisecondsPerMinute
            )
        );

    m_intervalCombo->addItem(
        tr("30 minutes"),
        QVariant::fromValue<qint64>(
            30 * MillisecondsPerMinute
            )
        );

    m_intervalCombo->addItem(
        tr("1 hour"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerHour
            )
        );

    m_intervalCombo->addItem(
        tr("3 hours"),
        QVariant::fromValue<qint64>(
            3 * MillisecondsPerHour
            )
        );

    m_intervalCombo->addItem(
        tr("6 hours"),
        QVariant::fromValue<qint64>(
            6 * MillisecondsPerHour
            )
        );

    m_intervalCombo->addItem(
        tr("1 day"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerDay
            )
        );

    int intervalPopupWidth = 0;

    for (int index = 0;
         index < m_intervalCombo->count();
         ++index) {
        intervalPopupWidth =
            std::max(
                intervalPopupWidth,
                m_intervalCombo
                    ->fontMetrics()
                    .horizontalAdvance(
                        m_intervalCombo
                            ->itemText(index)
                        )
                );
    }

    m_intervalCombo
        ->view()
        ->setMinimumWidth(
            intervalPopupWidth + 40
            );

    m_intervalCombo->setToolTip(
        tr(
            "Auto chooses a readable interval for "
            "the complete investigation. Selecting "
            "a specific interval preserves that "
            "resolution and lets you navigate "
            "through the timeline horizontally."
            )
        );

    controlsLayout->addWidget(
        intervalLabel
        );

    controlsLayout->addWidget(
        m_intervalCombo
        );

    controlsLayout->addSpacing(
        12
        );

    /*
     * ---------------------------------------------------------
     * Breakdown
     * ---------------------------------------------------------
     */
    auto *breakdownLayout =
        new QHBoxLayout(
            m_breakdownWidget
            );

    breakdownLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    breakdownLayout->setSpacing(
        4
        );

    auto *breakdownLabel =
        new QLabel(
            tr("Breakdown:"),
            m_breakdownWidget
            );

    m_breakdownCombo->setMinimumWidth(
        m_breakdownCombo
            ->fontMetrics()
            .horizontalAdvance(
                tr("Subsystem")
                )
        + 40
        );

    breakdownLayout->addWidget(
        breakdownLabel
        );

    breakdownLayout->addWidget(
        m_breakdownCombo
        );

    controlsLayout->addWidget(
        m_breakdownWidget
        );

    /*
     * ---------------------------------------------------------
     * Subsystem Top-N selection
     * ---------------------------------------------------------
     */
    auto *subsystemShowLayout =
        new QHBoxLayout(
            m_subsystemShowWidget
            );

    subsystemShowLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    subsystemShowLayout->setSpacing(
        4
        );

    auto *subsystemShowLabel =
        new QLabel(
            tr("Show:"),
            m_subsystemShowWidget
            );

    m_subsystemLimitCombo->addItem(
        tr("Top 5"),
        5
        );

    m_subsystemLimitCombo->addItem(
        tr("Top 10"),
        10
        );

    subsystemShowLayout->addWidget(
        subsystemShowLabel
        );

    subsystemShowLayout->addWidget(
        m_subsystemLimitCombo
        );

    controlsLayout->addWidget(
        m_subsystemShowWidget
        );

    m_subsystemShowWidget->setVisible(
        false
        );

    /*
     * ---------------------------------------------------------
     * Visible-range label
     * ---------------------------------------------------------
     */
    m_rangeLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        );

    m_rangeLabel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
        );

    controlsLayout->addSpacing(
        12
        );

    controlsLayout->addWidget(
        m_rangeLabel,
        1
        );

    timelineLayout->addLayout(
        controlsLayout
        );

    /*
     * ---------------------------------------------------------
     * Chart
     * ---------------------------------------------------------
     */
    m_chartView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Ignored
        );

    m_chartView->setMinimumHeight(
        0
        );

    m_chartView->setFrameShape(
        QFrame::NoFrame
        );

    m_chartView->setRenderHint(
        QPainter::Antialiasing
        );

    m_chartView
        ->setHorizontalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff
            );

    m_chartView
        ->setVerticalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff
            );

    m_chartView->setAcceptDrops(
        false
        );

    m_chartView->setToolTip(
        tr(
            "Double-click a bar to filter the "
            "investigation to that time bucket. "
            "Severity and subsystem breakdowns also "
            "filter to the selected series."
            )
        );

    m_chartView
        ->viewport()
        ->setAcceptDrops(
            false
            );

    timelineLayout->addWidget(
        m_chartView,
        1
        );

    /*
     * ---------------------------------------------------------
     * Fine-resolution horizontal navigation
     * ---------------------------------------------------------
     */
    m_scrollBar->setVisible(
        false
        );

    m_scrollBar->setSingleStep(
        1
        );

    /*
     * Fine-resolution windows may require scanning
     * a large record collection. Do not redraw
     * continuously while the thumb is being dragged.
     */
    m_scrollBar->setTracking(
        false
        );

    timelineLayout->addWidget(
        m_scrollBar
        );

    connect(
        m_scrollBar,
        &QScrollBar::valueChanged,
        this,
        [this](int) {
            /*
             * Do not invalidate the Y-axis cache.
             * Horizontal navigation must retain the
             * scale calculated for the complete
             * filtered investigation.
             */
            render();
        }
        );

    connect(
        m_scrollBar,
        &QScrollBar::sliderMoved,
        this,
        [this](int value) {
            updateRangeLabel(
                value
                );
        }
        );

    connect(
        m_intervalCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            m_scaleValid =
                false;

            {
                const QSignalBlocker blocker(
                    m_scrollBar
                    );

                m_scrollBar->setValue(
                    0
                    );
            }

            render();
        }
        );

    connect(
        m_breakdownCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            if (m_session == nullptr
                || !m_breakdownCombo
                        ->currentData()
                        .isValid()) {
                return;
            }

            const auto breakdown =
                static_cast<
                    InvestigationTimelineBreakdown>(
                    m_breakdownCombo
                        ->currentData()
                        .toInt()
                    );

            m_session->setTimelineBreakdown(
                breakdown
                );

            m_subsystemShowWidget->setVisible(
                breakdown
                == InvestigationTimelineBreakdown::
                Subsystem
                );

            m_scaleValid =
                false;

            render();
        }
        );

    connect(
        m_subsystemLimitCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            if (m_session == nullptr
                || !m_subsystemLimitCombo
                        ->currentData()
                        .isValid()) {
                return;
            }

            m_session->setSubsystemTrendLimit(
                m_subsystemLimitCombo
                    ->currentData()
                    .toInt()
                );

            m_scaleValid =
                false;

            render();
        }
        );

    rebuildBreakdownControls();
    showEmptyTimeline();
}

void InvestigationTimelinePanel::setSession(
    InvestigationSession *session
    )
{
    if (m_session == session) {
        rebuildBreakdownControls();
        return;
    }

    m_session =
        session;

    m_records.clear();

    m_scaleValid =
        false;

    m_scaleIntervalMilliseconds =
        0;

    m_scaleMaximum =
        1;

    {
        const QSignalBlocker blocker(
            m_scrollBar
            );

        m_scrollBar->setRange(
            0,
            0
            );

        m_scrollBar->setValue(
            0
            );
    }

    m_scrollBar->setVisible(
        false
        );

    rebuildBreakdownControls();

    if (m_session == nullptr) {
        showEmptyTimeline();
        return;
    }

    /*
     * Give the newly bound panel a coherent initial
     * representation immediately. applyFilters()
     * will shortly replace this with the same
     * recordsForAnalysis() state after all visible
     * filter controls have been restored.
     */
    m_records =
        m_session
            ->investigationController()
            ->recordsForAnalysis();

    render();
}

InvestigationSession *
InvestigationTimelinePanel::session() const
{
    return m_session;
}

void InvestigationTimelinePanel::updateRecords(
    const QVector<InvestigationRecord> &records
    )
{
    m_records =
        records;

    /*
     * A filter change can change the complete
     * investigation maximum even when bucket size
     * has not changed.
     */
    m_scaleValid =
        false;

    render();
}

void InvestigationTimelinePanel::clear()
{
    m_records.clear();

    m_scaleValid =
        false;

    showEmptyTimeline();
}

void InvestigationTimelinePanel::
    rebuildBreakdownControls()
{
    const QSignalBlocker breakdownBlocker(
        m_breakdownCombo
        );

    const QSignalBlocker limitBlocker(
        m_subsystemLimitCombo
        );

    m_breakdownCombo->clear();

    if (m_session == nullptr) {
        m_breakdownWidget->setVisible(
            false
            );

        m_subsystemShowWidget->setVisible(
            false
            );

        return;
    }

    /*
     * Add only dimensions supported by this
     * investigation.
     */
    if (m_session->hasSeverityData()) {
        m_breakdownCombo->addItem(
            tr("Severity"),
            static_cast<int>(
                InvestigationTimelineBreakdown::
                Severity
                )
            );
    }

    if (m_session->hasSubsystemData()) {
        m_breakdownCombo->addItem(
            tr("Subsystem"),
            static_cast<int>(
                InvestigationTimelineBreakdown::
                Subsystem
                )
            );
    }

    /*
     * With neither canonical dimension the chart
     * still renders its TOTAL series, but there is
     * no meaningful breakdown selector.
     */
    if (m_breakdownCombo->count() == 0) {
        m_breakdownWidget->setVisible(
            false
            );

        m_subsystemShowWidget->setVisible(
            false
            );

        return;
    }

    m_breakdownWidget->setVisible(
        true
        );

    int breakdownIndex =
        m_breakdownCombo->findData(
            static_cast<int>(
                m_session->timelineBreakdown()
                )
            );

    if (breakdownIndex < 0) {
        breakdownIndex =
            0;
    }

    m_breakdownCombo->setCurrentIndex(
        breakdownIndex
        );

    const auto effectiveBreakdown =
        static_cast<
            InvestigationTimelineBreakdown>(
            m_breakdownCombo
                ->currentData()
                .toInt()
            );

    /*
     * Persist a deterministic fallback when the
     * session's previous breakdown is unavailable
     * for this source.
     */
    m_session->setTimelineBreakdown(
        effectiveBreakdown
        );

    int limitIndex =
        m_subsystemLimitCombo->findData(
            m_session->subsystemTrendLimit()
            );

    if (limitIndex < 0) {
        limitIndex =
            m_subsystemLimitCombo
                ->findData(5);
    }

    m_subsystemLimitCombo->setCurrentIndex(
        limitIndex
        );

    m_session->setSubsystemTrendLimit(
        m_subsystemLimitCombo
            ->currentData()
            .toInt()
        );

    m_subsystemShowWidget->setVisible(
        effectiveBreakdown
        == InvestigationTimelineBreakdown::
        Subsystem
        );
}

void InvestigationTimelinePanel::
    showEmptyTimeline()
{
    {
        const QSignalBlocker blocker(
            m_scrollBar
            );

        m_scrollBar->setRange(
            0,
            0
            );

        m_scrollBar->setValue(
            0
            );
    }

    m_scrollBar->setVisible(
        false
        );

    m_rangeLabel->setText(
        tr("Visible: —")
        );

    auto *chart =
        new QChart();

    chart->setMargins(
        QMargins(
            0,
            0,
            0,
            0
            )
        );

    chart->setTitle(
        tr("No events to display")
        );

    m_chartView->setChart(
        chart
        );
}

std::optional<QDateTime>
    InvestigationTimelinePanel::
    effectiveFirstTimestamp() const
{
    if (m_session == nullptr
        || !m_session
                ->firstTimestamp()
                .has_value()) {
        return std::nullopt;
    }

    QDateTime effective =
        m_session
            ->firstTimestamp()
            .value();

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return effective;
    }

    const std::optional<QDateTime>
        &filterStart =
        controller
            ->proxyModel()
            ->timeRangeStart();

    if (filterStart.has_value()
        && filterStart.value()
               > effective) {
        effective =
            filterStart.value();
    }

    return effective;
}

std::optional<QDateTime>
    InvestigationTimelinePanel::
    effectiveLastTimestamp() const
{
    if (m_session == nullptr
        || !m_session
                ->lastTimestamp()
                .has_value()) {
        return std::nullopt;
    }

    QDateTime effective =
        m_session
            ->lastTimestamp()
            .value();

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return effective;
    }

    const std::optional<QDateTime>
        &filterEnd =
        controller
            ->proxyModel()
            ->timeRangeEnd();

    if (filterEnd.has_value()
        && filterEnd.value()
               < effective) {
        effective =
            filterEnd.value();
    }

    return effective;
}

void InvestigationTimelinePanel::render()
{
    if (m_session == nullptr) {
        showEmptyTimeline();
        return;
    }

    const std::optional<QDateTime>
        firstTimestamp =
        effectiveFirstTimestamp();

    const std::optional<QDateTime>
        lastTimestamp =
        effectiveLastTimestamp();

    if (!firstTimestamp.has_value()
        || !lastTimestamp.has_value()
        || firstTimestamp.value()
               > lastTimestamp.value()) {
        showEmptyTimeline();
        return;
    }

    const qint64 requestedIntervalMilliseconds =
        m_intervalCombo
            ->currentData()
            .toLongLong();

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  firstTimestamp.value(),
                  lastTimestamp.value()
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        showEmptyTimeline();
        return;
    }

    InvestigationTimelineBreakdown breakdown =
        InvestigationTimelineBreakdown::
        Severity;

    if (m_breakdownCombo
            ->currentData()
            .isValid()) {
        breakdown =
            static_cast<
                InvestigationTimelineBreakdown>(
                m_breakdownCombo
                    ->currentData()
                    .toInt()
                );
    }

    const bool subsystemBreakdown =
        breakdown
            == InvestigationTimelineBreakdown::
            Subsystem
        && m_session->hasSubsystemData();

    QStringList displayedSubsystems;

    if (subsystemBreakdown) {
        const auto frequencies =
            m_analyticsAnalyzer
                .subsystemFrequencies(
                    m_records
                    );

        const int requestedLimit =
            m_subsystemLimitCombo
                    ->currentData()
                    .isValid()
                ? m_subsystemLimitCombo
                      ->currentData()
                      .toInt()
                : 5;

        const int displayedCount =
            std::min(
                requestedLimit,
                static_cast<int>(
                    frequencies.size()
                    )
                );

        displayedSubsystems.reserve(
            displayedCount
            );

        for (int index = 0;
             index < displayedCount;
             ++index) {
            displayedSubsystems.append(
                frequencies
                    .at(index)
                    .value
                );
        }
    }

    /*
     * Keep one Y-axis scale for the complete
     * currently filtered investigation so
     * horizontal scrolling does not visually
     * exaggerate quieter windows.
     */
    if (!m_scaleValid
        || m_scaleIntervalMilliseconds
               != intervalMilliseconds) {
        if (subsystemBreakdown) {
            m_scaleMaximum =
                std::max(
                    1,
                    m_analyticsAnalyzer
                        .subsystemTrendScaleMaximum(
                            m_records,
                            firstTimestamp.value(),
                            lastTimestamp.value(),
                            intervalMilliseconds,
                            displayedSubsystems
                            )
                    );
        } else {
            const EventTimelineScale scale =
                m_timelineAnalyzer
                    .scaleForIntervalMilliseconds(
                        m_records,
                        firstTimestamp.value(),
                        lastTimestamp.value(),
                        intervalMilliseconds
                        );

            m_scaleMaximum =
                std::max(
                    1,
                    m_session->hasSeverityData()
                        ? scale.maximumSeriesCount
                        : scale.maximumTotalCount
                    );
        }

        m_scaleIntervalMilliseconds =
            intervalMilliseconds;

        m_scaleValid =
            true;
    }

    const qint64 totalBucketCount =
        m_timelineAnalyzer
            .intervalBucketCountMilliseconds(
                firstTimestamp.value(),
                lastTimestamp.value(),
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        showEmptyTimeline();
        return;
    }

    QVector<EventCountBucket> buckets;

    QVector<InvestigationValueTrendBucket>
        subsystemBuckets;

    if (automaticInterval) {
        if (subsystemBreakdown) {
            subsystemBuckets =
                m_analyticsAnalyzer
                    .subsystemTrends(
                        m_records,
                        firstTimestamp.value(),
                        lastTimestamp.value(),
                        intervalMilliseconds
                        );
        } else {
            buckets =
                m_timelineAnalyzer
                    .groupRecordsByIntervalMilliseconds(
                        m_records,
                        firstTimestamp.value(),
                        lastTimestamp.value(),
                        intervalMilliseconds
                        );
        }

        {
            const QSignalBlocker blocker(
                m_scrollBar
                );

            m_scrollBar->setRange(
                0,
                0
                );

            m_scrollBar->setValue(
                0
                );
        }

        m_scrollBar->setVisible(
            false
            );
    } else {
        const qint64 maximumStartBucketIndex =
            std::max<qint64>(
                0,
                totalBucketCount
                    - TimelineVisibleBucketCount
                );

        const int scrollMaximum =
            timelineScrollMaximum(
                totalBucketCount
                );

        int scrollValue =
            std::clamp(
                m_scrollBar->value(),
                0,
                scrollMaximum
                );

        int pageStep =
            1;

        if (maximumStartBucketIndex
            <= std::numeric_limits<int>::max()) {
            pageStep =
                std::max(
                    1,
                    std::min(
                        TimelineVisibleBucketCount,
                        std::max(
                            1,
                            scrollMaximum
                            )
                        )
                    );
        } else if (scrollMaximum > 0) {
            const long double pageFraction =
                static_cast<long double>(
                    TimelineVisibleBucketCount
                    )
                / static_cast<long double>(
                    totalBucketCount
                    );

            pageStep =
                std::max(
                    1,
                    static_cast<int>(
                        std::llround(
                            static_cast<long double>(
                                scrollMaximum
                                )
                            * pageFraction
                            )
                        )
                    );
        }

        {
            const QSignalBlocker blocker(
                m_scrollBar
                );

            m_scrollBar->setRange(
                0,
                scrollMaximum
                );

            m_scrollBar->setSingleStep(
                1
                );

            m_scrollBar->setPageStep(
                pageStep
                );

            m_scrollBar->setValue(
                scrollValue
                );
        }

        m_scrollBar->setVisible(
            maximumStartBucketIndex > 0
            );

        const qint64 startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                scrollValue
                );

        if (subsystemBreakdown) {
            subsystemBuckets =
                m_analyticsAnalyzer
                    .subsystemTrendsWindow(
                        m_records,
                        firstTimestamp.value(),
                        lastTimestamp.value(),
                        intervalMilliseconds,
                        startBucketIndex,
                        TimelineVisibleBucketCount
                        );
        } else {
            buckets =
                m_timelineAnalyzer
                    .groupRecordsByIntervalWindowMilliseconds(
                        m_records,
                        firstTimestamp.value(),
                        lastTimestamp.value(),
                        intervalMilliseconds,
                        startBucketIndex,
                        TimelineVisibleBucketCount
                        );
        }
    }

    if (
        (
            subsystemBreakdown
            && (
                subsystemBuckets.isEmpty()
                || displayedSubsystems.isEmpty()
                )
            )
        || (
            !subsystemBreakdown
            && buckets.isEmpty()
            )
        ) {
        showEmptyTimeline();
        return;
    }

    /*
     * ---------------------------------------------------------
     * Subsystem breakdown
     * ---------------------------------------------------------
     */
    if (subsystemBreakdown) {
        QStringList categories;

        categories.reserve(
            subsystemBuckets.size()
            );

        for (
            const InvestigationValueTrendBucket &bucket
            : std::as_const(subsystemBuckets)
            ) {
            categories.append(
                timelineDisplayLabel(
                    bucket.label,
                    intervalMilliseconds
                    )
                );
        }

        auto *series =
            new QBarSeries();

        for (const QString &subsystem
             : std::as_const(
                 displayedSubsystems
                 )) {
            auto *barSet =
                new QBarSet(
                    subsystem
                    );

            for (
                const InvestigationValueTrendBucket
                    &bucket
                : std::as_const(
                    subsystemBuckets
                    )
                ) {
                *barSet
                    << bucket.countFor(
                           subsystem
                           );
            }

            connect(
                barSet,
                &QBarSet::doubleClicked,
                this,
                [
                    this,
                    subsystem
            ](int index) {
                    requestBucketDrillDown(
                        index,
                        QString(),
                        subsystem
                        );
                },
                Qt::QueuedConnection
                );

            series->append(
                barSet
                );
        }

        auto *chart =
            new QChart();

        chart->setMargins(
            QMargins(
                0,
                0,
                0,
                0
                )
            );

        chart->addSeries(
            series
            );

        chart->setAnimationOptions(
            QChart::NoAnimation
            );

        QLegend *legend =
            chart->legend();

        legend->setAlignment(
            Qt::AlignBottom
            );

        legend->setContentsMargins(
            0,
            0,
            0,
            0
            );

        if (legend->layout() != nullptr) {
            legend
                ->layout()
                ->setContentsMargins(
                    0,
                    0,
                    0,
                    0
                    );
        }

        auto *axisX =
            new QBarCategoryAxis();

        axisX->append(
            categories
            );

        axisX->setTruncateLabels(
            false
            );

        chart->addAxis(
            axisX,
            Qt::AlignBottom
            );

        series->attachAxis(
            axisX
            );

        auto *axisY =
            new QValueAxis();

        configureEventCountAxis(
            axisY,
            m_scaleMaximum
            );

        chart->addAxis(
            axisY,
            Qt::AlignLeft
            );

        series->attachAxis(
            axisY
            );

        m_chartView->setChart(
            chart
            );

        updateRangeLabel(
            m_scrollBar->value()
            );

        return;
    }

    /*
     * ---------------------------------------------------------
     * Source without severity
     * ---------------------------------------------------------
     */
    if (!m_session->hasSeverityData()) {
        auto *totalSet =
            new QBarSet(
                tr("TOTAL")
                );

        connect(
            totalSet,
            &QBarSet::doubleClicked,
            this,
            [this](int index) {
                requestBucketDrillDown(
                    index,
                    QString(),
                    QString()
                    );
            },
            Qt::QueuedConnection
            );

        QStringList categories;

        for (const EventCountBucket &bucket
             : std::as_const(buckets)) {
            categories.append(
                timelineDisplayLabel(
                    bucket.label,
                    intervalMilliseconds
                    )
                );

            *totalSet
                << bucket.totalCount();
        }

        auto *series =
            new QBarSeries();

        series->append(
            totalSet
            );

        auto *chart =
            new QChart();

        chart->setMargins(
            QMargins(
                0,
                0,
                0,
                0
                )
            );

        chart->addSeries(
            series
            );

        chart->setAnimationOptions(
            QChart::NoAnimation
            );

        chart->legend()->setVisible(
            false
            );

        auto *axisX =
            new QBarCategoryAxis();

        axisX->append(
            categories
            );

        axisX->setTruncateLabels(
            false
            );

        chart->addAxis(
            axisX,
            Qt::AlignBottom
            );

        series->attachAxis(
            axisX
            );

        auto *axisY =
            new QValueAxis();

        configureEventCountAxis(
            axisY,
            m_scaleMaximum
            );

        chart->addAxis(
            axisY,
            Qt::AlignLeft
            );

        series->attachAxis(
            axisY
            );

        m_chartView->setChart(
            chart
            );

        updateRangeLabel(
            m_scrollBar->value()
            );

        return;
    }

    /*
     * ---------------------------------------------------------
     * Severity breakdown
     * ---------------------------------------------------------
     */
    auto *traceSet =
        new QBarSet(
            tr("TRACE")
            );

    auto *debugSet =
        new QBarSet(
            tr("DEBUG")
            );

    auto *infoSet =
        new QBarSet(
            tr("INFO")
            );

    auto *warnSet =
        new QBarSet(
            tr("WARN")
            );

    auto *errorSet =
        new QBarSet(
            tr("ERROR")
            );

    auto *criticalSet =
        new QBarSet(
            tr("CRITICAL")
            );

    /*
     * Preserve the existing semantic severity
     * colors exactly.
     */
    traceSet->setColor(
        QColor(
            QStringLiteral("#9E9E9E")
            )
        );

    debugSet->setColor(
        QColor(
            QStringLiteral("#607D8B")
            )
        );

    infoSet->setColor(
        QColor(
            QStringLiteral("#1976D2")
            )
        );

    warnSet->setColor(
        QColor(
            QStringLiteral("#F9A825")
            )
        );

    errorSet->setColor(
        QColor(
            QStringLiteral("#D32F2F")
            )
        );

    criticalSet->setColor(
        QColor(
            QStringLiteral("#7A0019")
            )
        );

    auto connectDrillDown =
        [this](
            QBarSet *barSet,
            const QString &severity
            ) {
            connect(
                barSet,
                &QBarSet::doubleClicked,
                this,
                [
                    this,
                    severity
            ](int index) {
                    requestBucketDrillDown(
                        index,
                        severity,
                        QString()
                        );
                },
                Qt::QueuedConnection
                );
        };

    connectDrillDown(
        traceSet,
        QStringLiteral("TRACE")
        );

    connectDrillDown(
        debugSet,
        QStringLiteral("DEBUG")
        );

    connectDrillDown(
        infoSet,
        QStringLiteral("INFO")
        );

    connectDrillDown(
        warnSet,
        QStringLiteral("WARN")
        );

    connectDrillDown(
        errorSet,
        QStringLiteral("ERROR")
        );

    connectDrillDown(
        criticalSet,
        QStringLiteral("CRITICAL")
        );

    bool hasUnspecifiedEvents =
        false;

    for (const EventCountBucket &bucket
         : std::as_const(buckets)) {
        if (bucket.unspecifiedCount > 0) {
            hasUnspecifiedEvents =
                true;

            break;
        }
    }

    QBarSet *unspecifiedSet =
        nullptr;

    if (hasUnspecifiedEvents) {
        unspecifiedSet =
            new QBarSet(
                tr("UNSPECIFIED")
                );

        unspecifiedSet->setColor(
            QColor(
                QStringLiteral("#BDBDBD")
                )
            );

        /*
         * Preserve existing behavior: clicking an
         * unspecified bar narrows to its time
         * bucket without manufacturing a canonical
         * severity value that the record does not
         * possess.
         */
        connectDrillDown(
            unspecifiedSet,
            QString()
            );
    }

    QStringList categories;

    for (const EventCountBucket &bucket
         : std::as_const(buckets)) {
        categories.append(
            timelineDisplayLabel(
                bucket.label,
                intervalMilliseconds
                )
            );

        *traceSet
            << bucket.traceCount;

        *debugSet
            << bucket.debugCount;

        *infoSet
            << bucket.infoCount;

        *warnSet
            << bucket.warningCount;

        *errorSet
            << bucket.errorCount;

        *criticalSet
            << bucket.criticalCount;

        if (unspecifiedSet != nullptr) {
            *unspecifiedSet
                << bucket.unspecifiedCount;
        }
    }

    auto *series =
        new QBarSeries();

    series->append(
        traceSet
        );

    series->append(
        debugSet
        );

    series->append(
        infoSet
        );

    series->append(
        warnSet
        );

    series->append(
        errorSet
        );

    series->append(
        criticalSet
        );

    if (unspecifiedSet != nullptr) {
        series->append(
            unspecifiedSet
            );
    }

    auto *chart =
        new QChart();

    chart->setMargins(
        QMargins(
            0,
            0,
            0,
            0
            )
        );

    chart->addSeries(
        series
        );

    chart->setAnimationOptions(
        QChart::NoAnimation
        );

    QLegend *legend =
        chart->legend();

    legend->setAlignment(
        Qt::AlignBottom
        );

    legend->setContentsMargins(
        0,
        0,
        0,
        0
        );

    if (legend->layout() != nullptr) {
        legend
            ->layout()
            ->setContentsMargins(
                0,
                0,
                0,
                0
                );
    }

    auto *axisX =
        new QBarCategoryAxis();

    axisX->append(
        categories
        );

    axisX->setTruncateLabels(
        false
        );

    chart->addAxis(
        axisX,
        Qt::AlignBottom
        );

    series->attachAxis(
        axisX
        );

    auto *axisY =
        new QValueAxis();

    configureEventCountAxis(
        axisY,
        m_scaleMaximum
        );

    chart->addAxis(
        axisY,
        Qt::AlignLeft
        );

    series->attachAxis(
        axisY
        );

    m_chartView->setChart(
        chart
        );

    updateRangeLabel(
        m_scrollBar->value()
        );
}

void InvestigationTimelinePanel::
    updateRangeLabel(
        int scrollValue
        )
{
    const std::optional<QDateTime>
        firstTimestamp =
        effectiveFirstTimestamp();

    const std::optional<QDateTime>
        lastTimestamp =
        effectiveLastTimestamp();

    if (!firstTimestamp.has_value()
        || !lastTimestamp.has_value()
        || firstTimestamp.value()
               > lastTimestamp.value()) {
        m_rangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    const qint64 requestedIntervalMilliseconds =
        m_intervalCombo
            ->currentData()
            .toLongLong();

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  firstTimestamp.value(),
                  lastTimestamp.value()
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        m_rangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    const qint64 totalBucketCount =
        m_timelineAnalyzer
            .intervalBucketCountMilliseconds(
                firstTimestamp.value(),
                lastTimestamp.value(),
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        m_rangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    qint64 startBucketIndex =
        0;

    qint64 visibleBucketCount =
        totalBucketCount;

    if (!automaticInterval) {
        startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                scrollValue
                );

        visibleBucketCount =
            std::min<qint64>(
                TimelineVisibleBucketCount,
                totalBucketCount
                    - startBucketIndex
                );
    }

    const qint64 firstBucketEpoch =
        normalizedTimelineBucketEpoch(
            firstTimestamp.value(),
            intervalMilliseconds
            );

    qint64 visibleFirstEpoch =
        firstBucketEpoch
        + startBucketIndex
              * intervalMilliseconds;

    visibleFirstEpoch =
        std::max(
            visibleFirstEpoch,
            firstTimestamp
                ->toMSecsSinceEpoch()
            );

    qint64 visibleLastEpoch =
        visibleFirstEpoch
        + visibleBucketCount
              * intervalMilliseconds
        - 1;

    visibleLastEpoch =
        std::min(
            visibleLastEpoch,
            lastTimestamp
                ->toMSecsSinceEpoch()
            );

    const QDateTime visibleFirst =
        QDateTime::fromMSecsSinceEpoch(
            visibleFirstEpoch,
            QTimeZone::UTC
            );

    const QDateTime visibleLast =
        QDateTime::fromMSecsSinceEpoch(
            visibleLastEpoch,
            QTimeZone::UTC
            );

    QString firstText;
    QString lastText;

    if (intervalMilliseconds < 1000) {
        firstText =
            visibleFirst.toString(
                QStringLiteral(
                    "yyyy-MM-dd HH:mm:ss.zzz"
                    )
                );

        if (visibleFirst.date()
            == visibleLast.date()) {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "HH:mm:ss.zzz"
                        )
                    );
        } else {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "yyyy-MM-dd HH:mm:ss.zzz"
                        )
                    );
        }
    } else {
        firstText =
            visibleFirst.toString(
                QStringLiteral(
                    "yyyy-MM-dd HH:mm:ss"
                    )
                );

        if (visibleFirst.date()
            == visibleLast.date()) {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "HH:mm:ss"
                        )
                    );
        } else {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "yyyy-MM-dd HH:mm:ss"
                        )
                    );
        }
    }

    const QString rangeText =
        tr("Visible: %1 – %2 UTC")
            .arg(
                firstText,
                lastText
                );

    m_rangeLabel->setText(
        rangeText
        );

    m_scrollBar->setToolTip(
        rangeText
        );
}

void InvestigationTimelinePanel::
    requestBucketDrillDown(
        int visibleBucketIndex,
        const QString &severity,
        const QString &subsystem
        )
{
    if (m_session == nullptr
        || visibleBucketIndex < 0) {
        return;
    }

    const std::optional<QDateTime>
        firstTimestamp =
        effectiveFirstTimestamp();

    const std::optional<QDateTime>
        lastTimestamp =
        effectiveLastTimestamp();

    if (!firstTimestamp.has_value()
        || !lastTimestamp.has_value()
        || firstTimestamp.value()
               > lastTimestamp.value()) {
        return;
    }

    const qint64 requestedIntervalMilliseconds =
        m_intervalCombo
            ->currentData()
            .toLongLong();

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  firstTimestamp.value(),
                  lastTimestamp.value()
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        return;
    }

    const qint64 totalBucketCount =
        m_timelineAnalyzer
            .intervalBucketCountMilliseconds(
                firstTimestamp.value(),
                lastTimestamp.value(),
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        return;
    }

    qint64 startBucketIndex =
        0;

    if (!automaticInterval) {
        startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                m_scrollBar->value()
                );
    }

    const qint64 absoluteBucketIndex =
        startBucketIndex
        + visibleBucketIndex;

    if (absoluteBucketIndex < 0
        || absoluteBucketIndex
               >= totalBucketCount) {
        return;
    }

    const qint64 firstBucketEpoch =
        normalizedTimelineBucketEpoch(
            firstTimestamp.value(),
            intervalMilliseconds
            );

    qint64 bucketStartEpoch =
        firstBucketEpoch
        + absoluteBucketIndex
              * intervalMilliseconds;

    qint64 bucketEndEpoch =
        bucketStartEpoch
        + intervalMilliseconds
        - 1;

    /*
     * Never broaden an already active time range.
     */
    bucketStartEpoch =
        std::max(
            bucketStartEpoch,
            firstTimestamp
                ->toMSecsSinceEpoch()
            );

    bucketEndEpoch =
        std::min(
            bucketEndEpoch,
            lastTimestamp
                ->toMSecsSinceEpoch()
            );

    if (bucketStartEpoch
        > bucketEndEpoch) {
        return;
    }

    InvestigationFilterProxyModel *proxyModel =
        m_session
            ->investigationController()
            ->proxyModel();

    /*
     * A clicked severity/subsystem must already
     * belong to an existing categorical filter if
     * one is active. Drill-down may narrow an
     * investigation, never broaden it.
     */
    if (!severity.isEmpty()) {
        const QStringList currentSeverities =
            proxyModel->severityFilters();

        if (!currentSeverities.isEmpty()
            && !currentSeverities.contains(
                severity
                )) {
            return;
        }
    }

    if (!subsystem.isEmpty()) {
        const QStringList currentSubsystems =
            proxyModel->subsystemFilters();

        if (!currentSubsystems.isEmpty()
            && !currentSubsystems.contains(
                subsystem
                )) {
            return;
        }
    }

    emit bucketDrillDownRequested(
        QDateTime::fromMSecsSinceEpoch(
            bucketStartEpoch,
            QTimeZone::UTC
            ),
        QDateTime::fromMSecsSinceEpoch(
            bucketEndEpoch,
            QTimeZone::UTC
            ),
        severity,
        subsystem
        );
}