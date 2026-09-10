#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/live/LiveLineSourceContext.h"

class LiveLineSourceContextTests
    : public QObject
{
    Q_OBJECT

private slots:
    void emptySourceStartsAtFirstPhysicalLine();

    void initializesAfterExistingPhysicalLines();

    void reportsExistingUnterminatedFinalLine();

    void advancesOnlyForCompleteFramedLines();

    void sourceGenerationResetsLinePositionAndPendingBytes();

    void initializationReportsMissingSource();
};

void LiveLineSourceContextTests::
    emptySourceStartsAtFirstPhysicalLine()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString path =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.close();

    LiveLineSourceContext context;

    const auto result =
        context.initializeFromExistingFile(
            path
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.existingPhysicalLineCount,
        qint64(0)
        );

    QVERIFY(
        result.endsAtLineBoundary
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(1)
        );

    QCOMPARE(
        context.sourceGeneration(),
        quint64(0)
        );
}

void LiveLineSourceContextTests::
    initializesAfterExistingPhysicalLines()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString path =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray existing(
        "first\n"
        "\n"
        "third\n"
        );

    QCOMPARE(
        file.write(
            existing
            ),
        qint64(
            existing.size()
            )
        );

    file.close();

    LiveLineSourceContext context;

    const auto result =
        context.initializeFromExistingFile(
            path
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.existingPhysicalLineCount,
        qint64(3)
        );

    QVERIFY(
        result.endsAtLineBoundary
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(4)
        );
}

void LiveLineSourceContextTests::
    reportsExistingUnterminatedFinalLine()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString path =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray existing(
        "first\n"
        "second"
        );

    QCOMPARE(
        file.write(
            existing
            ),
        qint64(
            existing.size()
            )
        );

    file.close();

    LiveLineSourceContext context;

    const auto result =
        context.initializeFromExistingFile(
            path
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.existingPhysicalLineCount,
        qint64(2)
        );

    QVERIFY(
        !result.endsAtLineBoundary
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(3)
        );
}

void LiveLineSourceContextTests::
    advancesOnlyForCompleteFramedLines()
{
    LiveLineSourceContext context;

    const LiveFramedLineBatch first =
        context.appendBytes(
            QByteArray(
                "first\n"
                "partial"
                )
            );

    QCOMPARE(
        first.firstPhysicalLineNumber,
        qint64(1)
        );

    QCOMPARE(
        first.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        first.lines,
        QVector<QByteArray>({
            QByteArray("first")
        })
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(2)
        );

    QCOMPARE(
        context.framer().pendingBytes(),
        QByteArray("partial")
        );

    const LiveFramedLineBatch second =
        context.appendBytes(
            QByteArray(
                "-record\n"
                "\n"
                )
            );

    QCOMPARE(
        second.firstPhysicalLineNumber,
        qint64(2)
        );

    QCOMPARE(
        second.lines,
        QVector<QByteArray>({
            QByteArray("partial-record"),
            QByteArray()
        })
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(4)
        );

    QVERIFY(
        context.framer()
            .pendingBytes()
            .isEmpty()
        );
}

void LiveLineSourceContextTests::
    sourceGenerationResetsLinePositionAndPendingBytes()
{
    LiveLineSourceContext context;

    const LiveFramedLineBatch first =
        context.appendBytes(
            QByteArray(
                "one\n"
                "unfinished"
                )
            );

    QCOMPARE(
        first.lines.size(),
        1
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(2)
        );

    QVERIFY(
        !context.framer()
             .pendingBytes()
             .isEmpty()
        );

    context.beginSourceGeneration(
        4
        );

    QCOMPARE(
        context.sourceGeneration(),
        quint64(4)
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(1)
        );

    QVERIFY(
        context.framer()
            .pendingBytes()
            .isEmpty()
        );

    const LiveFramedLineBatch next =
        context.appendBytes(
            QByteArray(
                "new-generation\n"
                )
            );

    QCOMPARE(
        next.firstPhysicalLineNumber,
        qint64(1)
        );

    QCOMPARE(
        next.sourceGeneration,
        quint64(4)
        );

    QCOMPARE(
        next.lines,
        QVector<QByteArray>({
            QByteArray("new-generation")
        })
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(2)
        );
}

void LiveLineSourceContextTests::
    initializationReportsMissingSource()
{
    LiveLineSourceContext context;

    const auto result =
        context.initializeFromExistingFile(
            QStringLiteral(
                "missing/live.log"
                )
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QCOMPARE(
        context.nextPhysicalLineNumber(),
        qint64(1)
        );

    QCOMPARE(
        context.sourceGeneration(),
        quint64(0)
        );
}

QTEST_MAIN(LiveLineSourceContextTests)

#include "LiveLineSourceContextTests.moc"