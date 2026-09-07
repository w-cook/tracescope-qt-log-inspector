#include <QtTest/QtTest>

#include <QBuffer>
#include <QXmlStreamReader>

#include "../tools/live-log-generator/renderers/StructuredXmlRenderer.h"

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

class StructuredXmlRendererTests
    : public QObject
{
    Q_OBJECT

private slots:
    void containerBoundariesAreRendered();
    void recordUsesCanonicalFields();
    void customAttributesAreRenderedAsNamedData();
    void specialCharactersAreEscaped();
    void customAttributesAreDeterministic();
    void scalarAttributeValuesAreRendered();
    void completeOutputFormsValidXmlDocument();
};

void StructuredXmlRendererTests::
    containerBoundariesAreRendered()
{
    StructuredXmlRenderer renderer;

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<session>\n"
            "    <events>\n"
            )
        );

    QVERIFY(
        renderer.recordSeparator()
            .isEmpty()
        );

    QCOMPARE(
        renderer.finalContent(),
        QByteArray(
            "    </events>\n"
            "</session>\n"
            )
        );
}

void StructuredXmlRendererTests::
    recordUsesCanonicalFields()
{
    StructuredXmlRenderer renderer;

    const QByteArray rendered =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "<timestamp>"
            "2026-09-07T12:00:02.500Z"
            "</timestamp>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<level>WARN</level>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<component>"
            "Transport"
            "</component>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<code>"
            "UPSTREAM_LATENCY"
            "</code>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<deviceId>"
            "gateway-17"
            "</deviceId>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<message>"
            "Latency increased above threshold."
            "</message>"
            )
        );
}

void StructuredXmlRendererTests::
    customAttributesAreRenderedAsNamedData()
{
    StructuredXmlRenderer renderer;

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

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"site\">"
            "north-yard"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"latencyMs\">"
            "1840"
            "</Data>"
            )
        );
}

void StructuredXmlRendererTests::
    specialCharactersAreEscaped()
{
    StructuredXmlRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.message =
        QStringLiteral(
            "A < B & C > D"
            );

    record.attributes.insert(
        QStringLiteral(
            "request&id"
            ),
        QStringLiteral(
            "alpha < beta & \"quoted\""
            )
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    /*
     * Parse the fragment inside a temporary root
     * element instead of relying only on raw
     * string escaping.
     */
    QByteArray wrapped =
        QByteArray("<root>")
        + rendered
        + QByteArray("</root>");

    QXmlStreamReader reader(
        wrapped
        );

    while (!reader.atEnd()) {
        reader.readNext();
    }

    QVERIFY(
        !reader.hasError()
        );

    QVERIFY(
        rendered.contains(
            "A &lt; B &amp; C &gt; D"
            )
        );

    QVERIFY(
        rendered.contains(
            "request&amp;id"
            )
        );

    QVERIFY(
        rendered.contains(
            "alpha &lt; beta &amp; "
            "&quot;quoted&quot;"
            )
        );
}

void StructuredXmlRendererTests::
    customAttributesAreDeterministic()
{
    StructuredXmlRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
        );

    record.attributes.insert(
        QStringLiteral("LatencyMs"),
        1840
        );

    record.attributes.insert(
        QStringLiteral("endpoint"),
        QStringLiteral("collector-a")
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    const qsizetype endpointIndex =
        rendered.indexOf(
            "Name=\"endpoint\""
            );

    const qsizetype latencyIndex =
        rendered.indexOf(
            "Name=\"LatencyMs\""
            );

    const qsizetype siteIndex =
        rendered.indexOf(
            "Name=\"site\""
            );

    QVERIFY(
        endpointIndex >= 0
        );

    QVERIFY(
        latencyIndex >= 0
        );

    QVERIFY(
        siteIndex >= 0
        );

    QVERIFY(
        endpointIndex
        < latencyIndex
        );

    QVERIFY(
        latencyIndex
        < siteIndex
        );
}

void StructuredXmlRendererTests::
    scalarAttributeValuesAreRendered()
{
    StructuredXmlRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.attributes.insert(
        QStringLiteral("retrying"),
        false
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        58
        );

    record.attributes.insert(
        QStringLiteral("ratio"),
        1.25
        );

    record.attributes.insert(
        QStringLiteral("optionalValue"),
        QVariant()
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"retrying\">"
            "false"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"latencyMs\">"
            "58"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"ratio\">"
            "1.25"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"optionalValue\">"
            "null"
            "</Data>"
            )
        );
}

void StructuredXmlRendererTests::
    completeOutputFormsValidXmlDocument()
{
    StructuredXmlRenderer renderer;

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

    second.timestampOffsetMs =
        2500;

    second.eventCode =
        QStringLiteral(
            "UPSTREAM_LATENCY"
            );

    second.message =
        QStringLiteral(
            "Latency increased."
            );

    QByteArray output =
        renderer.initialContent();

    output +=
        renderer.renderRecord(
            first,
            scenarioStart()
            );

    output +=
        renderer.renderRecord(
            second,
            scenarioStart()
            );

    output +=
        renderer.finalContent();

    QXmlStreamReader reader(
        output
        );

    int eventCount = 0;

    QStringList eventCodes;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement()) {
            continue;
        }

        if (reader.name()
            == QStringLiteral("event")) {
            ++eventCount;
        }

        if (reader.name()
            == QStringLiteral("code")) {
            eventCodes.append(
                reader.readElementText()
                );
        }
    }

    QVERIFY(
        !reader.hasError()
        );

    QCOMPARE(
        eventCount,
        2
        );

    QCOMPARE(
        eventCodes,
        QStringList({
            QStringLiteral("GW_START"),
            QStringLiteral(
                "UPSTREAM_LATENCY"
                )
        })
        );
}

QTEST_MAIN(StructuredXmlRendererTests)

#include "StructuredXmlRendererTests.moc"