#include <QtTest>

#include <QCoreApplication>

#include <utility>

#include "../src/domain/InvestigationRecord.h"
#include "../src/importing/ImportProfile.h"
#include "../src/importing/ImportResult.h"
#include "../src/ui/investigation/InvestigationEventDetailPanel.h"
#include "../src/ui/investigation/InvestigationEventPanel.h"
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

void processUi()
{
    QCoreApplication::processEvents();
}

}

class InvestigationPresentationStateTests
    : public QObject
{
    Q_OBJECT

private slots:
    void eventTablePresentationRoundTrips();
    void eventDetailScrollRoundTrips();
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
     * applied/clamped widget geometry. This is the
     * exact state that a workspace Save operation
     * would persist.
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
        saved.columnWidths,
        desired.columnWidths
        );

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

QTEST_MAIN(
    InvestigationPresentationStateTests
    )

#include "InvestigationPresentationStateTests.moc"