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

QTEST_MAIN(InvestigationAnalyticsAnalyzerTests)

#include "InvestigationAnalyticsAnalyzerTests.moc"