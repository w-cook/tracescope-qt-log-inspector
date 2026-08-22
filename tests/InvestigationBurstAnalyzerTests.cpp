#include <QtTest/QtTest>

#include <optional>

#include "../src/analysis/InvestigationBurstAnalyzer.h"

class InvestigationBurstAnalyzerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void elevatedThresholdIncludesExactWindowBoundary();
    void elevatedThresholdRejectsRecordOutsideWindow();
    void errorCriticalThresholdDetectsBurst();
    void criticalCountsTowardErrorCriticalThreshold();
    void overlappingQualifyingWindowsMerge();
    void nearbyQualifyingBurstsMergeWithinMergeGap();
    void burstsBeyondMergeGapRemainSeparate();
    void missingTimestampAndSeverityAreIgnored();
    void nonElevatedRecordsDoNotTriggerBursts();
    void burstSummarizesAvailableCanonicalFields();
    void inputOrderDoesNotAffectBurstResults();
    void invalidSettingsReturnNoBursts();
};

static InvestigationRecord
makeBurstRecord(
    const QString &timestamp,
    std::optional<RecordSeverity> severity,
    const QString &recordId,
    qint64 recordNumber,
    const std::optional<QString> &subsystem =
    std::nullopt,
    const std::optional<QString> &eventCode =
    std::nullopt,
    const std::optional<QString> &entityId =
    std::nullopt
    )
{
    InvestigationRecord record;

    record.timestamp =
        QDateTime::fromString(
            timestamp,
            Qt::ISODateWithMs
            );

    record.severity = severity;
    record.recordId = recordId;

    record.subsystem = subsystem;
    record.eventCode = eventCode;
    record.entityId = entityId;

    record.source.sourceName =
        QStringLiteral("sample.log");

    record.source.recordNumber =
        recordNumber;

    return record;
}

static BurstDetectionSettings
defaultTestSettings()
{
    BurstDetectionSettings settings;

    settings.windowMilliseconds = 1000;
    settings.elevatedEventThreshold = 3;
    settings.errorCriticalThreshold = 2;
    settings.mergeGapMilliseconds = 200;

    return settings;
}

void InvestigationBurstAnalyzerTests::
    elevatedThresholdIncludesExactWindowBoundary()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.500Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.errorCriticalThreshold = 99;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().totalElevatedCount(),
        3
        );

    QCOMPARE(
        bursts.first().durationMilliseconds(),
        1000
        );

    QVERIFY(
        bursts.first()
            .triggeredByElevatedThreshold
        );

    QVERIFY(
        !bursts.first()
             .triggeredByErrorCriticalThreshold
        );
}

void InvestigationBurstAnalyzerTests::
    elevatedThresholdRejectsRecordOutsideWindow()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.500Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.001Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.errorCriticalThreshold = 99;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QVERIFY(
        bursts.isEmpty()
        );
}

void InvestigationBurstAnalyzerTests::
    errorCriticalThresholdDetectsBurst()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Error,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.900Z"
                ),
            RecordSeverity::Error,
            QStringLiteral("record-3"),
            3
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.elevatedEventThreshold = 99;
    settings.errorCriticalThreshold = 2;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().warningCount,
        1
        );

    QCOMPARE(
        bursts.first().errorCount,
        2
        );

    QCOMPARE(
        bursts.first().criticalCount,
        0
        );

    QVERIFY(
        !bursts.first()
             .triggeredByElevatedThreshold
        );

    QVERIFY(
        bursts.first()
            .triggeredByErrorCriticalThreshold
        );
}

void InvestigationBurstAnalyzerTests::
    criticalCountsTowardErrorCriticalThreshold()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Error,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.500Z"
                ),
            RecordSeverity::Critical,
            QStringLiteral("record-2"),
            2
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.elevatedEventThreshold = 99;
    settings.errorCriticalThreshold = 2;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().errorCount,
        1
        );

    QCOMPARE(
        bursts.first().criticalCount,
        1
        );

    QCOMPARE(
        static_cast<int>(
            bursts.first().highestSeverity()
            ),
        static_cast<int>(
            RecordSeverity::Critical
            )
        );

    QVERIFY(
        bursts.first()
            .triggeredByErrorCriticalThreshold
        );
}

