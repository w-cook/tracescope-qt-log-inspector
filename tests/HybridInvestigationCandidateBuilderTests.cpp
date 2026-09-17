#include <QtTest>

#include "../src/domain/RecordIdentity.h"
#include "../src/workspace/HybridInvestigationCandidateBuilder.h"

namespace
{
InvestigationRecord makeRecord(
    const QString &sourcePath,
    qint64 recordNumber,
    quint64 sourceGeneration,
    const QString &rawSource
    )
{
    InvestigationRecord record;

    record.source.sourcePath =
        sourcePath;

    record.source.sourceName =
        QStringLiteral(
            "live.jsonl"
            );

    record.source.recordNumber =
        recordNumber;

    record.source.sourceGeneration =
        sourceGeneration;

    record.rawSource =
        rawSource;

    record.recordId =
        createStableRecordIdentity(
            record.source,
            record.rawSource
            );

    return record;
}
}

class HybridInvestigationCandidateBuilderTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sameGenerationReplayRemovesSnapshotOverlap();
    void newGenerationReplayPreservesHistoricalEvidence();
    void replayRecordsRetainReplayOrder();
    void replayDiagnosticUsesTargetGeneration();
    void cancelledReplayIsRejected();
};

void HybridInvestigationCandidateBuilderTests::
    sameGenerationReplayRemovesSnapshotOverlap()
{
    const QString sourcePath =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    InvestigationSessionSnapshot snapshot;

    snapshot.records.append(
        makeRecord(
            sourcePath,
            1,
            3,
            QStringLiteral(
                "record one"
                )
            )
        );

    snapshot.records.append(
        makeRecord(
            sourcePath,
            2,
            3,
            QStringLiteral(
                "record two"
                )
            )
        );

    snapshot.processedRecordCount = 2;

    ImportResult replay;

    /*
     * Static import produces generation-zero records.
     * The builder must rebase these to generation 3
     * before comparing their identities.
     */
    replay.records.append(
        makeRecord(
            sourcePath,
            1,
            0,
            QStringLiteral(
                "record one"
                )
            )
        );

    replay.records.append(
        makeRecord(
            sourcePath,
            2,
            0,
            QStringLiteral(
                "record two"
                )
            )
        );

    replay.records.append(
        makeRecord(
            sourcePath,
            3,
            0,
            QStringLiteral(
                "record three"
                )
            )
        );

    replay.processedRecordCount = 3;

    HybridInvestigationCandidateBuilder
        builder;

    const HybridInvestigationCandidateBuildResult
        result =
        builder.build(
            snapshot,
            std::move(replay),
            3
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.duplicateReplayRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.appendedReplayRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importResult.records.size(),
        3
        );

    QCOMPARE(
        result.importResult.processedRecordCount,
        qint64(3)
        );

    /*
     * Snapshot order is the durable investigation
     * baseline.
     */
    QCOMPARE(
        result
            .importResult
            .records
            .at(0)
            .rawSource,
        QStringLiteral(
            "record one"
            )
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(1)
            .rawSource,
        QStringLiteral(
            "record two"
            )
        );

    const InvestigationRecord &appended =
        result
            .importResult
            .records
            .at(2);

    QCOMPARE(
        appended.rawSource,
        QStringLiteral(
            "record three"
            )
        );

    QCOMPARE(
        appended.source.sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        appended.recordId,
        createStableRecordIdentity(
            appended.source,
            appended.rawSource
            )
        );
}

