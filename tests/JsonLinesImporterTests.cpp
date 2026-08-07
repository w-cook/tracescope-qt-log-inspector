#include <QtTest/QtTest>

#include <QFileInfo>
#include <QSet>
#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/JsonLinesImporter.h"

class JsonLinesImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void importLinesCreatesFlexibleInvestigationRecord();
    void importLinesPreservesCustomAttributes();
    void importLinesAcceptsMissingCanonicalFields();
    void importLinesReportsMalformedJson();
    void importLinesReportsInvalidCanonicalValues();
    void importLinesReportsNonObjectJson();
    void importFilePreservesSourceMetadata();
    void importFileReportsOpenFailure();
    void importerHasStableId();
    void importerHasDisplayName();
};

void JsonLinesImporterTests::importLinesCreatesFlexibleInvestigationRecord()
{
    JsonLinesImporter importer;

    const QString rawSource = QStringLiteral(
        R"({"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"})"
        );

    const ImportResult result = importer.importLines(
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

void JsonLinesImporterTests::importLinesPreservesCustomAttributes()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines({
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

void JsonLinesImporterTests::importLinesAcceptsMissingCanonicalFields()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines({
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

void JsonLinesImporterTests::importLinesReportsMalformedJson()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines(
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

void JsonLinesImporterTests::importLinesReportsInvalidCanonicalValues()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines({
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

void JsonLinesImporterTests::importLinesReportsNonObjectJson()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines({
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

void JsonLinesImporterTests::importFilePreservesSourceMetadata()
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

    JsonLinesImporter importer;

    const ImportResult result = importer.importFile(filePath);

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

void JsonLinesImporterTests::importFileReportsOpenFailure()
{
    JsonLinesImporter importer;

    const QString missingPath = QStringLiteral(
        "missing/nonexistent/session.jsonl"
        );

    const ImportResult result = importer.importFile(
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

void JsonLinesImporterTests::importerHasStableId()
{
    JsonLinesImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral("json-lines")
        );
}

void JsonLinesImporterTests::importerHasDisplayName()
{
    JsonLinesImporter importer;

    QCOMPARE(
        importer.displayName(),
        QStringLiteral("JSON Lines")
        );
}

QTEST_MAIN(JsonLinesImporterTests)

#include "JsonLinesImporterTests.moc"