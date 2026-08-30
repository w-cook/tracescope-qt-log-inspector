#include <QtTest>

#include <utility>

#include "../src/workspace/InvestigationComparisonPersistence.h"
#include "../src/workspace/InvestigationComparisonPersistenceSerialization.h"

class InvestigationComparisonPersistenceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void comparisonSnapshotRoundTrips();
    void comparisonJsonRoundTrips();
    void missingPresentationStateUsesDefaults();
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

void InvestigationComparisonPersistenceTests::
    comparisonJsonRoundTrips()
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
                "2026-08-27T11:55:00Z"
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
                "2026-08-28T11:55:00Z"
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

    /*
     * Overall record counts.
     */
    analysis.totalRecords.baselineCount =
        100;

    analysis.totalRecords.comparisonCount =
        135;

    /*
     * Timing.
     */
    analysis.baselineTiming
        .timestampedRecordCount =
        100;

    analysis.baselineTiming.firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:00:00Z"
                ),
            Qt::ISODate
            );

    analysis.baselineTiming.lastTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:01:00Z"
                ),
            Qt::ISODate
            );

    analysis.baselineTiming
        .durationMilliseconds =
        60000;

    analysis.baselineTiming.recordsPerMinute =
        100.0;

    analysis.comparisonTiming
        .timestampedRecordCount =
        135;

    analysis.comparisonTiming.firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00Z"
                ),
            Qt::ISODate
            );

    analysis.comparisonTiming.lastTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:01:30Z"
                ),
            Qt::ISODate
            );

    analysis.comparisonTiming
        .durationMilliseconds =
        90000;

    analysis.comparisonTiming.recordsPerMinute =
        90.0;

    /*
     * Severity comparison.
     */
    analysis.severity
        .baselinePopulatedRecordCount =
        100;

    analysis.severity
        .comparisonPopulatedRecordCount =
        135;

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

    /*
     * Event-code comparison.
     */
    analysis.eventCodes
        .baselinePopulatedRecordCount =
        90;

    analysis.eventCodes
        .comparisonPopulatedRecordCount =
        125;

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

    /*
     * Elevated subsystem comparison.
     */
    analysis.elevatedSubsystems
        .baselinePopulatedRecordCount =
        12;

    analysis.elevatedSubsystems
        .comparisonPopulatedRecordCount =
        24;

    InvestigationValueDifference
        subsystemDifference;

    subsystemDifference.value =
        QStringLiteral("Backend");

    subsystemDifference.baselineCount =
        4;

    subsystemDifference.comparisonCount =
        10;

    analysis.elevatedSubsystems
        .differences
        .append(
            subsystemDifference
            );

    /*
     * Elevated entity comparison.
     */
    analysis.elevatedEntities
        .baselinePopulatedRecordCount =
        10;

    analysis.elevatedEntities
        .comparisonPopulatedRecordCount =
        20;

    InvestigationValueDifference
        entityDifference;

    entityDifference.value =
        QStringLiteral("node-4");

    entityDifference.baselineCount =
        2;

    entityDifference.comparisonCount =
        8;

    analysis.elevatedEntities
        .differences
        .append(
            entityDifference
            );

    /*
     * Categorical custom-field comparison.
     */
    InvestigationCategoricalCustomFieldComparison
        categoricalField;

    categoricalField.fieldName =
        QStringLiteral("Deployment");

    InvestigationValueDifference
        categoricalDifference;

    categoricalDifference.value =
        QStringLiteral("canary");

    categoricalDifference.baselineCount =
        0;

    categoricalDifference.comparisonCount =
        17;

    categoricalField.changedValues.append(
        categoricalDifference
        );

    analysis.customFields
        .categoricalFields
        .append(
            categoricalField
            );

    /*
     * Numeric custom-field comparison.
     */
    InvestigationNumericCustomFieldComparison
        numericField;

    numericField.fieldName =
        QStringLiteral("LatencyMs");

    numericField.baseline.populatedRecordCount =
        80;

    numericField.baseline.minimum =
        2.0;

    numericField.baseline.median =
        7.5;

    numericField.baseline.maximum =
        24.0;

    numericField.comparison
        .populatedRecordCount =
        110;

    numericField.comparison.minimum =
        3.0;

    numericField.comparison.median =
        18.5;

    numericField.comparison.maximum =
        61.0;

    analysis.customFields
        .numericFields
        .append(
            numericField
            );

    /*
     * Burst comparison.
     */
    InvestigationBurstComparison
        burstComparison;

    burstComparison.settings =
        burstSettings;

    burstComparison.baseline.available =
        true;

    burstComparison.baseline.burstCount =
        2;

    burstComparison.baseline
        .elevatedRecordCountInBursts =
        14;

    burstComparison.baseline
        .peakBurstElevatedCount =
        8;

    burstComparison.baseline
        .longestBurstDurationMilliseconds =
        17000;

    burstComparison.baseline
        .dominantSubsystem =
        InvestigationValueFrequency {
            QStringLiteral("Backend"),
            9
        };

    burstComparison.comparison.available =
        true;

    burstComparison.comparison.burstCount =
        5;

    burstComparison.comparison
        .elevatedRecordCountInBursts =
        31;

    burstComparison.comparison
        .peakBurstElevatedCount =
        14;

    burstComparison.comparison
        .longestBurstDurationMilliseconds =
        29000;

    burstComparison.comparison
        .dominantSubsystem =
        InvestigationValueFrequency {
            QStringLiteral("Backend"),
            18
        };

    burstComparison.comparison
        .dominantEventCode =
        InvestigationValueFrequency {
            QStringLiteral("EVT-100"),
            12
        };

    burstComparison.comparison
        .dominantEntity =
        InvestigationValueFrequency {
            QStringLiteral("node-4"),
            10
        };

    analysis.bursts =
        burstComparison;

    InvestigationComparisonSnapshot original(
        QStringLiteral("comparison-123"),
        std::move(baselineSource),
        std::move(comparisonSource),
        burstSettings,
        std::move(analysis)
        );

    InvestigationComparisonPresentationState
        presentationState;

    presentationState.scroll.horizontalValue =
        17;

    presentationState.scroll.verticalValue =
        340;

    const PersistedInvestigationComparison
        persisted =
        InvestigationComparisonPersistence::
        capture(
            original,
            presentationState
            );

    const InvestigationComparisonPersistenceSerializer
        serializer;

    const QJsonObject json =
        serializer.serialize(
            persisted
            );

    const ComparisonPersistenceDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.comparison.has_value());

    /*
     * Comparison presentation state.
     */
    QCOMPARE(
        result.comparison
            ->presentationState
            .scroll
            .horizontalValue,
        17
        );

    QCOMPARE(
        result.comparison
            ->presentationState
            .scroll
            .verticalValue,
        340
        );

    const InvestigationComparisonSnapshot
        restored =
        InvestigationComparisonPersistence::
        restore(
            *result.comparison
            );

    /*
     * Snapshot identity and orientation.
     */
    QCOMPARE(
        restored.id(),
        original.id()
        );

    QCOMPARE(
        restored.baselineSource().sessionId,
        QStringLiteral(
            "baseline-session"
            )
        );

    QCOMPARE(
        restored.comparisonSource().sessionId,
        QStringLiteral(
            "comparison-session"
            )
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
        restored.comparisonSource()
            .sourceMetadata
            .sourcePath,
        QStringLiteral(
            "/logs/comparison.jsonl"
            )
        );

    /*
     * Requested burst settings.
     */
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
        restored.requestedBurstSettings()
            ->elevatedEventThreshold,
        7
        );

    /*
     * Overall counts.
     */
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

    /*
     * Timing.
     */
    QCOMPARE(
        restored.analysis()
            .baselineTiming
            .timestampedRecordCount,
        100
        );

    QVERIFY(
        restored.analysis()
            .baselineTiming
            .firstTimestamp
            .has_value()
        );

    QCOMPARE(
        restored.analysis()
            .baselineTiming
            .firstTimestamp
            ->toUTC(),
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:00:00Z"
                ),
            Qt::ISODate
            )
        );

    QCOMPARE(
        restored.analysis()
            .comparisonTiming
            .durationMilliseconds,
        90000
        );

    QVERIFY(
        restored.analysis()
            .comparisonTiming
            .recordsPerMinute
            .has_value()
        );

    QCOMPARE(
        *restored.analysis()
             .comparisonTiming
             .recordsPerMinute,
        90.0
        );

    /*
     * Severity.
     */
    QCOMPARE(
        restored.analysis()
            .severity
            .differences
            .size(),
        1
        );

    QVERIFY(
        restored.analysis()
            .severity
            .differences
            .first()
            .severity
        == RecordSeverity::Warning
        );

    QCOMPARE(
        restored.analysis()
            .severity
            .differences
            .first()
            .comparisonCount,
        11
        );

    /*
     * Event codes.
     */
    QCOMPARE(
        restored.analysis()
            .eventCodes
            .differences
            .first()
            .value,
        QStringLiteral("EVT-100")
        );

    QCOMPARE(
        restored.analysis()
            .eventCodes
            .differences
            .first()
            .comparisonCount,
        9
        );

    /*
     * Elevated subsystems.
     */
    QCOMPARE(
        restored.analysis()
            .elevatedSubsystems
            .differences
            .first()
            .value,
        QStringLiteral("Backend")
        );

    QCOMPARE(
        restored.analysis()
            .elevatedSubsystems
            .differences
            .first()
            .comparisonCount,
        10
        );

    /*
     * Elevated entities.
     */
    QCOMPARE(
        restored.analysis()
            .elevatedEntities
            .differences
            .first()
            .value,
        QStringLiteral("node-4")
        );

    QCOMPARE(
        restored.analysis()
            .elevatedEntities
            .differences
            .first()
            .comparisonCount,
        8
        );

    /*
     * Categorical custom fields.
     */
    QCOMPARE(
        restored.analysis()
            .customFields
            .categoricalFields
            .size(),
        1
        );

    QCOMPARE(
        restored.analysis()
            .customFields
            .categoricalFields
            .first()
            .fieldName,
        QStringLiteral("Deployment")
        );

    QCOMPARE(
        restored.analysis()
            .customFields
            .categoricalFields
            .first()
            .changedValues
            .first()
            .value,
        QStringLiteral("canary")
        );

    /*
     * Numeric custom fields.
     */
    QCOMPARE(
        restored.analysis()
            .customFields
            .numericFields
            .size(),
        1
        );

    QCOMPARE(
        restored.analysis()
            .customFields
            .numericFields
            .first()
            .fieldName,
        QStringLiteral("LatencyMs")
        );

    QCOMPARE(
        restored.analysis()
            .customFields
            .numericFields
            .first()
            .baseline
            .median,
        7.5
        );

    QCOMPARE(
        restored.analysis()
            .customFields
            .numericFields
            .first()
            .comparison
            .median,
        18.5
        );

    /*
     * Burst comparison and dominant values.
     */
    QVERIFY(
        restored.analysis()
            .bursts
            .has_value()
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->baseline
            .burstCount,
        2
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .burstCount,
        5
        );

    QVERIFY(
        restored.analysis()
            .bursts
            ->comparison
            .dominantSubsystem
            .has_value()
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .dominantSubsystem
            ->value,
        QStringLiteral("Backend")
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .dominantSubsystem
            ->count,
        18
        );

    QVERIFY(
        restored.analysis()
            .bursts
            ->comparison
            .dominantEventCode
            .has_value()
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .dominantEventCode
            ->value,
        QStringLiteral("EVT-100")
        );

    QVERIFY(
        restored.analysis()
            .bursts
            ->comparison
            .dominantEntity
            .has_value()
        );

    QCOMPARE(
        restored.analysis()
            .bursts
            ->comparison
            .dominantEntity
            ->value,
        QStringLiteral("node-4")
        );
}

