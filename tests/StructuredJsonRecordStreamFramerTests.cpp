#include <QtTest>

#include "../src/importing/StructuredJsonRecordStreamFramer.h"

class StructuredJsonRecordStreamFramerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void emitsCompleteRecordsFromOpenNestedArray();
    void retainsIncompleteRecordAcrossChunks();
    void handlesNestedObjectsAndArrays();
    void ignoresStructuralCharactersInsideStrings();
    void supportsRootRecordArray();
    void resetDiscardsPreviousGenerationState();
    void handlesEscapeSequenceAcrossChunkBoundary();
    void handlesPrefixAcrossChunkBoundaries();
    void handlesInputOneByteAtATime();
    void rejectsNonObjectRecord();
};

void StructuredJsonRecordStreamFramerTests::
    emitsCompleteRecordsFromOpenNestedArray()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult result =
        framer.appendBytes(
            QByteArray(
                "{\n"
                "  \"data\": {\n"
                "    \"records\": [\n"
                "      {\"message\":\"first\"},\n"
                "      {\"message\":\"second\"},\n"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"first\"}"
                ),
            QByteArray(
                "{\"message\":\"second\"}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    retainsIncompleteRecordAcrossChunks()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult first =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"partial"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    const StructuredJsonRecordFrameResult second =
        framer.appendBytes(
            QByteArray(
                " record\"},"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"partial record\"}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    handlesNestedObjectsAndArrays()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult result =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{"
                "\"message\":\"first\","
                "\"context\":{"
                "\"attempt\":2,"
                "\"flags\":[true,false]"
                "}"
                "},"
                "{"
                "\"message\":\"second\""
                "}"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0),
        QByteArray(
            "{"
            "\"message\":\"first\","
            "\"context\":{"
            "\"attempt\":2,"
            "\"flags\":[true,false]"
            "}"
            "}"
            )
        );

    QCOMPARE(
        result.records.at(1),
        QByteArray(
            "{\"message\":\"second\"}"
            )
        );
}

void StructuredJsonRecordStreamFramerTests::
    ignoresStructuralCharactersInsideStrings()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult result =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{"
                "\"message\":"
                "\"text containing { } [ ] and "
                "\\\"quoted text\\\"\""
                "},"
                "{"
                "\"message\":\"next\""
                "}"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0),
        QByteArray(
            "{"
            "\"message\":"
            "\"text containing { } [ ] and "
            "\\\"quoted text\\\"\""
            "}"
            )
        );
}

void StructuredJsonRecordStreamFramerTests::
    supportsRootRecordArray()
{
    StructuredJsonRecordStreamFramer framer;

    const StructuredJsonRecordFrameResult result =
        framer.appendBytes(
            QByteArray(
                "["
                "{\"message\":\"first\"},"
                "{\"message\":\"second\"}"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"first\"}"
                ),
            QByteArray(
                "{\"message\":\"second\"}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    resetDiscardsPreviousGenerationState()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult first =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"old"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    framer.reset();

    const StructuredJsonRecordFrameResult second =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{\"message\":\"new\"},"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"new\"}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    handlesEscapeSequenceAcrossChunkBoundary()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult first =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "{"
                "\"message\":"
                "\"escaped quote \\"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    const StructuredJsonRecordFrameResult second =
        framer.appendBytes(
            QByteArray(
                "\" still inside string\""
                "},"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records,
        QVector<QByteArray>({
            QByteArray(
                "{"
                "\"message\":"
                "\"escaped quote "
                "\\\" still inside string\""
                "}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    handlesPrefixAcrossChunkBoundaries()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult first =
        framer.appendBytes(
            QByteArray(
                "{\"da"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    const StructuredJsonRecordFrameResult second =
        framer.appendBytes(
            QByteArray(
                "ta\":{\"rec"
                )
            );

    QVERIFY(second.succeeded);
    QVERIFY(second.records.isEmpty());

    const StructuredJsonRecordFrameResult third =
        framer.appendBytes(
            QByteArray(
                "ords\":["
                "{\"message\":\"first\"},"
                )
            );

    QVERIFY(third.succeeded);

    QCOMPARE(
        third.records,
        QVector<QByteArray>({
            QByteArray(
                "{\"message\":\"first\"}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    handlesInputOneByteAtATime()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const QByteArray source(
        "{"
        "\"data\":{"
        "\"records\":["
        "{"
        "\"message\":\"first\","
        "\"nested\":{"
        "\"value\":\"a { tricky } value\""
        "}"
        "},"
        "{"
        "\"message\":\"second\","
        "\"values\":[1,2,3]"
        "}"
        "]"
        "}"
        "}"
        );

    QVector<QByteArray> records;

    for (const char byte : source) {
        const StructuredJsonRecordFrameResult
            result =
            framer.appendBytes(
                QByteArray(
                    1,
                    byte
                    )
                );

        QVERIFY(result.succeeded);

        records.append(
            result.records
            );
    }

    QCOMPARE(
        records,
        QVector<QByteArray>({
            QByteArray(
                "{"
                "\"message\":\"first\","
                "\"nested\":{"
                "\"value\":\"a { tricky } value\""
                "}"
                "}"
                ),
            QByteArray(
                "{"
                "\"message\":\"second\","
                "\"values\":[1,2,3]"
                "}"
                )
        })
        );
}

void StructuredJsonRecordStreamFramerTests::
    rejectsNonObjectRecord()
{
    StructuredJsonRecordStreamFramer framer(
        QStringLiteral("data.records")
        );

    const StructuredJsonRecordFrameResult result =
        framer.appendBytes(
            QByteArray(
                "{"
                "\"data\":{"
                "\"records\":["
                "\"not-an-object\","
                )
            );

    QVERIFY(!result.succeeded);

    QVERIFY(
        result.errorMessage.contains(
            QStringLiteral(
                "must be JSON objects"
                ),
            Qt::CaseInsensitive
            )
        );
}

QTEST_MAIN(
    StructuredJsonRecordStreamFramerTests
    )

#include "StructuredJsonRecordStreamFramerTests.moc"