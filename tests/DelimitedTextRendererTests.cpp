#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTemporaryDir>

#include "../tools/live-log-generator/LiveLogScenarioPlayer.h"
#include "../tools/live-log-generator/renderers/DelimitedTextRenderer.h"

class DelimitedTextRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void scenarioAttributeKeysAreDeterministic();
    void csvHeaderAndRecordAreRendered();
    void csvFieldsAreQuotedCorrectly();
    void tsvUsesTabDelimiter();
    void canonicalAttributeNamesAreIgnored();
    void truncateRegeneratesHeader();
    void replaceRegeneratesHeader();
    void rotateRegeneratesHeader();
    void canonicalAttributeNamesAreIgnoredCaseInsensitively();
    void attributeKeysAreDeduplicatedCaseInsensitively();
};

void DelimitedTextRendererTests::
    scenarioAttributeKeysAreDeterministic()
{
    LiveLogRecordStep first;

    first.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    first.record.attributes.insert(
        QStringLiteral("latencyMs"),
        42
        );

    LiveLogRecordStep second;

    second.record.attributes.insert(
        QStringLiteral("endpoint"),
        QStringLiteral("collector-a")
        );

    second.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("south")
        );

    LiveLogScenario scenario;

    scenario.steps.append(first);
    scenario.steps.append(second);

    QCOMPARE(
        DelimitedTextRenderer::
        attributeKeysForScenario(
            scenario
            ),
        QStringList({
            QStringLiteral("endpoint"),
            QStringLiteral("latencyMs"),
            QStringLiteral("site")
        })
        );
}

void DelimitedTextRendererTests::
    csvHeaderAndRecordAreRendered()
{
    const DelimitedTextRenderer renderer(
        QLatin1Char(','),
        {
            QStringLiteral("latencyMs"),
            QStringLiteral("site")
        }
        );

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "timestamp,level,subsystem,eventCode,"
            "entityId,message,latencyMs,site\n"
            )
        );

    LiveLogRecord record;

    record.timestampOffsetMs = 2500;
    record.severity = QStringLiteral("WARN");
    record.subsystem =
        QStringLiteral("Transport");
    record.eventCode =
        QStringLiteral("LATENCY_HIGH");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral("Latency increased.");

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
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
            "2026-09-06T12:00:02.500Z,"
            "WARN,Transport,LATENCY_HIGH,"
            "gateway-17,Latency increased.,"
            "1840,north-yard\n"
            )
        );
}

void DelimitedTextRendererTests::
    csvFieldsAreQuotedCorrectly()
{
    DelimitedTextRenderer renderer(
        QLatin1Char(','),
        {}
        );

    LiveLogRecord record;

    record.message =
        QStringLiteral(
            "Collector said \"retry, now\"."
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
            "\"Collector said \"\"retry, now\"\".\""
            )
        );
}

void DelimitedTextRendererTests::
    tsvUsesTabDelimiter()
{
    const DelimitedTextRenderer renderer(
        QLatin1Char('\t'),
        {
            QStringLiteral("site")
        }
        );

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "timestamp\tlevel\tsubsystem\t"
            "eventCode\tentityId\tmessage\tsite\n"
            )
        );
}

void DelimitedTextRendererTests::
    canonicalAttributeNamesAreIgnored()
{
    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("level"),
        QStringLiteral("FAKE")
        );

    step.record.attributes.insert(
        QStringLiteral("message"),
        QStringLiteral("Fake message")
        );

    step.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    QCOMPARE(
        DelimitedTextRenderer::
        attributeKeysForScenario(
            scenario
            ),
        QStringList({
            QStringLiteral("site")
        })
        );
}

void DelimitedTextRendererTests::
    truncateRegeneratesHeader()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.csv")
            );

    DelimitedTextRenderer renderer(
        QLatin1Char(','),
        {}
        );

    LiveLogRecord before;
    before.message =
        QStringLiteral("Before truncate.");

    LiveLogRecord after;
    after.message =
        QStringLiteral("After truncate.");

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            before
        }
        );

    scenario.steps.append(
        LiveLogTruncateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            after
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const auto result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                renderer,
                options
                );

    QVERIFY(result.isSuccess());

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray content =
        file.readAll();

    QVERIFY(
        content.startsWith(
            "timestamp,level,subsystem,"
            "eventCode,entityId,message\n"
            )
        );

    QVERIFY(
        !content.contains(
            "Before truncate."
            )
        );

    QVERIFY(
        content.contains(
            "After truncate."
            )
        );
}

