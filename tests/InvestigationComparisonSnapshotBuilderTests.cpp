#include <QtTest>

#include "../src/workspace/InvestigationComparisonSnapshotBuilder.h"

namespace
{
InvestigationRecord makeRecord(
    const QString &recordId,
    RecordSeverity severity,
    const QString &message = QString()
    )
{
    InvestigationRecord record;

    record.recordId =
        recordId;

    record.severity =
        severity;

    if (!message.isEmpty()) {
        record.message =
            message;
    }

    return record;
}

ImportResult makeResult(
    const QVector<InvestigationRecord> &records
    )
{
    ImportResult result;

    result.records =
        records;

    result.processedRecordCount =
        records.size();

    return result;
}
}

class InvestigationComparisonSnapshotBuilderTests
    : public QObject
{
    Q_OBJECT

private slots:
    void usesCompleteSessionRecordsDespiteFilters();
    void copiesSourceIdentityAndMetadata();
    void snapshotsRemainUnchangedAfterSourceReload();
    void preservesBurstRequestState();
    void assignsDistinctComparisonIds();
};

void InvestigationComparisonSnapshotBuilderTests::
    usesCompleteSessionRecordsDespiteFilters()
{
    ImportProfile profile;

    InvestigationSession baseline(
        QStringLiteral("baseline.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("baseline-1"),
                RecordSeverity::Info,
                QStringLiteral("Normal operation")
                ),
            makeRecord(
                QStringLiteral("baseline-2"),
                RecordSeverity::Error,
                QStringLiteral("Failure detected")
                )
        })
        );

    InvestigationSession comparison(
        QStringLiteral("comparison.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("comparison-1"),
                RecordSeverity::Warning,
                QStringLiteral("Warning detected")
                )
        })
        );

    baseline
        .investigationController()
        ->setFilters(
            QString(),
            QString(),
            QStringLiteral("Failure")
            );

    QCOMPARE(
        baseline
            .investigationController()
            ->recordsForAnalysis()
            .size(),
        1
        );

    InvestigationComparisonSnapshotBuilder builder;

    const InvestigationComparisonSnapshot snapshot =
        builder.build(
            baseline,
            comparison
            );

    QCOMPARE(
        snapshot.analysis()
            .totalRecords
            .baselineCount,
        2
        );

    QCOMPARE(
        snapshot.analysis()
            .totalRecords
            .comparisonCount,
        1
        );
}

void InvestigationComparisonSnapshotBuilderTests::
    copiesSourceIdentityAndMetadata()
{
    ImportProfile profile;

    InvestigationSession baseline(
        QStringLiteral("baseline.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("baseline-1"),
                RecordSeverity::Info
                )
        })
        );

    InvestigationSession comparison(
        QStringLiteral("comparison.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("comparison-1"),
                RecordSeverity::Info
                )
        })
        );

    InvestigationComparisonSnapshotBuilder builder;

    const InvestigationComparisonSnapshot snapshot =
        builder.build(
            baseline,
            comparison
            );

    QCOMPARE(
        snapshot.baselineSource().sessionId,
        baseline.id()
        );

    QCOMPARE(
        snapshot.comparisonSource().sessionId,
        comparison.id()
        );

    QCOMPARE(
        snapshot
            .baselineSource()
            .sourceMetadata
            .sourcePath,
        baseline
            .sourceMetadata()
            .sourcePath
        );

    QCOMPARE(
        snapshot
            .baselineSource()
            .sourceMetadata
            .sourceName,
        baseline
            .sourceMetadata()
            .sourceName
        );

    QCOMPARE(
        snapshot
            .comparisonSource()
            .sourceMetadata
            .sourcePath,
        comparison
            .sourceMetadata()
            .sourcePath
        );

    QCOMPARE(
        snapshot
            .comparisonSource()
            .sourceMetadata
            .sourceName,
        comparison
            .sourceMetadata()
            .sourceName
        );
}

