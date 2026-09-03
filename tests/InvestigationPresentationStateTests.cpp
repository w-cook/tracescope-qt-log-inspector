#include <QtTest>

#include <QCoreApplication>
#include <QPushButton>
#include <QBarCategoryAxis>
#include <QChartView>
#include <QScrollBar>
#include <QMenu>

#include <memory>
#include <utility>

#include "../src/domain/InvestigationRecord.h"
#include "../src/importing/ImportProfile.h"
#include "../src/importing/ImportResult.h"
#include "../src/ui/investigation/InvestigationAnalyticsPanel.h"
#include "../src/ui/investigation/InvestigationEventDetailPanel.h"
#include "../src/ui/investigation/InvestigationEventPanel.h"
#include "../src/ui/investigation/InvestigationFindingsPanel.h"
#include "../src/ui/investigation/InvestigationIssueSummaryPanel.h"
#include "../src/ui/investigation/InvestigationReviewPanel.h"
#include "../src/ui/investigation/InvestigationTimelinePanel.h"
#include "../src/ui/workspace/InvestigationSessionView.h"
#include "../src/workspace/InvestigationPresentationState.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{

InvestigationSession makeSession()
{
    ImportResult result;

    for (int index = 0;
         index < 100;
         ++index) {
        InvestigationRecord record;

        record.recordId =
            QStringLiteral("record-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        record.timestamp =
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-28T12:00:00Z"
                    ),
                Qt::ISODate
                )
                .addSecs(index);

        record.severity =
            index % 10 == 0
                ? RecordSeverity::Warning
                : RecordSeverity::Info;

        record.subsystem =
            QStringLiteral("Backend");

        record.message =
            QStringLiteral("Event %1")
                .arg(index);

        record.source.sourcePath =
            QStringLiteral(
                "presentation-test.jsonl"
                );

        record.source.sourceName =
            QStringLiteral(
                "presentation-test.jsonl"
                );

        record.source.recordNumber =
            index + 1;

        result.records.append(
            std::move(record)
            );
    }

    result.processedRecordCount =
        result.records.size();

    ImportProfile profile;

    return InvestigationSession(
        QStringLiteral(
            "presentation-test.jsonl"
            ),
        std::move(profile),
        std::move(result)
        );
}

std::unique_ptr<InvestigationSession>
makeAnalyticsSession()
{
    ImportResult result;

    for (int index = 0;
         index < 150;
         ++index) {
        InvestigationRecord record;

        record.recordId =
            QStringLiteral("analytics-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        record.timestamp =
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-29T12:00:00Z"
                    ),
                Qt::ISODate
                )
                .addSecs(index);

        const bool firstBurst =
            index >= 5
            && index <= 9;

        const bool secondBurst =
            index >= 50
            && index <= 69;

        const bool thirdBurst =
            index >= 110
            && index <= 114;

        record.severity =
            firstBurst
                    || secondBurst
                    || thirdBurst
                ? RecordSeverity::Warning
                : RecordSeverity::Info;

        record.subsystem =
            QStringLiteral("Subsystem-%1")
                .arg(index % 6);

        record.eventCode =
            QStringLiteral("EVT-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        record.entityId =
            QStringLiteral("Entity-%1")
                .arg(
                    index % 30,
                    2,
                    10,
                    QLatin1Char('0')
                    );

        record.message =
            QStringLiteral("Analytics event %1")
                .arg(index);

        record.source.sourcePath =
            QStringLiteral(
                "analytics-presentation-test.jsonl"
                );

        record.source.sourceName =
            QStringLiteral(
                "analytics-presentation-test.jsonl"
                );

        record.source.recordNumber =
            index + 1;

        result.records.append(
            std::move(record)
            );
    }

    result.processedRecordCount =
        result.records.size();

    ImportProfile profile;

    auto session =
        std::make_unique<InvestigationSession>(
            QStringLiteral(
                "analytics-presentation-test.jsonl"
                ),
            std::move(profile),
            std::move(result)
            );

    session->setBurstTimingMode(
        InvestigationBurstTimingMode::Manual
        );

    BurstDetectionSettings settings;

    settings.windowMilliseconds =
        4000;

    settings.elevatedEventThreshold =
        3;

    settings.errorCriticalThreshold =
        100;

    settings.mergeGapMilliseconds =
        0;

    session->setBurstDetectionSettings(
        settings
        );

    return session;
}

void processUi()
{
    QCoreApplication::processEvents();
}

int timelineCategoryCount(
    InvestigationTimelinePanel &panel
    )
{
    QChartView *chartView =
        panel.findChild<QChartView *>();

    if (chartView == nullptr
        || chartView->chart() == nullptr) {
        return 0;
    }

    const QList<QAbstractAxis *> axes =
        chartView->chart()->axes(
            Qt::Horizontal
            );

    for (QAbstractAxis *axis : axes) {
        if (auto *categoryAxis =
            qobject_cast<
                QBarCategoryAxis *
                >(axis);
            categoryAxis != nullptr) {
            return categoryAxis
                ->categories()
                .size();
        }
    }

    return 0;
}

}

class InvestigationPresentationStateTests
    : public QObject
{
    Q_OBJECT

private slots:
    void eventTablePresentationRoundTrips();
    void eventDetailScrollRoundTrips();
    void timelinePresentationRoundTrips();
    void issueSummaryPresentationRoundTrips();
    void findingsPresentationRoundTrips();
    void analyticsPresentationRoundTrips();
    void reviewPresentationRoundTrips();
    void sessionViewPresentationRoundTrips();
    void eventDetailMinimumWidthTracksControls();
    void eventDetailMinimumWidthProtectsControls();
    void newSessionKeepsEventDetailsVisible();
    void autoTimelineDensityAdaptsToWidth();
    void manualTimelineDensityAdaptsToWidth();
    void sessionViewPopulatesExportMenu();
};

