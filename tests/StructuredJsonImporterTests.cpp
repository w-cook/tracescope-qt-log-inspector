#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/StructuredJsonImporter.h"

namespace
{
const ImportDiagnostic *findDiagnostic(
    const ImportResult &result,
    const QString &code
    )
{
    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        if (diagnostic.code == code) {
            return &diagnostic;
        }
    }

    return nullptr;
}

QString writeTemporaryContent(
    QTemporaryFile &file,
    const QByteArray &content
    )
{
    if (!file.open()) {
        return {};
    }

    if (file.write(content)
        != content.size()) {
        return {};
    }

    file.flush();

    const QString path =
        file.fileName();

    file.close();

    return path;
}

ImportProfile structuredProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Structured JSON Test"
            );

    profile.importerId =
        QStringLiteral(
            "structured-json"
            );

    return profile;
}
}

class StructuredJsonImporterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void hasStableImporterIdentity();
    void importsRootArray();
    void importsRootObject();
    void importsNestedRecordArray();
    void importsNestedRecordObject();
    void appliesProfileMappings();
    void preservesUnmappedFields();
    void reportsMalformedDocument();
    void reportsMissingRecordPath();
    void rejectsScalarRecordPath();
    void rejectsRecordPathOnArrayRoot();
    void skipsNonObjectArrayEntries();
    void honorsRecordLimit();
    void importFilePreservesSourceMetadata();
    void importFileReportsOpenFailure();
};

void
    StructuredJsonImporterTests::
    hasStableImporterIdentity()
{
    StructuredJsonImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral(
            "structured-json"
            )
        );

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "Structured JSON"
            )
        );
}

void
    StructuredJsonImporterTests::
    importsRootArray()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "["
                "{\"timestamp\":\"2026-08-11T09:00:00Z\","
                "\"level\":\"INFO\","
                "\"message\":\"First\"},"
                "{\"timestamp\":\"2026-08-11T09:01:00Z\","
                "\"level\":\"WARN\","
                "\"message\":\"Second\"}"
                "]"
                )
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
            .message.value(),
        QStringLiteral("First")
        );

    QCOMPARE(
        result.records.at(1)
            .severity.value(),
        RecordSeverity::Warning
        );
}

void
    StructuredJsonImporterTests::
    importsRootObject()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{"
                "\"timestamp\":\"2026-08-11T09:00:00Z\","
                "\"level\":\"ERROR\","
                "\"message\":\"Single record\""
                "}"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "Single record"
            )
        );
}

void
    StructuredJsonImporterTests::
    importsNestedRecordArray()
{
    StructuredJsonImportConfig config;

    config.recordPath =
        QStringLiteral(
            "payload.events"
            );

    StructuredJsonImporter importer(
        config,
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{"
                "\"metadata\":{\"session\":\"S-1\"},"
                "\"payload\":{"
                "\"events\":["
                "{\"message\":\"One\"},"
                "{\"message\":\"Two\"}"
                "]"
                "}"
                "}"
                )
            );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .message.value(),
        QStringLiteral("One")
        );

    QCOMPARE(
        result.records.at(1)
            .message.value(),
        QStringLiteral("Two")
        );
}

void
    StructuredJsonImporterTests::
    importsNestedRecordObject()
{
    StructuredJsonImportConfig config;

    config.recordPath =
        QStringLiteral(
            "payload.event"
            );

    StructuredJsonImporter importer(
        config,
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{"
                "\"payload\":{"
                "\"event\":{"
                "\"level\":\"INFO\","
                "\"message\":\"Nested single\""
                "}"
                "}"
                "}"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "Nested single"
            )
        );
}

void
    StructuredJsonImporterTests::
    appliesProfileMappings()
{
    ImportProfile profile =
        structuredProfile();

    profile.canonicalFields.timestampPath =
        QStringLiteral("time");

    profile.canonicalFields.severityPath =
        QStringLiteral("priority");

    profile.canonicalFields.messagePath =
        QStringLiteral("details.text");

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral(
            "context.requestId"
            )
    });

    StructuredJsonImporter importer(
        {},
        profile
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "["
                "{"
                "\"time\":\"2026-08-11T09:00:00Z\","
                "\"priority\":\"NOTICE\","
                "\"details\":{\"text\":\"Mapped\"},"
                "\"context\":{\"requestId\":\"REQ-42\"}"
                "}"
                "]"
                )
            );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral("Mapped")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "Request ID"
                    )
                )
            .toString(),
        QStringLiteral("REQ-42")
        );
}

