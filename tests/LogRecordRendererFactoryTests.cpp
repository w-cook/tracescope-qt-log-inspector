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
    void structuredJsonRendererIsCreated();
    void structuredXmlRendererIsCreated();
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
            QStringLiteral("structured-json"),
            QStringLiteral("structured-xml")
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