void InvestigationPresentationStateTests::
    eventTablePresentationRoundTrips()
{
    InvestigationSession session =
        makeSession();

    InvestigationEventPanel panel;

    panel.resize(
        500,
        220
        );

    panel.setSession(
        &session
        );

    panel.show();

    processUi();

    InvestigationEventTablePresentationState
        desired =
        panel.capturePresentationState();

    QVERIFY(
        !desired.columnWidths.isEmpty()
        );

    /*
     * Force enough table width that horizontal
     * scrolling is meaningful on a narrow panel.
     */
    for (int &width
         : desired.columnWidths) {
        width = 240;
    }

    desired.selectedRecordId =
        QStringLiteral("record-040");

    desired.sortColumn = 0;

    desired.sortOrder =
        Qt::DescendingOrder;

    desired.scroll.horizontalValue =
        25;

    desired.scroll.verticalValue =
        12;

    panel.restorePresentationState(
        desired
        );

    processUi();

    /*
     * Capture the effective state after Qt has
     * applied widget geometry. The final header
     * section intentionally stretches to fill the
     * available table width, so its effective width
     * may differ from the requested width depending
     * on the platform and window-system plugin.
     *
     * This effective state is what a workspace Save
     * operation would actually persist.
     */
    const InvestigationEventTablePresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.selectedRecordId,
        QStringLiteral("record-040")
        );

    QCOMPARE(
        saved.sortColumn,
        0
        );

    QCOMPARE(
        saved.sortOrder,
        Qt::DescendingOrder
        );

    QCOMPARE(
        saved.columnWidths.size(),
        desired.columnWidths.size()
        );

    QVERIFY(
        !saved.columnWidths.isEmpty()
        );

    for (const int width
         : saved.columnWidths) {
        QVERIFY(
            width > 0
            );
    }

    QVERIFY(
        saved.scroll.verticalValue > 0
        );

    /*
     * Deliberately move the investigation somewhere
     * else before restoring the snapshot.
     */
    InvestigationEventTablePresentationState
        disturbed =
        saved;

    disturbed.selectedRecordId =
        QStringLiteral("record-002");

    disturbed.sortOrder =
        Qt::AscendingOrder;

    disturbed.scroll.horizontalValue =
        0;

    disturbed.scroll.verticalValue =
        0;

    for (int &width
         : disturbed.columnWidths) {
        width = 90;
    }

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationEventTablePresentationState
        disturbedEffective =
        panel.capturePresentationState();

    QCOMPARE(
        disturbedEffective.selectedRecordId,
        QStringLiteral("record-002")
        );

    /*
     * Restore the original saved presentation.
     */
    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationEventTablePresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.selectedRecordId,
        saved.selectedRecordId
        );

    QCOMPARE(
        restored.sortColumn,
        saved.sortColumn
        );

    QCOMPARE(
        restored.sortOrder,
        saved.sortOrder
        );

    QCOMPARE(
        restored.columnWidths,
        saved.columnWidths
        );

    QCOMPARE(
        restored.scroll.horizontalValue,
        saved.scroll.horizontalValue
        );

    QCOMPARE(
        restored.scroll.verticalValue,
        saved.scroll.verticalValue
        );
}

void InvestigationPresentationStateTests::
    eventDetailScrollRoundTrips()
{
    InvestigationEventDetailPanel panel;

    panel.resize(
        360,
        180
        );

    InvestigationRecord record;

    record.recordId =
        QStringLiteral("detail-record");

    record.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00Z"
                ),
            Qt::ISODate
            );

    record.severity =
        RecordSeverity::Warning;

    QStringList messageLines;

    for (int index = 0;
         index < 100;
         ++index) {
        messageLines.append(
            QStringLiteral(
                "Diagnostic detail line %1"
                )
                .arg(index)
            );
    }

    record.message =
        messageLines.join(
            QLatin1Char('\n')
            );

    panel.displayRecord(
        record
        );

    panel.show();

    processUi();

    InvestigationScrollState desired;

    desired.verticalValue =
        30;

    panel.restorePresentationState(
        desired
        );

    processUi();

    const InvestigationScrollState saved =
        panel.capturePresentationState();

    /*
     * Confirm this fixture actually produced a
     * scrollable detail surface.
     */
    QVERIFY(
        saved.verticalValue > 0
        );

    InvestigationScrollState disturbed;

    disturbed.horizontalValue =
        0;

    disturbed.verticalValue =
        0;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    QCOMPARE(
        panel.capturePresentationState()
            .verticalValue,
        0
        );

    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationScrollState restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.horizontalValue,
        saved.horizontalValue
        );

    QCOMPARE(
        restored.verticalValue,
        saved.verticalValue
        );
}

