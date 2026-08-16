#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>
#include <QFileInfo>

#include "../src/importing/KeyValueTextImporter.h"

class KeyValueTextImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void importerHasStableId();
    void importerHasDisplayName();

    void mapsCanonicalFields();
    void acceptsKeysInArbitraryOrder();
    void supportsQuotedValues();
    void supportsEscapedQuotedValues();

    void appliesSeverityAliases();
    void appliesTimestampRules();

    void mapsExplicitCustomFields();
    void preservesUnmappedFields();
    void unmappedFieldPreservationCanBeDisabled();

    void malformedAssignmentProducesDiagnostic();
    void unterminatedQuotedValueProducesDiagnostic();
    void duplicateKeyUsesLastValueAndWarns();

    void blankLinesAreIgnored();
    void preservesPhysicalLineNumbers();
    void importFileHonorsRecordLimit();
    void importFileReportsOpenFailure();
    void importFileReportsProgress();
    void importFileCanBeCancelled();
};

namespace
{
ImportProfile basicKeyValueProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Application logfmt"
            );

    profile.importerId =
        QStringLiteral(
            "key-value"
            );

    profile.canonicalFields.timestampPath =
        QStringLiteral("timestamp");

    profile.canonicalFields.severityPath =
        QStringLiteral("level");

    profile.canonicalFields.subsystemPath =
        QStringLiteral("subsystem");

    profile.canonicalFields.eventCodePath =
        QStringLiteral("event");

    profile.canonicalFields.entityIdPath =
        QStringLiteral("entity");

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    return profile;
}
}

void KeyValueTextImporterTests::
    importerHasStableId()
{
    KeyValueTextImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral(
            "key-value"
            )
        );
}

void KeyValueTextImporterTests::
    importerHasDisplayName()
{
    KeyValueTextImporter importer;

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "Key-Value / logfmt"
            )
        );
}

void KeyValueTextImporterTests::
    mapsCanonicalFields()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const QString rawSource =
        QStringLiteral(
            "timestamp=2026-08-12T08:00:14.125Z "
            "level=WARN "
            "subsystem=Orders "
            "event=SUPPLIER_SLOW "
            "entity=ORD-1842 "
            "message=Delayed"
            );

    const ImportResult result =
        importer.importLines(
            {
                rawSource
            },
            QStringLiteral(
                "samples/application.log"
                )
            );

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

    QVERIFY(
        result.diagnostics.isEmpty()
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 8, 12)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(8, 0, 14, 125)
        );

    QVERIFY(
        record.severity.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral("Orders")
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral(
            "SUPPLIER_SLOW"
            )
        );

    QCOMPARE(
        record.entityId.value(),
        QStringLiteral(
            "ORD-1842"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Delayed"
            )
        );

    QCOMPARE(
        record.rawSource,
        rawSource
        );

    QCOMPARE(
        record.source.sourcePath,
        QStringLiteral(
            "samples/application.log"
            )
        );

    QCOMPARE(
        record.source.sourceName,
        QStringLiteral(
            "application.log"
            )
        );

    QCOMPARE(
        record.source.recordNumber,
        qint64(1)
        );

    QVERIFY(
        !record.recordId.isEmpty()
        );

    QCOMPARE(
        record.recordId.size(),
        64
        );
}

void KeyValueTextImporterTests::
    acceptsKeysInArbitraryOrder()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "message=Completed "
                    "entity=ORD-2001 "
                    "subsystem=Orders "
                    "level=INFO "
                    "timestamp=2026-08-12T08:01:00Z "
                    "event=ORDER_COMPLETE"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral("Orders")
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral(
            "ORDER_COMPLETE"
            )
        );

    QCOMPARE(
        record.entityId.value(),
        QStringLiteral(
            "ORD-2001"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Completed"
            )
        );
}

void KeyValueTextImporterTests::
    supportsQuotedValues()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "timestamp=2026-08-12T08:02:00Z "
                    "level=INFO "
                    "subsystem=Orders "
                    "message=\"Order accepted for processing\""
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "Order accepted for processing"
            )
        );
}

void KeyValueTextImporterTests::
    supportsEscapedQuotedValues()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const QString rawSource =
        QStringLiteral(
            R"(timestamp=2026-08-12T08:03:00Z level=INFO subsystem=Worker message="Retry \"accepted\" at C:\\temp")"
            );

    const ImportResult result =
        importer.importLines(
            {
                rawSource
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            R"(Retry "accepted" at C:\temp)"
            )
        );
}

void KeyValueTextImporterTests::
    appliesSeverityAliases()
{
    ImportProfile profile =
        basicKeyValueProfile();

    profile.severityAliases.insert(
        QStringLiteral("WRN"),
        RecordSeverity::Warning
        );

    KeyValueTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=WRN message=Slow"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.records.first()
            .severity.has_value()
        );

    QCOMPARE(
        result.records.first()
            .severity.value(),
        RecordSeverity::Warning
        );
}

void KeyValueTextImporterTests::
    appliesTimestampRules()
{
    ImportProfile profile =
        basicKeyValueProfile();

    profile.timestampRules.clear();

    TimestampRule rule;

    rule.type =
        TimestampRuleType::QtFormat;

    rule.format =
        QStringLiteral(
            "yyyy-MM-dd HH:mm:ss.zzz"
            );

    profile.timestampRules.append(
        rule
        );

    KeyValueTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "timestamp=\"2026-08-12 08:04:12.345\" "
                    "level=INFO "
                    "message=Started"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const auto timestamp =
        result.records.first()
            .timestamp;

    QVERIFY(
        timestamp.has_value()
        );

    QCOMPARE(
        timestamp->date(),
        QDate(2026, 8, 12)
        );

    QCOMPARE(
        timestamp->time(),
        QTime(8, 4, 12, 345)
        );
}

