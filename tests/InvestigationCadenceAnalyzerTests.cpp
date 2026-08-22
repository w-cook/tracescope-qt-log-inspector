#include <QtTest/QtTest>

#include "../src/analysis/InvestigationCadenceAnalyzer.h"

class InvestigationCadenceAnalyzerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void regularCadenceProducesAdaptiveRecommendation();
    void extremeIdleGapDoesNotDominateRecommendation();
    void zeroGapsAreCountedButExcludedFromStatistics();
    void inputOrderDoesNotAffectCadence();
    void invalidTimestampsAreIgnored();
    void insufficientPositiveGapsUseFallbackRecommendation();
};

static InvestigationRecord
makeCadenceRecord(
    const QString &timestamp
    )
{
    InvestigationRecord record;

    record.timestamp =
        QDateTime::fromString(
            timestamp,
            Qt::ISODateWithMs
            );

    return record;
}

void InvestigationCadenceAnalyzerTests::
    regularCadenceProducesAdaptiveRecommendation()
{
    QVector<InvestigationRecord> records;

    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    for (int index = 0;
         index <= 10;
         ++index) {
        InvestigationRecord record;

        record.timestamp =
            firstTimestamp.addMSecs(
                index * 100
                );

        records.append(record);
    }

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    QCOMPARE(
        cadence.timestampCount,
        11
        );

    QCOMPARE(
        cadence.positiveGapCount,
        10
        );

    QCOMPARE(
        cadence.zeroGapCount,
        0
        );

    QCOMPARE(
        cadence.minimumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.medianPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.meanPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.p90PositiveGapMilliseconds,
        100
        );

    QVERIFY(
        !cadence.usesFallbackRecommendation
        );

    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        2000
        );

    QCOMPARE(
        cadence.recommendedMergeGapMilliseconds,
        500
        );
}

void InvestigationCadenceAnalyzerTests::
    extremeIdleGapDoesNotDominateRecommendation()
{
    QVector<InvestigationRecord> records;

    QDateTime timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    InvestigationRecord firstRecord;
    firstRecord.timestamp = timestamp;

    records.append(
        firstRecord
        );

    /*
     * Nine normal 100-ms gaps.
     */
    for (int index = 0;
         index < 9;
         ++index) {
        timestamp =
            timestamp.addMSecs(100);

        InvestigationRecord record;
        record.timestamp = timestamp;

        records.append(record);
    }

    /*
     * One extreme idle gap.
     */
    timestamp =
        timestamp.addMSecs(
            10 * 1000
            );

    InvestigationRecord finalRecord;
    finalRecord.timestamp = timestamp;

    records.append(
        finalRecord
        );

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    QCOMPARE(
        cadence.positiveGapCount,
        10
        );

    QCOMPARE(
        cadence.medianPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.p90PositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        10000
        );

    QVERIFY(
        cadence.meanPositiveGapMilliseconds
        > 100.0
        );

    /*
     * The mean is capped by P90, so the single
     * 10-second idle period must not inflate the
     * recommended burst window.
     */
    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        2000
        );
}

void InvestigationCadenceAnalyzerTests::
    zeroGapsAreCountedButExcludedFromStatistics()
{
    QVector<InvestigationRecord> records;

    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    for (int index = 0;
         index <= 10;
         ++index) {
        InvestigationRecord record;

        record.timestamp =
            firstTimestamp.addMSecs(
                index * 100
                );

        records.append(record);
    }

    /*
     * Add another record with the same timestamp
     * as the existing 500-ms record.
     */
    InvestigationRecord simultaneousRecord;

    simultaneousRecord.timestamp =
        firstTimestamp.addMSecs(
            500
            );

    records.append(
        simultaneousRecord
        );

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    QCOMPARE(
        cadence.timestampCount,
        12
        );

    QCOMPARE(
        cadence.zeroGapCount,
        1
        );

    QCOMPARE(
        cadence.positiveGapCount,
        10
        );

    QCOMPARE(
        cadence.minimumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.medianPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.meanPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.p90PositiveGapMilliseconds,
        100
        );

    QVERIFY(
        !cadence.usesFallbackRecommendation
        );

    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        2000
        );

    QCOMPARE(
        cadence.recommendedMergeGapMilliseconds,
        500
        );
}