void
    StructuredJsonImporterTests::
    preservesUnmappedFields()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "["
                "{"
                "\"message\":\"Hello\","
                "\"host\":\"worker-03\","
                "\"durationMs\":187"
                "}"
                "]"
                )
            );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("host")
                )
            .toString(),
        QStringLiteral("worker-03")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "durationMs"
                    )
                )
            .toInt(),
        187
        );
}

void
    StructuredJsonImporterTests::
    reportsMalformedDocument()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{\"events\":["
                )
            );

    QCOMPARE(
        result.records.size(),
        0
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "MALFORMED_JSON_DOCUMENT"
                )
            )
        != nullptr
        );
}

void
    StructuredJsonImporterTests::
    reportsMissingRecordPath()
{
    StructuredJsonImportConfig config;

    config.recordPath =
        QStringLiteral(
            "payload.events"
            );

    StructuredJsonImporter importer(
        config,
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{\"payload\":{}}"
                )
            );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "JSON_RECORD_PATH_NOT_FOUND"
                )
            )
        != nullptr
        );
}

void
    StructuredJsonImporterTests::
    rejectsScalarRecordPath()
{
    StructuredJsonImportConfig config;

    config.recordPath =
        QStringLiteral("payload.count");

    StructuredJsonImporter importer(
        config,
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "{\"payload\":{\"count\":4}}"
                )
            );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "JSON_RECORD_PATH_NOT_CONTAINER"
                )
            )
        != nullptr
        );
}

void
    StructuredJsonImporterTests::
    rejectsRecordPathOnArrayRoot()
{
    StructuredJsonImportConfig config;

    config.recordPath =
        QStringLiteral("events");

    StructuredJsonImporter importer(
        config,
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "[{\"message\":\"One\"}]"
                )
            );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "JSON_RECORD_PATH_REQUIRES_OBJECT_ROOT"
                )
            )
        != nullptr
        );
}

void
    StructuredJsonImporterTests::
    skipsNonObjectArrayEntries()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "["
                "{\"message\":\"One\"},"
                "\"not-an-object\","
                "{\"message\":\"Three\"}"
                "]"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(1)
        );

    const ImportDiagnostic *diagnostic =
        findDiagnostic(
            result,
            QStringLiteral(
                "STRUCTURED_JSON_RECORD_NOT_OBJECT"
                )
            );

    if (diagnostic == nullptr) {
        QFAIL(
            "Expected STRUCTURED_JSON_RECORD_NOT_OBJECT diagnostic."
            );
    }

    QVERIFY(
        diagnostic->source.has_value()
        );

    QCOMPARE(
        diagnostic->source
            ->recordNumber,
        qint64(2)
        );
}

void
    StructuredJsonImporterTests::
    honorsRecordLimit()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "["
                "{\"message\":\"One\"},"
                "{\"message\":\"Two\"},"
                "{\"message\":\"Three\"}"
                "]"
                ),
            {},
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

    QVERIFY(
        result.sourceTruncated
        );
}

void
    StructuredJsonImporterTests::
    importFilePreservesSourceMetadata()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QByteArrayLiteral(
                "["
                "{\"message\":\"One\"},"
                "{\"message\":\"Two\"}"
                "]"
                )
            );

    QVERIFY(!path.isEmpty());

    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importFile(path);

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .source.sourcePath,
        path
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QVERIFY(
        !result.records.at(0)
             .recordId
             .isEmpty()
        );
}

void
    StructuredJsonImporterTests::
    importFileReportsOpenFailure()
{
    StructuredJsonImporter importer(
        {},
        structuredProfile()
        );

    const ImportResult result =
        importer.importFile(
            QStringLiteral(
                "file-that-does-not-exist.json"
                )
            );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                )
            )
        != nullptr
        );
}

QTEST_MAIN(
    StructuredJsonImporterTests
    )

#include "StructuredJsonImporterTests.moc"