void KeyValueTextImporterTests::
    mapsExplicitCustomFields()
{
    ImportProfile profile =
        basicKeyValueProfile();

    profile.customFields.append({
        QStringLiteral(
            "Request ID"
            ),
        QStringLiteral(
            "requestId"
            )
    });

    KeyValueTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO "
                    "message=Accepted "
                    "requestId=REQ-4002"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral(
                    "Request ID"
                    )
                )
            .toString(),
        QStringLiteral(
            "REQ-4002"
            )
        );
}

void KeyValueTextImporterTests::
    preservesUnmappedFields()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO "
                    "message=Accepted "
                    "requestId=REQ-4003 "
                    "host=api-02"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const auto &attributes =
        result.records.first()
            .customAttributes;

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "requestId"
                          )
                      )
            .toString(),
        QStringLiteral(
            "REQ-4003"
            )
        );

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "host"
                          )
                      )
            .toString(),
        QStringLiteral(
            "api-02"
            )
        );
}

void KeyValueTextImporterTests::
    unmappedFieldPreservationCanBeDisabled()
{
    ImportProfile profile =
        basicKeyValueProfile();

    profile.preserveUnmappedFields =
        false;

    KeyValueTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO "
                    "message=Accepted "
                    "requestId=REQ-4004"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.records.first()
            .customAttributes
            .isEmpty()
        );
}

void KeyValueTextImporterTests::
    malformedAssignmentProducesDiagnostic()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level INFO message=Broken"
                    )
            }
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "KEY_VALUE_RECORD_MALFORMED"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(
        result.diagnostics.first()
            .source.has_value()
        );
}

void KeyValueTextImporterTests::
    unterminatedQuotedValueProducesDiagnostic()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO "
                    "message=\"This never closes"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        0
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "KEY_VALUE_RECORD_MALFORMED"
            )
        );

    QVERIFY(
        result.diagnostics.first()
            .message
            .contains(
                QStringLiteral(
                    "not terminated"
                    )
                )
        );
}

void KeyValueTextImporterTests::
    duplicateKeyUsesLastValueAndWarns()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO "
                    "level=ERROR "
                    "message=Failure"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.records.first()
            .severity.has_value()
        );

    QCOMPARE(
        result.records.first()
            .severity.value(),
        RecordSeverity::Error
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "KEY_VALUE_DUPLICATE_KEY"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Warning
        );
}

void KeyValueTextImporterTests::
    blankLinesAreIgnored()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QString(),
                QStringLiteral("   "),
                QStringLiteral(
                    "level=INFO message=Valid"
                    )
            }
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.diagnostics.isEmpty()
        );
}

void KeyValueTextImporterTests::
    preservesPhysicalLineNumbers()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "level=INFO message=First"
                    ),
                QString(),
                QStringLiteral(
                    "level=WARN message=Third"
                    )
            },
            QStringLiteral(
                "samples/logfmt.log"
                )
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
}

void KeyValueTextImporterTests::
    importFileHonorsRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(
        &file
        );

    stream
        << "level=INFO message=First\n"
        << "level=WARN message=Second\n"
        << "level=ERROR message=Third\n";

    stream.flush();

    const QString filePath =
        file.fileName();

    file.close();

    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importFile(
            filePath,
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

void KeyValueTextImporterTests::
    importFileReportsOpenFailure()
{
    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importFile(
            QStringLiteral(
                "this-file-does-not-exist.log"
                )
            );

    QCOMPARE(
        result.records.size(),
        0
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "FILE_OPEN_FAILED"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Error
        );
}

void KeyValueTextImporterTests::
    importFileReportsProgress()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "level=INFO message=One\n"
        << "level=WARN message=Two\n";

    stream.flush();

    const QString filePath =
        file.fileName();

    file.close();

    QVector<ImportProgress> progress;

    ImportExecutionContext context;

    context.reportProgress =
        [&progress](
            const ImportProgress &value
            ) {
            progress.append(
                value
                );
        };

    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importFile(
            filePath,
            ILogImporter::UnlimitedRecordLimit,
            context
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
        !progress.isEmpty()
        );

    QCOMPARE(
        progress.first().bytesProcessed,
        qint64(0)
        );

    QCOMPARE(
        progress.last().bytesProcessed,
        QFileInfo(filePath).size()
        );

    QCOMPARE(
        progress.last().processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        progress.last().totalBytes,
        QFileInfo(filePath).size()
        );
}

void KeyValueTextImporterTests::
    importFileCanBeCancelled()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "level=INFO message=One\n"
        << "level=WARN message=Two\n"
        << "level=ERROR message=Three\n";

    stream.flush();

    const QString filePath =
        file.fileName();

    file.close();

    int cancellationChecks = 0;

    ImportExecutionContext context;

    context.isCancellationRequested =
        [&cancellationChecks]() {
            ++cancellationChecks;

            /*
             * Allow the first record to be read,
             * then cancel before the second.
             */
            return cancellationChecks > 1;
        };

    KeyValueTextImporter importer(
        basicKeyValueProfile()
        );

    const ImportResult result =
        importer.importFile(
            filePath,
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QVERIFY(
        result.cancelled
        );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        !result.sourceTruncated
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral("One")
        );
}

QTEST_APPLESS_MAIN(
    KeyValueTextImporterTests
    )

#include "KeyValueTextImporterTests.moc"