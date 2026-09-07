#include <QtTest/QtTest>

#include "../tools/live-log-generator/LogRecordRendererFactory.h"

class LogRecordRendererFactoryTests
    : public QObject
{
    Q_OBJECT

private slots:
    void supportedFormatsAreReported();
    void jsonLinesRendererIsCreated();
    void csvRendererUsesScenarioSchema();
    void tsvRendererUsesScenarioSchema();
    void logfmtRendererIsCreated();
    void invalidLogfmtAttributeKeyIsRejected();
    void logfmtAllowsAttributeKeysSupportedByImporter();
    void syslogRfc5424RendererIsCreated();
    void invalidRfc5424AppNameIsRejected();
    void invalidRfc5424MessageIdIsRejected();
    void invalidRfc5424StructuredDataNameIsRejected();
    void syslogRfc3164RendererIsCreated();
    void invalidRfc3164TagIsRejected();
    void syslogMessageWithLineBreakIsRejected();
    void structuredJsonRendererIsCreated();
    void structuredXmlRendererIsCreated();
    void windowsEventXmlRendererIsCreated();
    void unsupportedFormatIsRejected();
};

void LogRecordRendererFactoryTests::
    supportedFormatsAreReported()
{
    QCOMPARE(
        LogRecordRendererFactory::
        supportedFormats(),
        QStringList({
            QStringLiteral("jsonl"),
            QStringLiteral("csv"),
            QStringLiteral("tsv"),
            QStringLiteral("logfmt"),
            QStringLiteral("syslog-rfc5424"),
            QStringLiteral("syslog-rfc3164"),
            QStringLiteral("structured-json"),
            QStringLiteral("structured-xml"),
            QStringLiteral("windows-event-xml")
        })
        );
}

void LogRecordRendererFactoryTests::
    jsonLinesRendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("JSONL"),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QVERIFY(
        result.renderer
            ->initialContent()
            .isEmpty()
        );
}

void LogRecordRendererFactoryTests::
    csvRendererUsesScenarioSchema()
{
    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    step.record.attributes.insert(
        QStringLiteral("latencyMs"),
        42
        );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("csv"),
            scenario
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        result.renderer
            ->initialContent(),
        QByteArray(
            "timestamp,level,subsystem,eventCode,"
            "entityId,message,latencyMs,site\n"
            )
        );
}

void LogRecordRendererFactoryTests::
    tsvRendererUsesScenarioSchema()
{
    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("tsv"),
            scenario
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        result.renderer
            ->initialContent(),
        QByteArray(
            "timestamp\tlevel\tsubsystem\t"
            "eventCode\tentityId\tmessage\tsite\n"
            )
        );
}

void LogRecordRendererFactoryTests::
    logfmtRendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("LOGFMT"),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QVERIFY(
        result.renderer
            ->initialContent()
            .isEmpty()
        );

    LiveLogRecord record;

    record.severity =
        QStringLiteral("INFO");
    record.subsystem =
        QStringLiteral("Gateway");
    record.eventCode =
        QStringLiteral("STARTED");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral("Started");

    const QByteArray rendered =
        result.renderer->renderRecord(
            record,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-09-06T12:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QVERIFY(
        rendered.startsWith(
            "timestamp=2026-09-06T12:00:00.000Z "
            )
        );

    QVERIFY(
        rendered.contains(
            "level=INFO"
            )
        );
}

void LogRecordRendererFactoryTests::
    invalidLogfmtAttributeKeyIsRejected()
{
    LiveLogScenario scenario;

    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("request id"),
        QStringLiteral("abc-123")
        );

    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("logfmt"),
            scenario
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_LOGFMT_ATTRIBUTE_KEY"
            )
        );

    QVERIFY(
        result.errorMessage.contains(
            QStringLiteral("request id")
            )
        );
}

void LogRecordRendererFactoryTests::
    logfmtAllowsAttributeKeysSupportedByImporter()
{
    LiveLogScenario scenario;

    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("request-id"),
        QStringLiteral("abc-123")
        );

    step.record.attributes.insert(
        QStringLiteral("http.status"),
        200
        );

    step.record.attributes.insert(
        QStringLiteral("retry_count"),
        2
        );

    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("logfmt"),
            scenario
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);
}

void LogRecordRendererFactoryTests::
    syslogRfc5424RendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "SYSLOG-RFC5424"
                ),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QVERIFY(
        result.renderer
            ->initialContent()
            .isEmpty()
        );
}