void InvestigationBurstAnalyzerTests::
    overlappingQualifyingWindowsMerge()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.800Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.200Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-4"),
            4
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.errorCriticalThreshold = 99;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().totalElevatedCount(),
        4
        );

    QCOMPARE(
        bursts.first().durationMilliseconds(),
        1200
        );

    QCOMPARE(
        bursts.first().recordIds,
        QVector<QString>({
            QStringLiteral("record-1"),
            QStringLiteral("record-2"),
            QStringLiteral("record-3"),
            QStringLiteral("record-4")
        })
        );
}

void InvestigationBurstAnalyzerTests::
    nearbyQualifyingBurstsMergeWithinMergeGap()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.050Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.100Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.500Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-4"),
            4
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.550Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-5"),
            5
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.600Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-6"),
            6
            )
    };

    BurstDetectionSettings settings;

    settings.windowMilliseconds = 100;
    settings.elevatedEventThreshold = 3;
    settings.errorCriticalThreshold = 99;

    /*
     * First qualifying burst ends at 100 ms.
     * Second starts at 500 ms.
     * The 400-ms gap is within this merge
     * tolerance, so they form one episode.
     */
    settings.mergeGapMilliseconds = 400;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().totalElevatedCount(),
        6
        );

    QCOMPARE(
        bursts.first().durationMilliseconds(),
        600
        );
}

void InvestigationBurstAnalyzerTests::
    burstsBeyondMergeGapRemainSeparate()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.050Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.100Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.500Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-4"),
            4
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.550Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-5"),
            5
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.600Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-6"),
            6
            )
    };

    BurstDetectionSettings settings;

    settings.windowMilliseconds = 100;
    settings.elevatedEventThreshold = 3;
    settings.errorCriticalThreshold = 99;
    settings.mergeGapMilliseconds = 399;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        2
        );

    QCOMPARE(
        bursts.at(0).totalElevatedCount(),
        3
        );

    QCOMPARE(
        bursts.at(1).totalElevatedCount(),
        3
        );
}

void InvestigationBurstAnalyzerTests::
    missingTimestampAndSeverityAreIgnored()
{
    QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.200Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            )
    };

    InvestigationRecord missingTimestamp;

    missingTimestamp.severity =
        RecordSeverity::Critical;

    missingTimestamp.recordId =
        QStringLiteral("missing-timestamp");

    records.append(
        missingTimestamp
        );

    InvestigationRecord missingSeverity;

    missingSeverity.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.100Z"
                ),
            Qt::ISODateWithMs
            );

    missingSeverity.recordId =
        QStringLiteral("missing-severity");

    records.append(
        missingSeverity
        );

    InvestigationBurstAnalyzer analyzer;

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.errorCriticalThreshold = 99;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().totalElevatedCount(),
        3
        );

    QVERIFY(
        !bursts.first().recordIds.contains(
            QStringLiteral(
                "missing-timestamp"
                )
            )
        );

    QVERIFY(
        !bursts.first().recordIds.contains(
            QStringLiteral(
                "missing-severity"
                )
            )
        );
}

void InvestigationBurstAnalyzerTests::
    nonElevatedRecordsDoNotTriggerBursts()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Trace,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.100Z"
                ),
            RecordSeverity::Debug,
            QStringLiteral("record-2"),
            2
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.200Z"
                ),
            RecordSeverity::Info,
            QStringLiteral("record-3"),
            3
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.300Z"
                ),
            RecordSeverity::Info,
            QStringLiteral("record-4"),
            4
            )
    };

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            defaultTestSettings()
            );

    QVERIFY(
        bursts.isEmpty()
        );
}