void InvestigationComparisonPersistenceTests::
    missingPresentationStateUsesDefaults()
{
    PersistedInvestigationComparison
        comparison;

    comparison.comparisonId =
        QStringLiteral(
            "comparison-legacy"
            );

    comparison.baselineSource.sessionId =
        QStringLiteral(
            "baseline-session"
            );

    comparison.baselineSource.sourcePath =
        QStringLiteral(
            "/logs/baseline.jsonl"
            );

    comparison.baselineSource.sourceName =
        QStringLiteral(
            "baseline.jsonl"
            );

    comparison.comparisonSource.sessionId =
        QStringLiteral(
            "comparison-session"
            );

    comparison.comparisonSource.sourcePath =
        QStringLiteral(
            "/logs/comparison.jsonl"
            );

    comparison.comparisonSource.sourceName =
        QStringLiteral(
            "comparison.jsonl"
            );

    /*
     * Give the current-format object non-default
     * presentation state so removing the JSON field
     * genuinely tests the legacy/default path.
     */
    comparison.presentationState
        .scroll
        .horizontalValue =
        17;

    comparison.presentationState
        .scroll
        .verticalValue =
        340;

    const InvestigationComparisonPersistenceSerializer
        serializer;

    QJsonObject json =
        serializer.serialize(
            comparison
            );

    QVERIFY(
        json.contains(
            QStringLiteral(
                "presentationState"
                )
            )
        );

    /*
     * Simulate a schema-version-1 comparison written
     * before comparison presentation-state
     * persistence existed.
     */
    json.remove(
        QStringLiteral(
            "presentationState"
            )
        );

    const ComparisonPersistenceDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(
        result.isSuccess()
        );

    QVERIFY(
        result.comparison.has_value()
        );

    QCOMPARE(
        result.comparison
            ->presentationState
            .scroll
            .horizontalValue,
        0
        );

    QCOMPARE(
        result.comparison
            ->presentationState
            .scroll
            .verticalValue,
        0
        );
}

QTEST_MAIN(
    InvestigationComparisonPersistenceTests
    )

#include "InvestigationComparisonPersistenceTests.moc"