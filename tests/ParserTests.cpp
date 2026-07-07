#include <QtTest/QtTest>

#include "../src/parsing/JsonLineLogParser.h"

class ParserTests : public QObject
{
    Q_OBJECT

private slots:
    void parseLinesReturnsEmptyCollectionForEmptyInput();
};

void ParserTests::parseLinesReturnsEmptyCollectionForEmptyInput()
{
    JsonLineLogParser parser;

    const auto events = parser.parseLines({});

    QCOMPARE(events.size(), 0);
}

QTEST_MAIN(ParserTests)

#include "ParserTests.moc"