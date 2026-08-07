#include <QtTest/QtTest>

#include "../src/domain/RecordSeverity.h"

class RecordSeverityTests : public QObject
{
    Q_OBJECT

private slots:
    void parseRecordSeverityParsesCanonicalValues();
    void parseRecordSeverityIgnoresCaseAndWhitespace();
    void parseRecordSeverityParsesCommonAliases();
    void parseRecordSeverityReturnsEmptyForUnrecognizedValue();
    void recordSeverityToStringReturnsCanonicalLabels();
};

void RecordSeverityTests::parseRecordSeverityParsesCanonicalValues()
{
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("TRACE")),
        std::optional<RecordSeverity>(RecordSeverity::Trace)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("DEBUG")),
        std::optional<RecordSeverity>(RecordSeverity::Debug)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("INFO")),
        std::optional<RecordSeverity>(RecordSeverity::Info)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("WARN")),
        std::optional<RecordSeverity>(RecordSeverity::Warning)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("ERROR")),
        std::optional<RecordSeverity>(RecordSeverity::Error)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("CRITICAL")),
        std::optional<RecordSeverity>(RecordSeverity::Critical)
        );
}

void RecordSeverityTests::parseRecordSeverityIgnoresCaseAndWhitespace()
{
    const auto severity = parseRecordSeverity(
        QStringLiteral("  warning  ")
        );

    QVERIFY(severity.has_value());
    QCOMPARE(*severity, RecordSeverity::Warning);
}

void RecordSeverityTests::parseRecordSeverityParsesCommonAliases()
{
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("INFORMATION")),
        std::optional<RecordSeverity>(RecordSeverity::Info)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("WARNING")),
        std::optional<RecordSeverity>(RecordSeverity::Warning)
        );
    QCOMPARE(
        parseRecordSeverity(QStringLiteral("FATAL")),
        std::optional<RecordSeverity>(RecordSeverity::Critical)
        );
}

void RecordSeverityTests::parseRecordSeverityReturnsEmptyForUnrecognizedValue()
{
    QVERIFY(!parseRecordSeverity(QStringLiteral("NOTICE")).has_value());
    QVERIFY(!parseRecordSeverity(QString()).has_value());
}

void RecordSeverityTests::recordSeverityToStringReturnsCanonicalLabels()
{
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Trace),
        QStringLiteral("TRACE")
        );
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Debug),
        QStringLiteral("DEBUG")
        );
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Info),
        QStringLiteral("INFO")
        );
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Warning),
        QStringLiteral("WARN")
        );
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Error),
        QStringLiteral("ERROR")
        );
    QCOMPARE(
        recordSeverityToString(RecordSeverity::Critical),
        QStringLiteral("CRITICAL")
        );
}

QTEST_MAIN(RecordSeverityTests)

#include "RecordSeverityTests.moc"