void HybridInvestigationCandidateBuilderTests::
    newGenerationReplayPreservesHistoricalEvidence()
{
    const QString sourcePath =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    InvestigationSessionSnapshot snapshot;

    const InvestigationRecord historical =
        makeRecord(
            sourcePath,
            1,
            3,
            QStringLiteral(
                "old generation"
                )
            );

    snapshot.records.append(
        historical
        );

    snapshot.processedRecordCount = 1;

    ImportResult replay;

    /*
     * The replacement happens to reuse physical record
     * number one. Generation 4 must still make it a new
     * investigation record.
     */
    replay.records.append(
        makeRecord(
            sourcePath,
            1,
            0,
            QStringLiteral(
                "new generation"
                )
            )
        );

    replay.processedRecordCount = 1;

    HybridInvestigationCandidateBuilder
        builder;

    const HybridInvestigationCandidateBuildResult
        result =
        builder.build(
            snapshot,
            std::move(replay),
            4
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.duplicateReplayRecordCount,
        qint64(0)
        );

    QCOMPARE(
        result.appendedReplayRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QCOMPARE(
        result
            .importResult
            .records
            .first()
            .recordId,
        historical.recordId
        );

    const InvestigationRecord &replacement =
        result
            .importResult
            .records
            .last();

    QCOMPARE(
        replacement.source.sourceGeneration,
        quint64(4)
        );

    QVERIFY(
        replacement.recordId
        != historical.recordId
        );
}

void HybridInvestigationCandidateBuilderTests::
    replayRecordsRetainReplayOrder()
{
    const QString sourcePath =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    InvestigationSessionSnapshot snapshot;

    snapshot.records.append(
        makeRecord(
            sourcePath,
            1,
            2,
            QStringLiteral(
                "baseline"
                )
            )
        );

    snapshot.processedRecordCount = 1;

    ImportResult replay;

    replay.records.append(
        makeRecord(
            sourcePath,
            1,
            0,
            QStringLiteral(
                "baseline"
                )
            )
        );

    replay.records.append(
        makeRecord(
            sourcePath,
            2,
            0,
            QStringLiteral(
                "new two"
                )
            )
        );

    replay.records.append(
        makeRecord(
            sourcePath,
            3,
            0,
            QStringLiteral(
                "new three"
                )
            )
        );

    replay.processedRecordCount = 3;

    HybridInvestigationCandidateBuilder
        builder;

    const auto result =
        builder.build(
            snapshot,
            std::move(replay),
            2
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.importResult.records.size(),
        3
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(1)
            .rawSource,
        QStringLiteral(
            "new two"
            )
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(2)
            .rawSource,
        QStringLiteral(
            "new three"
            )
        );
}

void HybridInvestigationCandidateBuilderTests::
    replayDiagnosticUsesTargetGeneration()
{
    InvestigationSessionSnapshot snapshot;

    ImportResult replay;

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral(
            "TEST"
            );

    diagnostic.message =
        QStringLiteral(
            "test diagnostic"
            );

    RecordSourceMetadata source;

    source.sourcePath =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    source.recordNumber = 5;
    source.sourceGeneration = 0;

    diagnostic.source =
        source;

    replay.diagnostics.append(
        diagnostic
        );

    HybridInvestigationCandidateBuilder
        builder;

    const auto result =
        builder.build(
            snapshot,
            std::move(replay),
            7
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.importResult.diagnostics.size(),
        1
        );

    QVERIFY(
        result
            .importResult
            .diagnostics
            .first()
            .source
            .has_value()
        );

    QCOMPARE(
        result
            .importResult
            .diagnostics
            .first()
            .source
            ->sourceGeneration,
        quint64(7)
        );
}

void HybridInvestigationCandidateBuilderTests::
    cancelledReplayIsRejected()
{
    InvestigationSessionSnapshot snapshot;

    snapshot.records.append(
        makeRecord(
            QStringLiteral(
                "C:/logs/live.jsonl"
                ),
            1,
            3,
            QStringLiteral(
                "durable evidence"
                )
            )
        );

    snapshot.processedRecordCount = 1;

    ImportResult replay;

    replay.cancelled = true;

    HybridInvestigationCandidateBuilder
        builder;

    const auto result =
        builder.build(
            snapshot,
            std::move(replay),
            3
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        result.importResult.records.isEmpty()
        );
}

QTEST_MAIN(
    HybridInvestigationCandidateBuilderTests
    )

#include "HybridInvestigationCandidateBuilderTests.moc"