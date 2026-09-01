#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>

#include "../src/exporting/InvestigationRecordExportFormatter.h"

class InvestigationRecordExportFormatterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void structuredJsonIncludesAvailableRecordData();
    void structuredJsonOmitsUnavailableCanonicalFields();
    void structuredJsonPreservesEscapedAndStructuredValues();
    void formattedTextIncludesAvailableRecordData();
    void outputIsDeterministicAcrossCustomAttributeInsertionOrder();
};

void InvestigationRecordExportFormatterTests::
    structuredJsonIncludesAvailableRecordData()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-147");

    record.timestamp =
        QDateTime::fromString(
            "2026-08-31T14:32:18.245Z",
            Qt::ISODateWithMs
            );

    record.severity =
        RecordSeverity::Error;

    record.subsystem =
        QStringLiteral("Payments");

    record.eventCode =
        QStringLiteral("DB_TIMEOUT");

    record.entityId =
        QStringLiteral("order-1842");

    record.message =
        QStringLiteral(
            "Database request timed out"
            );

    record.customAttributes.insert(
        QStringLiteral("durationMs"),
        5032
        );

    record.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("app-02")
        );

    record.source.sourcePath =
        QStringLiteral(
            "C:/logs/order-fulfillment.jsonl"
            );

    record.source.sourceName =
        QStringLiteral(
            "order-fulfillment.jsonl"
            );

    record.source.recordNumber = 147;

    record.rawSource =
        QStringLiteral(
            R"({"level":"error","message":"Database request timed out"})"
            );

    InvestigationRecordExportFormatter formatter;

    const QString output =
        formatter.toStructuredJson(record);

    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            output.toUtf8(),
            &error
            );

    QCOMPARE(
        error.error,
        QJsonParseError::NoError
        );

    QVERIFY(document.isObject());

    const QJsonObject object =
        document.object();

    QCOMPARE(
        object.value("recordId").toString(),
        QString("record-147")
        );

    QCOMPARE(
        object.value("timestamp").toString(),
        QString("2026-08-31T14:32:18.245Z")
        );

    QCOMPARE(
        object.value("severity").toString(),
        QString("ERROR")
        );

    QCOMPARE(
        object.value("subsystem").toString(),
        QString("Payments")
        );

    QCOMPARE(
        object.value("eventCode").toString(),
        QString("DB_TIMEOUT")
        );

    QCOMPARE(
        object.value("entityId").toString(),
        QString("order-1842")
        );

    QCOMPARE(
        object.value("message").toString(),
        QString("Database request timed out")
        );

    const QJsonObject attributes =
        object.value(
                  "customAttributes"
                  ).toObject();

    QCOMPARE(
        attributes.value("durationMs").toInt(),
        5032
        );

    QCOMPARE(
        attributes.value("host").toString(),
        QString("app-02")
        );

    const QJsonObject source =
        object.value("source").toObject();

    QCOMPARE(
        source.value("sourcePath").toString(),
        QString(
            "C:/logs/order-fulfillment.jsonl"
            )
        );

    QCOMPARE(
        source.value("sourceName").toString(),
        QString("order-fulfillment.jsonl")
        );

    QCOMPARE(
        source.value("recordNumber").toInteger(),
        147
        );

    QCOMPARE(
        object.value("rawSource").toString(),
        record.rawSource
        );
}

void InvestigationRecordExportFormatterTests::
    structuredJsonOmitsUnavailableCanonicalFields()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-minimal");

    record.source.sourceName =
        QStringLiteral("minimal.log");

    record.source.recordNumber = 3;

    InvestigationRecordExportFormatter formatter;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            formatter.toStructuredJson(
                         record
                         ).toUtf8()
            );

    QVERIFY(document.isObject());

    const QJsonObject object =
        document.object();

    QVERIFY(
        !object.contains("timestamp")
        );

    QVERIFY(
        !object.contains("severity")
        );

    QVERIFY(
        !object.contains("subsystem")
        );

    QVERIFY(
        !object.contains("eventCode")
        );

    QVERIFY(
        !object.contains("entityId")
        );

    QVERIFY(
        !object.contains("message")
        );

    QVERIFY(
        object.contains("customAttributes")
        );

    QVERIFY(
        object.value(
                  "customAttributes"
                  ).toObject().isEmpty()
        );

    QVERIFY(object.contains("source"));
    QVERIFY(object.contains("rawSource"));
}

void InvestigationRecordExportFormatterTests::
    structuredJsonPreservesEscapedAndStructuredValues()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-special");

    record.message =
        QString::fromUtf8(
            "Quoted \"value\" <tag> & Unicode: café\n"
            "second line"
            );

    QVariantMap metrics;
    metrics.insert(
        QStringLiteral("latencyMs"),
        42
        );
    metrics.insert(
        QStringLiteral("healthy"),
        false
        );

    QVariantList tags;
    tags.append(
        QStringLiteral("primary")
        );
    tags.append(
        QStringLiteral("βeta")
        );

    record.customAttributes.insert(
        QStringLiteral("metrics"),
        metrics
        );

    record.customAttributes.insert(
        QStringLiteral("tags"),
        tags
        );

    InvestigationRecordExportFormatter formatter;

    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            formatter.toStructuredJson(
                         record
                         ).toUtf8(),
            &error
            );

    QCOMPARE(
        error.error,
        QJsonParseError::NoError
        );

    const QJsonObject object =
        document.object();

    QCOMPARE(
        object.value("message").toString(),
        record.message.value()
        );

    const QJsonObject attributes =
        object.value(
                  "customAttributes"
                  ).toObject();

    const QJsonObject metricsObject =
        attributes.value(
                      "metrics"
                      ).toObject();

    QCOMPARE(
        metricsObject.value(
                         "latencyMs"
                         ).toInt(),
        42
        );

    QCOMPARE(
        metricsObject.value(
                         "healthy"
                         ).toBool(),
        false
        );

    const QJsonArray tagArray =
        attributes.value(
                      "tags"
                      ).toArray();

    QCOMPARE(tagArray.size(), 2);

    QCOMPARE(
        tagArray[0].toString(),
        QString("primary")
        );

    QCOMPARE(
        tagArray[1].toString(),
        QString::fromUtf8("βeta")
        );
}

