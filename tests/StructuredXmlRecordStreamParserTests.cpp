#include <QtTest>

#include "../src/importing/StructuredXmlRecordStreamParser.h"

class StructuredXmlRecordStreamParserTests
    : public QObject
{
    Q_OBJECT

private slots:
    void emitsCompleteRecordFromOpenNestedContainer();
    void retainsIncompleteRecordAcrossChunks();
    void handlesChunkBoundaryInsideAttribute();
    void mapsNestedElementsAttributesAndNamedData();
    void supportsWindowsEventXmlRecord();
    void handlesInputOneByteAtATime();
    void rejectsMalformedXml();
    void resetDiscardsIncompleteRecordState();
    void reportsClosedDocument();
};

void StructuredXmlRecordStreamParserTests::
    emitsCompleteRecordFromOpenNestedContainer()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        result =
        parser.appendBytes(
            QByteArray(
                "<?xml version=\"1.0\"?>"
                "<session>"
                "<events>"
                "<event>"
                "<message>first</message>"
                "</event>"
                "<event>"
                "<message>second</message>"
                "</event>"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("first")
        );

    QCOMPARE(
        result.records.at(1)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("second")
        );

    QVERIFY(
        parser.recordContainerLocated()
        );

    QVERIFY(
        parser.recordPathLocated()
        );

    QVERIFY(
        !parser.documentClosed()
        );

    QVERIFY(
        !parser.hasIncompleteRecord()
        );
}

void StructuredXmlRecordStreamParserTests::
    retainsIncompleteRecordAcrossChunks()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        first =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<details>"
                "<message>partial"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    QVERIFY(
        parser.recordContainerLocated()
        );

    QVERIFY(
        parser.recordPathLocated()
        );

    QVERIFY(
        parser.hasIncompleteRecord()
        );

    const StructuredXmlRecordStreamParseResult
        second =
        parser.appendBytes(
            QByteArray(
                " record"
                "</message>"
                "</details>"
                "</event>"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records.size(),
        1
        );

    const QJsonObject details =
        second.records.at(0)
            .object
            .value(
                QStringLiteral("details")
                )
            .toObject();

    QCOMPARE(
        details.value(
                   QStringLiteral("message")
                   )
            .toString(),
        QStringLiteral(
            "partial record"
            )
        );

    QVERIFY(
        !parser.hasIncompleteRecord()
        );
}

void StructuredXmlRecordStreamParserTests::
    handlesChunkBoundaryInsideAttribute()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        first =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<Data Na"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    const StructuredXmlRecordStreamParseResult
        second =
        parser.appendBytes(
            QByteArray(
                "me=\"Attempt\">"
                "3"
                "</Data>"
                "</event>"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records.size(),
        1
        );

    const QJsonObject namedData =
        second.records.at(0)
            .object
            .value(
                QStringLiteral("NamedData")
                )
            .toObject();

    QCOMPARE(
        namedData.value(
                     QStringLiteral("Attempt")
                     )
            .toString(),
        QStringLiteral("3")
        );
}

void StructuredXmlRecordStreamParserTests::
    mapsNestedElementsAttributesAndNamedData()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        result =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<metadata>"
                "<timestamp>"
                "2026-09-13T12:00:00.000Z"
                "</timestamp>"
                "<source name=\"worker\">"
                "backend"
                "</source>"
                "</metadata>"
                "<Data Name=\"Attempt\">2</Data>"
                "<Data Name=\"Region\">east</Data>"
                "<tag>one</tag>"
                "<tag>two</tag>"
                "</event>"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        1
        );

    const QJsonObject object =
        result.records.at(0).object;

    const QJsonObject metadata =
        object.value(
                  QStringLiteral("metadata")
                  )
            .toObject();

    QCOMPARE(
        metadata.value(
                    QStringLiteral("timestamp")
                    )
            .toString(),
        QStringLiteral(
            "2026-09-13T12:00:00.000Z"
            )
        );

    const QJsonObject source =
        metadata.value(
                    QStringLiteral("source")
                    )
            .toObject();

    QCOMPARE(
        source.value(
                  QStringLiteral("@name")
                  )
            .toString(),
        QStringLiteral("worker")
        );

    QCOMPARE(
        source.value(
                  QStringLiteral("#text")
                  )
            .toString(),
        QStringLiteral("backend")
        );

    const QJsonObject namedData =
        object.value(
                  QStringLiteral("NamedData")
                  )
            .toObject();

    QCOMPARE(
        namedData.value(
                     QStringLiteral("Attempt")
                     )
            .toString(),
        QStringLiteral("2")
        );

    QCOMPARE(
        namedData.value(
                     QStringLiteral("Region")
                     )
            .toString(),
        QStringLiteral("east")
        );

    const QJsonArray tags =
        object.value(
                  QStringLiteral("tag")
                  )
            .toArray();

    QCOMPARE(
        tags.size(),
        2
        );

    QCOMPARE(
        tags.at(0).toString(),
        QStringLiteral("one")
        );

    QCOMPARE(
        tags.at(1).toString(),
        QStringLiteral("two")
        );

    QVERIFY(
        result.records.at(0)
            .rawSource
            .contains(
                QStringLiteral(
                    "<event>"
                    )
                )
        );

    QVERIFY(
        result.records.at(0)
            .rawSource
            .contains(
                QStringLiteral(
                    "Name=\"Attempt\""
                    )
                )
        );
}