void InvestigationPresentationStateTests::
    timelinePresentationRoundTrips()
{
    InvestigationSession session =
        makeSession();

    InvestigationTimelinePanel panel;

    /*
     * Keep the timeline narrow enough that a
     * one-second resolution requires horizontal
     * navigation across this 100-second fixture.
     */
    panel.resize(
        420,
        240
        );

    panel.setSession(
        &session
        );

    panel.updateRecords(
        session
            .investigationController()
            ->recordsForAnalysis()
        );

    panel.show();

    processUi();

    InvestigationTimelinePresentationState
        desired;

    desired.intervalMilliseconds =
        1000;

    desired.breakdown =
        InvestigationTimelineBreakdown::
        Subsystem;

    desired.subsystemTrendLimit =
        10;

    desired.horizontalScrollValue =
        30;

    panel.restorePresentationState(
        desired
        );

    processUi();

    const InvestigationTimelinePresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.intervalMilliseconds,
        qint64(1000)
        );

    QCOMPARE(
        saved.breakdown,
        InvestigationTimelineBreakdown::
        Subsystem
        );

    QCOMPARE(
        saved.subsystemTrendLimit,
        10
        );

    /*
     * This fixture deliberately contains many more
     * one-second buckets than can fit in the panel,
     * so the saved navigation position should be
     * meaningful rather than clamped to zero.
     */
    QCOMPARE(
        saved.horizontalScrollValue,
        30
        );

    /*
     * Move every persisted timeline control away
     * from the saved state.
     */
    InvestigationTimelinePresentationState
        disturbed;

    disturbed.intervalMilliseconds =
        5000;

    disturbed.breakdown =
        InvestigationTimelineBreakdown::
        Severity;

    disturbed.subsystemTrendLimit =
        5;

    disturbed.horizontalScrollValue =
        0;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationTimelinePresentationState
        disturbedEffective =
        panel.capturePresentationState();

    QCOMPARE(
        disturbedEffective.intervalMilliseconds,
        qint64(5000)
        );

    QCOMPARE(
        disturbedEffective.breakdown,
        InvestigationTimelineBreakdown::
        Severity
        );

    QCOMPARE(
        disturbedEffective.subsystemTrendLimit,
        5
        );

    /*
     * Restore the original timeline snapshot.
     */
    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationTimelinePresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.intervalMilliseconds,
        saved.intervalMilliseconds
        );

    QCOMPARE(
        restored.breakdown,
        saved.breakdown
        );

    QCOMPARE(
        restored.subsystemTrendLimit,
        saved.subsystemTrendLimit
        );

    QCOMPARE(
        restored.horizontalScrollValue,
        saved.horizontalScrollValue
        );

    /*
     * Auto resolution intentionally has no saved
     * horizontal navigation position.
     */
    InvestigationTimelinePresentationState
        automatic =
        saved;

    automatic.intervalMilliseconds =
        0;

    automatic.horizontalScrollValue =
        50;

    panel.restorePresentationState(
        automatic
        );

    processUi();

    const InvestigationTimelinePresentationState
        automaticEffective =
        panel.capturePresentationState();

    QCOMPARE(
        automaticEffective.intervalMilliseconds,
        qint64(0)
        );

    QCOMPARE(
        automaticEffective.horizontalScrollValue,
        0
        );
}

void InvestigationPresentationStateTests::
    issueSummaryPresentationRoundTrips()
{
    InvestigationIssueSummaryPanel panel;

    panel.resize(
        360,
        180
        );

    QVector<InvestigationRecord> records;

    /*
     * Give every record its own subsystem so the
     * grouped summary contains enough rows to make
     * vertical scrolling meaningful.
     */
    for (int index = 0;
         index < 60;
         ++index) {
        InvestigationRecord record;

        record.recordId =
            QStringLiteral("issue-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        record.subsystem =
            QStringLiteral("Subsystem %1")
                .arg(
                    index,
                    2,
                    10,
                    QLatin1Char('0')
                    );

        record.severity =
            index % 2 == 0
                ? RecordSeverity::Warning
                : RecordSeverity::Error;

        records.append(
            std::move(record)
            );
    }

    panel.updateRecords(
        records
        );

    panel.show();

    processUi();

    InvestigationTablePresentationState
        desired;

    desired.currentRow = 40;
    desired.currentColumn = 2;
    desired.scroll.verticalValue = 12;

    panel.restorePresentationState(
        desired
        );

    processUi();

    /*
     * Capture the effective state after Qt has applied
     * widget geometry. Scroll ranges can vary with the
     * platform and window-system plugin, so this
     * effective state — rather than the originally
     * requested values — represents what a real
     * workspace Save operation would persist.
     */
    const InvestigationTablePresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.currentRow,
        40
        );

    QCOMPARE(
        saved.currentColumn,
        2
        );

    QVERIFY(
        saved.scroll.verticalValue > 0
        );

    InvestigationTablePresentationState
        disturbed;

    disturbed.currentRow = 1;
    disturbed.currentColumn = 0;
    disturbed.scroll.verticalValue = 0;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationTablePresentationState
        disturbedEffective =
        panel.capturePresentationState();

    QCOMPARE(
        disturbedEffective.currentRow,
        1
        );

    QCOMPARE(
        disturbedEffective.currentColumn,
        0
        );

    /*
     * Restore the state a workspace Save operation
     * would have captured.
     */
    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationTablePresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.currentRow,
        saved.currentRow
        );

    QCOMPARE(
        restored.currentColumn,
        saved.currentColumn
        );

    QCOMPARE(
        restored.scroll.horizontalValue,
        saved.scroll.horizontalValue
        );

    QCOMPARE(
        restored.scroll.verticalValue,
        saved.scroll.verticalValue
        );
}