void InvestigationComparisonSnapshotBuilderTests::
    snapshotsRemainUnchangedAfterSourceReload()
{
    ImportProfile profile;

    InvestigationSession baseline(
        QStringLiteral("baseline.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("baseline-1"),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral("baseline-2"),
                RecordSeverity::Warning
                )
        })
        );

    InvestigationSession comparison(
        QStringLiteral("comparison.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("comparison-1"),
                RecordSeverity::Error
                )
        })
        );

    InvestigationComparisonSnapshotBuilder builder;

    const InvestigationComparisonSnapshot snapshot =
        builder.build(
            baseline,
            comparison
            );

    QCOMPARE(
        snapshot.analysis()
            .totalRecords
            .baselineCount,
        2
        );

    baseline.reload(
        makeResult({
            makeRecord(
                QStringLiteral("replacement-1"),
                RecordSeverity::Critical
                )
        })
        );

    QCOMPARE(
        baseline.importedRecordCount(),
        1
        );

    QCOMPARE(
        snapshot.analysis()
            .totalRecords
            .baselineCount,
        2
        );
}

void InvestigationComparisonSnapshotBuilderTests::
    preservesBurstRequestState()
{
    ImportProfile profile;

    InvestigationSession baseline(
        QStringLiteral("baseline.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("baseline-1"),
                RecordSeverity::Warning
                )
        })
        );

    InvestigationSession comparison(
        QStringLiteral("comparison.jsonl"),
        profile,
        makeResult({
            makeRecord(
                QStringLiteral("comparison-1"),
                RecordSeverity::Error
                )
        })
        );

    InvestigationComparisonSnapshotBuilder builder;

    const InvestigationComparisonSnapshot
        withoutBursts =
        builder.build(
            baseline,
            comparison
            );

    QVERIFY(
        !withoutBursts
             .burstComparisonRequested()
        );

    QVERIFY(
        !withoutBursts
             .analysis()
             .bursts
             .has_value()
        );

    BurstDetectionSettings settings;

    settings.windowMilliseconds =
        10'000;

    settings.elevatedEventThreshold =
        2;

    settings.errorCriticalThreshold =
        1;

    settings.mergeGapMilliseconds =
        1'000;

    const InvestigationComparisonSnapshot
        withBursts =
        builder.build(
            baseline,
            comparison,
            settings
            );

    QVERIFY(
        withBursts
            .burstComparisonRequested()
        );

    QVERIFY(
        withBursts
            .requestedBurstSettings()
            .has_value()
        );

    QVERIFY(
        withBursts
            .analysis()
            .bursts
            .has_value()
        );

    /*
     * Neither source has usable timestamp data.
     * Burst comparison was requested and performed,
     * but the data is not comparable.
     */
    QVERIFY(
        !withBursts
             .analysis()
             .bursts
             ->comparable()
        );

    QCOMPARE(
        withBursts
            .analysis()
            .bursts
            ->settings
            .windowMilliseconds,
        settings.windowMilliseconds
        );

    QCOMPARE(
        withBursts
            .analysis()
            .bursts
            ->settings
            .elevatedEventThreshold,
        settings.elevatedEventThreshold
        );
}

void InvestigationComparisonSnapshotBuilderTests::
    assignsDistinctComparisonIds()
{
    ImportProfile profile;

    InvestigationSession baseline(
        QStringLiteral("baseline.jsonl"),
        profile,
        makeResult({})
        );

    InvestigationSession comparison(
        QStringLiteral("comparison.jsonl"),
        profile,
        makeResult({})
        );

    InvestigationComparisonSnapshotBuilder builder;

    const InvestigationComparisonSnapshot first =
        builder.build(
            baseline,
            comparison
            );

    const InvestigationComparisonSnapshot second =
        builder.build(
            baseline,
            comparison
            );

    QVERIFY(
        !first.id().isEmpty()
        );

    QVERIFY(
        !second.id().isEmpty()
        );

    QVERIFY(
        first.id()
        != second.id()
        );
}

QTEST_MAIN(
    InvestigationComparisonSnapshotBuilderTests
    )

#include "InvestigationComparisonSnapshotBuilderTests.moc"