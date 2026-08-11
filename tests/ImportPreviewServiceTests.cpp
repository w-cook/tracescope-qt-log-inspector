#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/ImportPreviewService.h"

class ImportPreviewServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void validProfilePreviewsRecords();
    void csvProfilePreviewsRecords();
    void tsvProfilePreviewsRecords();
    void structuredJsonProfilePreviewsRecords();
    void previewHonorsRecordLimit();
    void previewPreservesPhysicalRecordNumbers();
    void invalidProfilePreventsPreview();
    void unsupportedImporterPreventsPreview();
    void previewUsesProfileMappings();
    void previewReturnsImportDiagnostics();
};

QString writeTemporaryContent(
    QTemporaryFile &file,
    const QString &content
    )
{
    if (!file.open()) {
        return {};
    }

    QTextStream stream(&file);

    stream << content;
    stream.flush();

    const QString path =
        file.fileName();

    file.close();

    return path;
}

ImportProfile validPreviewProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview JSON Lines"
            );

    return profile;
}

void ImportPreviewServiceTests::validProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "{\"message\":\"First\"}\n"
                "{\"message\":\"Second\"}\n"
                )
            );

    QVERIFY(!path.isEmpty());

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            validPreviewProfile()
            );

    QVERIFY(result.canDisplayPreview());

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QVERIFY(!result.sourceTruncated);
}

void ImportPreviewServiceTests::csvProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp,level,message,requestId\n"
                "2026-08-11T08:00:00Z,INFO,"
                "First,REQ-1\n"
                "2026-08-11T08:01:00Z,WARN,"
                "Second,REQ-2\n"
                )
            );

    QVERIFY(!path.isEmpty());

    ImportProfile profile;

    profile.name =
        QStringLiteral("Preview CSV");

    profile.importerId =
        QStringLiteral("csv");

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            profile
            );

    QVERIFY(result.canDisplayPreview());

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .message.value(),
        QStringLiteral("First")
        );

    QCOMPARE(
        result.importResult
            .records.at(1)
            .severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .customAttributes
            .value(
                QStringLiteral("requestId")
                )
            .toString(),
        QStringLiteral("REQ-1")
        );
}

void ImportPreviewServiceTests::tsvProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp\tlevel\tmessage\n"
                "2026-08-11T08:00:00Z\tERROR\t"
                "Database unavailable\n"
                )
            );

    QVERIFY(!path.isEmpty());

    ImportProfile profile;

    profile.name =
        QStringLiteral("Preview TSV");

    profile.importerId =
        QStringLiteral("tsv");

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            profile
            );

    QVERIFY(result.canDisplayPreview());

    QCOMPARE(
        result.importResult.records.size(),
        1
        );

    QCOMPARE(
        result.importResult
            .records.first()
            .message.value(),
        QStringLiteral(
            "Database unavailable"
            )
        );

    QCOMPARE(
        result.importResult
            .records.first()
            .severity.value(),
        RecordSeverity::Error
        );
}

void ImportPreviewServiceTests::structuredJsonProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "{"
                "\"metadata\":{"
                "\"session\":\"S-42\""
                "},"
                "\"payload\":{"
                "\"events\":["
                "{"
                "\"timestamp\":"
                "\"2026-08-11T08:00:00Z\","
                "\"level\":\"INFO\","
                "\"message\":\"First\","
                "\"requestId\":\"REQ-1\""
                "},"
                "{"
                "\"timestamp\":"
                "\"2026-08-11T08:01:00Z\","
                "\"level\":\"WARN\","
                "\"message\":\"Second\","
                "\"requestId\":\"REQ-2\""
                "}"
                "]"
                "}"
                "}"
                )
            );

    QVERIFY(!path.isEmpty());

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview Structured JSON"
            );

    profile.importerId =
        QStringLiteral(
            "structured-json"
            );

    profile.recordPath =
        QStringLiteral(
            "payload.events"
            );

    const ImportPreviewResult result =
        ImportPreviewService()
            .previewFile(
                path,
                profile
                );

    QVERIFY(
        result.canDisplayPreview()
        );

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .message.value(),
        QStringLiteral("First")
        );

    QCOMPARE(
        result.importResult
            .records.at(1)
            .severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .customAttributes
            .value(
                QStringLiteral(
                    "requestId"
                    )
                )
            .toString(),
        QStringLiteral("REQ-1")
        );
}