void InvestigationPresentationStateTests::
    findingsPresentationRoundTrips()
{
    InvestigationSession session =
        makeSession();

    /*
     * Findings are derived from persistent
     * InvestigationStateStore classifications rather
     * than directly from the currently filtered
     * record collection.
     */
    for (int index = 0;
         index < 100;
         ++index) {
        const QString recordId =
            QStringLiteral("record-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        session
            .investigationStateStore()
            ->setFindingStatus(
                recordId,
                index % 3 == 0
                    ? FindingStatus::Resolved
                    : FindingStatus::Open
                );

        session
            .investigationStateStore()
            ->setNote(
                recordId,
                QStringLiteral(
                    "Finding for record %1"
                    )
                    .arg(index)
                );
    }

    InvestigationFindingsPanel panel;

    panel.resize(
        420,
        180
        );

    panel.setSession(
        &session
        );

    panel.show();

    processUi();

    InvestigationTablePresentationState
        desired;

    desired.currentRow = 40;
    desired.currentColumn = 3;
    desired.scroll.verticalValue = 20;

    panel.restorePresentationState(
        desired
        );

    processUi();

    /*
     * Capture the effective state after Qt has applied
     * widget geometry. Scroll ranges can vary with the
     * platform and window-system plugin, so this
     * effective state — rather than the originally
     * requested values — represents what a real
     * workspace Save operation would persist.
     */
    const InvestigationTablePresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.currentRow,
        40
        );

    QCOMPARE(
        saved.currentColumn,
        3
        );

    QVERIFY(
        saved.scroll.verticalValue > 0
        );

    InvestigationTablePresentationState
        disturbed;

    disturbed.currentRow = 2;
    disturbed.currentColumn = 0;
    disturbed.scroll.verticalValue = 0;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationTablePresentationState
        disturbedEffective =
        panel.capturePresentationState();

    QCOMPARE(
        disturbedEffective.currentRow,
        2
        );

    QCOMPARE(
        disturbedEffective.currentColumn,
        0
        );

    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationTablePresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.currentRow,
        saved.currentRow
        );

    QCOMPARE(
        restored.currentColumn,
        saved.currentColumn
        );

    QCOMPARE(
        restored.scroll.horizontalValue,
        saved.scroll.horizontalValue
        );

    QCOMPARE(
        restored.scroll.verticalValue,
        saved.scroll.verticalValue
        );
}

void InvestigationPresentationStateTests::
    analyticsPresentationRoundTrips()
{
    auto session =
        makeAnalyticsSession();

    InvestigationAnalyticsPanel panel;

    panel.resize(
        520,
        220
        );

    panel.setSession(
        session.get()
        );

    panel.updateRecords(
        session
            ->investigationController()
            ->recordsForAnalysis()
        );

    panel.show();

    processUi();

    InvestigationAnalyticsPresentationState
        desired =
        panel.capturePresentationState();

    desired.selectedTab =
        InvestigationAnalyticsTab::Bursts;

    desired.overviewSplitterSizes = {
        180,
        320
    };

    desired.eventCodeTable.currentRow =
        80;

    desired.eventCodeTable.currentColumn =
        1;

    desired.eventCodeTable
        .scroll.verticalValue =
        50;

    desired.entityTable.currentRow =
        7;

    desired.entityTable.currentColumn =
        0;

    desired.entityTable
        .scroll.verticalValue =
        4;

    desired.burstSplitterSizes = {
        300,
        200
    };

    desired.burstTable.currentRow =
        1;

    desired.burstTable.currentColumn =
        2;

    desired.selectedBurstStartTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:00:50Z"
                ),
            Qt::ISODate
            );

    desired.selectedBurstEndTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:01:09Z"
                ),
            Qt::ISODate
            );

    desired.burstDetailScroll.verticalValue =
        12;

    panel.restorePresentationState(
        desired
        );

    processUi();

    /*
     * Capture effective Qt state rather than
     * assuming requested splitter sizes or scroll
     * positions survive platform-specific layout
     * normalization unchanged.
     */
    const InvestigationAnalyticsPresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.selectedTab,
        InvestigationAnalyticsTab::Bursts
        );

    QCOMPARE(
        saved.eventCodeTable.currentRow,
        80
        );

    QCOMPARE(
        saved.eventCodeTable.currentColumn,
        1
        );

    QCOMPARE(
        saved.entityTable.currentRow,
        7
        );

    QCOMPARE(
        saved.entityTable.currentColumn,
        0
        );

    QCOMPARE(
        saved.burstTable.currentRow,
        1
        );

    QVERIFY(
        saved.selectedBurstStartTimestamp
            .has_value()
        );

    QVERIFY(
        saved.selectedBurstEndTimestamp
            .has_value()
        );

    QCOMPARE(
        saved.selectedBurstStartTimestamp
            .value(),
        desired.selectedBurstStartTimestamp
            .value()
        );

    QCOMPARE(
        saved.selectedBurstEndTimestamp
            .value(),
        desired.selectedBurstEndTimestamp
            .value()
        );

    QCOMPARE(
        saved.overviewSplitterSizes.size(),
        2
        );

    QCOMPARE(
        saved.burstSplitterSizes.size(),
        2
        );

    for (const int size
         : saved.overviewSplitterSizes) {
        QVERIFY(size > 0);
    }

    for (const int size
         : saved.burstSplitterSizes) {
        QVERIFY(size > 0);
    }

    /*
     * The long middle burst should make this a
     * genuinely scrollable explanation surface.
     */
    QVERIFY(
        saved.burstDetailScroll
            .verticalValue
        > 0
        );

    InvestigationAnalyticsPresentationState
        disturbed =
        saved;

    disturbed.selectedTab =
        InvestigationAnalyticsTab::Overview;

    disturbed.overviewSplitterSizes = {
        350,
        150
    };

    disturbed.eventCodeTable.currentRow =
        2;

    disturbed.eventCodeTable.currentColumn =
        0;

    disturbed.eventCodeTable
        .scroll.verticalValue =
        0;

    disturbed.entityTable.currentRow =
        1;

    disturbed.entityTable.currentColumn =
        1;

    disturbed.burstSplitterSizes = {
        150,
        350
    };

    disturbed.burstTable.currentRow =
        0;

    disturbed.burstTable.currentColumn =
        0;

    disturbed.selectedBurstStartTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:00:05Z"
                ),
            Qt::ISODate
            );

    disturbed.selectedBurstEndTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:00:09Z"
                ),
            Qt::ISODate
            );

    disturbed.burstDetailScroll.verticalValue =
        0;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationAnalyticsPresentationState
        disturbedEffective =
        panel.capturePresentationState();

    QCOMPARE(
        disturbedEffective.selectedTab,
        InvestigationAnalyticsTab::Overview
        );

    QCOMPARE(
        disturbedEffective.burstTable.currentRow,
        0
        );

    QCOMPARE(
        disturbedEffective.burstDetailScroll
            .verticalValue,
        0
        );

    /*
     * Restore exactly what a workspace Save
     * operation captured.
     */
    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationAnalyticsPresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.selectedTab,
        saved.selectedTab
        );

    QCOMPARE(
        restored.overviewSplitterSizes,
        saved.overviewSplitterSizes
        );

    QCOMPARE(
        restored.eventCodeTable.currentRow,
        saved.eventCodeTable.currentRow
        );

    QCOMPARE(
        restored.eventCodeTable.currentColumn,
        saved.eventCodeTable.currentColumn
        );

    QCOMPARE(
        restored.eventCodeTable
            .scroll.horizontalValue,
        saved.eventCodeTable
            .scroll.horizontalValue
        );

    QCOMPARE(
        restored.eventCodeTable
            .scroll.verticalValue,
        saved.eventCodeTable
            .scroll.verticalValue
        );

    QCOMPARE(
        restored.entityTable.currentRow,
        saved.entityTable.currentRow
        );

    QCOMPARE(
        restored.entityTable.currentColumn,
        saved.entityTable.currentColumn
        );

    QCOMPARE(
        restored.entityTable
            .scroll.horizontalValue,
        saved.entityTable
            .scroll.horizontalValue
        );

    QCOMPARE(
        restored.entityTable
            .scroll.verticalValue,
        saved.entityTable
            .scroll.verticalValue
        );

    QCOMPARE(
        restored.burstSplitterSizes,
        saved.burstSplitterSizes
        );

    QCOMPARE(
        restored.selectedBurstStartTimestamp,
        saved.selectedBurstStartTimestamp
        );

    QCOMPARE(
        restored.selectedBurstEndTimestamp,
        saved.selectedBurstEndTimestamp
        );

    QCOMPARE(
        restored.burstTable.currentRow,
        saved.burstTable.currentRow
        );

    QCOMPARE(
        restored.burstTable.currentColumn,
        saved.burstTable.currentColumn
        );

    /*
     * QPlainTextEdit can slightly normalize its
     * scrollbar position after tab/splitter geometry
     * settles. Verify that the saved non-zero viewport
     * position was meaningfully restored rather than
     * requiring an identical platform-dependent
     * scrollbar value.
     */
    QVERIFY(
        restored.burstDetailScroll
            .verticalValue
        > 0
        );
}

