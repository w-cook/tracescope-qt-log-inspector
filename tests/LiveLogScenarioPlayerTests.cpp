#include <QtTest/QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "../tools/live-log-generator/LiveLogScenarioPlayer.h"
#include "../tools/live-log-generator/renderers/JsonLinesRenderer.h"

namespace
{
class ContainerRenderer final
    : public ILogRecordRenderer
{
public:
    QByteArray initialContent() const override
    {
        return QByteArray("[");
    }

    QByteArray recordSeparator() const override
    {
        return QByteArray(",");
    }

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &
        ) const override
    {
        return QByteArray("\"")
        + record.message.toUtf8()
            + QByteArray("\"");
    }

    QByteArray finalContent() const override
    {
        return QByteArray("]");
    }
};

LiveLogRecord record(
    qint64 timestampOffsetMs,
    const QString &message
    )
{
    LiveLogRecord result;

    result.timestampOffsetMs =
        timestampOffsetMs;

    result.severity =
        QStringLiteral("INFO");

    result.subsystem =
        QStringLiteral("Gateway");

    result.eventCode =
        QStringLiteral("TEST_EVENT");

    result.entityId =
        QStringLiteral("gateway-17");

    result.message =
        message;

    return result;
}

QByteArray readFile(
    const QString &path
    )
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}
}

class LiveLogScenarioPlayerTests : public QObject
{
    Q_OBJECT

private slots:
    void normalRecordsAreWritten();
    void waitsAreScaledByPlaybackSpeed();
    void partialRecordIsVisibleDuringHold();
    void truncateRemovesPreviousContent();
    void replaceRemovesPreviousContent();
    void rotatePreservesPreviousFile();
    void rendererBoundaryContentWrapsRecords();
    void truncateResetsRecordSeparatorState();
    void rotateFinalizesPreviousContainer();
    void invalidPlaybackSpeedIsRejected();
};

void LiveLogScenarioPlayerTests::
    normalRecordsAreWritten()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral("First record.")
                )
        }
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                2500,
                QStringLiteral("Second record.")
                )
        }
        );

    LiveLogScenarioPlayerOptions options;

    options.outputPath = outputPath;
    options.speed = 1.0;

    options.scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const LiveLogScenarioPlayer player(
        [](qint64) {}
        );

    const LiveLogScenarioPlayResult result =
        player.play(
            scenario,
            JsonLinesRenderer(),
            options
            );

    QVERIFY(result.isSuccess());

    const QList<QByteArray> lines =
        readFile(outputPath)
            .split('\n');

    QCOMPARE(lines.size(), 3);

    const QJsonObject first =
        QJsonDocument::fromJson(
            lines.at(0)
            ).object();

    const QJsonObject second =
        QJsonDocument::fromJson(
            lines.at(1)
            ).object();

    QCOMPARE(
        first.value(
                 QStringLiteral("message")
                 ).toString(),
        QStringLiteral("First record.")
        );

    QCOMPARE(
        first.value(
                 QStringLiteral("timestamp")
                 ).toString(),
        QStringLiteral(
            "2026-09-06T12:00:00.000Z"
            )
        );

    QCOMPARE(
        second.value(
                  QStringLiteral("message")
                  ).toString(),
        QStringLiteral("Second record.")
        );

    QCOMPARE(
        second.value(
                  QStringLiteral("timestamp")
                  ).toString(),
        QStringLiteral(
            "2026-09-06T12:00:02.500Z"
            )
        );
}

void LiveLogScenarioPlayerTests::
    waitsAreScaledByPlaybackSpeed()
{
    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogWaitStep {
            2000
        }
        );

    scenario.steps.append(
        LiveLogWaitStep {
            500
        }
        );

    QList<qint64> sleeps;

    LiveLogScenarioPlayer player(
        [&sleeps](qint64 milliseconds) {
            sleeps.append(milliseconds);
        }
        );

    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LiveLogScenarioPlayerOptions options;

    options.outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    options.speed = 4.0;

    const LiveLogScenarioPlayResult result =
        player.play(
            scenario,
            JsonLinesRenderer(),
            options
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(sleeps.size(), 2);
    QCOMPARE(sleeps.at(0), qint64(500));
    QCOMPARE(sleeps.at(1), qint64(125));
}

