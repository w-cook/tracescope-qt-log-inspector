#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/live/LiveStructuredJsonSourceContext.h"

namespace
{
void writeFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}
}

class LiveStructuredJsonSourceContextTests
    : public QObject
{
    Q_OBJECT

private slots:
    void initializesOpenContainerAndContinuesPartialRecord();
    void respectsCapturedBaselineBoundary();
    void batchesNewlyCompletedRecords();
    void newGenerationResetsFramingAndRecordNumber();
};

void LiveStructuredJsonSourceContextTests::
    initializesOpenContainerAndContinuesPartialRecord()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath(
            QStringLiteral(
                "live.json"
                )
            );

    const QByteArray baseline(
        "{"
        "\"data\":{"
        "\"records\":["
        "{\"message\":\"first\"},"
        "{\"message\":\"sec"
        );

    writeFile(
        path,
        baseline
        );

    LiveStructuredJsonSourceContext context(
        QStringLiteral(
            "data.records"
            )
        );

    const auto initialization =
        context.initializeFromExistingFile(
            path,
            baseline.size(),
            0
            );

    QVERIFY(initialization.succeeded);

    QCOMPARE(
        initialization
            .existingRecordBatch
            .records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"first\"}"
                )
        })
        );

    QCOMPARE(
        initialization
            .existingRecordBatch
            .firstRecordNumber,
        qint64(1)
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(2)
        );

    const auto continuation =
        context.appendBytes(
            QByteArray(
                "ond\"},"
                )
            );

    QVERIFY(continuation.succeeded);

    QCOMPARE(
        continuation.batch.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"second\"}"
                )
        })
        );

    QCOMPARE(
        continuation.batch.firstRecordNumber,
        qint64(2)
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(3)
        );
}

void LiveStructuredJsonSourceContextTests::
    respectsCapturedBaselineBoundary()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath(
            QStringLiteral(
                "live.json"
                )
            );

    const QByteArray capturedBaseline(
        "{"
        "\"data\":{"
        "\"records\":["
        "{\"message\":\"first\"},"
        "{\"message\":\"part"
        );

    const QByteArray laterBytes(
        "ial\"},"
        "{\"message\":\"third\"},"
        );

    writeFile(
        path,
        capturedBaseline
            + laterBytes
        );

    LiveStructuredJsonSourceContext context(
        QStringLiteral(
            "data.records"
            )
        );

    const auto initialization =
        context.initializeFromExistingFile(
            path,
            capturedBaseline.size(),
            0
            );

    QVERIFY(initialization.succeeded);

    QCOMPARE(
        initialization
            .existingRecordBatch
            .records.size(),
        1
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(2)
        );

    const auto appended =
        context.appendBytes(
            laterBytes
            );

    QVERIFY(appended.succeeded);

    QCOMPARE(
        appended.batch.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"partial\"}"
                ),
            QByteArray(
                "{\"message\":\"third\"}"
                )
        })
        );

    QCOMPARE(
        appended.batch.firstRecordNumber,
        qint64(2)
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(4)
        );
}

void LiveStructuredJsonSourceContextTests::
    batchesNewlyCompletedRecords()
{
    LiveStructuredJsonSourceContext context(
        QStringLiteral(
            "data.records"
            )
        );

    context.beginSourceGeneration(
        4
        );

    const auto result =
        context.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"one\"},"
                "{\"message\":\"two\"},"
                "{\"message\":\"three\"},"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.batch.records.size(),
        3
        );

    QCOMPARE(
        result.batch.firstRecordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.batch.sourceGeneration,
        quint64(4)
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(4)
        );
}

void LiveStructuredJsonSourceContextTests::
    newGenerationResetsFramingAndRecordNumber()
{
    LiveStructuredJsonSourceContext context(
        QStringLiteral(
            "data.records"
            )
        );

    context.beginSourceGeneration(
        2
        );

    const auto oldGeneration =
        context.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"first\"},"
                "{\"message\":\"old-partial"
                )
            );

    QVERIFY(oldGeneration.succeeded);

    QCOMPARE(
        oldGeneration.batch.records.size(),
        1
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(2)
        );

    context.beginSourceGeneration(
        3
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(1)
        );

    QCOMPARE(
        context.sourceGeneration(),
        quint64(3)
        );

    const auto newGeneration =
        context.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"replacement\"},"
                )
            );

    QVERIFY(newGeneration.succeeded);

    QCOMPARE(
        newGeneration.batch.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"replacement\"}"
                )
        })
        );

    QCOMPARE(
        newGeneration.batch.firstRecordNumber,
        qint64(1)
        );

    QCOMPARE(
        newGeneration.batch.sourceGeneration,
        quint64(3)
        );
}

QTEST_MAIN(
    LiveStructuredJsonSourceContextTests
    )

#include "LiveStructuredJsonSourceContextTests.moc"