void InvestigationBurstAnalyzerTests::
    burstSummarizesAvailableCanonicalFields()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1,
            QStringLiteral("Comms"),
            QStringLiteral("LINK_TIMEOUT"),
            QStringLiteral("radio-3")
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.200Z"
                ),
            RecordSeverity::Error,
            QStringLiteral("record-2"),
            2,
            QStringLiteral("Comms"),
            QStringLiteral("LINK_TIMEOUT"),
            QStringLiteral("radio-7")
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            RecordSeverity::Critical,
            QStringLiteral("record-3"),
            3,
            QStringLiteral("Tracking"),
            QStringLiteral("TRACK_LOST"),
            std::nullopt
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    const InvestigationBurst &burst =
        bursts.first();

    QCOMPARE(
        burst.warningCount,
        1
        );

    QCOMPARE(
        burst.errorCount,
        1
        );

    QCOMPARE(
        burst.criticalCount,
        1
        );

    QCOMPARE(
        burst.subsystemCounts.value(
            QStringLiteral("Comms")
            ),
        2
        );

    QCOMPARE(
        burst.subsystemCounts.value(
            QStringLiteral("Tracking")
            ),
        1
        );

    QCOMPARE(
        burst.eventCodeCounts.value(
            QStringLiteral("LINK_TIMEOUT")
            ),
        2
        );

    QCOMPARE(
        burst.eventCodeCounts.value(
            QStringLiteral("TRACK_LOST")
            ),
        1
        );

    QCOMPARE(
        burst.entityCounts.value(
            QStringLiteral("radio-3")
            ),
        1
        );

    QCOMPARE(
        burst.entityCounts.value(
            QStringLiteral("radio-7")
            ),
        1
        );

    QCOMPARE(
        burst.entityCounts.size(),
        2
        );

    QCOMPARE(
        static_cast<int>(
            burst.highestSeverity()
            ),
        static_cast<int>(
            RecordSeverity::Critical
            )
        );

    QCOMPARE(
        burst.settings.windowMilliseconds,
        settings.windowMilliseconds
        );

    QCOMPARE(
        burst.settings.elevatedEventThreshold,
        settings.elevatedEventThreshold
        );

    QCOMPARE(
        burst.settings.errorCriticalThreshold,
        settings.errorCriticalThreshold
        );

    QCOMPARE(
        burst.settings.mergeGapMilliseconds,
        settings.mergeGapMilliseconds
        );

    QVERIFY(
        burst.triggeredByElevatedThreshold
        );

    QVERIFY(
        burst.triggeredByErrorCriticalThreshold
        );
}

void InvestigationBurstAnalyzerTests::
    inputOrderDoesNotAffectBurstResults()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-3"),
            3
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-1"),
            1
            ),
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.200Z"
                ),
            RecordSeverity::Warning,
            QStringLiteral("record-2"),
            2
            )
    };

    BurstDetectionSettings settings =
        defaultTestSettings();

    settings.errorCriticalThreshold = 99;

    InvestigationBurstAnalyzer analyzer;

    const auto bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    QCOMPARE(
        bursts.size(),
        1
        );

    QCOMPARE(
        bursts.first().recordIds,
        QVector<QString>({
            QStringLiteral("record-1"),
            QStringLiteral("record-2"),
            QStringLiteral("record-3")
        })
        );

    QCOMPARE(
        bursts.first().startTimestamp,
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            )
        );

    QCOMPARE(
        bursts.first().endTimestamp,
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.400Z"
                ),
            Qt::ISODateWithMs
            )
        );
}

void InvestigationBurstAnalyzerTests::
    invalidSettingsReturnNoBursts()
{
    const QVector<InvestigationRecord> records = {
        makeBurstRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            RecordSeverity::Critical,
            QStringLiteral("record-1"),
            1
            )
    };

    InvestigationBurstAnalyzer analyzer;

    BurstDetectionSettings invalidWindow =
        defaultTestSettings();

    invalidWindow.windowMilliseconds = 0;

    QVERIFY(
        analyzer.detectBursts(
            records,
            invalidWindow
            )
            .isEmpty()
        );

    BurstDetectionSettings invalidElevatedThreshold =
        defaultTestSettings();

    invalidElevatedThreshold
        .elevatedEventThreshold = 0;

    QVERIFY(
        analyzer.detectBursts(
            records,
            invalidElevatedThreshold
            )
            .isEmpty()
        );

    BurstDetectionSettings invalidErrorThreshold =
        defaultTestSettings();

    invalidErrorThreshold
        .errorCriticalThreshold = 0;

    QVERIFY(
        analyzer.detectBursts(
            records,
            invalidErrorThreshold
            )
            .isEmpty()
        );

    BurstDetectionSettings invalidMergeGap =
        defaultTestSettings();

    invalidMergeGap.mergeGapMilliseconds = -1;

    QVERIFY(
        analyzer.detectBursts(
            records,
            invalidMergeGap
            )
            .isEmpty()
        );
}

QTEST_MAIN(InvestigationBurstAnalyzerTests)

#include "InvestigationBurstAnalyzerTests.moc"