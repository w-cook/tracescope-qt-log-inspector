#include <QtTest/QtTest>

#include "../src/domain/InvestigationRecord.h"
#include "../src/domain/RecordIdentity.h"
#include "../src/domain/RecordTimestamp.h"

class InvestigationRecordTests : public QObject
{
    Q_OBJECT

private slots:
    void defaultRecordHasNoCanonicalValues();
    void recordPreservesCanonicalAndCustomValues();
    void parseRecordTimestampParsesIsoTimestampWithMilliseconds();
    void parseRecordTimestampParsesIsoTimestampWithoutMilliseconds();
    void parseRecordTimestampReturnsEmptyForInvalidInput();
    void stableIdentityIsDeterministic();
    void stableIdentityNormalizesPathSeparators();
    void stableIdentityChangesWhenRecordChanges();
};

void InvestigationRecordTests::defaultRecordHasNoCanonicalValues()
{
    const InvestigationRecord record;

    QVERIFY(record.recordId.isEmpty());
    QVERIFY(!record.timestamp.has_value());
    QVERIFY(!record.severity.has_value());
    QVERIFY(!record.subsystem.has_value());
    QVERIFY(!record.eventCode.has_value());
    QVERIFY(!record.entityId.has_value());
    QVERIFY(!record.message.has_value());
    QVERIFY(record.customAttributes.isEmpty());
    QVERIFY(record.rawSource.isEmpty());
    QCOMPARE(record.source.recordNumber, qint64(0));
}

void InvestigationRecordTests::recordPreservesCanonicalAndCustomValues()
{
    InvestigationRecord record;

    record.timestamp = QDateTime::fromString(
        QStringLiteral("2026-07-07T10:14:22.381Z"),
        Qt::ISODateWithMs
        );
    record.severity = RecordSeverity::Warning;
    record.subsystem = QStringLiteral("Tracking");
    record.eventCode = QStringLiteral("TRACK_LOST");
    record.entityId = QStringLiteral("TRK-402");
    record.message = QStringLiteral("Track lost for 1200ms");

    record.customAttributes.insert(
        QStringLiteral("durationMs"),
        1200
        );
    record.customAttributes.insert(
        QStringLiteral("recovered"),
        false
        );

    record.rawSource = QStringLiteral(
        R"({"level":"WARN","durationMs":1200})"
        );
    record.source.sourcePath = QStringLiteral("samples/session.jsonl");
    record.source.sourceName = QStringLiteral("session.jsonl");
    record.source.recordNumber = 12;

    QVERIFY(record.timestamp.has_value());
    QCOMPARE(record.severity, std::optional<RecordSeverity>(
                                  RecordSeverity::Warning
                                  ));
    QCOMPARE(
        record.subsystem,
        std::optional<QString>(QStringLiteral("Tracking"))
        );
    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("durationMs")
                                   ).toInt(),
        1200
        );
    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("recovered")
                                   ).toBool(),
        false
        );
    QCOMPARE(record.source.recordNumber, qint64(12));
    QVERIFY(!record.rawSource.isEmpty());
}

void InvestigationRecordTests::
    parseRecordTimestampParsesIsoTimestampWithMilliseconds()
{
    const auto timestamp = parseRecordTimestamp(
        QStringLiteral("2026-07-07T10:14:22.381Z")
        );

    QVERIFY(timestamp.has_value());
    QCOMPARE(timestamp->date(), QDate(2026, 7, 7));
    QCOMPARE(timestamp->time(), QTime(10, 14, 22, 381));
    QCOMPARE(timestamp->offsetFromUtc(), 0);
}

void InvestigationRecordTests::
    parseRecordTimestampParsesIsoTimestampWithoutMilliseconds()
{
    const auto timestamp = parseRecordTimestamp(
        QStringLiteral("2026-07-07T10:14:22-04:00")
        );

    QVERIFY(timestamp.has_value());
    QCOMPARE(timestamp->date(), QDate(2026, 7, 7));
    QCOMPARE(timestamp->time(), QTime(10, 14, 22));
    QCOMPARE(timestamp->offsetFromUtc(), -4 * 60 * 60);
}

void InvestigationRecordTests::
    parseRecordTimestampReturnsEmptyForInvalidInput()
{
    QVERIFY(!parseRecordTimestamp(
                 QStringLiteral("not-a-timestamp")
                 ).has_value());

    QVERIFY(!parseRecordTimestamp(QString()).has_value());
}

void InvestigationRecordTests::stableIdentityIsDeterministic()
{
    RecordSourceMetadata source;
    source.sourcePath = QStringLiteral("samples/session.jsonl");
    source.sourceName = QStringLiteral("session.jsonl");
    source.recordNumber = 4;

    const QString rawSource = QStringLiteral(
        R"({"level":"ERROR","message":"Packet loss"})"
        );

    const QString first = createStableRecordIdentity(
        source,
        rawSource
        );
    const QString second = createStableRecordIdentity(
        source,
        rawSource
        );

    QVERIFY(!first.isEmpty());
    QCOMPARE(first.size(), 64);
    QCOMPARE(first, second);
}

void InvestigationRecordTests::stableIdentityNormalizesPathSeparators()
{
    RecordSourceMetadata firstSource;
    firstSource.sourcePath = QStringLiteral(
        "samples\\nested\\session.jsonl"
        );
    firstSource.recordNumber = 4;

    RecordSourceMetadata secondSource;
    secondSource.sourcePath = QStringLiteral(
        "samples/nested/session.jsonl"
        );
    secondSource.recordNumber = 4;

    const QString rawSource = QStringLiteral(
        R"({"level":"INFO"})"
        );

    QCOMPARE(
        createStableRecordIdentity(firstSource, rawSource),
        createStableRecordIdentity(secondSource, rawSource)
        );
}

void InvestigationRecordTests::stableIdentityChangesWhenRecordChanges()
{
    RecordSourceMetadata source;
    source.sourcePath = QStringLiteral("samples/session.jsonl");
    source.recordNumber = 4;

    const QString originalIdentity = createStableRecordIdentity(
        source,
        QStringLiteral(R"({"level":"INFO"})")
        );

    const QString changedContentIdentity = createStableRecordIdentity(
        source,
        QStringLiteral(R"({"level":"ERROR"})")
        );

    source.recordNumber = 5;

    const QString changedRecordNumberIdentity =
        createStableRecordIdentity(
            source,
            QStringLiteral(R"({"level":"INFO"})")
            );

    QVERIFY(originalIdentity != changedContentIdentity);
    QVERIFY(originalIdentity != changedRecordNumberIdentity);
}

QTEST_MAIN(InvestigationRecordTests)

#include "InvestigationRecordTests.moc"