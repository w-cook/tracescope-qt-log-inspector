#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>

#include "../tools/live-log-generator/renderers/JsonLinesRenderer.h"

class JsonLinesRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void initialContentIsEmpty();
    void recordUsesCanonicalJsonLinesFields();
    void timestampOffsetIsApplied();
    void customAttributesAreRendered();
    void canonicalFieldsOverrideConflictingAttributes();
};

void JsonLinesRendererTests::
    initialContentIsEmpty()
{
    const JsonLinesRenderer renderer;

    QVERIFY(renderer.initialContent().isEmpty());
}

void JsonLinesRendererTests::
    recordUsesCanonicalJsonLinesFields()
{
    LiveLogRecord record;

    record.timestampOffsetMs = 0;
    record.severity = QStringLiteral("WARN");
    record.subsystem = QStringLiteral("Transport");
    record.eventCode =
        QStringLiteral("UPSTREAM_LATENCY");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral(
            "Latency exceeded expected range."
            );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        JsonLinesRenderer().renderRecord(
            record,
            scenarioStart
            );

    QVERIFY(rendered.endsWith('\n'));

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
            "2026-09-06T12:00:00.000Z"
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
        QStringLiteral("UPSTREAM_LATENCY")
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
            "Latency exceeded expected range."
            )
        );
}

void JsonLinesRendererTests::
    timestampOffsetIsApplied()
{
    LiveLogRecord record;

    record.timestampOffsetMs = 3750;

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        JsonLinesRenderer().renderRecord(
            record,
            scenarioStart
            );

    const QJsonObject object =
        QJsonDocument::fromJson(
            rendered.trimmed()
            ).object();

    QCOMPARE(
        object.value(
                  QStringLiteral("timestamp")
                  ).toString(),
        QStringLiteral(
            "2026-09-06T12:00:03.750Z"
            )
        );
}

void JsonLinesRendererTests::
    customAttributesAreRendered()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("endpoint"),
        QStringLiteral("collector-a")
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    record.attributes.insert(
        QStringLiteral("retrying"),
        true
        );

    record.attributes.insert(
        QStringLiteral("correlationId"),
        QVariant()
        );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QJsonObject object =
        QJsonDocument::fromJson(
            JsonLinesRenderer()
                .renderRecord(
                    record,
                    scenarioStart
                    )
                .trimmed()
            ).object();

    QCOMPARE(
        object.value(
                  QStringLiteral("endpoint")
                  ).toString(),
        QStringLiteral("collector-a")
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
        true
        );

    QVERIFY(
        object.contains(
            QStringLiteral("correlationId")
            )
        );

    QVERIFY(
        object.value(
                  QStringLiteral("correlationId")
                  ).isNull()
        );
}

void JsonLinesRendererTests::
    canonicalFieldsOverrideConflictingAttributes()
{
    LiveLogRecord record;

    record.severity =
        QStringLiteral("ERROR");

    record.message =
        QStringLiteral("Canonical message.");

    record.attributes.insert(
        QStringLiteral("level"),
        QStringLiteral("FAKE")
        );

    record.attributes.insert(
        QStringLiteral("message"),
        QStringLiteral(
            "Attribute message."
            )
        );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QJsonObject object =
        QJsonDocument::fromJson(
            JsonLinesRenderer()
                .renderRecord(
                    record,
                    scenarioStart
                    )
                .trimmed()
            ).object();

    QCOMPARE(
        object.value(
                  QStringLiteral("level")
                  ).toString(),
        QStringLiteral("ERROR")
        );

    QCOMPARE(
        object.value(
                  QStringLiteral("message")
                  ).toString(),
        QStringLiteral(
            "Canonical message."
            )
        );
}

QTEST_MAIN(JsonLinesRendererTests)

#include "JsonLinesRendererTests.moc"