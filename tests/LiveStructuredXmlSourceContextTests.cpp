#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/live/LiveStructuredXmlSourceContext.h"

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

class LiveStructuredXmlSourceContextTests
    : public QObject
{
    Q_OBJECT

private slots:
    void initializesOpenContainerAndContinuesPartialRecord();
    void respectsCapturedBaselineBoundary();
    void batchesNewlyCompletedRecords();
    void newGenerationResetsParserAndRecordNumber();
    void rejectsClosedBaselineContainer();
};

void LiveStructuredXmlSourceContextTests::
    initializesOpenContainerAndContinuesPartialRecord()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath(
            QStringLiteral("live.xml")
            );

    const QByteArray baseline(
        "<session>"
        "<events>"
        "<event>"
        "<message>first</message>"
        "</event>"
        "<event>"
        "<message>sec"
        );

    writeFile(
        path,
        baseline
        );

    LiveStructuredXmlSourceContext context(
        QStringLiteral(
            "session.events.event"
            )
        );

    const auto initialization =
        context.initializeFromExistingFile(
            path,
            baseline.size(),
            0
            );

    QVERIFY2(
        initialization.succeeded,
        qPrintable(
            initialization.errorMessage
            )
        );

    QCOMPARE(
        initialization
            .existingRecordBatch
            .records
            .size(),
        1
        );

    QCOMPARE(
        initialization
            .existingRecordBatch
            .records
            .first()
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("first")
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(2)
        );

    const auto continuation =
        context.appendBytes(
            QByteArray(
                "ond</message>"
                "</event>"
                )
            );

    QVERIFY2(
        continuation.succeeded,
        qPrintable(
            continuation.errorMessage
            )
        );

    QCOMPARE(
        continuation.batch.records.size(),
        1
        );

    QCOMPARE(
        continuation.batch.firstRecordNumber,
        qint64(2)
        );

    QCOMPARE(
        continuation.batch.records.first()
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("second")
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(3)
        );
}

void LiveStructuredXmlSourceContextTests::
    respectsCapturedBaselineBoundary()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath(
            QStringLiteral("live.xml")
            );

    const QByteArray capturedBaseline(
        "<session>"
        "<events>"
        "<event>"
        "<message>first</message>"
        "</event>"
        "<event>"
        "<message>part"
        );

    const QByteArray laterBytes(
        "ial</message>"
        "</event>"
        "<event>"
        "<message>third</message>"
        "</event>"
        );

    writeFile(
        path,
        capturedBaseline
            + laterBytes
        );

    LiveStructuredXmlSourceContext context(
        QStringLiteral(
            "session.events.event"
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
            .records
            .size(),
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
        appended.batch.records.size(),
        2
        );

    QCOMPARE(
        appended.batch.firstRecordNumber,
        qint64(2)
        );

    QCOMPARE(
        appended.batch.records.at(0)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("partial")
        );

    QCOMPARE(
        appended.batch.records.at(1)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("third")
        );

    QCOMPARE(
        context.nextRecordNumber(),
        qint64(4)
        );
}

void LiveStructuredXmlSourceContextTests::
    batchesNewlyCompletedRecords()
{
    LiveStructuredXmlSourceContext context(
        QStringLiteral(
            "session.events.event"
            )
        );

    context.beginSourceGeneration(
        4
        );

    const auto result =
        context.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event><message>one</message></event>"
                "<event><message>two</message></event>"
                "<event><message>three</message></event>"
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

void LiveStructuredXmlSourceContextTests::
    newGenerationResetsParserAndRecordNumber()
{
    LiveStructuredXmlSourceContext context(
        QStringLiteral(
            "session.events.event"
            )
        );

    context.beginSourceGeneration(
        2
        );

    const auto oldGeneration =
        context.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<message>first</message>"
                "</event>"
                "<event>"
                "<message>old-partial"
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
                "<session>"
                "<events>"
                "<event>"
                "<message>replacement</message>"
                "</event>"
                )
            );

    QVERIFY(newGeneration.succeeded);

    QCOMPARE(
        newGeneration.batch.records.size(),
        1
        );

    QCOMPARE(
        newGeneration.batch.firstRecordNumber,
        qint64(1)
        );

    QCOMPARE(
        newGeneration.batch.sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        newGeneration.batch.records.first()
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("replacement")
        );
}

void LiveStructuredXmlSourceContextTests::
    rejectsClosedBaselineContainer()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath(
            QStringLiteral("closed.xml")
            );

    const QByteArray source(
        "<session>"
        "<events>"
        "<event>"
        "<message>complete</message>"
        "</event>"
        "</events>"
        "</session>"
        );

    writeFile(
        path,
        source
        );

    LiveStructuredXmlSourceContext context(
        QStringLiteral(
            "session.events.event"
            )
        );

    const auto result =
        context.initializeFromExistingFile(
            path,
            source.size(),
            0
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        result.errorMessage.contains(
            QStringLiteral("closed"),
            Qt::CaseInsensitive
            )
        );
}

QTEST_MAIN(
    LiveStructuredXmlSourceContextTests
    )

#include "LiveStructuredXmlSourceContextTests.moc"