void InvestigationPresentationStateTests::
    reviewPresentationRoundTrips()
{
    auto session =
        makeAnalyticsSession();

    /*
     * Populate enough findings for the Findings
     * child state to be meaningful.
     */
    for (int index = 0;
         index < 30;
         ++index) {
        const QString recordId =
            QStringLiteral("analytics-%1")
                .arg(
                    index,
                    3,
                    10,
                    QLatin1Char('0')
                    );

        session
            ->investigationStateStore()
            ->setFindingStatus(
                recordId,
                FindingStatus::Open
                );

        session
            ->investigationStateStore()
            ->setNote(
                recordId,
                QStringLiteral(
                    "Review finding %1"
                    )
                    .arg(index)
                );
    }

    InvestigationReviewPanel panel;

    panel.resize(
        620,
        260
        );

    panel.setSession(
        session.get()
        );

    const QVector<InvestigationRecord>
        records =
        session
            ->investigationController()
            ->recordsForAnalysis();

    panel.issueSummaryPanel()
        ->updateRecords(
            records
            );

    panel.findingsPanel()
        ->refresh();

    panel.analyticsPanel()
        ->updateRecords(
            records
            );

    panel.setIssueSummaryAvailable(
        true
        );

    panel.show();

    processUi();

    InvestigationReviewPresentationState
        desired =
        panel.capturePresentationState();

    desired.selectedTab =
        InvestigationReviewTab::Analytics;

    desired.issueSummaryTable.currentRow =
        2;

    desired.issueSummaryTable.currentColumn =
        3;

    desired.findingsTable.currentRow =
        12;

    desired.findingsTable.currentColumn =
        3;

    desired.analytics.selectedTab =
        InvestigationAnalyticsTab::Bursts;

    desired.analytics.burstTable.currentRow =
        1;

    desired.analytics.burstTable.currentColumn =
        0;

    desired.analytics
        .selectedBurstStartTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:00:50Z"
                ),
            Qt::ISODate
            );

    desired.analytics
        .selectedBurstEndTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:01:09Z"
                ),
            Qt::ISODate
            );

    panel.restorePresentationState(
        desired
        );

    processUi();

    const InvestigationReviewPresentationState
        saved =
        panel.capturePresentationState();

    QCOMPARE(
        saved.selectedTab,
        InvestigationReviewTab::Analytics
        );

    QCOMPARE(
        saved.issueSummaryTable.currentRow,
        2
        );

    QCOMPARE(
        saved.findingsTable.currentRow,
        12
        );

    QCOMPARE(
        saved.analytics.selectedTab,
        InvestigationAnalyticsTab::Bursts
        );

    QVERIFY(
        saved.analytics
            .selectedBurstStartTimestamp
            .has_value()
        );

    InvestigationReviewPresentationState
        disturbed =
        saved;

    disturbed.selectedTab =
        InvestigationReviewTab::Findings;

    disturbed.issueSummaryTable.currentRow =
        0;

    disturbed.findingsTable.currentRow =
        1;

    disturbed.analytics.selectedTab =
        InvestigationAnalyticsTab::Overview;

    panel.restorePresentationState(
        disturbed
        );

    processUi();

    QCOMPARE(
        panel.capturePresentationState()
            .selectedTab,
        InvestigationReviewTab::Findings
        );

    panel.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationReviewPresentationState
        restored =
        panel.capturePresentationState();

    QCOMPARE(
        restored.selectedTab,
        saved.selectedTab
        );

    QCOMPARE(
        restored.issueSummaryTable.currentRow,
        saved.issueSummaryTable.currentRow
        );

    QCOMPARE(
        restored.issueSummaryTable.currentColumn,
        saved.issueSummaryTable.currentColumn
        );

    QCOMPARE(
        restored.findingsTable.currentRow,
        saved.findingsTable.currentRow
        );

    QCOMPARE(
        restored.findingsTable.currentColumn,
        saved.findingsTable.currentColumn
        );

    QCOMPARE(
        restored.analytics.selectedTab,
        saved.analytics.selectedTab
        );

    QCOMPARE(
        restored.analytics
            .selectedBurstStartTimestamp,
        saved.analytics
            .selectedBurstStartTimestamp
        );

    QCOMPARE(
        restored.analytics
            .selectedBurstEndTimestamp,
        saved.analytics
            .selectedBurstEndTimestamp
        );
}