void LogRecordRendererFactoryTests::
    syslogRfc3164RendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "SYSLOG-RFC3164"
                ),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QVERIFY(
        result.renderer
            ->initialContent()
            .isEmpty()
        );
}

void LogRecordRendererFactoryTests::
    invalidRfc5424AppNameIsRejected()
{
    LiveLogRecordStep step;

    step.record.subsystem =
        QStringLiteral(
            "gateway service"
            );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc5424"
                ),
            scenario
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_RFC5424_APP_NAME"
            )
        );
}

void LogRecordRendererFactoryTests::
    invalidRfc5424MessageIdIsRejected()
{
    LiveLogRecordStep step;

    step.record.eventCode =
        QStringLiteral(
            "EVENT CODE"
            );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc5424"
                ),
            scenario
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_RFC5424_MESSAGE_ID"
            )
        );
}

void LogRecordRendererFactoryTests::
    invalidRfc5424StructuredDataNameIsRejected()
{
    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral(
            "request id"
            ),
        QStringLiteral(
            "REQ-123"
            )
        );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc5424"
                ),
            scenario
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_RFC5424_STRUCTURED_DATA_NAME"
            )
        );
}

void LogRecordRendererFactoryTests::
    invalidRfc3164TagIsRejected()
{
    LiveLogRecordStep step;

    step.record.subsystem =
        QStringLiteral(
            "gateway service"
            );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc3164"
                ),
            scenario
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_RFC3164_TAG"
            )
        );
}

void LogRecordRendererFactoryTests::
    syslogMessageWithLineBreakIsRejected()
{
    LiveLogRecordStep step;

    step.record.message =
        QStringLiteral(
            "First line\nSecond line"
            );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    const auto rfc5424 =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc5424"
                ),
            scenario
            );

    QVERIFY(!rfc5424.isSuccess());

    QCOMPARE(
        rfc5424.errorCode,
        QStringLiteral(
            "INVALID_SYSLOG_MESSAGE"
            )
        );

    const auto rfc3164 =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "syslog-rfc3164"
                ),
            scenario
            );

    QVERIFY(!rfc3164.isSuccess());

    QCOMPARE(
        rfc3164.errorCode,
        QStringLiteral(
            "INVALID_SYSLOG_MESSAGE"
            )
        );
}

void LogRecordRendererFactoryTests::
    structuredJsonRendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "STRUCTURED-JSON"
                ),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QCOMPARE(
        result.renderer
            ->initialContent(),
        QByteArray(
            "{\n"
            "  \"data\": {\n"
            "    \"records\": [\n"
            )
        );

    QCOMPARE(
        result.renderer
            ->recordSeparator(),
        QByteArray(",\n")
        );

    QCOMPARE(
        result.renderer
            ->finalContent(),
        QByteArray(
            "\n"
            "    ]\n"
            "  }\n"
            "}\n"
            )
        );
}

void LogRecordRendererFactoryTests::
    structuredXmlRendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "STRUCTURED-XML"
                ),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QCOMPARE(
        result.renderer
            ->initialContent(),
        QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<session>\n"
            "    <events>\n"
            )
        );

    QVERIFY(
        result.renderer
            ->recordSeparator()
            .isEmpty()
        );

    QCOMPARE(
        result.renderer
            ->finalContent(),
        QByteArray(
            "    </events>\n"
            "</session>\n"
            )
        );
}

void LogRecordRendererFactoryTests::
    windowsEventXmlRendererIsCreated()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral(
                "WINDOWS-EVENT-XML"
                ),
            LiveLogScenario()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.renderer);

    QCOMPARE(
        result.renderer
            ->initialContent(),
        QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Events>\n"
            )
        );

    QVERIFY(
        result.renderer
            ->recordSeparator()
            .isEmpty()
        );

    QCOMPARE(
        result.renderer
            ->finalContent(),
        QByteArray(
            "</Events>\n"
            )
        );
}

void LogRecordRendererFactoryTests::
    unsupportedFormatIsRejected()
{
    const auto result =
        LogRecordRendererFactory::create(
            QStringLiteral("bananas"),
            LiveLogScenario()
            );

    QVERIFY(!result.isSuccess());
    QVERIFY(!result.renderer);

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNSUPPORTED_OUTPUT_FORMAT"
            )
        );
}

QTEST_MAIN(LogRecordRendererFactoryTests)

#include "LogRecordRendererFactoryTests.moc"