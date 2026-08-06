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
    void importLinesCreatesFlexibleInvestigationRecord();
    void importLinesPreservesCustomAttributes();
    void importLinesAcceptsMissingCanonicalFields();
    void importLinesReportsMalformedJson();
    void importLinesReportsInvalidCanonicalValues();
    void importLinesReportsNonObjectJson();
    void importFilePreservesSourceMetadata();
    void importFileReportsOpenFailure();
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

void ParserTests::importLinesCreatesFlexibleInvestigationRecord()
{
    JsonLineLogParser parser;

    const QString rawSource = QStringLiteral(
        R"({"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"})"
        );

    const ImportResult result = parser.importLines(
        {rawSource},
        QStringLiteral("samples/session.jsonl")
        );

    QCOMPARE(result.processedRecordCount, qint64(1));
    QCOMPARE(result.importedRecordCount(), qint64(1));
    QCOMPARE(result.skippedRecordCount(), qint64(0));
    QVERIFY(result.diagnostics.isEmpty());

    const InvestigationRecord &record = result.records.first();

    QVERIFY(record.timestamp.has_value());
    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 7, 7)
        );
    QCOMPARE(
        record.timestamp->time(),
        QTime(10, 14, 22, 381)
        );

    QCOMPARE(
        record.severity,
        std::optional<RecordSeverity>(
            RecordSeverity::Warning
            )
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral("Tracking")
            )
        );

    QCOMPARE(
        record.eventCode,
        std::optional<QString>(
            QStringLiteral("TRACK_LOST")
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral("Track 402 lost for 1200ms")
            )
        );

    QCOMPARE(
        record.entityId,
        std::optional<QString>(
            QStringLiteral("TRK-402")
            )
        );

    QCOMPARE(record.rawSource, rawSource);
    QCOMPARE(
        record.source.sourcePath,
        QStringLiteral("samples/session.jsonl")
        );
    QCOMPARE(
        record.source.sourceName,
        QStringLiteral("session.jsonl")
        );
    QCOMPARE(record.source.recordNumber, qint64(1));

    QVERIFY(!record.recordId.isEmpty());
    QCOMPARE(record.recordId.size(), 64);
}

void ParserTests::importLinesPreservesCustomAttributes()
{
    JsonLineLogParser parser;

    const ImportResult result = parser.importLines({
        R"({"timestamp":"2026-07-07T10:14:22Z","level":"INFO","message":"Request completed","durationMs":184,"success":true,"temperature":42.5,"tags":["api","completed"],"context":{"region":"east"}})"
    });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record = result.records.first();

    QCOMPARE(record.customAttributes.size(), 5);

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("durationMs")
                                   ).toInt(),
        184
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("success")
                                   ).toBool(),
        true
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("temperature")
                                   ).toDouble(),
        42.5
        );

    const QVariantList tags = record.customAttributes.value(
                                                         QStringLiteral("tags")
                                                         ).toList();

    QCOMPARE(tags.size(), 2);
    QCOMPARE(tags.at(0).toString(), QStringLiteral("api"));
    QCOMPARE(tags.at(1).toString(), QStringLiteral("completed"));

    const QVariantMap context = record.customAttributes.value(
                                                           QStringLiteral("context")
                                                           ).toMap();

    QCOMPARE(
        context.value(QStringLiteral("region")).toString(),
        QStringLiteral("east")
        );

    QVERIFY(!record.customAttributes.contains(
        QStringLiteral("timestamp")
        ));
    QVERIFY(!record.customAttributes.contains(
        QStringLiteral("level")
        ));
    QVERIFY(!record.customAttributes.contains(
        QStringLiteral("message")
        ));
}

void ParserTests::importLinesAcceptsMissingCanonicalFields()
{
    JsonLineLogParser parser;

    const ImportResult result = parser.importLines({
        R"({"requestId":"REQ-204","durationMs":318,"completed":true})"
    });

    QCOMPARE(result.processedRecordCount, qint64(1));
    QCOMPARE(result.importedRecordCount(), qint64(1));
    QVERIFY(result.diagnostics.isEmpty());

    const InvestigationRecord &record = result.records.first();

    QVERIFY(!record.timestamp.has_value());
    QVERIFY(!record.severity.has_value());
    QVERIFY(!record.subsystem.has_value());
    QVERIFY(!record.eventCode.has_value());
    QVERIFY(!record.message.has_value());
    QVERIFY(!record.entityId.has_value());

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("requestId")
                                   ).toString(),
        QStringLiteral("REQ-204")
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("durationMs")
                                   ).toInt(),
        318
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("completed")
                                   ).toBool(),
        true
        );
}

