#include <QtTest/QtTest>

#include "../src/analysis/InvestigationAnalyticsAnalyzer.h"

class InvestigationAnalyticsAnalyzerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void eventCodeFrequenciesCountsValues();
    void eventCodeFrequenciesIgnoresMissingAndBlankValues();
    void eventCodeFrequenciesSortsByCountDescending();
    void eventCodeFrequenciesUsesValueAsTieBreaker();
    void entityFrequenciesCountsValues();
    void entityFrequenciesIgnoresMissingAndBlankValues();
    void subsystemTrendsCountsValuesByBucket();
    void subsystemTrendsPreservesEmptyBuckets();
    void subsystemTrendsIgnoresMissingSubsystems();
    void subsystemTrendsSkipsInvalidTimestamps();
    void subsystemTrendsRejectsInvalidRange();
    void subsystemFrequenciesCountsAndSortsValues();
    void subsystemTrendsWindowMaterializesOnlyRequestedBuckets();
    void subsystemTrendScaleUsesSelectedSubsystems();
};

static InvestigationRecord
makeEventCodeRecord(
    const std::optional<QString> &eventCode
    )
{
    InvestigationRecord record;
    record.eventCode = eventCode;

    return record;
}

static InvestigationRecord
makeEntityRecord(
    const std::optional<QString> &entityId
    )
{
    InvestigationRecord record;
    record.entityId = entityId;

    return record;
}

static InvestigationRecord
makeSubsystemTrendRecord(
    const QString &timestamp,
    const std::optional<QString> &subsystem
    )
{
    InvestigationRecord record;

    record.timestamp =
        QDateTime::fromString(
            timestamp,
            Qt::ISODateWithMs
            );

    record.subsystem = subsystem;

    return record;
}

