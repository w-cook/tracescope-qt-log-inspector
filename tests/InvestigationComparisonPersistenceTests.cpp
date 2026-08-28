#include <QtTest>

#include <utility>

#include "../src/workspace/InvestigationComparisonPersistence.h"

class InvestigationComparisonPersistenceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void comparisonSnapshotRoundTrips();
};

void InvestigationComparisonPersistenceTests::
    comparisonSnapshotRoundTrips()
{
    InvestigationComparisonSourceSnapshot
        baselineSource;

    baselineSource.sessionId =
        QStringLiteral("baseline-session");

    baselineSource.sourceMetadata.sourcePath =
        QStringLiteral(
            "/logs/baseline.jsonl"
            );

    baselineSource.sourceMetadata.sourceName =
        QStringLiteral(
            "baseline.jsonl"
            );

    baselineSource.sourceMetadata.sourceSizeBytes =
        12345;

    baselineSource.sourceMetadata
        .sourceLastModified =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:00:00Z"
                ),
            Qt::ISODate
            );

    baselineSource.sourceMetadata.importedAtUtc =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:05:00Z"
                ),
            Qt::ISODate
            );

    InvestigationComparisonSourceSnapshot
        comparisonSource;

    comparisonSource.sessionId =
        QStringLiteral(
            "comparison-session"
            );

    comparisonSource.sourceMetadata.sourcePath =
        QStringLiteral(
            "/logs/comparison.jsonl"
            );

    comparisonSource.sourceMetadata.sourceName =
        QStringLiteral(
            "comparison.jsonl"
            );

    comparisonSource.sourceMetadata
        .sourceSizeBytes =
        23456;

    comparisonSource.sourceMetadata
        .sourceLastModified =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00Z"
                ),
            Qt::ISODate
            );

    comparisonSource.sourceMetadata.importedAtUtc =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:05:00Z"
                ),
            Qt::ISODate
            );

    BurstDetectionSettings burstSettings;

    burstSettings.windowMilliseconds =
        45000;

    burstSettings.elevatedEventThreshold =
        7;

    burstSettings.errorCriticalThreshold =
        4;

    burstSettings.mergeGapMilliseconds =
        8000;

    InvestigationSessionComparison analysis;

    analysis.totalRecords.baselineCount =
        100;

    analysis.totalRecords.comparisonCount =
        135;

    analysis.baselineTiming
        .timestampedRecordCount =
        100;

    analysis.baselineTiming
        .durationMilliseconds =
        60000;

    analysis.baselineTiming.recordsPerMinute =
        100.0;

    analysis.comparisonTiming
        .timestampedRecordCount =
        135;

    analysis.comparisonTiming
        .durationMilliseconds =
        90000;

    analysis.comparisonTiming.recordsPerMinute =
        90.0;

    InvestigationSeverityDifference
        severityDifference;

    severityDifference.severity =
        RecordSeverity::Warning;

    severityDifference.baselineCount =
        4;

    severityDifference.comparisonCount =
        11;

    analysis.severity.differences.append(
        severityDifference
        );

    InvestigationValueDifference
        eventCodeDifference;

    eventCodeDifference.value =
        QStringLiteral("EVT-100");

    eventCodeDifference.baselineCount =
        3;

    eventCodeDifference.comparisonCount =
        9;

    analysis.eventCodes.differences.append(
        eventCodeDifference
        );

    InvestigationBurstComparison
        burstComparison;

    burstComparison.settings =
        burstSettings;

    burstComparison.baseline.available =
        true;

    burstComparison.baseline.burstCount =
        2;

    burstComparison.comparison.available =
        true;

    burstComparison.comparison.burstCount =
        5;

    analysis.bursts =
        burstComparison;

    InvestigationComparisonSnapshot original(
        QStringLiteral("comparison-123"),
        std::move(baselineSource),
        std::move(comparisonSource),
        burstSettings,
        std::move(analysis)
        );

    const PersistedInvestigationComparison
        persisted =
        InvestigationComparisonPersistence::
        capture(original);

    QCOMPARE(
        persisted.comparisonId,
        QStringLiteral("comparison-123")
        );

    QCOMPARE(
        persisted.baselineSource.sessionId,
        QStringLiteral("baseline-session")
        );

    QCOMPARE(
        persisted.comparisonSource.sessionId,
        QStringLiteral(
            "comparison-session"
            )
        );

    QVERIFY(
        persisted.requestedBurstSettings
            .has_value()
        );

    QCOMPARE(
        persisted.analysis
            .totalRecords
            .baselineCount,
        100
        );

    QCOMPARE(
        persisted.analysis
            .totalRecords
            .comparisonCount,
        135
        );

    const InvestigationComparisonSnapshot
        restored =
        InvestigationComparisonPersistence::
        restore(persisted);

    QCOMPARE(
        restored.id(),
        original.id()
        );

    QCOMPARE(
        restored.baselineSource().sessionId,
        QStringLiteral("baseline-session")
        );

    QCOMPARE(
        restored.baselineSource()
            .sourceMetadata
            .sourcePath,
        QStringLiteral(
            "/logs/baseline.jsonl"
            )
        );

    QCOMPARE(
        restored.comparisonSource().sessionId,
        QStringLiteral(
            "comparison-session"
            )
        );

    QCOMPARE(
        restored.comparisonSource()
            .sourceMetadata
            .sourceSizeBytes,
        23456
        );

    QVERIFY(
        restored.requestedBurstSettings()
            .has_value()
        );

    QCOMPARE(
        restored.requestedBurstSettings()
            ->windowMilliseconds,
        45000
        );

    QCOMPARE(
        restored.analysis()
            .totalRecords
            .baselineCount,
        100
        );

    QCOMPARE(
        restored.analysis()
            .totalRecords
            .comparisonCount,
        135
        );

    QCOMPARE(
        restored.analysis()
            .severity
            .differences
            .size(),
        1
        );

    QCOMPARE(
        restored.analysis()
            .severity
            .differences
            .first()
            .comparisonCount,
        11
        );

    QCOMPARE(
        restored.analysis()
            .eventCodes
            .differences
            .first()
            .value,
        QStringLiteral("EVT-100")
        );

    QVERIFY(
        restored.analysis().bursts.has_value()
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .burstCount,
        5
        );
}

QTEST_MAIN(
    InvestigationComparisonPersistenceTests
    )

#include "InvestigationComparisonPersistenceTests.moc"