#include <QtTest/QtTest>

#include <memory>
#include <utility>

#include "../src/exporting/InvestigationReportSnapshotBuilder.h"

#include "../src/workspace/InvestigationComparisonSnapshot.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{

QDateTime timestamp(
    int secondsFromStart = 0
    )
{
    return QDateTime::fromString(
               QStringLiteral(
                   "2026-09-03T13:00:00.000Z"
                   ),
               Qt::ISODateWithMs
               )
        .addSecs(secondsFromStart);
}

InvestigationRecord makeRecord(
    const QString &recordId,
    const QString &sourceName,
    qint64 recordNumber,
    int secondsFromStart = 0
    )
{
    InvestigationRecord record;

    record.recordId = recordId;
    record.timestamp = timestamp(secondsFromStart);
    record.severity = RecordSeverity::Warning;
    record.subsystem = QStringLiteral("Test");
    record.eventCode = QStringLiteral("TEST_EVENT");
    record.message =
        QStringLiteral("Message for %1")
            .arg(recordId);

    record.source.sourcePath =
        QStringLiteral("C:/private/%1")
            .arg(sourceName);

    record.source.sourceName =
        sourceName;

    record.source.recordNumber =
        recordNumber;

    return record;
}

std::unique_ptr<InvestigationSession>
makeSession(
    const QString &sessionId,
    const QString &sourceName,
    const QString &importerId,
    QVector<InvestigationRecord> records
    )
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("%1 profile")
            .arg(importerId);

    profile.importerId =
        importerId;

    ImportResult result;

    result.records =
        std::move(records);

    result.processedRecordCount =
        result.records.size();

    return std::make_unique<InvestigationSession>(
        sessionId,
        QStringLiteral("C:/private/%1")
            .arg(sourceName),
        std::move(profile),
        std::move(result)
        );
}

void addNote(
    InvestigationSession &session,
    const QString &recordId
    )
{
    session
        .investigationStateStore()
        ->setNote(
            recordId,
            QStringLiteral("Evidence note")
            );
}

InvestigationComparisonSnapshot makeComparison(
    const QString &comparisonId,
    const QString &baselineSessionId,
    const QString &baselineName,
    const QString &comparisonSessionId,
    const QString &comparisonName,
    qint64 baselineCount,
    qint64 comparisonCount
    )
{
    InvestigationComparisonSourceSnapshot baseline;

    baseline.sessionId =
        baselineSessionId;

    baseline.sourceMetadata.sourceName =
        baselineName;

    InvestigationComparisonSourceSnapshot comparison;

    comparison.sessionId =
        comparisonSessionId;

    comparison.sourceMetadata.sourceName =
        comparisonName;

    InvestigationSessionComparison analysis;

    analysis.totalRecords.baselineCount =
        baselineCount;

    analysis.totalRecords.comparisonCount =
        comparisonCount;

    return InvestigationComparisonSnapshot(
        comparisonId,
        std::move(baseline),
        std::move(comparison),
        std::nullopt,
        std::move(analysis)
        );
}

}

class InvestigationReportSnapshotBuilderTests
    : public QObject
{
    Q_OBJECT

private slots:
    void buildsSingleSessionReport();
    void buildsHeterogeneousMultiSourceReport();
    void preservesImmutableComparisonAnalysis();
    void buildsMixedReportWithoutImplicitDependencies();
};

