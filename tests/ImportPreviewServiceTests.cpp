#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/BuiltInImportProfilePresets.h"
#include "../src/importing/ImportPreviewService.h"

class ImportPreviewServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void validProfilePreviewsRecords();
    void csvProfilePreviewsRecords();
    void tsvProfilePreviewsRecords();
    void structuredJsonProfilePreviewsRecords();
    void regexTextProfilePreviewsRecords();
    void keyValueProfilePreviewsRecords();
    void syslogProfilePreviewsRecords();
    void xmlProfilePreviewsRecords();
    void windowsEventXmlPresetPreviewsRecords();
    void previewHonorsRecordLimit();
    void previewPreservesPhysicalRecordNumbers();
    void invalidProfilePreventsPreview();
    void unsupportedImporterPreventsPreview();
    void previewUsesProfileMappings();
    void previewReturnsImportDiagnostics();
    void combinedWebAccessPresetPreviewsRecords();
    void previewPropagatesCancellation();
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

void ImportPreviewServiceTests::regexTextProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "2026-08-11T12:00:00Z "
                "[NOTICE] [Orders] [worker-1] "
                "Order accepted\n"
                "2026-08-11T12:01:00Z "
                "[FAIL] [Payments] [worker-4] "
                "Payment rejected\n"
                )
            );

    QVERIFY(!path.isEmpty());

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview Regex Text"
            );

    profile.importerId =
        QStringLiteral(
            "regex-text"
            );

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+\[(?<thread>[^\]]+)\]\s+(?<message>.*)$)"
            );

    profile.canonicalFields.severityPath =
        QStringLiteral("severity");

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    profile.severityAliases.insert(
        QStringLiteral("FAIL"),
        RecordSeverity::Error
        );

    profile.customFields.append({
        QStringLiteral("Thread"),
        QStringLiteral("thread")
    });

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
        QStringLiteral(
            "Order accepted"
            )
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        result.importResult
            .records.at(1)
            .severity.value(),
        RecordSeverity::Error
        );

    QCOMPARE(
        result.importResult
            .records.at(1)
            .subsystem.value(),
        QStringLiteral(
            "Payments"
            )
        );

    QCOMPARE(
        result.importResult
            .records.at(0)
            .customAttributes
            .value(
                QStringLiteral(
                    "Thread"
                    )
                )
            .toString(),
        QStringLiteral(
            "worker-1"
            )
        );
}

void ImportPreviewServiceTests::keyValueProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp=2026-08-12T08:10:00Z "
                "level=INFO "
                "subsystem=Orders "
                "eventCode=ORDER_ACCEPTED "
                "entityId=ORD-4101 "
                "requestId=REQ-7101 "
                "message=\"Order accepted for processing\"\n"

                "timestamp=2026-08-12T08:11:00Z "
                "level=WARN "
                "subsystem=Inventory "
                "eventCode=LOW_STOCK "
                "entityId=SKU-440 "
                "requestId=REQ-7102 "
                "message=\"Available quantity below threshold\"\n"
                )
            );

    QVERIFY(
        !path.isEmpty()
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview Key-Value"
            );

    profile.importerId =
        QStringLiteral(
            "key-value"
            );

    profile.customFields.append({
        QStringLiteral(
            "Request ID"
            ),
        QStringLiteral(
            "requestId"
            )
    });

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

    const InvestigationRecord &first =
        result.importResult.records.at(0);

    const InvestigationRecord &second =
        result.importResult.records.at(1);

    QCOMPARE(
        first.message.value(),
        QStringLiteral(
            "Order accepted for processing"
            )
        );

    QCOMPARE(
        first.severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        first.subsystem.value(),
        QStringLiteral(
            "Orders"
            )
        );

    QCOMPARE(
        first.eventCode.value(),
        QStringLiteral(
            "ORDER_ACCEPTED"
            )
        );

    QCOMPARE(
        first.entityId.value(),
        QStringLiteral(
            "ORD-4101"
            )
        );

    QCOMPARE(
        first.customAttributes
            .value(
                QStringLiteral(
                    "Request ID"
                    )
                )
            .toString(),
        QStringLiteral(
            "REQ-7101"
            )
        );

    QCOMPARE(
        second.severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        second.subsystem.value(),
        QStringLiteral(
            "Inventory"
            )
        );

    QCOMPARE(
        second.eventCode.value(),
        QStringLiteral(
            "LOW_STOCK"
            )
        );

    QCOMPARE(
        second.entityId.value(),
        QStringLiteral(
            "SKU-440"
            )
        );

    QCOMPARE(
        second.customAttributes
            .value(
                QStringLiteral(
                    "Request ID"
                    )
                )
            .toString(),
        QStringLiteral(
            "REQ-7102"
            )
        );
}