void StructuredXmlRecordStreamParserTests::
    supportsWindowsEventXmlRecord()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "Events.Event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        result =
        parser.appendBytes(
            QByteArray(
                "<?xml version=\"1.0\"?>"
                "<Events>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<Provider Name=\"CheckoutApi\"/>"
                "<EventID>4102</EventID>"
                "<Level>2</Level>"
                "<TimeCreated "
                "SystemTime=\"2026-09-13T12:00:00.000Z\"/>"
                "</System>"
                "<EventData>"
                "<Data Name=\"EventCode\">"
                "PAYMENT_FAILURE"
                "</Data>"
                "<Data Name=\"DeviceId\">"
                "checkout-01"
                "</Data>"
                "</EventData>"
                "<RenderingInfo>"
                "<Message>"
                "Payment processing failed"
                "</Message>"
                "</RenderingInfo>"
                "</Event>"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        1
        );

    const QJsonObject object =
        result.records.at(0).object;

    const QJsonObject system =
        object.value(
                  QStringLiteral("System")
                  )
            .toObject();

    const QJsonObject provider =
        system.value(
                  QStringLiteral("Provider")
                  )
            .toObject();

    QCOMPARE(
        provider.value(
                    QStringLiteral("@Name")
                    )
            .toString(),
        QStringLiteral("CheckoutApi")
        );

    QCOMPARE(
        system.value(
                  QStringLiteral("Level")
                  )
            .toString(),
        QStringLiteral("2")
        );

    const QJsonObject timeCreated =
        system.value(
                  QStringLiteral("TimeCreated")
                  )
            .toObject();

    QCOMPARE(
        timeCreated.value(
                       QStringLiteral(
                           "@SystemTime"
                           )
                       )
            .toString(),
        QStringLiteral(
            "2026-09-13T12:00:00.000Z"
            )
        );

    const QJsonObject eventData =
        object.value(
                  QStringLiteral("EventData")
                  )
            .toObject();

    const QJsonObject namedData =
        eventData.value(
                     QStringLiteral("NamedData")
                     )
            .toObject();

    QCOMPARE(
        namedData.value(
                     QStringLiteral("EventCode")
                     )
            .toString(),
        QStringLiteral(
            "PAYMENT_FAILURE"
            )
        );

    QCOMPARE(
        namedData.value(
                     QStringLiteral("DeviceId")
                     )
            .toString(),
        QStringLiteral(
            "checkout-01"
            )
        );

    QVERIFY(
        parser.recordContainerLocated()
        );

    QVERIFY(
        !parser.documentClosed()
        );
}

void StructuredXmlRecordStreamParserTests::
    handlesInputOneByteAtATime()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const QByteArray source(
        "<session>"
        "<events>"
        "<event>"
        "<message>"
        "first &amp; important"
        "</message>"
        "</event>"
        "<event>"
        "<message>second</message>"
        "</event>"
        );

    QVector<StructuredXmlRecord> records;

    for (const char byte : source) {
        const StructuredXmlRecordStreamParseResult
            result =
            parser.appendBytes(
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
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral(
            "first & important"
            )
        );

    QCOMPARE(
        records.at(1)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("second")
        );
}

void StructuredXmlRecordStreamParserTests::
    rejectsMalformedXml()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        result =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<message>broken"
                "</event>"
                )
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        result.errorMessage.contains(
            QStringLiteral("line"),
            Qt::CaseInsensitive
            )
        );

    /*
     * A genuine syntax error is terminal for this
     * physical source generation.
     */
    const StructuredXmlRecordStreamParseResult
        subsequent =
        parser.appendBytes(
            QByteArray(
                "</events>"
                "</session>"
                )
            );

    QVERIFY(
        !subsequent.succeeded
        );
}

void StructuredXmlRecordStreamParserTests::
    resetDiscardsIncompleteRecordState()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        first =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<message>old"
                )
            );

    QVERIFY(first.succeeded);
    QVERIFY(first.records.isEmpty());

    QVERIFY(
        parser.hasIncompleteRecord()
        );

    parser.reset();

    QVERIFY(
        !parser.hasIncompleteRecord()
        );

    QVERIFY(
        !parser.recordContainerLocated()
        );

    QVERIFY(
        !parser.recordPathLocated()
        );

    const StructuredXmlRecordStreamParseResult
        second =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<message>new</message>"
                "</event>"
                )
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.records.size(),
        1
        );

    QCOMPARE(
        second.records.at(0)
            .object
            .value(
                QStringLiteral("message")
                )
            .toString(),
        QStringLiteral("new")
        );
}

void StructuredXmlRecordStreamParserTests::
    reportsClosedDocument()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const StructuredXmlRecordStreamParseResult
        result =
        parser.appendBytes(
            QByteArray(
                "<?xml version=\"1.0\"?>"
                "<session>"
                "<events>"
                "<event>"
                "<message>complete</message>"
                "</event>"
                "</events>"
                "</session>"
                )
            );

    QVERIFY(result.succeeded);

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        parser.documentClosed()
        );

    QVERIFY(
        !parser.hasIncompleteRecord()
        );
}

QTEST_MAIN(
    StructuredXmlRecordStreamParserTests
    )

#include "StructuredXmlRecordStreamParserTests.moc"