#include <QtTest/QtTest>

#include <QJsonValue>

#include "../tools/live-log-generator/renderers/KeyValueTextRenderer.h"

class KeyValueTextRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void initialContentIsEmpty();
    void recordIsRenderedInDeterministicOrder();
    void unsafeStringValuesAreQuotedAndEscaped();
    void scalarAttributeValuesAreRendered();
    void canonicalAttributeNamesAreIgnoredCaseInsensitively();
    void attributeKeysAreDeduplicatedCaseInsensitively();
};

void KeyValueTextRendererTests::
    initialContentIsEmpty()
{
    const KeyValueTextRenderer renderer;

    QVERIFY(
        renderer.initialContent().isEmpty()
        );
}

void KeyValueTextRendererTests::
    recordIsRenderedInDeterministicOrder()
{
    const KeyValueTextRenderer renderer;

    LiveLogRecord record;

    record.timestampOffsetMs = 2500;
    record.severity =
        QStringLiteral("WARN");
    record.subsystem =
        QStringLiteral("Transport");
    record.eventCode =
        QStringLiteral("LATENCY_HIGH");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral(
            "Latency increased above threshold."
            );

    /*
     * Insert the custom attributes in the
     * opposite order from their expected
     * serialized order. The renderer must not
     * depend on QHash iteration order.
     */
    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    const QDateTime start =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QCOMPARE(
        renderer.renderRecord(
            record,
            start
            ),
        QByteArray(
            "timestamp=2026-09-06T12:00:02.500Z "
            "level=WARN "
            "subsystem=Transport "
            "eventCode=LATENCY_HIGH "
            "entityId=gateway-17 "
            "latencyMs=1840 "
            "site=north-yard "
            "message=\"Latency increased above threshold.\"\n"
            )
        );
}

void KeyValueTextRendererTests::
    unsafeStringValuesAreQuotedAndEscaped()
{
    const KeyValueTextRenderer renderer;

    LiveLogRecord record;

    record.message =
        QStringLiteral(
            "Collector said \"retry\".\n"
            "Path C:\\logs\tready"
            );

    const QDateTime start =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            start
            );

    QVERIFY(
        rendered.contains(
            "message=\"Collector said "
            "\\\"retry\\\".\\n"
            "Path C:\\\\logs\\tready\""
            )
        );
}

void KeyValueTextRendererTests::
    scalarAttributeValuesAreRendered()
{
    const KeyValueTextRenderer renderer;

    LiveLogRecord record;

    record.message =
        QStringLiteral("Complete");

    record.attributes.insert(
        QStringLiteral("count"),
        42
        );

    record.attributes.insert(
        QStringLiteral("ratio"),
        3.5
        );

    record.attributes.insert(
        QStringLiteral("active"),
        true
        );

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
        );

    record.attributes.insert(
        QStringLiteral("optional"),
        QJsonValue(
            QJsonValue::Null
            ).toVariant()
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-09-06T12:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QVERIFY(
        rendered.contains(
            "active=true"
            )
        );

    QVERIFY(
        rendered.contains(
            "count=42"
            )
        );

    QVERIFY(
        rendered.contains(
            "optional=null"
            )
        );

    QVERIFY(
        rendered.contains(
            "ratio=3.5"
            )
        );

    QVERIFY(
        rendered.contains(
            "site=north-yard"
            )
        );
}

void KeyValueTextRendererTests::
    canonicalAttributeNamesAreIgnoredCaseInsensitively()
{
    const KeyValueTextRenderer renderer;

    LiveLogRecord record;

    record.severity =
        QStringLiteral("WARN");
    record.subsystem =
        QStringLiteral("Transport");
    record.eventCode =
        QStringLiteral("LATENCY_HIGH");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral("Real message.");

    record.attributes.insert(
        QStringLiteral("LEVEL"),
        QStringLiteral("FAKE")
        );

    record.attributes.insert(
        QStringLiteral("Subsystem"),
        QStringLiteral("FakeSubsystem")
        );

    record.attributes.insert(
        QStringLiteral("EVENTCODE"),
        QStringLiteral("FAKE_EVENT")
        );

    record.attributes.insert(
        QStringLiteral("EntityId"),
        QStringLiteral("fake-entity")
        );

    record.attributes.insert(
        QStringLiteral("MESSAGE"),
        QStringLiteral("Fake message")
        );

    record.attributes.insert(
        QStringLiteral("TIMESTAMP"),
        QStringLiteral("1900-01-01T00:00:00Z")
        );

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-09-06T12:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QCOMPARE(
        rendered,
        QByteArray(
            "timestamp=2026-09-06T12:00:00.000Z "
            "level=WARN "
            "subsystem=Transport "
            "eventCode=LATENCY_HIGH "
            "entityId=gateway-17 "
            "site=north "
            "message=\"Real message.\"\n"
            )
        );

    QVERIFY(
        !rendered.contains(
            "FAKE"
            )
        );

    QVERIFY(
        !rendered.contains(
            "1900-01-01"
            )
        );
}

void KeyValueTextRendererTests::
    attributeKeysAreDeduplicatedCaseInsensitively()
{
    const KeyValueTextRenderer renderer;

    LiveLogRecord record;

    record.message =
        QStringLiteral("Complete");

    /*
     * Both names are legal QHash keys, but they
     * represent the same logfmt field when key
     * matching is treated case-insensitively.
     *
     * Deterministic sorting places "Site" before
     * "site", so the first spelling/value wins.
     */
    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("south")
        );

    record.attributes.insert(
        QStringLiteral("Site"),
        QStringLiteral("north")
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-09-06T12:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QVERIFY(
        rendered.contains(
            "Site=north"
            )
        );

    QVERIFY(
        !rendered.contains(
            "site=south"
            )
        );

    QCOMPARE(
        rendered.count(
            "Site="
            )
            + rendered.count(
                "site="
                ),
        1
        );
}

QTEST_MAIN(KeyValueTextRendererTests)

#include "KeyValueTextRendererTests.moc"