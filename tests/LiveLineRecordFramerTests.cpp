#include <QtTest>

#include "../src/live/LiveLineRecordFramer.h"

class LiveLineRecordFramerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void emitsCompleteLines();
    void retainsIncompleteTrailingLine();
    void completesLineAcrossMultipleChunks();
    void preservesBlankPhysicalLines();
    void handlesCrLfAcrossChunkBoundary();
    void resetDiscardsIncompletePreviousGeneration();
};

void LiveLineRecordFramerTests::
    emitsCompleteLines()
{
    LiveLineRecordFramer framer;

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "first\n"
                "second\n"
                )
            );

    QCOMPARE(
        lines,
        QVector<QByteArray>({
            QByteArray("first"),
            QByteArray("second")
        })
        );

    QVERIFY(
        framer.pendingBytes().isEmpty()
        );
}

void LiveLineRecordFramerTests::
    retainsIncompleteTrailingLine()
{
    LiveLineRecordFramer framer;

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "complete\n"
                "partial"
                )
            );

    QCOMPARE(
        lines,
        QVector<QByteArray>({
            QByteArray("complete")
        })
        );

    QCOMPARE(
        framer.pendingBytes(),
        QByteArray("partial")
        );
}

void LiveLineRecordFramerTests::
    completesLineAcrossMultipleChunks()
{
    LiveLineRecordFramer framer;

    QVERIFY(
        framer
            .appendBytes(
                QByteArray("partial-")
                )
            .isEmpty()
        );

    QCOMPARE(
        framer.pendingBytes(),
        QByteArray("partial-")
        );

    QVERIFY(
        framer
            .appendBytes(
                QByteArray("record")
                )
            .isEmpty()
        );

    QCOMPARE(
        framer.pendingBytes(),
        QByteArray(
            "partial-record"
            )
        );

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "\nnext\n"
                )
            );

    QCOMPARE(
        lines,
        QVector<QByteArray>({
            QByteArray("partial-record"),
            QByteArray("next")
        })
        );

    QVERIFY(
        framer.pendingBytes().isEmpty()
        );
}

void LiveLineRecordFramerTests::
    preservesBlankPhysicalLines()
{
    LiveLineRecordFramer framer;

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "first\n"
                "\n"
                "third\n"
                )
            );

    QCOMPARE(
        lines.size(),
        3
        );

    QCOMPARE(
        lines.at(0),
        QByteArray("first")
        );

    QVERIFY(
        lines.at(1).isEmpty()
        );

    QCOMPARE(
        lines.at(2),
        QByteArray("third")
        );
}

void LiveLineRecordFramerTests::
    handlesCrLfAcrossChunkBoundary()
{
    LiveLineRecordFramer framer;

    QVERIFY(
        framer
            .appendBytes(
                QByteArray(
                    "first\r"
                    )
                )
            .isEmpty()
        );

    QCOMPARE(
        framer.pendingBytes(),
        QByteArray("first\r")
        );

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "\nsecond\r\n"
                )
            );

    QCOMPARE(
        lines,
        QVector<QByteArray>({
            QByteArray("first"),
            QByteArray("second")
        })
        );

    QVERIFY(
        framer.pendingBytes().isEmpty()
        );
}

void LiveLineRecordFramerTests::
    resetDiscardsIncompletePreviousGeneration()
{
    LiveLineRecordFramer framer;

    QVERIFY(
        framer
            .appendBytes(
                QByteArray(
                    "old-generation-partial"
                    )
                )
            .isEmpty()
        );

    QVERIFY(
        !framer.pendingBytes().isEmpty()
        );

    framer.reset();

    QVERIFY(
        framer.pendingBytes().isEmpty()
        );

    const QVector<QByteArray> lines =
        framer.appendBytes(
            QByteArray(
                "new-generation\n"
                )
            );

    QCOMPARE(
        lines,
        QVector<QByteArray>({
            QByteArray("new-generation")
        })
        );
}

QTEST_MAIN(LiveLineRecordFramerTests)

#include "LiveLineRecordFramerTests.moc"