#include <QtTest/QtTest>

#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>

#include "../src/persistence/InvestigationRecordSerialization.h"

class InvestigationRecordSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsCompleteRecord();
    void roundTripsMissingOptionalFields();
    void rejectsInvalidSourceGeneration();
};

void InvestigationRecordSerializationTests::
    roundTripsCompleteRecord()
{
    InvestigationRecord original;

    original.recordId =
        QStringLiteral("record-42");

    original.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-15T17:42:31.125Z"
                ),
            Qt::ISODateWithMs
            );

    original.severity =
        RecordSeverity::Error;

    original.subsystem =
        QStringLiteral("Gateway");

    original.eventCode =
        QStringLiteral("QUEUE_TIMEOUT");

    original.entityId =
        QStringLiteral("request-1842");

    original.message =
        QStringLiteral(
            "Queue processing timed out"
            );

    original.customAttributes.insert(
        QStringLiteral("attempt"),
        3
        );

    original.customAttributes.insert(
        QStringLiteral("healthy"),
        false
        );

    QVariantMap metrics;

    metrics.insert(
        QStringLiteral("durationMs"),
        5032
        );

    QVariantList tags;

    tags.append(
        QStringLiteral("live")
        );

    tags.append(
        QStringLiteral("gateway")
        );

    metrics.insert(
        QStringLiteral("tags"),
        tags
        );

    original.customAttributes.insert(
        QStringLiteral("metrics"),
        metrics
        );

    original.rawSource =
        QStringLiteral(
            R"({"level":"error","message":"Queue processing timed out"})"
            );

    original.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    original.source.logicalSourceKey =
        QStringLiteral(
            "C:/original/gateway.jsonl"
            );

    original.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    original.source.recordNumber = 42;

    original.source.sourceGeneration = 3;

    InvestigationRecordSerializer serializer;

    const QJsonObject json =
        serializer.serialize(
            original
            );

    const InvestigationRecordDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(result.isSuccess());

    const InvestigationRecord &restored =
        *result.record;

    QCOMPARE(
        restored.recordId,
        original.recordId
        );

    QCOMPARE(
        restored.timestamp,
        original.timestamp
        );

    QCOMPARE(
        restored.severity,
        original.severity
        );

    QCOMPARE(
        restored.subsystem,
        original.subsystem
        );

    QCOMPARE(
        restored.eventCode,
        original.eventCode
        );

    QCOMPARE(
        restored.entityId,
        original.entityId
        );

    QCOMPARE(
        restored.message,
        original.message
        );

    QCOMPARE(
        restored.rawSource,
        original.rawSource
        );

    QCOMPARE(
        restored.source.sourcePath,
        original.source.sourcePath
        );

    QCOMPARE(
        restored.source.logicalSourceKey,
        original.source.logicalSourceKey
        );

    QCOMPARE(
        restored.source.sourceName,
        original.source.sourceName
        );

    QCOMPARE(
        restored.source.recordNumber,
        original.source.recordNumber
        );

    QCOMPARE(
        restored.source.sourceGeneration,
        original.source.sourceGeneration
        );

    QCOMPARE(
        restored.customAttributes.value(
                                     QStringLiteral("attempt")
                                     ).toInt(),
        3
        );

    QCOMPARE(
        restored.customAttributes.value(
                                     QStringLiteral("healthy")
                                     ).toBool(),
        false
        );

    const QVariantMap restoredMetrics =
        restored.customAttributes.value(
                                     QStringLiteral("metrics")
                                     ).toMap();

    QCOMPARE(
        restoredMetrics.value(
                           QStringLiteral("durationMs")
                           ).toInt(),
        5032
        );

    const QVariantList restoredTags =
        restoredMetrics.value(
                           QStringLiteral("tags")
                           ).toList();

    QCOMPARE(restoredTags.size(), 2);

    QCOMPARE(
        restoredTags.at(0).toString(),
        QStringLiteral("live")
        );

    QCOMPARE(
        restoredTags.at(1).toString(),
        QStringLiteral("gateway")
        );
}

void InvestigationRecordSerializationTests::
    roundTripsMissingOptionalFields()
{
    InvestigationRecord original;

    original.recordId =
        QStringLiteral("minimal-record");

    original.rawSource =
        QStringLiteral("minimal");

    original.source.sourceName =
        QStringLiteral("minimal.log");

    original.source.recordNumber = 1;

    InvestigationRecordSerializer serializer;

    QJsonObject json =
        serializer.serialize(
            original
            );

    QJsonObject source =
        json.value(
                QStringLiteral("source")
                )
            .toObject();

    source.remove(
        QStringLiteral(
            "logicalSourceKey"
            )
        );

    json.insert(
        QStringLiteral("source"),
        source
        );

    const InvestigationRecordDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(result.isSuccess());

    const InvestigationRecord &restored =
        *result.record;

    QVERIFY(!restored.timestamp.has_value());
    QVERIFY(!restored.severity.has_value());
    QVERIFY(!restored.subsystem.has_value());
    QVERIFY(!restored.eventCode.has_value());
    QVERIFY(!restored.entityId.has_value());
    QVERIFY(!restored.message.has_value());

    QVERIFY(
        restored.customAttributes.isEmpty()
        );

    QCOMPARE(
        restored.source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        restored.source.logicalSourceKey,
        restored.source.sourceName
        );
}

void InvestigationRecordSerializationTests::
    rejectsInvalidSourceGeneration()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-invalid-test");

    record.rawSource =
        QStringLiteral("test");

    InvestigationRecordSerializer serializer;

    QJsonObject json =
        serializer.serialize(
            record
            );

    QJsonObject source =
        json.value(
                QStringLiteral("source")
                ).toObject();

    source.insert(
        QStringLiteral("sourceGeneration"),
        -1
        );

    json.insert(
        QStringLiteral("source"),
        source
        );

    const InvestigationRecordDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SOURCE_GENERATION"
            )
        );
}

QTEST_MAIN(
    InvestigationRecordSerializationTests
    )

#include "InvestigationRecordSerializationTests.moc"