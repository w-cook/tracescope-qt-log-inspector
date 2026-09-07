#include <QtTest/QtTest>

#include "../tools/live-log-generator/renderers/SyslogRenderer.h"

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

class SyslogRendererTests
    : public QObject
{
    Q_OBJECT

private slots:
    void boundaryContentIsEmpty();
    void rfc5424UsesExpectedFields();
    void rfc3164UsesExpectedFields();
    void severityMapsToExpectedPriority();
    void rfc5424PreservesCustomAttributes();
    void rfc5424StructuredDataIsEscaped();
    void rfc5424AttributesAreDeterministic();
    void rfc3164DoesNotInventUnsupportedFields();
    void rfc5424ValidationAcceptsSupportedNames();
    void rfc5424ValidationRejectsInvalidNames();
    void rfc3164TagValidationMatchesImporterShape();
};

void SyslogRendererTests::
    boundaryContentIsEmpty()
{
    const SyslogRenderer rfc5424(
        SyslogFormat::Rfc5424
        );

    const SyslogRenderer rfc3164(
        SyslogFormat::Rfc3164
        );

    QVERIFY(
        rfc5424.initialContent()
            .isEmpty()
        );

    QVERIFY(
        rfc5424.recordSeparator()
            .isEmpty()
        );

    QVERIFY(
        rfc5424.finalContent()
            .isEmpty()
        );

    QVERIFY(
        rfc3164.initialContent()
            .isEmpty()
        );

    QVERIFY(
        rfc3164.recordSeparator()
            .isEmpty()
        );

    QVERIFY(
        rfc3164.finalContent()
            .isEmpty()
        );
}

void SyslogRendererTests::
    rfc5424UsesExpectedFields()
{
    const SyslogRenderer renderer(
        SyslogFormat::Rfc5424
        );

    const QByteArray rendered =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    QVERIFY(
        rendered.startsWith(
            "<132>1 "
            "2026-09-07T12:00:02.500Z "
            "tracescope-live "
            "Transport - "
            "UPSTREAM_LATENCY "
            )
        );

    QVERIFY(
        rendered.contains(
            "[tracescope@32473 "
            "entityId=\"gateway-17\"]"
            )
        );

    QVERIFY(
        rendered.endsWith(
            "Latency increased above threshold.\n"
            )
        );
}

void SyslogRendererTests::
    rfc3164UsesExpectedFields()
{
    const SyslogRenderer renderer(
        SyslogFormat::Rfc3164
        );

    const QByteArray rendered =
        renderer.renderRecord(
            createRecord(),
            scenarioStart()
            );

    QCOMPARE(
        rendered,
        QByteArray(
            "<132>Sep  7 12:00:02 "
            "tracescope-live "
            "Transport: "
            "Latency increased above threshold.\n"
            )
        );
}

void SyslogRendererTests::
    severityMapsToExpectedPriority()
{
    struct SeverityCase
    {
        QString severity;
        QByteArray expectedPrefix;
    };

    const QList<SeverityCase> cases = {
        {
            QStringLiteral("CRITICAL"),
            QByteArray("<130>")
        },
        {
            QStringLiteral("ERROR"),
            QByteArray("<131>")
        },
        {
            QStringLiteral("WARN"),
            QByteArray("<132>")
        },
        {
            QStringLiteral("WARNING"),
            QByteArray("<132>")
        },
        {
            QStringLiteral("INFO"),
            QByteArray("<134>")
        },
        {
            QStringLiteral("DEBUG"),
            QByteArray("<135>")
        },
        {
            QStringLiteral("TRACE"),
            QByteArray("<135>")
        }
    };

    for (const SeverityCase &testCase : cases) {
        LiveLogRecord record =
            createRecord();

        record.severity =
            testCase.severity;

        const SyslogRenderer renderer(
            SyslogFormat::Rfc5424
            );

        const QByteArray rendered =
            renderer.renderRecord(
                record,
                scenarioStart()
                );

        QVERIFY2(
            rendered.startsWith(
                testCase.expectedPrefix
                ),
            qPrintable(
                QStringLiteral(
                    "Unexpected priority for severity %1"
                    )
                    .arg(
                        testCase.severity
                        )
                )
            );
    }
}

