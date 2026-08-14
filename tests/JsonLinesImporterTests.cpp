#include <QtTest/QtTest>

#include <optional>

#include <QFileInfo>
#include <QSet>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>

#include "../src/importing/JsonLinesImporter.h"

class JsonLinesImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void importerHasStableId();
    void importerHasDisplayName();

    void importLinesCreatesFlexibleInvestigationRecord();
    void importLinesPreservesCustomAttributes();
    void importLinesAcceptsMissingCanonicalFields();
    void importLinesReportsMalformedJson();
    void importLinesReportsInvalidCanonicalValues();
    void importLinesReportsNonObjectJson();

    void importFilePreservesSourceMetadata();
    void importFileReportsOpenFailure();
    void importFileHonorsRecordLimit();

    void importFileReportsProgress();
    void importFileCanBeCancelled();

    void customConfigMapsAlternativeTopLevelFields();
    void customConfigMapsNestedFields();
    void nestedMappingsPreserveSourceAttributes();
    void nestedMappedFieldsDoNotCreateContainerAttributes();
    void emptyPathLeavesCanonicalFieldUnset();
    void configuredInvalidTimestampReportsWarning();
    void configuredUnmappedSeverityReportsWarning();

    void profileMapsExplicitCustomFields();
    void profileCanDisableUnmappedFieldPreservation();
    void profileSeverityAliasMapsCustomValue();
    void profileSeverityAliasIsCaseInsensitive();
    void profileQtTimestampRuleParsesCustomFormat();
    void profileTimestampRulesUseFirstSuccessfulRule();
};

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

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.timestamp.has_value());

    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 7, 7)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(10, 14, 22, 381)
        );

    QVERIFY(record.severity.has_value());

    QCOMPARE(
        *record.severity,
        RecordSeverity::Warning
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
            QStringLiteral(
                "Track 402 lost for 1200ms"
                )
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

    QCOMPARE(
        record.source.recordNumber,
        qint64(1)
        );

    QVERIFY(!record.recordId.isEmpty());

    QCOMPARE(
        record.recordId.size(),
        64
        );
}

void JsonLinesImporterTests::importLinesPreservesCustomAttributes()
{
    JsonLinesImporter importer;

    const ImportResult result = importer.importLines({
        R"({"timestamp":"2026-07-07T10:14:22Z","level":"INFO","message":"Request completed","durationMs":184,"success":true,"temperature":42.5,"tags":["api","completed"],"context":{"region":"east"}})"
    });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.size(),
        5
        );

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

    const QVariantList tags =
        record.customAttributes.value(
            QStringLiteral("tags")
            ).toList();

    QCOMPARE(tags.size(), 2);

    QCOMPARE(
        tags.at(0).toString(),
        QStringLiteral("api")
        );

    QCOMPARE(
        tags.at(1).toString(),
        QStringLiteral("completed")
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral(
                "context.region"
                )
            ).toString(),
        QStringLiteral("east")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("context")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("timestamp")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("level")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("message")
            )
        );
}