void InvestigationReportSnapshotBuilderTests::
    buildsSingleSessionReport()
{
    auto session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("gateway.jsonl"),
            QStringLiteral("json-lines"),
            {
                makeRecord(
                    QStringLiteral("gateway-1"),
                    QStringLiteral("gateway.jsonl"),
                    1
                    )
            }
            );

    addNote(
        *session,
        QStringLiteral("gateway-1")
        );

    InvestigationReportConfiguration configuration;

    configuration.title =
        QStringLiteral("Gateway Investigation");

    configuration.context =
        QStringLiteral("Intermittent request failure");

    configuration.sessionIds = {
        QStringLiteral("gateway")
    };

    configuration.includeSupportingEvidence =
        true;

    configuration.includeTechnicalAppendix =
        false;

    const QDateTime generatedAt =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-03T14:23:45.000Z"
                ),
            Qt::ISODateWithMs
            );

    InvestigationReportSnapshotBuilder builder;

    const InvestigationReportSnapshot snapshot =
        builder.build(
            configuration,
            {
                {
                    session.get(),
                    QStringLiteral("Gateway")
                }
            },
            {},
            generatedAt
            );

    QCOMPARE(
        snapshot.title,
        QStringLiteral("Gateway Investigation")
        );

    QCOMPARE(
        snapshot.context,
        QStringLiteral(
            "Intermittent request failure"
            )
        );

    QCOMPARE(
        snapshot.generatedAtUtc,
        generatedAt
        );

    QCOMPARE(snapshot.sessions.size(), 1);
    QVERIFY(snapshot.comparisons.isEmpty());

    QCOMPARE(
        snapshot.sessions.front().source.sessionId,
        QStringLiteral("gateway")
        );

    QCOMPARE(
        snapshot.sessions.front().source.sourceName,
        QStringLiteral("gateway.jsonl")
        );

    QCOMPARE(
        snapshot.sourceTimeCoverage.size(),
        1
        );

    QCOMPARE(
        snapshot.crossSourceChronology.size(),
        1
        );

    /*
     * The root snapshot remains shareable just like
     * its underlying session snapshot.
     */
    QVERIFY(
        snapshot
            .sessions
            .front()
            .evidenceRecords
            .front()
            .record
            .source
            .sourcePath
            .isEmpty()
        );
}

void InvestigationReportSnapshotBuilderTests::
    buildsHeterogeneousMultiSourceReport()
{
    auto controller =
        makeSession(
            QStringLiteral("controller"),
            QStringLiteral("controller.xml"),
            QStringLiteral("xml"),
            {
                /*
                 * Deliberately store record 2 first.
                 * Cross-source chronology should use
                 * source record number to break equal
                 * timestamps deterministically.
                 */
                makeRecord(
                    QStringLiteral("controller-2"),
                    QStringLiteral("controller.xml"),
                    2
                    ),
                makeRecord(
                    QStringLiteral("controller-1"),
                    QStringLiteral("controller.xml"),
                    1
                    )
            }
            );

    auto gateway =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("gateway.jsonl"),
            QStringLiteral("json-lines"),
            {
                makeRecord(
                    QStringLiteral("gateway-1"),
                    QStringLiteral("gateway.jsonl"),
                    1
                    )
            }
            );

    addNote(
        *controller,
        QStringLiteral("controller-1")
        );

    addNote(
        *controller,
        QStringLiteral("controller-2")
        );

    addNote(
        *gateway,
        QStringLiteral("gateway-1")
        );

    InvestigationReportConfiguration configuration;

    /*
     * Configuration order, rather than available-input
     * order, defines report source order.
     */
    configuration.sessionIds = {
        QStringLiteral("controller"),
        QStringLiteral("gateway")
    };

    InvestigationReportSnapshotBuilder builder;

    const InvestigationReportSnapshot snapshot =
        builder.build(
            configuration,
            {
                {
                    gateway.get(),
                    QStringLiteral("Gateway")
                },
                {
                    controller.get(),
                    QStringLiteral("Controller")
                }
            },
            {},
            timestamp(100)
            );

    QCOMPARE(snapshot.sessions.size(), 2);

    QCOMPARE(
        snapshot.sessions.at(0).source.sessionId,
        QStringLiteral("controller")
        );

    QCOMPARE(
        snapshot.sessions.at(1).source.sessionId,
        QStringLiteral("gateway")
        );

    QCOMPARE(
        snapshot
            .sessions
            .at(0)
            .source
            .importerId,
        QStringLiteral("xml")
        );

    QCOMPARE(
        snapshot
            .sessions
            .at(1)
            .source
            .importerId,
        QStringLiteral("json-lines")
        );

    QCOMPARE(
        snapshot.sourceTimeCoverage.size(),
        2
        );

    QCOMPARE(
        snapshot.crossSourceChronology.size(),
        3
        );

    /*
     * Every evidence record has the same timestamp.
     *
     * Tie order:
     *   1. selected session order
     *   2. source record number
     *   3. stable record ID
     */
    QCOMPARE(
        snapshot
            .crossSourceChronology
            .at(0)
            .recordId,
        QStringLiteral("controller-1")
        );

    QCOMPARE(
        snapshot
            .crossSourceChronology
            .at(1)
            .recordId,
        QStringLiteral("controller-2")
        );

    QCOMPARE(
        snapshot
            .crossSourceChronology
            .at(2)
            .recordId,
        QStringLiteral("gateway-1")
        );
}

