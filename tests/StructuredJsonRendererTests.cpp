#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "../tools/live-log-generator/renderers/StructuredJsonRenderer.h"

namespace
{
LiveLogRecord createRecord()
{
    LiveLogRecord record;

    record.timestampOffsetMs = 2500;

    record.severity =
        QStringLiteral("WARN");

    record.subsystem =
        QStringLiteral("Transport");

    record.eventCode =
        QStringLiteral("UPSTREAM_LATENCY");

    record.entityId =
        QStringLiteral("gateway-17");

    record.message =
        QStringLiteral(
            "Latency increased above threshold."
            );

    return record;
}

QDateTime scenarioStart()
{
    return QDateTime::fromString(
        QStringLiteral(
            "2026-09-07T12:00:00.000Z"
            ),
        Qt::ISODateWithMs
        );
}
}

class StructuredJsonRendererTests
    : public QObject
{
    Q_OBJECT

private slots:
    void containerBoundariesAreRendered();
    void recordUsesCanonicalFields();
    void customAttributesArePreserved();
    void canonicalFieldsOverrideCollidingAttributes();
    void completeOutputFormsStructuredJsonDocument();
};

void StructuredJsonRendererTests::
    containerBoundariesAreRendered()
{
    StructuredJsonRenderer renderer;

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "{\n"
            "  \"data\": {\n"
            "    \"records\": [\n"
            )
        );

    QCOMPARE(
        renderer.recordSeparator(),
        QByteArray(",\n")
        );

    QCOMPARE(
        renderer.finalContent(),
        QByteArray(
            "\n"
            "    ]\n"
            "  }\n"
            "}\n"
            )
        );
}

void StructuredJsonRendererTests::
    recordUsesCanonicalFields()
{
    StructuredJsonRenderer renderer;

    const QByteArray rendered =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            rendered.trimmed()
            );

    QVERIFY(document.isObject());

    const QJsonObject object =
        document.object();

    QCOMPARE(
        object.value(
                  QStringLiteral("timestamp")
                  ).toString(),
        QStringLiteral(
            "2026-09-07T12:00:02.500Z"
            )
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("level")
                  ).toString(),
        QStringLiteral("WARN")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("subsystem")
                  ).toString(),
        QStringLiteral("Transport")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("eventCode")
                  ).toString(),
        QStringLiteral(
            "UPSTREAM_LATENCY"
            )
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("entityId")
                  ).toString(),
        QStringLiteral("gateway-17")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("message")
                  ).toString(),
        QStringLiteral(
            "Latency increased above threshold."
            )
        );
}

void StructuredJsonRendererTests::
    customAttributesArePreserved()
{
    StructuredJsonRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    record.attributes.insert(
        QStringLiteral("retrying"),
        false
        );

    record.attributes.insert(
        QStringLiteral("optionalValue"),
        QVariant()
        );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            renderer.renderRecord(
                        record,
                        scenarioStart()
                        )
                .trimmed()
            );

    QVERIFY(document.isObject());

    const QJsonObject object =
        document.object();

    QCOMPARE(
        object.value(
                  QStringLiteral("site")
                  ).toString(),
        QStringLiteral("north-yard")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("latencyMs")
                  ).toInt(),
        1840
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("retrying")
                  ).toBool(),
        false
        );

    QVERIFY(
        object.contains(
            QStringLiteral("optionalValue")
            )
        );

    QVERIFY(
        object.value(
                  QStringLiteral("optionalValue")
                  ).isNull()
        );
}