void LiveLogScenarioPlayerTests::
    partialRecordIsVisibleDuringHold()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    LiveLogRecordStep step;

    step.record =
        record(
            0,
            QStringLiteral(
                "Partially written record."
                )
            );

    step.write.mode =
        LiveLogWriteMode::Partial;

    step.write.splitFraction = 0.5;
    step.write.holdMs = 1000;

    LiveLogScenario scenario;
    scenario.steps.append(step);

    QByteArray contentDuringHold;
    QList<qint64> sleeps;

    LiveLogScenarioPlayer player(
        [&](
            qint64 milliseconds
            ) {
            sleeps.append(milliseconds);

            contentDuringHold =
                readFile(outputPath);
        }
        );

    LiveLogScenarioPlayerOptions options;

    options.outputPath = outputPath;

    options.scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-06T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const LiveLogScenarioPlayResult result =
        player.play(
            scenario,
            JsonLinesRenderer(),
            options
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(sleeps.size(), 1);
    QCOMPARE(sleeps.first(), qint64(1000));

    QVERIFY(!contentDuringHold.isEmpty());

    const QByteArray finalContent =
        readFile(outputPath);

    QVERIFY(
        contentDuringHold.size()
        < finalContent.size()
        );

    QVERIFY(
        !contentDuringHold.endsWith('\n')
        );

    QVERIFY(
        finalContent.endsWith('\n')
        );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            finalContent.trimmed()
            );

    QVERIFY(document.isObject());

    QCOMPARE(
        document.object()
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral(
            "Partially written record."
            )
        );
}

void LiveLogScenarioPlayerTests::
    truncateRemovesPreviousContent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral("Before truncate.")
                )
        }
        );

    scenario.steps.append(
        LiveLogTruncateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral("After truncate.")
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                JsonLinesRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    const QByteArray content =
        readFile(outputPath);

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

void LiveLogScenarioPlayerTests::
    replaceRemovesPreviousContent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral("Before replace.")
                )
        }
        );

    scenario.steps.append(
        LiveLogReplaceStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral("After replace.")
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                JsonLinesRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    const QByteArray content =
        readFile(outputPath);

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

void LiveLogScenarioPlayerTests::
    rotatePreservesPreviousFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    const QString rotatedPath =
        QStringLiteral("%1.1")
            .arg(outputPath);

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral("Before rotate.")
                )
        }
        );

    scenario.steps.append(
        LiveLogRotateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral("After rotate.")
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                JsonLinesRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFile::exists(rotatedPath));

    const QByteArray rotatedContent =
        readFile(rotatedPath);

    const QByteArray activeContent =
        readFile(outputPath);

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
}

void LiveLogScenarioPlayerTests::
    rendererBoundaryContentWrapsRecords()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.json")
            );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral(
                    "First record."
                    )
                )
        }
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral(
                    "Second record."
                    )
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                ContainerRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        readFile(outputPath),
        QByteArray(
            "[\"First record.\","
            "\"Second record.\"]"
            )
        );
}

void LiveLogScenarioPlayerTests::
    truncateResetsRecordSeparatorState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.json")
            );

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral(
                    "Before truncate."
                    )
                )
        }
        );

    scenario.steps.append(
        LiveLogTruncateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral(
                    "After truncate."
                    )
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                ContainerRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    const QByteArray content =
        readFile(outputPath);

    QCOMPARE(
        content,
        QByteArray(
            "[\"After truncate.\"]"
            )
        );

    QVERIFY(
        !content.contains(
            "Before truncate."
            )
        );

    QVERIFY(
        !content.startsWith(
            "[,"
            )
        );
}

void LiveLogScenarioPlayerTests::
    rotateFinalizesPreviousContainer()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString outputPath =
        directory.filePath(
            QStringLiteral("live.json")
            );

    const QString rotatedPath =
        QStringLiteral("%1.1")
            .arg(outputPath);

    LiveLogScenario scenario;

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                0,
                QStringLiteral(
                    "Before rotate."
                    )
                )
        }
        );

    scenario.steps.append(
        LiveLogRotateStep{}
        );

    scenario.steps.append(
        LiveLogRecordStep {
            record(
                1000,
                QStringLiteral(
                    "After rotate."
                    )
                )
        }
        );

    LiveLogScenarioPlayerOptions options;
    options.outputPath = outputPath;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer(
            [](qint64) {}
            )
            .play(
                scenario,
                ContainerRenderer(),
                options
                );

    QVERIFY(result.isSuccess());

    QVERIFY(
        QFile::exists(
            rotatedPath
            )
        );

    QVERIFY(
        QFile::exists(
            outputPath
            )
        );

    QCOMPARE(
        readFile(rotatedPath),
        QByteArray(
            "[\"Before rotate.\"]"
            )
        );

    QCOMPARE(
        readFile(outputPath),
        QByteArray(
            "[\"After rotate.\"]"
            )
        );
}

void LiveLogScenarioPlayerTests::
    invalidPlaybackSpeedIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LiveLogScenarioPlayerOptions options;

    options.outputPath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    options.speed = 0.0;

    const LiveLogScenarioPlayResult result =
        LiveLogScenarioPlayer().play(
            LiveLogScenario(),
            JsonLinesRenderer(),
            options
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_PLAYBACK_SPEED"
            )
        );

    QVERIFY(
        !QFile::exists(
            options.outputPath
            )
        );
}

QTEST_MAIN(LiveLogScenarioPlayerTests)

#include "LiveLogScenarioPlayerTests.moc"