#include <QtTest/QtTest>

#include "../src/parsing/JsonLineLogParser.h"

class ParserTests : public QObject
{
    Q_OBJECT

private slots:
    void parseLinesReturnsEmptyCollectionForEmptyInput();
    void parseLinesParsesSingleValidEvent();
    void parseLinesSkipsEmptyLines();
    void parseLinesSkipsMalformedJson();
    void parseFileParsesValidJsonLinesFromFile();
};

void ParserTests::parseLinesReturnsEmptyCollectionForEmptyInput()
{
    JsonLineLogParser parser;

    const auto events = parser.parseLines({});

    QCOMPARE(events.size(), 0);
}

void ParserTests::parseLinesParsesSingleValidEvent()
{
    JsonLineLogParser parser;

    const auto events = parser.parseLines({
        R"({"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"})"
    });

    QCOMPARE(events.size(), 1);
    QCOMPARE(events[0].timestamp, QString("2026-07-07T10:14:22.381Z"));
    QCOMPARE(events[0].level, QString("WARN"));
    QCOMPARE(events[0].subsystem, QString("Tracking"));
    QCOMPARE(events[0].eventCode, QString("TRACK_LOST"));
    QCOMPARE(events[0].message, QString("Track 402 lost for 1200ms"));
    QCOMPARE(events[0].entityId, QString("TRK-402"));
}

void ParserTests::parseLinesSkipsEmptyLines()
{
    JsonLineLogParser parser;

    const auto events = parser.parseLines({
        "",
        "   ",
        R"({"timestamp":"2026-07-07T10:14:23.014Z","level":"ERROR","subsystem":"Comms","eventCode":"PACKET_DROP","message":"Packet loss exceeded threshold","entityId":"LINK-A"})"
    });

    QCOMPARE(events.size(), 1);
    QCOMPARE(events[0].level, QString("ERROR"));
    QCOMPARE(events[0].subsystem, QString("Comms"));
}

void ParserTests::parseLinesSkipsMalformedJson()
{
    JsonLineLogParser parser;

    const auto events = parser.parseLines({
        R"({"timestamp":"2026-07-07T10:14:22.381Z","level":"INFO","subsystem":"Startup","eventCode":"SESSION_START","message":"Telemetry session initialized","entityId":"SYS-001"})",
        R"({"timestamp":"broken", "level":)",
        R"({"timestamp":"2026-07-07T10:14:24.219Z","level":"WARN","subsystem":"Power","eventCode":"VOLTAGE_DIP","message":"Voltage dipped below nominal range","entityId":"PWR-02"})"
    });

    QCOMPARE(events.size(), 2);
    QCOMPARE(events[0].eventCode, QString("SESSION_START"));
    QCOMPARE(events[1].eventCode, QString("VOLTAGE_DIP"));
}

void ParserTests::parseFileParsesValidJsonLinesFromFile()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    QTextStream stream(&file);
    stream << R"({"timestamp":"2026-07-07T10:14:22.381Z","level":"INFO","subsystem":"Startup","eventCode":"SESSION_START","message":"Telemetry session initialized","entityId":"SYS-001"})" << "\n";
    stream << R"({"timestamp":"2026-07-07T10:14:23.014Z","level":"ERROR","subsystem":"Comms","eventCode":"PACKET_DROP","message":"Packet loss exceeded threshold","entityId":"LINK-A"})" << "\n";
    stream.flush();

    const QString filePath = file.fileName();
    file.close();

    JsonLineLogParser parser;

    const auto events = parser.parseFile(filePath);

    QCOMPARE(events.size(), 2);
    QCOMPARE(events[0].eventCode, QString("SESSION_START"));
    QCOMPARE(events[1].eventCode, QString("PACKET_DROP"));
}

QTEST_MAIN(ParserTests)

#include "ParserTests.moc"