void JsonLinesImporterTests::importLinesAcceptsMissingCanonicalFields()
{
    JsonLinesImporter importer;

    const ImportResult result =
        importer.importLines({
            R"({"requestId":"REQ-204","durationMs":318,"completed":true})"
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(1)
        );

    QVERIFY(result.diagnostics.isEmpty());

    const InvestigationRecord &record =
        result.records.first();

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

    const ImportResult result =
        importer.importLines(
            {
                QString(),
                R"({"message":"Valid record"})",
                R"({"message":)",
                QStringLiteral("   ")
            },
            QStringLiteral(
                "samples/mixed.jsonl"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

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
        QStringLiteral(
            "samples/mixed.jsonl"
            )
        );

    // Physical source line, including
    // the blank source line above it.
    QCOMPARE(
        diagnostic.source->recordNumber,
        qint64(3)
        );
}

void JsonLinesImporterTests::importLinesReportsInvalidCanonicalValues()
{
    JsonLinesImporter importer;

    const ImportResult result =
        importer.importLines({
            R"({"timestamp":"July-ish","level":"NOTICE","message":"Still useful"})"
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.diagnostics.size(),
        2
        );

    QVERIFY(result.hasWarnings());
    QVERIFY(!result.hasErrors());

    const InvestigationRecord &record =
        result.records.first();

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
        diagnosticCodes.insert(
            diagnostic.code
            );

        QCOMPARE(
            diagnostic.severity,
            ImportDiagnosticSeverity::Warning
            );

        QVERIFY(
            diagnostic.source.has_value()
            );

        QCOMPARE(
            diagnostic.source->recordNumber,
            qint64(1)
            );
    }

    QVERIFY(
        diagnosticCodes.contains(
            QStringLiteral(
                "INVALID_TIMESTAMP"
                )
            )
        );

    QVERIFY(
        diagnosticCodes.contains(
            QStringLiteral(
                "UNMAPPED_SEVERITY"
                )
            )
        );
}

void JsonLinesImporterTests::importLinesReportsNonObjectJson()
{
    JsonLinesImporter importer;

    const ImportResult result =
        importer.importLines({
            R"(["INFO","Tracking","Session started"])",
            R"([{"message":"First"},{"message":"Second"}])",
            R"([])"
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(3)
        );

    QCOMPARE(
        result.diagnostics.size(),
        3
        );

    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        QCOMPARE(
            diagnostic.code,
            QStringLiteral(
                "JSON_VALUE_NOT_OBJECT"
                )
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

    const QString filePath =
        file.fileName();

    const QString fileName =
        QFileInfo(filePath).fileName();

    file.close();

    JsonLinesImporter importer;

    const ImportResult result =
        importer.importFile(filePath);

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

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

    const QString missingPath =
        QStringLiteral(
            "missing/nonexistent/session.jsonl"
            );

    const ImportResult result =
        importer.importFile(
            missingPath
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(0)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QVERIFY(result.hasErrors());

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral(
            "FILE_OPEN_FAILED"
            )
        );

    QCOMPARE(
        diagnostic.severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(
        diagnostic.source.has_value()
        );

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

void JsonLinesImporterTests::importFileHonorsRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    QTextStream stream(&file);

    stream
        << "{\"message\":\"One\"}\n"
        << "\n"
        << "{\"message\":\"Two\"}\n"
        << "{\"message\":\"Three\"}\n";

    stream.flush();

    const QString path =
        file.fileName();

    file.close();

    JsonLinesImporter importer;

    const ImportResult result =
        importer.importFile(
            path,
            2
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(3)
        );

    QVERIFY(result.sourceTruncated);
}

void JsonLinesImporterTests::importFileReportsProgress()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    file.write(
        "{\"message\":\"One\"}\n"
        "{\"message\":\"Two\"}\n"
        );

    file.flush();

    const QString path =
        file.fileName();

    const qint64 expectedSize =
        file.size();

    file.close();

    QVector<ImportProgress> updates;

    ImportExecutionContext context;

    context.reportProgress =
        [&updates](
            const ImportProgress &progress
            ) {
            updates.append(progress);
        };

    JsonLinesImporter importer;

    const ImportResult result =
        importer.importFile(
            path,
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QVERIFY(!updates.isEmpty());

    QCOMPARE(
        updates.first().bytesProcessed,
        qint64(0)
        );

    QCOMPARE(
        updates.first().totalBytes,
        expectedSize
        );

    QCOMPARE(
        updates.last().bytesProcessed,
        expectedSize
        );

    QCOMPARE(
        updates.last().totalBytes,
        expectedSize
        );

    QCOMPARE(
        updates.last().processedRecordCount,
        qint64(2)
        );
}

void JsonLinesImporterTests::importFileCanBeCancelled()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    file.write(
        "{\"message\":\"One\"}\n"
        "{\"message\":\"Two\"}\n"
        "{\"message\":\"Three\"}\n"
        );

    file.flush();

    const QString path =
        file.fileName();

    file.close();

    int cancellationChecks = 0;

    ImportExecutionContext context;

    context.isCancellationRequested =
        [&cancellationChecks]() {
            ++cancellationChecks;

            return cancellationChecks > 1;
        };

    JsonLinesImporter importer;

    const ImportResult result =
        importer.importFile(
            path,
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QVERIFY(result.cancelled);

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(!result.sourceTruncated);
}

void JsonLinesImporterTests::customConfigMapsAlternativeTopLevelFields()
{
    JsonLinesImportConfig config;

    config.timestampPath =
        QStringLiteral("time");

    config.severityPath =
        QStringLiteral("severity");

    config.subsystemPath =
        QStringLiteral("service");

    config.eventCodePath =
        QStringLiteral("code");

    config.entityIdPath =
        QStringLiteral("resourceId");

    config.messagePath =
        QStringLiteral("text");

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"time":"2026-08-07T09:30:00Z","severity":"ERROR","service":"Orders","code":"ORDER_FAILED","resourceId":"ORD-482","text":"Order submission failed","retryCount":3})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.diagnostics.isEmpty()
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.timestamp.has_value());

    QVERIFY(record.severity.has_value());

    QCOMPARE(
        *record.severity,
        RecordSeverity::Error
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral("Orders")
            )
        );

    QCOMPARE(
        record.eventCode,
        std::optional<QString>(
            QStringLiteral("ORDER_FAILED")
            )
        );

    QCOMPARE(
        record.entityId,
        std::optional<QString>(
            QStringLiteral("ORD-482")
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral(
                "Order submission failed"
                )
            )
        );

    // Configured top-level canonical fields
    // should not also appear as custom attributes.
    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("time")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("severity")
            )
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("retryCount")
            ).toInt(),
        3
        );
}