void InvestigationCadenceAnalyzerTests::
    inputOrderDoesNotAffectCadence()
{
    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QVector<InvestigationRecord> records;

    /*
     * Add records in reverse chronological order.
     * The analyzer should sort timestamps before
     * calculating gaps.
     */
    for (int index = 10;
         index >= 0;
         --index) {
        InvestigationRecord record;

        record.timestamp =
            firstTimestamp.addMSecs(
                index * 100
                );

        records.append(record);
    }

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    QCOMPARE(
        cadence.timestampCount,
        11
        );

    QCOMPARE(
        cadence.positiveGapCount,
        10
        );

    QCOMPARE(
        cadence.zeroGapCount,
        0
        );

    QCOMPARE(
        cadence.minimumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.medianPositiveGapMilliseconds,
        100.0
        );

    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        2000
        );
}

void InvestigationCadenceAnalyzerTests::
    invalidTimestampsAreIgnored()
{
    QVector<InvestigationRecord> records;

    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    for (int index = 0;
         index <= 10;
         ++index) {
        InvestigationRecord record;

        record.timestamp =
            firstTimestamp.addMSecs(
                index * 100
                );

        records.append(record);
    }

    InvestigationRecord missingTimestamp;

    records.append(
        missingTimestamp
        );

    InvestigationRecord invalidTimestamp;

    invalidTimestamp.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "not-a-timestamp"
                ),
            Qt::ISODateWithMs
            );

    records.append(
        invalidTimestamp
        );

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    /*
     * Only the eleven valid timestamps should
     * contribute to the cadence calculation.
     */
    QCOMPARE(
        cadence.timestampCount,
        11
        );

    QCOMPARE(
        cadence.positiveGapCount,
        10
        );

    QCOMPARE(
        cadence.zeroGapCount,
        0
        );

    QCOMPARE(
        cadence.minimumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        100
        );

    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        2000
        );

    QVERIFY(
        !cadence.usesFallbackRecommendation
        );
}

void InvestigationCadenceAnalyzerTests::
    insufficientPositiveGapsUseFallbackRecommendation()
{
    const QVector<InvestigationRecord> records = {
        makeCadenceRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                )
            ),
        makeCadenceRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.000Z"
                )
            ),
        makeCadenceRecord(
            QStringLiteral(
                "2026-08-22T10:00:02.000Z"
                )
            )
    };

    InvestigationCadenceAnalyzer analyzer;

    const InvestigationCadence cadence =
        analyzer.analyze(records);

    QCOMPARE(
        cadence.timestampCount,
        3
        );

    QCOMPARE(
        cadence.positiveGapCount,
        2
        );

    QCOMPARE(
        cadence.zeroGapCount,
        0
        );

    /*
     * We still calculate the available descriptive
     * statistics even though there is not enough
     * data to derive an adaptive recommendation.
     */
    QCOMPARE(
        cadence.minimumPositiveGapMilliseconds,
        1000
        );

    QCOMPARE(
        cadence.maximumPositiveGapMilliseconds,
        1000
        );

    QCOMPARE(
        cadence.medianPositiveGapMilliseconds,
        1000.0
        );

    QCOMPARE(
        cadence.meanPositiveGapMilliseconds,
        1000.0
        );

    QCOMPARE(
        cadence.p90PositiveGapMilliseconds,
        1000
        );

    QVERIFY(
        cadence.usesFallbackRecommendation
        );

    QCOMPARE(
        cadence.recommendedBurstWindowMilliseconds,
        30000
        );

    QCOMPARE(
        cadence.recommendedMergeGapMilliseconds,
        5000
        );
}

QTEST_MAIN(InvestigationCadenceAnalyzerTests)

#include "InvestigationCadenceAnalyzerTests.moc"