void InvestigationRecordExportFormatterTests::
    formattedTextIncludesAvailableRecordData()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-21");

    record.timestamp =
        QDateTime::fromString(
            "2026-09-01T11:05:03.125Z",
            Qt::ISODateWithMs
            );

    record.severity =
        RecordSeverity::Warning;

    record.subsystem =
        QStringLiteral("Gateway");

    record.eventCode =
        QStringLiteral("QUEUE_DELAY");

    record.entityId =
        QStringLiteral("request-442");

    record.message =
        QStringLiteral("Queue delay detected");

    record.customAttributes.insert(
        QStringLiteral("queueDepth"),
        18
        );

    record.source.sourceName =
        QStringLiteral("gateway.jsonl");

    record.source.recordNumber = 21;

    record.rawSource =
        QStringLiteral(
            R"({"queueDepth":18})"
            );

    InvestigationRecordExportFormatter formatter;

    const QString output =
        formatter.toFormattedText(record);

    QVERIFY(
        output.contains(
            "Record ID: record-21"
            )
        );

    QVERIFY(
        output.contains(
            "Timestamp: "
            "2026-09-01T11:05:03.125Z"
            )
        );

    QVERIFY(
        output.contains(
            "Severity: WARN"
            )
        );

    QVERIFY(
        output.contains(
            "Subsystem: Gateway"
            )
        );

    QVERIFY(
        output.contains(
            "Event Code: QUEUE_DELAY"
            )
        );

    QVERIFY(
        output.contains(
            "Entity ID: request-442"
            )
        );

    QVERIFY(
        output.contains(
            "Message: Queue delay detected"
            )
        );

    QVERIFY(
        output.contains(
            "Custom Attributes:\n"
            "  queueDepth: 18"
            )
        );

    QVERIFY(
        output.contains(
            "Source:\n"
            "  Name: gateway.jsonl\n"
            "  Record: 21"
            )
        );

    QVERIFY(
        output.contains(
            "Raw Source:\n"
            R"({"queueDepth":18})"
            )
        );
}

void InvestigationRecordExportFormatterTests::
    outputIsDeterministicAcrossCustomAttributeInsertionOrder()
{
    InvestigationRecord firstRecord;
    firstRecord.recordId =
        QStringLiteral("record-order");

    firstRecord.customAttributes.insert(
        QStringLiteral("zeta"),
        QStringLiteral("last")
        );

    firstRecord.customAttributes.insert(
        QStringLiteral("Alpha"),
        QStringLiteral("first")
        );

    firstRecord.customAttributes.insert(
        QStringLiteral("alpha"),
        QStringLiteral("second")
        );

    firstRecord.customAttributes.insert(
        QStringLiteral("middle"),
        QStringLiteral("center")
        );

    InvestigationRecord secondRecord;
    secondRecord.recordId =
        firstRecord.recordId;

    secondRecord.customAttributes.insert(
        QStringLiteral("middle"),
        QStringLiteral("center")
        );

    secondRecord.customAttributes.insert(
        QStringLiteral("alpha"),
        QStringLiteral("second")
        );

    secondRecord.customAttributes.insert(
        QStringLiteral("Alpha"),
        QStringLiteral("first")
        );

    secondRecord.customAttributes.insert(
        QStringLiteral("zeta"),
        QStringLiteral("last")
        );

    InvestigationRecordExportFormatter formatter;

    QCOMPARE(
        formatter.toStructuredJson(
            firstRecord
            ),
        formatter.toStructuredJson(
            secondRecord
            )
        );

    QCOMPARE(
        formatter.toFormattedText(
            firstRecord
            ),
        formatter.toFormattedText(
            secondRecord
            )
        );

    const QString text =
        formatter.toFormattedText(
            firstRecord
            );

    const qsizetype alphaUpper =
        text.indexOf(
            "  Alpha: first"
            );

    const qsizetype alphaLower =
        text.indexOf(
            "  alpha: second"
            );

    const qsizetype middle =
        text.indexOf(
            "  middle: center"
            );

    const qsizetype zeta =
        text.indexOf(
            "  zeta: last"
            );

    QVERIFY(alphaUpper >= 0);
    QVERIFY(alphaLower > alphaUpper);
    QVERIFY(middle > alphaLower);
    QVERIFY(zeta > middle);
}

QTEST_MAIN(
    InvestigationRecordExportFormatterTests
    )

#include "InvestigationRecordExportFormatterTests.moc"