void JsonLinesImporterTests::customConfigMapsNestedFields()
{
    JsonLinesImportConfig config;

    config.timestampPath =
        QStringLiteral(
            "metadata.occurredAt"
            );

    config.severityPath =
        QStringLiteral(
            "metadata.severity"
            );

    config.subsystemPath =
        QStringLiteral(
            "metadata.service"
            );

    config.eventCodePath =
        QStringLiteral("event.code");

    config.messagePath =
        QStringLiteral("event.text");

    config.entityIdPath =
        QStringLiteral("event.entity");

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"metadata":{"occurredAt":"2026-08-07T09:30:00Z","severity":"WARN","service":"Orders"},"event":{"code":"ORDER_DELAYED","text":"Supplier response exceeded threshold","entity":"ORD-482"},"durationMs":1420})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.diagnostics.isEmpty()
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.timestamp.has_value());
    QVERIFY(record.severity.has_value());

    QCOMPARE(
        *record.severity,
        RecordSeverity::Warning
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral("Orders")
            )
        );

    QCOMPARE(
        record.eventCode,
        std::optional<QString>(
            QStringLiteral("ORDER_DELAYED")
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral(
                "Supplier response exceeded threshold"
                )
            )
        );

    QCOMPARE(
        record.entityId,
        std::optional<QString>(
            QStringLiteral("ORD-482")
            )
        );
}

void JsonLinesImporterTests::nestedMappingsPreserveSourceAttributes()
{
    JsonLinesImportConfig config;

    config.timestampPath =
        QStringLiteral(
            "metadata.occurredAt"
            );

    config.severityPath =
        QStringLiteral(
            "metadata.severity"
            );

    config.subsystemPath =
        QStringLiteral(
            "metadata.service"
            );

    config.eventCodePath =
        QStringLiteral("event.code");

    config.messagePath =
        QStringLiteral("event.text");

    config.entityIdPath =
        QStringLiteral("event.entity");

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"metadata":{"occurredAt":"2026-08-07T09:30:00Z","severity":"WARN","service":"Orders","region":"east"},"event":{"code":"ORDER_DELAYED","text":"Supplier response exceeded threshold","entity":"ORD-482","attempt":2},"durationMs":1420})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    // Nested canonical mappings should exclude only
    // the mapped leaves while preserving unrelated
    // source values under their full paths.
    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("metadata")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("event")
            )
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("metadata.region")
            ).toString(),
        QStringLiteral("east")
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("event.attempt")
            ).toInt(),
        2
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("durationMs")
            ).toInt(),
        1420
        );

    // Canonical source leaves should not also
    // appear as custom attributes.
    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("metadata.service")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("event.code")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("event.text")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("event.entity")
            )
        );
}

void JsonLinesImporterTests::nestedMappedFieldsDoNotCreateContainerAttributes()
{
    ImportProfile profile;

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral("context.requestId")
    });

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({
            "message":"Completed",
            "context":{
                "requestId":"REQ-42",
                "region":"us-east"
            }
        })"
        });

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("Request ID")
            ).toString(),
        QStringLiteral("REQ-42")
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("context.region")
            ).toString(),
        QStringLiteral("us-east")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("context")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral(
                "context.requestId"
                )
            )
        );
}

void JsonLinesImporterTests::emptyPathLeavesCanonicalFieldUnset()
{
    JsonLinesImportConfig config;

    config.messagePath = QString();

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"timestamp":"2026-08-07T09:30:00Z","level":"INFO","message":"This remains source data","requestId":"REQ-204"})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        !record.message.has_value()
        );

    // Because the message path is disabled,
    // "message" remains ordinary source data.
    QVERIFY(
        record.customAttributes.contains(
            QStringLiteral("message")
            )
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("message")
                                   ).toString(),
        QStringLiteral(
            "This remains source data"
            )
        );
}

void JsonLinesImporterTests::configuredInvalidTimestampReportsWarning()
{
    JsonLinesImportConfig config;

    config.timestampPath =
        QStringLiteral(
            "metadata.occurredAt"
            );

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"metadata":{"occurredAt":"sometime yesterday"},"message":"Record remains useful"})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        !record.timestamp.has_value()
        );

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral(
            "INVALID_TIMESTAMP"
            )
        );

    QCOMPARE(
        diagnostic.severity,
        ImportDiagnosticSeverity::Warning
        );

    QVERIFY(
        diagnostic.source.has_value()
        );

    QCOMPARE(
        diagnostic.source->recordNumber,
        qint64(1)
        );
}