void ImportPreviewServiceTests::previewHonorsRecordLimit()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "{\"message\":\"One\"}\n"
                "{\"message\":\"Two\"}\n"
                "{\"message\":\"Three\"}\n"
                "{\"message\":\"Four\"}\n"
                )
            );

    QVERIFY(!path.isEmpty());

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            validPreviewProfile(),
            2
            );

    QVERIFY(result.canDisplayPreview());

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QVERIFY(result.sourceTruncated);
}

void ImportPreviewServiceTests::previewPreservesPhysicalRecordNumbers()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "\n"
                "{\"message\":\"First\"}\n"
                "\n"
                "{\"message\":\"Second\"}\n"
                )
            );

    QVERIFY(!path.isEmpty());

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            validPreviewProfile(),
            2
            );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QCOMPARE(
        result.importResult.records.at(0)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        result.importResult.records.at(1)
            .source.recordNumber,
        qint64(4)
        );
}

void ImportPreviewServiceTests::invalidProfilePreventsPreview()
{
    ImportProfile profile;

    // Name deliberately left empty.

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            QStringLiteral("anything.jsonl"),
            profile
            );

    QVERIFY(!result.canDisplayPreview());
    QVERIFY(!result.profileValidation.isValid());

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(0)
        );
}

void ImportPreviewServiceTests::unsupportedImporterPreventsPreview()
{
    ImportProfile profile =
        validPreviewProfile();

    profile.importerId =
        QStringLiteral(
            "unsupported-importer"
            );

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            QStringLiteral("anything.log"),
            profile
            );

    QVERIFY(!result.canDisplayPreview());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "PREVIEW_IMPORTER_UNSUPPORTED"
            )
        );
}

void ImportPreviewServiceTests::previewUsesProfileMappings()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                R"({"time":"2026-08-09T08:00:00Z","severity":"NOTICE","text":"Completed","context":{"requestId":"REQ-42"}})"
                "\n"
                )
            );

    QVERIFY(!path.isEmpty());

    ImportProfile profile =
        validPreviewProfile();

    profile.canonicalFields.timestampPath =
        QStringLiteral("time");

    profile.canonicalFields.severityPath =
        QStringLiteral("severity");

    profile.canonicalFields.messagePath =
        QStringLiteral("text");

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

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            profile
            );

    QVERIFY(result.canDisplayPreview());

    const InvestigationRecord &record =
        result.importResult.records.first();

    QVERIFY(record.timestamp.has_value());
    QVERIFY(record.severity.has_value());

    QCOMPARE(
        *record.severity,
        RecordSeverity::Info
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral("Completed")
        );

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("Request ID")
                                   ).toString(),
        QStringLiteral("REQ-42")
        );
}

void ImportPreviewServiceTests::previewReturnsImportDiagnostics()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "{\"message\":\"Valid\"}\n"
                "{\"message\":\n"
                "{\"message\":\"Also valid\"}\n"
                )
            );

    QVERIFY(!path.isEmpty());

    const ImportPreviewResult result =
        ImportPreviewService().previewFile(
            path,
            validPreviewProfile()
            );

    QVERIFY(result.canDisplayPreview());

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result.importResult.records.size(),
        2
        );

    QCOMPARE(
        result.importResult
            .diagnostics.size(),
        1
        );

    QCOMPARE(
        result.importResult
            .diagnostics.first().code,
        QStringLiteral("MALFORMED_JSON")
        );
}

QTEST_MAIN(ImportPreviewServiceTests)

#include "ImportPreviewServiceTests.moc"