void ImportPreviewServiceTests::syslogProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "<165>1 "
                "2026-08-12T08:20:18.427Z "
                "api-01 "
                "orders-service "
                "4242 "
                "ORDER_RECEIVED "
                "- "
                "Order received for processing\n"

                "<132>1 "
                "2026-08-12T08:21:04.812Z "
                "worker-02 "
                "supplier-gateway "
                "8301 "
                "SUPPLIER_DELAY "
                "- "
                "Supplier response exceeded expected latency\n"
                )
            );

    QVERIFY(
        !path.isEmpty()
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview Syslog"
            );

    profile.importerId =
        QStringLiteral(
            "syslog"
            );

    profile.customFields.append({
        QStringLiteral(
            "Hostname"
            ),
        QStringLiteral(
            "hostname"
            )
    });

    profile.customFields.append({
        QStringLiteral(
            "Process ID"
            ),
        QStringLiteral(
            "processId"
            )
    });

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
        result.importResult
            .records.size(),
        2
        );

    const InvestigationRecord &first =
        result.importResult
            .records.at(0);

    QCOMPARE(
        first.severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        first.subsystem.value(),
        QStringLiteral(
            "orders-service"
            )
        );

    QCOMPARE(
        first.eventCode.value(),
        QStringLiteral(
            "ORDER_RECEIVED"
            )
        );

    QCOMPARE(
        first.message.value(),
        QStringLiteral(
            "Order received for processing"
            )
        );

    QCOMPARE(
        first.customAttributes
            .value(
                QStringLiteral(
                    "Hostname"
                    )
                )
            .toString(),
        QStringLiteral(
            "api-01"
            )
        );

    QCOMPARE(
        first.customAttributes
            .value(
                QStringLiteral(
                    "Process ID"
                    )
                )
            .toString(),
        QStringLiteral(
            "4242"
            )
        );

    const InvestigationRecord &second =
        result.importResult
            .records.at(1);

    QCOMPARE(
        second.severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        second.subsystem.value(),
        QStringLiteral(
            "supplier-gateway"
            )
        );

    QCOMPARE(
        second.eventCode.value(),
        QStringLiteral(
            "SUPPLIER_DELAY"
            )
        );
}

void ImportPreviewServiceTests::xmlProfilePreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "<session>"
                "<events>"
                "<event>"
                "<timestamp>"
                "2026-08-13T08:00:00Z"
                "</timestamp>"
                "<level>INFO</level>"
                "<message>First</message>"
                "</event>"
                "<event>"
                "<timestamp>"
                "2026-08-13T08:01:00Z"
                "</timestamp>"
                "<level>WARN</level>"
                "<message>Second</message>"
                "</event>"
                "</events>"
                "</session>"
                )
            );

    QVERIFY(
        !path.isEmpty()
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Preview Structured XML"
            );

    profile.importerId =
        QStringLiteral("xml");

    profile.recordPath =
        QStringLiteral(
            "session.events.event"
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
}