void DelimitedTextRendererTests::
    replaceRegeneratesHeader()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.csv")
            );

    DelimitedTextRenderer renderer(
        QLatin1Char(','),
        {
            QStringLiteral("site")
        }
        );

    LiveLogRecord before;
    before.message =
        QStringLiteral("Before replace.");
    before.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogRecord after;
    after.message =
        QStringLiteral("After replace.");
    after.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("south")
        );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            before
        }
        );

    scenario.steps.append(
        LiveLogReplaceStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            after
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const auto result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                renderer,
                options
                );

    QVERIFY(result.isSuccess());

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray content =
        file.readAll();

    QVERIFY(
        content.startsWith(
            "timestamp,level,subsystem,"
            "eventCode,entityId,message,site\n"
            )
        );

    QVERIFY(
        !content.contains(
            "Before replace."
            )
        );

    QVERIFY(
        content.contains(
            "After replace."
            )
        );
}

void DelimitedTextRendererTests::
    rotateRegeneratesHeader()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.csv")
            );

    const QString rotatedPath =
        QStringLiteral("%1.1")
            .arg(outputPath);

    DelimitedTextRenderer renderer(
        QLatin1Char(','),
        {
            QStringLiteral("site")
        }
        );

    LiveLogRecord before;
    before.message =
        QStringLiteral("Before rotate.");
    before.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogRecord after;
    after.message =
        QStringLiteral("After rotate.");
    after.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("south")
        );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            before
        }
        );

    scenario.steps.append(
        LiveLogRotateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            after
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const auto result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                renderer,
                options
                );

    QVERIFY(result.isSuccess());

    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFile::exists(rotatedPath));

    const QByteArray activeContent =
        [&]() {
            QFile file(outputPath);
            if (!file.open(QIODevice::ReadOnly)) {
                return QByteArray {};
            }

            return file.readAll();
        }();

    const QByteArray rotatedContent =
        [&]() {
            QFile file(rotatedPath);
            if (!file.open(QIODevice::ReadOnly)) {
                return QByteArray {};
            }

            return file.readAll();
        }();

    const QByteArray expectedHeader(
        "timestamp,level,subsystem,"
        "eventCode,entityId,message,site\n"
        );

    QVERIFY(
        activeContent.startsWith(
            expectedHeader
            )
        );

    QVERIFY(
        rotatedContent.startsWith(
            expectedHeader
            )
        );

    QVERIFY(
        activeContent.contains(
            "After rotate."
            )
        );

    QVERIFY(
        !activeContent.contains(
            "Before rotate."
            )
        );

    QVERIFY(
        rotatedContent.contains(
            "Before rotate."
            )
        );

    QVERIFY(
        !rotatedContent.contains(
            "After rotate."
            )
        );
}

void DelimitedTextRendererTests::
    canonicalAttributeNamesAreIgnoredCaseInsensitively()
{
    LiveLogRecordStep step;

    step.record.attributes.insert(
        QStringLiteral("Level"),
        QStringLiteral("FAKE")
        );

    step.record.attributes.insert(
        QStringLiteral("MESSAGE"),
        QStringLiteral("Fake message")
        );

    step.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogScenario scenario;
    scenario.steps.append(step);

    QCOMPARE(
        DelimitedTextRenderer::
        attributeKeysForScenario(scenario),
        QStringList({
            QStringLiteral("site")
        })
        );
}

void DelimitedTextRendererTests::
    attributeKeysAreDeduplicatedCaseInsensitively()
{
    LiveLogRecordStep first;
    first.record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north")
        );

    LiveLogRecordStep second;
    second.record.attributes.insert(
        QStringLiteral("Site"),
        QStringLiteral("south")
        );

    LiveLogScenario scenario;
    scenario.steps.append(first);
    scenario.steps.append(second);

    const QStringList keys =
        DelimitedTextRenderer::
        attributeKeysForScenario(scenario);

    QCOMPARE(keys.size(), 1);

    QCOMPARE(
        keys.first().toCaseFolded(),
        QStringLiteral("site")
        );
}

QTEST_MAIN(DelimitedTextRendererTests)

#include "DelimitedTextRendererTests.moc"