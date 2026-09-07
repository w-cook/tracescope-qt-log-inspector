#include <QtTest/QtTest>

#include <QXmlStreamReader>
#include <QRegularExpression>

#include "../tools/live-log-generator/renderers/WindowsEventXmlRenderer.h"

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

class WindowsEventXmlRendererTests
    : public QObject
{
    Q_OBJECT

private slots:
    void containerBoundariesAreRendered();
    void canonicalFieldsAreRendered();
    void severityUsesWindowsLevelNumbers();
    void customAttributesAreRenderedAsNamedData();
    void reservedNamedDataCannotOverrideCanonicalValues();
    void specialCharactersAreEscaped();
    void customAttributesAreDeterministic();
    void stableEventIdIsDeterministic();
    void completeOutputFormsValidXmlDocument();
};

void WindowsEventXmlRendererTests::
    containerBoundariesAreRendered()
{
    WindowsEventXmlRenderer renderer;

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Events>\n"
            )
        );

    QVERIFY(
        renderer.recordSeparator()
            .isEmpty()
        );

    QCOMPARE(
        renderer.finalContent(),
        QByteArray(
            "</Events>\n"
            )
        );
}

void WindowsEventXmlRendererTests::
    canonicalFieldsAreRendered()
{
    WindowsEventXmlRenderer renderer;

    const QByteArray rendered =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "Provider Name=\"Transport\""
            )
        );

    QVERIFY(
        rendered.contains(
            "<Level>3</Level>"
            )
        );

    QVERIFY(
        rendered.contains(
            "SystemTime=\"2026-09-07T12:00:02.500Z\""
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"EventCode\">"
            "UPSTREAM_LATENCY"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"DeviceId\">"
            "gateway-17"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Message>"
            "Latency increased above threshold."
            "</Message>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Level>Warning</Level>"
            )
        );
}

void WindowsEventXmlRendererTests::
    severityUsesWindowsLevelNumbers()
{
    WindowsEventXmlRenderer renderer;

    struct SeverityCase
    {
        QString severity;
        QByteArray expectedSystemLevel;
        QByteArray expectedRenderedLevel;
    };

    const QList<SeverityCase> cases = {
        {
            QStringLiteral("CRITICAL"),
            QByteArray("<Level>1</Level>"),
            QByteArray("<Level>Critical</Level>")
        },
        {
            QStringLiteral("ERROR"),
            QByteArray("<Level>2</Level>"),
            QByteArray("<Level>Error</Level>")
        },
        {
            QStringLiteral("WARN"),
            QByteArray("<Level>3</Level>"),
            QByteArray("<Level>Warning</Level>")
        },
        {
            QStringLiteral("INFO"),
            QByteArray("<Level>4</Level>"),
            QByteArray("<Level>Information</Level>")
        },
        {
            QStringLiteral("DEBUG"),
            QByteArray("<Level>5</Level>"),
            QByteArray("<Level>Verbose</Level>")
        },
        {
            QStringLiteral("TRACE"),
            QByteArray("<Level>5</Level>"),
            QByteArray("<Level>Verbose</Level>")
        }
    };

    for (const SeverityCase &testCase : cases) {
        LiveLogRecord record =
            createRecord();

        record.severity =
            testCase.severity;

        const QByteArray rendered =
            renderer.renderRecord(
                record,
                scenarioStart()
                );

        QVERIFY2(
            rendered.contains(
                testCase.expectedSystemLevel
                ),
            qPrintable(
                QStringLiteral(
                    "Missing expected System.Level for %1"
                    )
                    .arg(
                        testCase.severity
                        )
                )
            );

        QVERIFY2(
            rendered.contains(
                testCase.expectedRenderedLevel
                ),
            qPrintable(
                QStringLiteral(
                    "Missing expected rendered level for %1"
                    )
                    .arg(
                        testCase.severity
                        )
                )
            );
    }
}