void ParserTests::importLinesReportsMalformedJson()
{
    JsonLineLogParser parser;

    const ImportResult result = parser.importLines(
        {
            QString(),
            R"({"message":"Valid record"})",
            R"({"message":)",
            QStringLiteral("   ")
        },
        QStringLiteral("samples/mixed.jsonl")
        );

    QCOMPARE(result.processedRecordCount, qint64(2));
    QCOMPARE(result.importedRecordCount(), qint64(1));
    QCOMPARE(result.skippedRecordCount(), qint64(1));

    QCOMPARE(result.diagnostics.size(), 1);
    QVERIFY(result.hasErrors());
    QVERIFY(!result.hasWarnings());

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral("MALFORMED_JSON")
        );

    QCOMPARE(
        diagnostic.severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(diagnostic.source.has_value());
    QCOMPARE(
        diagnostic.source->sourcePath,
        QStringLiteral("samples/mixed.jsonl")
        );

    // Physical source line, including the blank line above it.
    QCOMPARE(
        diagnostic.source->recordNumber,
        qint64(3)
        );
}

void ParserTests::importLinesReportsInvalidCanonicalValues()
{
    JsonLineLogParser parser;

    const ImportResult result = parser.importLines({
        R"({"timestamp":"July-ish","level":"NOTICE","message":"Still useful"})"
    });

    QCOMPARE(result.processedRecordCount, qint64(1));
    QCOMPARE(result.importedRecordCount(), qint64(1));
    QCOMPARE(result.skippedRecordCount(), qint64(0));

    QCOMPARE(result.diagnostics.size(), 2);
    QVERIFY(result.hasWarnings());
    QVERIFY(!result.hasErrors());

    const InvestigationRecord &record = result.records.first();

    QVERIFY(!record.timestamp.has_value());
    QVERIFY(!record.severity.has_value());

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral("Still useful")
            )
        );

    QSet<QString> diagnosticCodes;

    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        diagnosticCodes.insert(diagnostic.code);

        QCOMPARE(
            diagnostic.severity,
            ImportDiagnosticSeverity::Warning
            );

        QVERIFY(diagnostic.source.has_value());
        QCOMPARE(
            diagnostic.source->recordNumber,
            qint64(1)
            );
    }

    QVERIFY(diagnosticCodes.contains(
        QStringLiteral("INVALID_TIMESTAMP")
        ));

    QVERIFY(diagnosticCodes.contains(
        QStringLiteral("UNMAPPED_SEVERITY")
        ));
}

void ParserTests::importLinesReportsNonObjectJson()
{
    JsonLineLogParser parser;

    const ImportResult result = parser.importLines({
        R"(["INFO","Tracking","Session started"])",
        R"([{"message":"First"},{"message":"Second"}])",
        R"([])"
    });

    QCOMPARE(result.processedRecordCount, qint64(3));
    QCOMPARE(result.importedRecordCount(), qint64(0));
    QCOMPARE(result.skippedRecordCount(), qint64(3));
    QCOMPARE(result.diagnostics.size(), 3);

    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        QCOMPARE(
            diagnostic.code,
            QStringLiteral("JSON_VALUE_NOT_OBJECT")
            );

        QCOMPARE(
            diagnostic.severity,
            ImportDiagnosticSeverity::Error
            );
    }
}

void ParserTests::importFilePreservesSourceMetadata()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    QTextStream stream(&file);

    stream
        << R"({"timestamp":"2026-07-07T10:14:22Z","level":"INFO","message":"First"})"
        << "\n";

    stream << "\n";

    stream
        << R"({"timestamp":"2026-07-07T10:14:23Z","level":"ERROR","message":"Third"})"
        << "\n";

    stream.flush();

    const QString filePath = file.fileName();
    const QString fileName = QFileInfo(filePath).fileName();

    file.close();

    JsonLineLogParser parser;

    const ImportResult result = parser.importFile(filePath);

    QCOMPARE(result.processedRecordCount, qint64(2));
    QCOMPARE(result.importedRecordCount(), qint64(2));
    QCOMPARE(result.records.size(), 2);

    QCOMPARE(
        result.records.at(0).source.sourcePath,
        filePath
        );
    QCOMPARE(
        result.records.at(0).source.sourceName,
        fileName
        );
    QCOMPARE(
        result.records.at(0).source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.records.at(1).source.sourcePath,
        filePath
        );
    QCOMPARE(
        result.records.at(1).source.sourceName,
        fileName
        );
    QCOMPARE(
        result.records.at(1).source.recordNumber,
        qint64(3)
        );
}

void ParserTests::importFileReportsOpenFailure()
{
    JsonLineLogParser parser;

    const QString missingPath = QStringLiteral(
        "missing/nonexistent/session.jsonl"
        );

    const ImportResult result = parser.importFile(
        missingPath
        );

    QCOMPARE(result.processedRecordCount, qint64(0));
    QCOMPARE(result.importedRecordCount(), qint64(0));
    QCOMPARE(result.skippedRecordCount(), qint64(0));

    QCOMPARE(result.diagnostics.size(), 1);
    QVERIFY(result.hasErrors());

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral("FILE_OPEN_FAILED")
        );

    QCOMPARE(
        diagnostic.severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(diagnostic.source.has_value());
    QCOMPARE(
        diagnostic.source->sourcePath,
        missingPath
        );
    QCOMPARE(
        diagnostic.source->sourceName,
        QStringLiteral("session.jsonl")
        );
    QCOMPARE(
        diagnostic.source->recordNumber,
        qint64(0)
        );
}

QTEST_MAIN(ParserTests)

#include "ParserTests.moc"