void JsonLinesImporterTests::configuredUnmappedSeverityReportsWarning()
{
    JsonLinesImportConfig config;

    config.severityPath =
        QStringLiteral(
            "metadata.severity"
            );

    JsonLinesImporter importer(config);

    const ImportResult result =
        importer.importLines({
            R"({"metadata":{"severity":"NOTICE"},"message":"Record remains useful"})"
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        !record.severity.has_value()
        );

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral(
            "UNMAPPED_SEVERITY"
            )
        );

    QCOMPARE(
        diagnostic.severity,
        ImportDiagnosticSeverity::Warning
        );

    QVERIFY(
        diagnostic.source.has_value()
        );

    QCOMPARE(
        diagnostic.source->recordNumber,
        qint64(1)
        );
}

void JsonLinesImporterTests::
    profileMapsExplicitCustomFields()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Custom mapping");

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral(
            "context.requestId"
            )
    });

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"timestamp":"2026-08-09T10:00:00Z","level":"INFO","message":"Completed","context":{"requestId":"REQ-42"},"durationMs":184})"
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("Request ID")
            ).toString(),
        QStringLiteral("REQ-42")
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("Request ID")
            ).toString(),
        QStringLiteral("REQ-42")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("context")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("context.requestId")
            )
        );

    QVERIFY(
        record.customAttributes.contains(
            QStringLiteral("durationMs")
            )
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("durationMs")
            ).toInt(),
        184
        );
}

void JsonLinesImporterTests::
    profileCanDisableUnmappedFieldPreservation()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Mapped only");

    profile.preserveUnmappedFields = false;

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral(
            "context.requestId"
            )
    });

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"message":"Completed","context":{"requestId":"REQ-42"},"durationMs":184,"host":"api-02"})"
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.size(),
        1
        );

    QCOMPARE(
        record.customAttributes.value(
            QStringLiteral("Request ID")
            ).toString(),
        QStringLiteral("REQ-42")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("durationMs")
            )
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("host")
            )
        );
}

void JsonLinesImporterTests::
    profileSeverityAliasMapsCustomValue()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Severity aliases");

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"level":"NOTICE","message":"Request accepted"})"
        });

    QCOMPARE(result.records.size(), 1);

    QVERIFY(
        result.records.first()
            .severity
            .has_value()
        );

    QCOMPARE(
        *result.records.first().severity,
        RecordSeverity::Info
        );

    QVERIFY(result.diagnostics.isEmpty());
}

void JsonLinesImporterTests::
    profileSeverityAliasIsCaseInsensitive()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Severity aliases");

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Warning
        );

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"level":"notice"})"
        });

    QVERIFY(
        result.records.first()
            .severity
            .has_value()
        );

    QCOMPARE(
        *result.records.first().severity,
        RecordSeverity::Warning
        );
}

void JsonLinesImporterTests::
    profileQtTimestampRuleParsesCustomFormat()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Custom timestamp");

    profile.timestampRules.clear();

    profile.timestampRules.append({
        TimestampRuleType::QtFormat,
        QStringLiteral(
            "yyyy/MM/dd HH:mm:ss.zzz"
            )
    });

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"timestamp":"2026/08/09 07:42:31.125","message":"Started"})"
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.timestamp.has_value());

    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 8, 9)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(7, 42, 31, 125)
        );

    QVERIFY(result.diagnostics.isEmpty());
}

void JsonLinesImporterTests::
    profileTimestampRulesUseFirstSuccessfulRule()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Timestamp fallback");

    profile.timestampRules.clear();

    profile.timestampRules.append({
        TimestampRuleType::QtFormat,
        QStringLiteral(
            "MM-dd-yyyy HH:mm:ss"
            )
    });

    profile.timestampRules.append({
        TimestampRuleType::Iso8601,
        QString()
    });

    JsonLinesImporter importer(profile);

    const ImportResult result =
        importer.importLines({
            R"({"timestamp":"2026-08-09T07:45:00Z"})"
        });

    QVERIFY(
        result.records.first()
            .timestamp
            .has_value()
        );

    QCOMPARE(
        result.records.first()
            .timestamp->date(),
        QDate(2026, 8, 9)
        );

    QVERIFY(result.diagnostics.isEmpty());
}

QTEST_MAIN(JsonLinesImporterTests)

#include "JsonLinesImporterTests.moc"