void WindowsEventXmlRendererTests::
    customAttributesAreRenderedAsNamedData()
{
    WindowsEventXmlRenderer renderer;

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

    QVERIFY(
        rendered.contains(
            "<Data Name=\"retrying\">"
            "false"
            "</Data>"
            )
        );
}

void WindowsEventXmlRendererTests::
    reservedNamedDataCannotOverrideCanonicalValues()
{
    WindowsEventXmlRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.attributes.insert(
        QStringLiteral("EventCode"),
        QStringLiteral("WRONG_EVENT")
        );

    record.attributes.insert(
        QStringLiteral("deviceid"),
        QStringLiteral("wrong-device")
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"EventCode\">"
            "UPSTREAM_LATENCY"
            "</Data>"
            )
        );

    QVERIFY(
        rendered.contains(
            "<Data Name=\"DeviceId\">"
            "gateway-17"
            "</Data>"
            )
        );

    QVERIFY(
        !rendered.contains(
            "WRONG_EVENT"
            )
        );

    QVERIFY(
        !rendered.contains(
            "wrong-device"
            )
        );
}

void WindowsEventXmlRendererTests::
    specialCharactersAreEscaped()
{
    WindowsEventXmlRenderer renderer;

    LiveLogRecord record =
        createRecord();

    record.message =
        QStringLiteral(
            "A < B & C > D"
            );

    record.attributes.insert(
        QStringLiteral("request&id"),
        QStringLiteral(
            "alpha < beta & \"quoted\""
            )
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

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

void WindowsEventXmlRendererTests::
    customAttributesAreDeterministic()
{
    WindowsEventXmlRenderer renderer;

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

void WindowsEventXmlRendererTests::
    stableEventIdIsDeterministic()
{
    WindowsEventXmlRenderer renderer;

    const QByteArray first =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    const QByteArray second =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    QRegularExpression expression(
        QStringLiteral(
            "<EventID>(\\d+)</EventID>"
            )
        );

    const QRegularExpressionMatch firstMatch =
        expression.match(
            QString::fromUtf8(
                first
                )
            );

    const QRegularExpressionMatch secondMatch =
        expression.match(
            QString::fromUtf8(
                second
                )
            );

    QVERIFY(
        firstMatch.hasMatch()
        );

    QVERIFY(
        secondMatch.hasMatch()
        );

    QCOMPARE(
        firstMatch.captured(1),
        secondMatch.captured(1)
        );

    const int eventId =
        firstMatch
            .captured(1)
            .toInt();

    QVERIFY(
        eventId >= 1000
        );

    QVERIFY(
        eventId < 60000
        );
}

void WindowsEventXmlRendererTests::
    completeOutputFormsValidXmlDocument()
{
    WindowsEventXmlRenderer renderer;

    LiveLogRecord first =
        createRecord();

    first.timestampOffsetMs = 0;

    first.eventCode =
        QStringLiteral("GW_START");

    first.message =
        QStringLiteral(
            "Gateway started."
            );

    LiveLogRecord second =
        createRecord();

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
    QStringList messages;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement()) {
            continue;
        }

        if (reader.name()
            == QStringLiteral("Event")) {
            ++eventCount;
        }

        if (reader.name()
                == QStringLiteral("Data")
            && reader.attributes()
                       .value(
                           QStringLiteral("Name")
                           )
                   == QStringLiteral(
                       "EventCode"
                       )) {
            eventCodes.append(
                reader.readElementText()
                );

            continue;
        }

        if (reader.name()
            == QStringLiteral("Message")) {
            messages.append(
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

    QCOMPARE(
        messages,
        QStringList({
            QStringLiteral(
                "Gateway started."
                ),
            QStringLiteral(
                "Latency increased."
                )
        })
        );
}

QTEST_MAIN(WindowsEventXmlRendererTests)

#include "WindowsEventXmlRendererTests.moc"