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
            QStringLiteral("tsv")
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