void InvestigationReportSnapshotBuilderTests::
    preservesImmutableComparisonAnalysis()
{
    /*
     * Live sessions intentionally contain only one
     * record each.
     */
    auto baseline =
        makeSession(
            QStringLiteral("baseline"),
            QStringLiteral("known-good.jsonl"),
            QStringLiteral("json-lines"),
            {
                makeRecord(
                    QStringLiteral("baseline-1"),
                    QStringLiteral("known-good.jsonl"),
                    1
                    )
            }
            );

    auto degraded =
        makeSession(
            QStringLiteral("degraded"),
            QStringLiteral("degraded.jsonl"),
            QStringLiteral("json-lines"),
            {
                makeRecord(
                    QStringLiteral("degraded-1"),
                    QStringLiteral("degraded.jsonl"),
                    1
                    )
            }
            );

    /*
     * But the immutable comparison was captured from
     * a different point in time with 100 vs 130
     * records. The report must preserve those values,
     * not recalculate using the current sessions.
     */
    InvestigationComparisonSnapshot comparison =
        makeComparison(
            QStringLiteral("known-good-to-degraded"),
            QStringLiteral("baseline"),
            QStringLiteral("known-good.jsonl"),
            QStringLiteral("degraded"),
            QStringLiteral("degraded.jsonl"),
            100,
            130
            );

    InvestigationReportConfiguration configuration;

    configuration.sessionIds = {
        QStringLiteral("baseline"),
        QStringLiteral("degraded")
    };

    configuration.comparisonIds = {
        QStringLiteral("known-good-to-degraded")
    };

    InvestigationReportSnapshotBuilder builder;

    const InvestigationReportSnapshot snapshot =
        builder.build(
            configuration,
            {
                {
                    baseline.get(),
                    QStringLiteral("Known Good")
                },
                {
                    degraded.get(),
                    QStringLiteral("Degraded")
                }
            },
            {
                {
                    &comparison,
                    QStringLiteral(
                        "Known Good → Degraded"
                        )
                }
            },
            timestamp(200)
            );

    QCOMPARE(snapshot.sessions.size(), 2);
    QCOMPARE(snapshot.comparisons.size(), 1);

    const InvestigationReportComparisonSnapshot
        &captured =
        snapshot.comparisons.front();

    QCOMPARE(
        captured.baselineSessionId,
        QStringLiteral("baseline")
        );

    QCOMPARE(
        captured.comparisonSessionId,
        QStringLiteral("degraded")
        );

    QCOMPARE(
        captured.analysis.totalRecords.baselineCount,
        100
        );

    QCOMPARE(
        captured.analysis.totalRecords.comparisonCount,
        130
        );

    QCOMPARE(
        captured.analysis.totalRecords.delta(),
        30
        );

    /*
     * Current session sizes prove the comparison was
     * not recalculated during report assembly.
     */
    QCOMPARE(
        snapshot
            .sessions
            .at(0)
            .recordContext
            .totalRecordCount,
        1
        );

    QCOMPARE(
        snapshot
            .sessions
            .at(1)
            .recordContext
            .totalRecordCount,
        1
        );
}