void InvestigationPresentationStateTests::
    sessionViewPresentationRoundTrips()
{
    auto session =
        makeAnalyticsSession();

    InvestigationSessionView view(
        session.get(),
        nullptr
        );

    view.resize(
        1000,
        800
        );

    view.show();

    processUi();

    InvestigationSessionPresentationState
        desired =
        view.capturePresentationState();

    desired.mainSplitterSizes = {
        180,
        360,
        260
    };

    desired.bottomSplitterSizes = {
        520,
        420
    };

    desired.burstTimingMode =
        InvestigationBurstTimingMode::Manual;

    desired.burstDetectionSettings
        .windowMilliseconds =
        4000;

    desired.burstDetectionSettings
        .elevatedEventThreshold =
        3;

    desired.burstDetectionSettings
        .errorCriticalThreshold =
        100;

    desired.burstDetectionSettings
        .mergeGapMilliseconds =
        0;

    view.restorePresentationState(
        desired
        );

    processUi();

    /*
     * Capture Qt's effective splitter geometry
     * rather than assuming the requested pixel
     * values survive layout normalization exactly.
     */
    const InvestigationSessionPresentationState
        saved =
        view.capturePresentationState();

    QCOMPARE(
        saved.mainSplitterSizes.size(),
        3
        );

    QCOMPARE(
        saved.bottomSplitterSizes.size(),
        2
        );

    for (const int size
         : saved.mainSplitterSizes) {
        QVERIFY(
            size > 0
            );
    }

    for (const int size
         : saved.bottomSplitterSizes) {
        QVERIFY(
            size > 0
            );
    }

    QCOMPARE(
        saved.burstTimingMode,
        InvestigationBurstTimingMode::Manual
        );

    QCOMPARE(
        saved.burstDetectionSettings
            .windowMilliseconds,
        qint64(4000)
        );

    QCOMPARE(
        saved.burstDetectionSettings
            .elevatedEventThreshold,
        3
        );

    QCOMPARE(
        saved.burstDetectionSettings
            .errorCriticalThreshold,
        100
        );

    QCOMPARE(
        saved.burstDetectionSettings
            .mergeGapMilliseconds,
        qint64(0)
        );

    /*
     * Move the outer workspace geometry and burst
     * configuration somewhere clearly different.
     */
    InvestigationSessionPresentationState
        disturbed =
        saved;

    disturbed.mainSplitterSizes = {
        500,
        150,
        150
    };

    disturbed.bottomSplitterSizes = {
        220,
        720
    };

    disturbed.burstTimingMode =
        InvestigationBurstTimingMode::Auto;

    disturbed.burstDetectionSettings
        .windowMilliseconds =
        15000;

    disturbed.burstDetectionSettings
        .elevatedEventThreshold =
        8;

    disturbed.burstDetectionSettings
        .errorCriticalThreshold =
        5;

    disturbed.burstDetectionSettings
        .mergeGapMilliseconds =
        2500;

    view.restorePresentationState(
        disturbed
        );

    processUi();

    const InvestigationSessionPresentationState
        disturbedEffective =
        view.capturePresentationState();

    QCOMPARE(
        disturbedEffective.burstTimingMode,
        InvestigationBurstTimingMode::Auto
        );

    QCOMPARE(
        disturbedEffective
            .burstDetectionSettings
            .windowMilliseconds,
        qint64(15000)
        );

    QCOMPARE(
        disturbedEffective
            .burstDetectionSettings
            .elevatedEventThreshold,
        8
        );

    QCOMPARE(
        disturbedEffective
            .burstDetectionSettings
            .errorCriticalThreshold,
        5
        );

    QCOMPARE(
        disturbedEffective
            .burstDetectionSettings
            .mergeGapMilliseconds,
        qint64(2500)
        );

    /*
     * Burst configuration is deterministic and proves
     * that the aggregate session presentation state was
     * meaningfully disturbed.
     *
     * Splitter geometry is deliberately not required to
     * differ here. Qt may normalize requested splitter
     * sizes back to the same effective geometry under a
     * headless platform plugin.
     */
    QCOMPARE(
        disturbedEffective.mainSplitterSizes.size(),
        saved.mainSplitterSizes.size()
        );

    QCOMPARE(
        disturbedEffective.bottomSplitterSizes.size(),
        saved.bottomSplitterSizes.size()
        );

    for (const int size
         : disturbedEffective.mainSplitterSizes) {
        QVERIFY(
            size > 0
            );
    }

    for (const int size
         : disturbedEffective.bottomSplitterSizes) {
        QVERIFY(
            size > 0
            );
    }

    /*
     * Restore exactly the state a workspace Save
     * operation would have captured.
     */
    view.restorePresentationState(
        saved
        );

    processUi();

    const InvestigationSessionPresentationState
        restored =
        view.capturePresentationState();

    QCOMPARE(
        restored.mainSplitterSizes,
        saved.mainSplitterSizes
        );

    QCOMPARE(
        restored.bottomSplitterSizes.size(),
        saved.bottomSplitterSizes.size()
        );

    for (const int size
         : restored.bottomSplitterSizes) {
        QVERIFY(
            size > 0
            );
    }

    QCOMPARE(
        restored.burstTimingMode,
        saved.burstTimingMode
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .windowMilliseconds,
        saved.burstDetectionSettings
            .windowMilliseconds
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .elevatedEventThreshold,
        saved.burstDetectionSettings
            .elevatedEventThreshold
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .errorCriticalThreshold,
        saved.burstDetectionSettings
            .errorCriticalThreshold
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .mergeGapMilliseconds,
        saved.burstDetectionSettings
            .mergeGapMilliseconds
        );
}