void InvestigationAnalyticsAnalyzerTests::
    eventCodeFrequenciesCountsValues()
{
    const QVector<InvestigationRecord> records = {
        makeEventCodeRecord(
            QStringLiteral("COMM-100")
            ),
        makeEventCodeRecord(
            QStringLiteral("COMM-100")
            ),
        makeEventCodeRecord(
            QStringLiteral("TRACK-200")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.eventCodeFrequencies(records);

    QCOMPARE(frequencies.size(), 2);

    QCOMPARE(
        frequencies.at(0).value,
        QStringLiteral("COMM-100")
        );

    QCOMPARE(
        frequencies.at(0).count,
        2
        );

    QCOMPARE(
        frequencies.at(1).value,
        QStringLiteral("TRACK-200")
        );

    QCOMPARE(
        frequencies.at(1).count,
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    eventCodeFrequenciesIgnoresMissingAndBlankValues()
{
    const QVector<InvestigationRecord> records = {
        makeEventCodeRecord(
            std::nullopt
            ),
        makeEventCodeRecord(
            QString()
            ),
        makeEventCodeRecord(
            QStringLiteral("   ")
            ),
        makeEventCodeRecord(
            QStringLiteral("COMM-100")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.eventCodeFrequencies(records);

    QCOMPARE(frequencies.size(), 1);

    QCOMPARE(
        frequencies.first().value,
        QStringLiteral("COMM-100")
        );

    QCOMPARE(
        frequencies.first().count,
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    eventCodeFrequenciesSortsByCountDescending()
{
    const QVector<InvestigationRecord> records = {
        makeEventCodeRecord(
            QStringLiteral("EVENT-A")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-B")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-B")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-C")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-C")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-C")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.eventCodeFrequencies(records);

    QCOMPARE(frequencies.size(), 3);

    QCOMPARE(
        frequencies.at(0).value,
        QStringLiteral("EVENT-C")
        );

    QCOMPARE(
        frequencies.at(0).count,
        3
        );

    QCOMPARE(
        frequencies.at(1).value,
        QStringLiteral("EVENT-B")
        );

    QCOMPARE(
        frequencies.at(1).count,
        2
        );

    QCOMPARE(
        frequencies.at(2).value,
        QStringLiteral("EVENT-A")
        );

    QCOMPARE(
        frequencies.at(2).count,
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    eventCodeFrequenciesUsesValueAsTieBreaker()
{
    const QVector<InvestigationRecord> records = {
        makeEventCodeRecord(
            QStringLiteral("EVENT-C")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-A")
            ),
        makeEventCodeRecord(
            QStringLiteral("EVENT-B")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.eventCodeFrequencies(records);

    QCOMPARE(frequencies.size(), 3);

    QCOMPARE(
        frequencies.at(0).value,
        QStringLiteral("EVENT-A")
        );

    QCOMPARE(
        frequencies.at(1).value,
        QStringLiteral("EVENT-B")
        );

    QCOMPARE(
        frequencies.at(2).value,
        QStringLiteral("EVENT-C")
        );
}

void InvestigationAnalyticsAnalyzerTests::
    entityFrequenciesCountsValues()
{
    const QVector<InvestigationRecord> records = {
        makeEntityRecord(
            QStringLiteral("sensor-17")
            ),
        makeEntityRecord(
            QStringLiteral("sensor-17")
            ),
        makeEntityRecord(
            QStringLiteral("sensor-42")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.entityFrequencies(records);

    QCOMPARE(frequencies.size(), 2);

    QCOMPARE(
        frequencies.at(0).value,
        QStringLiteral("sensor-17")
        );

    QCOMPARE(
        frequencies.at(0).count,
        2
        );

    QCOMPARE(
        frequencies.at(1).value,
        QStringLiteral("sensor-42")
        );

    QCOMPARE(
        frequencies.at(1).count,
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    entityFrequenciesIgnoresMissingAndBlankValues()
{
    const QVector<InvestigationRecord> records = {
        makeEntityRecord(
            std::nullopt
            ),
        makeEntityRecord(
            QString()
            ),
        makeEntityRecord(
            QStringLiteral(" ")
            ),
        makeEntityRecord(
            QStringLiteral("device-3")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.entityFrequencies(records);

    QCOMPARE(frequencies.size(), 1);

    QCOMPARE(
        frequencies.first().value,
        QStringLiteral("device-3")
        );

    QCOMPARE(
        frequencies.first().count,
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsCountsValuesByBucket()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:10.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:25.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:40.000Z"
                ),
            QStringLiteral("Tracking")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:01:10.000Z"
                ),
            QStringLiteral("Tracking")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto trends =
        analyzer.subsystemTrends(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:01:59.999Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000
            );

    QCOMPARE(
        trends.size(),
        2
        );

    QCOMPARE(
        trends.at(0).label,
        QStringLiteral("10:00")
        );

    QCOMPARE(
        trends.at(0).countFor(
            QStringLiteral("Comms")
            ),
        2
        );

    QCOMPARE(
        trends.at(0).countFor(
            QStringLiteral("Tracking")
            ),
        1
        );

    QCOMPARE(
        trends.at(0).totalCount(),
        3
        );

    QCOMPARE(
        trends.at(1).label,
        QStringLiteral("10:01")
        );

    QCOMPARE(
        trends.at(1).countFor(
            QStringLiteral("Comms")
            ),
        0
        );

    QCOMPARE(
        trends.at(1).countFor(
            QStringLiteral("Tracking")
            ),
        1
        );

    QCOMPARE(
        trends.at(1).totalCount(),
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsPreservesEmptyBuckets()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:10.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:02:10.000Z"
                ),
            QStringLiteral("Tracking")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto trends =
        analyzer.subsystemTrends(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:02:59.999Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000
            );

    QCOMPARE(
        trends.size(),
        3
        );

    QCOMPARE(
        trends.at(0).label,
        QStringLiteral("10:00")
        );

    QCOMPARE(
        trends.at(1).label,
        QStringLiteral("10:01")
        );

    QCOMPARE(
        trends.at(2).label,
        QStringLiteral("10:02")
        );

    QCOMPARE(
        trends.at(0).totalCount(),
        1
        );

    QCOMPARE(
        trends.at(1).totalCount(),
        0
        );

    QCOMPARE(
        trends.at(2).totalCount(),
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsIgnoresMissingSubsystems()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:10.000Z"
                ),
            std::nullopt
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:20.000Z"
                ),
            QStringLiteral("   ")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:30.000Z"
                ),
            QStringLiteral("Comms")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto trends =
        analyzer.subsystemTrends(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:59.999Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000
            );

    QCOMPARE(
        trends.size(),
        1
        );

    QCOMPARE(
        trends.first().totalCount(),
        1
        );

    QCOMPARE(
        trends.first().countFor(
            QStringLiteral("Comms")
            ),
        1
        );

    QCOMPARE(
        trends.first().countsByValue.size(),
        1
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsSkipsInvalidTimestamps()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "not-a-timestamp"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:30.000Z"
                ),
            QStringLiteral("Tracking")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto trends =
        analyzer.subsystemTrends(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:59.999Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000
            );

    QCOMPARE(
        trends.size(),
        1
        );

    QCOMPARE(
        trends.first().totalCount(),
        1
        );

    QCOMPARE(
        trends.first().countFor(
            QStringLiteral("Tracking")
            ),
        1
        );

    QCOMPARE(
        trends.first().countFor(
            QStringLiteral("Comms")
            ),
        0
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsRejectsInvalidRange()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:30.000Z"
                ),
            QStringLiteral("Comms")
            )
    };

    const QDateTime validFirstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QDateTime validLastTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:05:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    InvestigationAnalyticsAnalyzer analyzer;

    const auto zeroInterval =
        analyzer.subsystemTrends(
            records,
            validFirstTimestamp,
            validLastTimestamp,
            0
            );

    QVERIFY(
        zeroInterval.isEmpty()
        );

    const auto reversedRange =
        analyzer.subsystemTrends(
            records,
            validLastTimestamp,
            validFirstTimestamp,
            60 * 1000
            );

    QVERIFY(
        reversedRange.isEmpty()
        );

    const auto invalidFirstTimestamp =
        analyzer.subsystemTrends(
            records,
            QDateTime(),
            validLastTimestamp,
            60 * 1000
            );

    QVERIFY(
        invalidFirstTimestamp.isEmpty()
        );

    const auto invalidLastTimestamp =
        analyzer.subsystemTrends(
            records,
            validFirstTimestamp,
            QDateTime(),
            60 * 1000
            );

    QVERIFY(
        invalidLastTimestamp.isEmpty()
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemFrequenciesCountsAndSortsValues()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            QStringLiteral("Tracking")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:02.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:03.000Z"
                ),
            QStringLiteral("Guidance")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto frequencies =
        analyzer.subsystemFrequencies(
            records
            );

    QCOMPARE(frequencies.size(), 3);

    QCOMPARE(
        frequencies.at(0).value,
        QStringLiteral("Comms")
        );

    QCOMPARE(
        frequencies.at(0).count,
        2
        );

    /*
     * Equal counts use the analyzer's existing
     * deterministic lexical tie-break.
     */
    QCOMPARE(
        frequencies.at(1).value,
        QStringLiteral("Guidance")
        );

    QCOMPARE(
        frequencies.at(2).value,
        QStringLiteral("Tracking")
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendsWindowMaterializesOnlyRequestedBuckets()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.001Z"
                ),
            QStringLiteral("Tracking")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:10.000Z"
                ),
            QStringLiteral("Outside")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const auto trends =
        analyzer.subsystemTrendsWindow(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T11:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            1,
            1000,
            2
            );

    QCOMPARE(trends.size(), 2);

    QCOMPARE(
        trends.at(0).countFor(
            QStringLiteral("Comms")
            ),
        1
        );

    QCOMPARE(
        trends.at(1).countFor(
            QStringLiteral("Tracking")
            ),
        1
        );

    QCOMPARE(
        trends.at(0).countFor(
            QStringLiteral("Outside")
            ),
        0
        );
}

void InvestigationAnalyticsAnalyzerTests::
    subsystemTrendScaleUsesSelectedSubsystems()
{
    const QVector<InvestigationRecord> records = {
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:01.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:02.000Z"
                ),
            QStringLiteral("Comms")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:03.000Z"
                ),
            QStringLiteral("Tracking")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:04.000Z"
                ),
            QStringLiteral("Ignored")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:05.000Z"
                ),
            QStringLiteral("Ignored")
            ),
        makeSubsystemTrendRecord(
            QStringLiteral(
                "2026-08-22T10:00:06.000Z"
                ),
            QStringLiteral("Ignored")
            )
    };

    InvestigationAnalyticsAnalyzer analyzer;

    const int maximum =
        analyzer.subsystemTrendScaleMaximum(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:59.999Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000,
            {
                QStringLiteral("Comms"),
                QStringLiteral("Tracking")
            }
            );

    QCOMPARE(maximum, 2);
}

QTEST_MAIN(InvestigationAnalyticsAnalyzerTests)

#include "InvestigationAnalyticsAnalyzerTests.moc"