void SyslogRendererTests::
    rfc5424PreservesCustomAttributes()
{
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
        QStringLiteral("ratio"),
        1.25
        );

    record.attributes.insert(
        QStringLiteral("optionalValue"),
        QVariant()
        );

    const SyslogRenderer renderer(
        SyslogFormat::Rfc5424
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    QVERIFY(
        rendered.contains(
            "entityId=\"gateway-17\""
            )
        );

    QVERIFY(
        rendered.contains(
            "site=\"north-yard\""
            )
        );

    QVERIFY(
        rendered.contains(
            "latencyMs=\"1840\""
            )
        );

    QVERIFY(
        rendered.contains(
            "retrying=\"false\""
            )
        );

    QVERIFY(
        rendered.contains(
            "ratio=\"1.25\""
            )
        );

    QVERIFY(
        rendered.contains(
            "optionalValue=\"null\""
            )
        );
}

void SyslogRendererTests::
    rfc5424StructuredDataIsEscaped()
{
    LiveLogRecord record =
        createRecord();

    record.entityId =
        QStringLiteral(
            R"(gateway\"17])"
            );

    record.attributes.insert(
        QStringLiteral("detail"),
        QStringLiteral(
            R"(Retry "accepted" at C:\temp with bracket ])"
            )
        );

    const SyslogRenderer renderer(
        SyslogFormat::Rfc5424
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    /*
     * RFC 5424 SD-PARAM values escape
     * backslash, double quote, and ']'.
     */
    QVERIFY(
        rendered.contains(
            "entityId=\"gateway\\\\\\\"17\\]\""
            )
        );

    QVERIFY(
        rendered.contains(
            "detail=\"Retry \\\"accepted\\\" "
            "at C:\\\\temp with bracket \\]\""
            )
        );
}

void SyslogRendererTests::
    rfc5424AttributesAreDeterministic()
{
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

    const SyslogRenderer renderer(
        SyslogFormat::Rfc5424
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    const qsizetype endpointIndex =
        rendered.indexOf(
            "endpoint=\""
            );

    const qsizetype latencyIndex =
        rendered.indexOf(
            "LatencyMs=\""
            );

    const qsizetype siteIndex =
        rendered.indexOf(
            "site=\""
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

void SyslogRendererTests::
    rfc3164DoesNotInventUnsupportedFields()
{
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

    const SyslogRenderer renderer(
        SyslogFormat::Rfc3164
        );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart()
            );

    QVERIFY(
        !rendered.contains(
            "UPSTREAM_LATENCY"
            )
        );

    QVERIFY(
        !rendered.contains(
            "gateway-17"
            )
        );

    QVERIFY(
        !rendered.contains(
            "north-yard"
            )
        );

    QVERIFY(
        !rendered.contains(
            "1840"
            )
        );

    QVERIFY(
        rendered.contains(
            "Transport: "
            "Latency increased above threshold."
            )
        );
}

void SyslogRendererTests::
    rfc5424ValidationAcceptsSupportedNames()
{
    QVERIFY(
        SyslogRenderer::
        isValidRfc5424AppName(
            QStringLiteral(
                "gateway-service"
                )
            )
        );

    QVERIFY(
        SyslogRenderer::
        isValidRfc5424MessageId(
            QStringLiteral(
                "UPSTREAM_TIMEOUT"
                )
            )
        );

    QVERIFY(
        SyslogRenderer::
        isValidRfc5424StructuredDataName(
            QStringLiteral(
                "latencyMs"
                )
            )
        );

    QVERIFY(
        SyslogRenderer::
        isValidRfc5424StructuredDataName(
            QStringLiteral(
                "retry_count"
                )
            )
        );
}

void SyslogRendererTests::
    rfc5424ValidationRejectsInvalidNames()
{
    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424AppName(
            QString()
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424AppName(
            QStringLiteral(
                "contains space"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424MessageId(
            QString()
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424MessageId(
            QStringLiteral(
                "contains space"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424StructuredDataName(
            QStringLiteral(
                "request id"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424StructuredDataName(
            QStringLiteral(
                "request=id"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc5424StructuredDataName(
            QStringLiteral(
                "request]id"
                )
            )
        );
}

void SyslogRendererTests::
    rfc3164TagValidationMatchesImporterShape()
{
    QVERIFY(
        SyslogRenderer::
        isValidRfc3164Tag(
            QStringLiteral(
                "gateway-service"
                )
            )
        );

    QVERIFY(
        SyslogRenderer::
        isValidRfc3164Tag(
            QStringLiteral(
                "gateway.service_1"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc3164Tag(
            QStringLiteral(
                "gateway service"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc3164Tag(
            QStringLiteral(
                "gateway:service"
                )
            )
        );

    QVERIFY(
        !SyslogRenderer::
        isValidRfc3164Tag(
            QString()
            )
        );
}

QTEST_MAIN(SyslogRendererTests)

#include "SyslogRendererTests.moc"