void InvestigationReportSnapshotBuilderTests::
    buildsMixedReportWithoutImplicitDependencies()
{
    auto gateway =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("gateway.jsonl"),
            QStringLiteral("json-lines"),
            {
                makeRecord(
                    QStringLiteral("gateway-1"),
                    QStringLiteral("gateway.jsonl"),
                    1
                    )
            }
            );

    auto scheduler =
        makeSession(
            QStringLiteral("scheduler"),
            QStringLiteral("scheduler.csv"),
            QStringLiteral("csv"),
            {
                makeRecord(
                    QStringLiteral("scheduler-1"),
                    QStringLiteral("scheduler.csv"),
                    1,
                    5
                    )
            }
            );

    auto controller =
        makeSession(
            QStringLiteral("controller"),
            QStringLiteral("controller.xml"),
            QStringLiteral("xml"),
            {
                makeRecord(
                    QStringLiteral("controller-1"),
                    QStringLiteral("controller.xml"),
                    1,
                    10
                    )
            }
            );

    InvestigationComparisonSnapshot comparison =
        makeComparison(
            QStringLiteral("gateway-comparison"),
            QStringLiteral("gateway"),
            QStringLiteral("gateway.jsonl"),
            QStringLiteral("degraded-gateway"),
            QStringLiteral("degraded-gateway.jsonl"),
            80,
            95
            );

    InvestigationReportConfiguration configuration;

    /*
     * scheduler/controller are heterogeneous,
     * independent investigation sources.
     *
     * gateway participates in a comparison whose
     * degraded-gateway source is not selected/open
     * as a full session section. That remains valid.
     */
    configuration.sessionIds = {
        QStringLiteral("scheduler"),
        QStringLiteral("gateway"),
        QStringLiteral("scheduler"),
        QStringLiteral("controller"),
        QStringLiteral("missing-session")
    };

    configuration.comparisonIds = {
        QStringLiteral("gateway-comparison"),
        QStringLiteral("gateway-comparison"),
        QStringLiteral("missing-comparison")
    };

    InvestigationReportSnapshotBuilder builder;

    const InvestigationReportSnapshot snapshot =
        builder.build(
            configuration,
            {
                {
                    gateway.get(),
                    QStringLiteral("Gateway")
                },
                {
                    scheduler.get(),
                    QStringLiteral("Scheduler")
                },
                {
                    controller.get(),
                    QStringLiteral("Controller")
                }
            },
            {
                {
                    &comparison,
                    QStringLiteral(
                        "Gateway Known Good → Degraded"
                        )
                }
            },
            timestamp(300)
            );

    /*
     * Duplicate IDs are deduplicated, unresolved IDs
     * are ignored, and configuration order is retained.
     */
    QCOMPARE(snapshot.sessions.size(), 3);

    QCOMPARE(
        snapshot.sessions.at(0).source.sessionId,
        QStringLiteral("scheduler")
        );

    QCOMPARE(
        snapshot.sessions.at(1).source.sessionId,
        QStringLiteral("gateway")
        );

    QCOMPARE(
        snapshot.sessions.at(2).source.sessionId,
        QStringLiteral("controller")
        );

    QCOMPARE(snapshot.comparisons.size(), 1);

    QCOMPARE(
        snapshot
            .comparisons
            .front()
            .comparisonId,
        QStringLiteral("gateway-comparison")
        );

    /*
     * The builder does not silently add the comparison's
     * degraded-gateway dependency as a full session.
     * Dependency selection belongs to the export dialog.
     */
    for (const InvestigationReportSessionSnapshot
             &session : snapshot.sessions) {
        QVERIFY(
            session.source.sessionId
            != QStringLiteral("degraded-gateway")
            );
    }
}

QTEST_APPLESS_MAIN(
    InvestigationReportSnapshotBuilderTests
    )

#include "InvestigationReportSnapshotBuilderTests.moc"