void ImportPreviewServiceTests::windowsEventXmlPresetPreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "<Events>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<Provider "
                "Name=\"Sample-Provider\" "
                "Guid=\"{11111111-1111-1111-1111-111111111111}\"/>"
                "<EventID Qualifiers=\"0\">4102</EventID>"
                "<Level>3</Level>"
                "<TimeCreated "
                "SystemTime=\"2026-08-13T12:04:00Z\"/>"
                "<EventRecordID>12004</EventRecordID>"
                "<Execution "
                "ProcessID=\"8124\" "
                "ThreadID=\"6408\"/>"
                "<Channel>Application</Channel>"
                "<Computer>ENG-WS-07</Computer>"
                "</System>"
                "<EventData>"
                "<Data Name=\"DeviceId\">TRACKER-03</Data>"
                "<Data Name=\"Result\">Timeout</Data>"
                "</EventData>"
                "<RenderingInfo>"
                "<Message>"
                "Tracking update timed out."
                "</Message>"
                "</RenderingInfo>"
                "</Event>"
                "</Events>"
                )
            );

    QVERIFY(!path.isEmpty());

    const std::optional<ImportProfile> preset =
        builtInImportProfilePreset(
            BuiltInImportProfilePresetIds::
            WindowsEventXmlCollection
            );

    QVERIFY(preset.has_value());

    const ImportPreviewResult result =
        ImportPreviewService()
            .previewFile(
                path,
                preset.value()
                );

    QVERIFY(
        result.canDisplayPreview()
        );

    QCOMPARE(
        result.importResult.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.importResult.records.first();

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral(
            "Sample-Provider"
            )
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral("4102")
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Tracking update timed out."
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "Channel"
                    )
                )
            .toString(),
        QStringLiteral(
            "Application"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.DeviceId"
                    )
                )
            .toString(),
        QStringLiteral(
            "TRACKER-03"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.Result"
                    )
                )
            .toString(),
        QStringLiteral(
            "Timeout"
            )
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

void ImportPreviewServiceTests::
    combinedWebAccessPresetPreviewsRecords()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "198.51.100.10 - - "
                "[12/Aug/2026:10:10:02 -0400] "
                "\"GET / HTTP/1.1\" "
                "200 1834 "
                "\"-\" "
                "\"Mozilla/5.0\"\n"
                )
            );

    QVERIFY(
        !path.isEmpty()
        );

    const std::optional<ImportProfile> profile =
        builtInImportProfilePreset(
            BuiltInImportProfilePresetIds::
            ApacheNginxCombined
            );

    QVERIFY(
        profile.has_value()
        );

    const ImportPreviewResult result =
        ImportPreviewService()
            .previewFile(
                path,
                profile.value()
                );

    QVERIFY(
        result.canDisplayPreview()
        );

    QCOMPARE(
        result.importResult.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.importResult.records.first();

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "GET / HTTP/1.1"
            )
        );

    QVERIFY(
        !record.severity.has_value()
        );

    QVERIFY(
        !record.subsystem.has_value()
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "HTTP Status"
                    )
                )
            .toString(),
        QStringLiteral("200")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "User Agent"
                    )
                )
            .toString(),
        QStringLiteral(
            "Mozilla/5.0"
            )
        );

    QVERIFY(
        record.timestamp.has_value()
        );
}

void
    ImportPreviewServiceTests::
    previewPropagatesCancellation()
{
    QTemporaryFile file;

    const QString path =
        writeTemporaryContent(
            file,
            QStringLiteral(
                "<session>"
                "<event><message>First</message></event>"
                "<event><message>Second</message></event>"
                "</session>"
                )
            );

    QVERIFY(
        !path.isEmpty()
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Cancellable XML Preview"
            );

    profile.importerId =
        QStringLiteral("xml");

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    ImportExecutionContext context;

    context.isCancellationRequested =
        []() {
            return true;
        };

    const ImportPreviewResult result =
        ImportPreviewService()
            .previewFile(
                path,
                profile,
                ImportPreviewService::
                DefaultMaxProcessedRecords,
                context
                );

    QVERIFY(
        result.importResult.cancelled
        );

    QCOMPARE(
        result.importResult
            .processedRecordCount,
        qint64(0)
        );

    QCOMPARE(
        result.importResult.records.size(),
        0
        );
}

QTEST_MAIN(ImportPreviewServiceTests)

#include "ImportPreviewServiceTests.moc"