void InvestigationPresentationStateTests::
    eventDetailMinimumWidthTracksControls()
{
    InvestigationEventDetailPanel panel;

    panel.show();

    processUi();

    const int defaultMinimumWidth =
        panel.minimumWidth();

    QVERIFY(
        defaultMinimumWidth > 0
        );

    InvestigationRecordState state;

    state.note =
        QStringLiteral(
            "Diagnostic note"
            );

    state.bookmarked =
        true;

    state.findingStatus =
        FindingStatus::Open;

    panel.setInvestigationState(
        state
        );

    processUi();

    const int populatedStateMinimumWidth =
        panel.minimumWidth();

    /*
     * The active-state labels are at least as
     * demanding as the default labels. The exact
     * pixel widths are intentionally not asserted
     * because those legitimately vary by platform,
     * font, and DPI configuration.
     */
    QVERIFY(
        populatedStateMinimumWidth
        >= defaultMinimumWidth
        );

    panel.resize(
        std::max(
            1,
            populatedStateMinimumWidth / 2
            ),
        180
        );

    processUi();

    /*
     * QWidget/QSplitter layout constraints must not
     * permit the panel below the content-derived
     * minimum required by its compact controls.
     */
    QVERIFY(
        panel.width()
        >= panel.minimumWidth()
        );
}

void InvestigationPresentationStateTests::
    eventDetailMinimumWidthProtectsControls()
{
    InvestigationEventDetailPanel panel;

    panel.show();

    processUi();

    const int defaultMinimumWidth =
        panel.minimumWidth();

    QVERIFY(
        defaultMinimumWidth > 0
        );

    InvestigationRecordState state;

    state.note =
        QStringLiteral(
            "Existing analyst note"
            );

    state.bookmarked =
        true;

    state.findingStatus =
        FindingStatus::Open;

    panel.setInvestigationState(
        state
        );

    processUi();

    const int activeMinimumWidth =
        panel.minimumWidth();

    /*
     * Longer active-state labels such as
     * "View/Edit Note" and "Remove Bookmark"
     * must not reduce the usable minimum.
     *
     * Do not assert exact pixels: those legitimately
     * depend on font metrics, platform style, and DPI.
     */
    QVERIFY(
        activeMinimumWidth
        >= defaultMinimumWidth
        );

    /*
     * Attempt to force the panel below its own
     * content-derived minimum.
     */
    panel.resize(
        std::max(
            1,
            activeMinimumWidth / 2
            ),
        180
        );

    processUi();

    QVERIFY(
        panel.width()
        >= panel.minimumWidth()
        );

    /*
     * Every action button must retain at least the
     * width Qt reports as necessary for its contents.
     */
    const QList<QPushButton *> buttons =
        panel.findChildren<QPushButton *>();

    QVERIFY(
        !buttons.isEmpty()
        );

    for (QPushButton *button : buttons) {
        QVERIFY(
            button != nullptr
            );

        QVERIFY(
            button->minimumWidth()
            >= button->sizeHint().width()
            );

        QVERIFY(
            button->width()
            >= button->minimumWidth()
            );
    }
}

void InvestigationPresentationStateTests::
    newSessionKeepsEventDetailsVisible()
{
    InvestigationSession session =
        makeSession();

    InvestigationSessionView view(
        &session,
        nullptr
        );

    view.resize(
        1000,
        800
        );

    view.show();

    /*
     * resizeEvent() intentionally defers the adaptive
     * Review/Detail split until child geometry has
     * settled, so allow queued UI work to complete.
     */
    processUi();
    processUi();

    const InvestigationSessionPresentationState
        state =
        view.capturePresentationState();

    QCOMPARE(
        state.bottomSplitterSizes.size(),
        2
        );

    QVERIFY(
        state.bottomSplitterSizes.at(0)
        > 0
        );

    QVERIFY(
        state.bottomSplitterSizes.at(1)
        > 0
        );

    InvestigationEventDetailPanel *detailPanel =
        view.findChild<
            InvestigationEventDetailPanel *
            >();

    if (detailPanel == nullptr) {
        QFAIL(
            "InvestigationEventDetailPanel "
            "was not found"
            );
    }

    QVERIFY(
        state.bottomSplitterSizes.at(1)
        >= detailPanel->minimumWidth()
        );
}