void StructuredJsonRendererTests::
    canonicalFieldsOverrideCollidingAttributes()
{
    StructuredJsonRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.attributes.insert(
        QStringLiteral("timestamp"),
        QStringLiteral(
            "1900-01-01T00:00:00.000Z"
            )
        );

    record.attributes.insert(
        QStringLiteral("level"),
        QStringLiteral("DEBUG")
        );

    record.attributes.insert(
        QStringLiteral("subsystem"),
        QStringLiteral("WrongSubsystem")
        );

    record.attributes.insert(
        QStringLiteral("eventCode"),
        QStringLiteral("WRONG_EVENT")
        );

    record.attributes.insert(
        QStringLiteral("entityId"),
        QStringLiteral("wrong-entity")
        );

    record.attributes.insert(
        QStringLiteral("message"),
        QStringLiteral("Wrong message.")
        );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            renderer.renderRecord(
                        record,
                        scenarioStart()
                        )
                .trimmed()
            );

    QVERIFY(document.isObject());

    const QJsonObject object =
        document.object();

    QCOMPARE(
        object.value(
                  QStringLiteral("timestamp")
                  ).toString(),
        QStringLiteral(
            "2026-09-07T12:00:02.500Z"
            )
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("level")
                  ).toString(),
        QStringLiteral("WARN")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("subsystem")
                  ).toString(),
        QStringLiteral("Transport")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("eventCode")
                  ).toString(),
        QStringLiteral(
            "UPSTREAM_LATENCY"
            )
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("entityId")
                  ).toString(),
        QStringLiteral("gateway-17")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("message")
                  ).toString(),
        QStringLiteral(
            "Latency increased above threshold."
            )
        );
}

void StructuredJsonRendererTests::
    completeOutputFormsStructuredJsonDocument()
{
    StructuredJsonRenderer renderer;

    LiveLogRecord first;
    first.timestampOffsetMs = 0;

    first.severity =
        QStringLiteral("INFO");

    first.subsystem =
        QStringLiteral("Gateway");

    first.eventCode =
        QStringLiteral("GW_START");

    first.entityId =
        QStringLiteral("gateway-17");

    first.message =
        QStringLiteral(
            "Gateway started."
            );

    LiveLogRecord second = first;

    second.timestampOffsetMs = 2500;

    second.eventCode =
        QStringLiteral(
            "UPSTREAM_LATENCY"
            );

    second.message =
        QStringLiteral(
            "Latency increased."
            );

    const QDateTime start =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-07T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QByteArray output =
        renderer.initialContent();

    output +=
        renderer.renderRecord(
            first,
            start
            );

    output +=
        renderer.recordSeparator();

    output +=
        renderer.renderRecord(
            second,
            start
            );

    output +=
        renderer.finalContent();

    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            output,
            &error
            );

    QCOMPARE(
        error.error,
        QJsonParseError::NoError
        );

    QVERIFY(document.isObject());

    const QJsonObject root =
        document.object();

    QVERIFY(
        root.value(
                QStringLiteral("data")
                ).isObject()
        );

    const QJsonObject data =
        root.value(
                QStringLiteral("data")
                )
            .toObject();

    QVERIFY(
        data.value(
                QStringLiteral("records")
                ).isArray()
        );

    const QJsonArray records =
        data.value(
                QStringLiteral("records")
                )
            .toArray();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0)
            .toObject()
            .value(
                QStringLiteral("eventCode")
                )
            .toString(),
        QStringLiteral("GW_START")
        );

    QCOMPARE(
        records.at(0)
            .toObject()
            .value(
                QStringLiteral("timestamp")
                )
            .toString(),
        QStringLiteral(
            "2026-09-07T12:00:00.000Z"
            )
        );

    QCOMPARE(
        records.at(1)
            .toObject()
            .value(
                QStringLiteral("eventCode")
                )
            .toString(),
        QStringLiteral(
            "UPSTREAM_LATENCY"
            )
        );

    QCOMPARE(
        records.at(1)
            .toObject()
            .value(
                QStringLiteral("timestamp")
                )
            .toString(),
        QStringLiteral(
            "2026-09-07T12:00:02.500Z"
            )
        );
}

QTEST_MAIN(StructuredJsonRendererTests)

#include "StructuredJsonRendererTests.moc"