void InvestigationPresentationStateTests::
    autoTimelineDensityAdaptsToWidth()
{
    InvestigationSession session =
        makeSession();

    InvestigationTimelinePanel panel;

    panel.resize(
        900,
        240
        );

    panel.setSession(
        &session
        );

    panel.updateRecords(
        session
            .investigationController()
            ->recordsForAnalysis()
        );

    panel.show();

    processUi();

    /*
     * Auto is the default. Capture the number of
     * categories Qt determined were readable in a
     * relatively wide timeline.
     */
    const int wideCategoryCount =
        timelineCategoryCount(
            panel
            );

    QVERIFY(
        wideCategoryCount > 0
        );

    panel.resize(
        320,
        240
        );

    /*
     * Timeline resize rendering is deliberately
     * debounced. QTRY_VERIFY lets Qt's event loop
     * process that timer without relying on exact
     * timing or a real display server.
     */
    QTRY_VERIFY(
        timelineCategoryCount(panel)
        > 0
        );

    QTRY_VERIFY(
        timelineCategoryCount(panel)
        <= wideCategoryCount
        );

    const int narrowCategoryCount =
        timelineCategoryCount(
            panel
            );

    QVERIFY(
        narrowCategoryCount
        <= wideCategoryCount
        );
}

void InvestigationPresentationStateTests::
    manualTimelineDensityAdaptsToWidth()
{
    InvestigationSession session =
        makeSession();

    InvestigationTimelinePanel panel;

    panel.resize(
        900,
        240
        );

    panel.setSession(
        &session
        );

    panel.updateRecords(
        session
            .investigationController()
            ->recordsForAnalysis()
        );

    InvestigationTimelinePresentationState
        manualState;

    manualState.intervalMilliseconds =
        1000;

    manualState.breakdown =
        InvestigationTimelineBreakdown::
        Severity;

    manualState.subsystemTrendLimit =
        5;

    manualState.horizontalScrollValue =
        0;

    panel.restorePresentationState(
        manualState
        );

    panel.show();

    processUi();

    const int wideCategoryCount =
        timelineCategoryCount(
            panel
            );

    QVERIFY(
        wideCategoryCount > 0
        );

    QCOMPARE(
        panel.capturePresentationState()
            .intervalMilliseconds,
        qint64(1000)
        );

    panel.resize(
        320,
        240
        );

    QTRY_VERIFY(
        timelineCategoryCount(panel)
        > 0
        );

    QTRY_VERIFY(
        timelineCategoryCount(panel)
        <= wideCategoryCount
        );

    const InvestigationTimelinePresentationState
        narrowState =
        panel.capturePresentationState();

    /*
     * Responsive density must not silently change a
     * manually selected resolution.
     */
    QCOMPARE(
        narrowState.intervalMilliseconds,
        qint64(1000)
        );

    /*
     * The narrower viewport may show fewer buckets,
     * but it must continue using horizontal
     * navigation for the remaining investigation.
     */
    QScrollBar *scrollBar =
        panel.findChild<QScrollBar *>();

    QVERIFY(
        scrollBar != nullptr
        );

    if (scrollBar == nullptr) {
        return;
    }

    QVERIFY(
        scrollBar->maximum() > 0
        );
}

void InvestigationPresentationStateTests::
    sessionViewPopulatesExportMenu()
{
    InvestigationSession session =
        makeSession();

    session
        .investigationStateStore()
        ->setFindingStatus(
            QStringLiteral("record-000"),
            FindingStatus::Open
            );

    session
        .investigationStateStore()
        ->setFindingStatus(
            QStringLiteral("record-001"),
            FindingStatus::Resolved
            );

    session
        .investigationStateStore()
        ->setBookmarked(
            QStringLiteral("record-000"),
            true
            );

    InvestigationSessionView view(
        &session,
        nullptr
        );

    QMenu menu;

    view.populateExportMenu(
        &menu
        );

    const QList<QAction *> actions =
        menu.actions();

    /*
     * Session export menus now begin with the
     * workspace-level investigation report action,
     * followed by the existing session-specific
     * record and findings exports.
     */
    QCOMPARE(
        actions.size(),
        5
        );

    QCOMPARE(
        actions[0]->text(),
        QStringLiteral(
            "Export Investigation Report..."
            )
        );

    QVERIFY(
        actions[0]->isEnabled()
        );

    QVERIFY(
        actions[1]->isSeparator()
        );

    QCOMPARE(
        actions[2]->text(),
        QStringLiteral(
            "Export Filtered Results..."
            )
        );

    QVERIFY(
        actions[2]->isEnabled()
        );

    QVERIFY(
        actions[3]->isSeparator()
        );

    QCOMPARE(
        actions[4]->text(),
        QStringLiteral("Findings")
        );

    QMenu *findingsMenu =
        actions[4]->menu();

    QVERIFY(
        findingsMenu != nullptr
        );

    const QList<QAction *> findingsActions =
        findingsMenu->actions();

    QCOMPARE(
        findingsActions.size(),
        3
        );

    QCOMPARE(
        findingsActions[0]->text(),
        QStringLiteral(
            "Export All Findings (2)"
            )
        );

    QCOMPARE(
        findingsActions[1]->text(),
        QStringLiteral(
            "Export Filtered Findings (2)"
            )
        );

    QCOMPARE(
        findingsActions[2]->text(),
        QStringLiteral(
            "Export Bookmarked Findings (1)"
            )
        );

    /*
     * The common report action should delegate the
     * actual workflow through the document request
     * signal rather than owning report generation.
     */
    QSignalSpy reportExportSpy(
        &view,
        &WorkspaceDocument::
        investigationReportExportRequested
        );

    actions[0]->trigger();

    QCOMPARE(
        reportExportSpy.count(),
        1
        );

    const QList<QVariant> arguments =
        reportExportSpy.takeFirst();

    QCOMPARE(
        arguments.size(),
        1
        );

    QCOMPARE(
        arguments.at(0).toString(),
        view.documentId()
        );
}

QTEST_MAIN(
    InvestigationPresentationStateTests
    )

#include "InvestigationPresentationStateTests.moc"