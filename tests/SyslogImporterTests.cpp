#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/SyslogImporter.h"

class SyslogImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void importerHasStableId();
    void importerHasDisplayName();

    void importsRfc5424Record();
    void mapsRfc5424PrioritySeverity();
    void preservesRfc5424StructuredData();
    void handlesRfc5424NilValues();

    void importsRfc3164Record();
    void parsesRfc3164TagAndProcessId();
    void infersRfc3164YearNearReferenceDate();
    void reportsRfc3164YearInference();

    void malformedPriorityProducesDiagnostic();
    void unsupportedRecordProducesDiagnostic();
    void blankLinesAreIgnored();
    void preservesPhysicalLineNumbers();

    void importFileHonorsRecordLimit();
    void importFileReportsOpenFailure();
};

namespace
{
ImportProfile basicSyslogProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Syslog"
            );

    profile.importerId =
        QStringLiteral(
            "syslog"
            );

    profile.canonicalFields.timestampPath =
        QStringLiteral(
            "timestamp"
            );

    profile.canonicalFields.severityPath =
        QStringLiteral(
            "level"
            );

    profile.canonicalFields.subsystemPath =
        QStringLiteral(
            "subsystem"
            );

    profile.canonicalFields.eventCodePath =
        QStringLiteral(
            "eventCode"
            );

    profile.canonicalFields.entityIdPath =
        QStringLiteral(
            "entityId"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "message"
            );

    return profile;
}

SyslogImporter createImporter()
{
    return SyslogImporter(
        basicSyslogProfile(),
        QDate(
            2026,
            8,
            12
            )
        );
}
}

void SyslogImporterTests::
    importerHasStableId()
{
    SyslogImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral(
            "syslog"
            )
        );
}

void SyslogImporterTests::
    importerHasDisplayName()
{
    SyslogImporter importer;

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "Syslog (RFC 5424 / RFC 3164)"
            )
        );
}

void SyslogImporterTests::
    importsRfc5424Record()
{
    SyslogImporter importer =
        createImporter();

    const QString rawSource =
        QStringLiteral(
            "<165>1 "
            "2026-08-12T08:20:18.427Z "
            "api-01 "
            "orders-service "
            "4242 "
            "ORDER_RECEIVED "
            "- "
            "Order received for processing"
            );

    const ImportResult result =
        importer.importLines(
            {
                rawSource
            },
            QStringLiteral(
                "samples/syslog.log"
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
        QDate(
            2026,
            8,
            12
            )
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(
            8,
            20,
            18,
            427
            )
        );

    QVERIFY(
        record.severity.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral(
            "orders-service"
            )
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral(
            "ORDER_RECEIVED"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Order received for processing"
            )
        );

    QCOMPARE(
        record.rawSource,
        rawSource
        );

    QCOMPARE(
        record.source.sourcePath,
        QStringLiteral(
            "samples/syslog.log"
            )
        );

    QCOMPARE(
        record.source.sourceName,
        QStringLiteral(
            "syslog.log"
            )
        );

    QCOMPARE(
        record.source.recordNumber,
        qint64(1)
        );

    QVERIFY(
        !record.recordId.isEmpty()
        );
}

void SyslogImporterTests::
    mapsRfc5424PrioritySeverity()
{
    SyslogImporter importer =
        createImporter();

    const QStringList lines {
        QStringLiteral(
            "<16>1 2026-08-12T08:00:00Z "
            "host app - - - Emergency"
            ),
        QStringLiteral(
            "<17>1 2026-08-12T08:00:01Z "
            "host app - - - Alert"
            ),
        QStringLiteral(
            "<18>1 2026-08-12T08:00:02Z "
            "host app - - - Critical"
            ),
        QStringLiteral(
            "<19>1 2026-08-12T08:00:03Z "
            "host app - - - Error"
            ),
        QStringLiteral(
            "<20>1 2026-08-12T08:00:04Z "
            "host app - - - Warning"
            ),
        QStringLiteral(
            "<21>1 2026-08-12T08:00:05Z "
            "host app - - - Notice"
            ),
        QStringLiteral(
            "<22>1 2026-08-12T08:00:06Z "
            "host app - - - Informational"
            ),
        QStringLiteral(
            "<23>1 2026-08-12T08:00:07Z "
            "host app - - - Debug"
            )
    };

    const ImportResult result =
        importer.importLines(
            lines
            );

    QCOMPARE(
        result.records.size(),
        8
        );

    QCOMPARE(
        result.records.at(0)
            .severity.value(),
        RecordSeverity::Critical
        );

    QCOMPARE(
        result.records.at(1)
            .severity.value(),
        RecordSeverity::Critical
        );

    QCOMPARE(
        result.records.at(2)
            .severity.value(),
        RecordSeverity::Critical
        );

    QCOMPARE(
        result.records.at(3)
            .severity.value(),
        RecordSeverity::Error
        );

    QCOMPARE(
        result.records.at(4)
            .severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        result.records.at(5)
            .severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        result.records.at(6)
            .severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        result.records.at(7)
            .severity.value(),
        RecordSeverity::Debug
        );
}

void SyslogImporterTests::
    preservesRfc5424StructuredData()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<165>1 "
                    "2026-08-12T08:21:04.812Z "
                    "worker-02 "
                    "supplier-gateway "
                    "8301 "
                    "SUPPLIER_DELAY "
                    "[request@32473 "
                    "requestId=\"REQ-9201\" "
                    "supplier=\"Northwind\"] "
                    "Supplier response exceeded latency"
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
                          "structuredData"
                          )
                      )
            .toString(),
        QStringLiteral(
            "[request@32473 "
            "requestId=\"REQ-9201\" "
            "supplier=\"Northwind\"]"
            )
        );

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "hostname"
                          )
                      )
            .toString(),
        QStringLiteral(
            "worker-02"
            )
        );

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "processId"
                          )
                      )
            .toString(),
        QStringLiteral(
            "8301"
            )
        );

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "syslogFormat"
                          )
                      )
            .toString(),
        QStringLiteral(
            "RFC 5424"
            )
        );
}

void SyslogImporterTests::
    handlesRfc5424NilValues()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<14>1 "
                    "2026-08-12T08:22:00Z "
                    "- - - - - "
                    "Service available"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        !record.subsystem.has_value()
        );

    QVERIFY(
        !record.eventCode.has_value()
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Service available"
            )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "hostname"
                     )
                 )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "processId"
                     )
                 )
        );
}

void SyslogImporterTests::
    importsRfc3164Record()
{
    SyslogImporter importer =
        createImporter();

    const QString rawSource =
        QStringLiteral(
            "<34>Aug 12 08:24:16 "
            "worker-04 "
            "telemetry: "
            "Queue unavailable"
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

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.timestamp->date(),
        QDate(
            2026,
            8,
            12
            )
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(
            8,
            24,
            16
            )
        );

    QVERIFY(
        record.severity.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Critical
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral(
            "telemetry"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Queue unavailable"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "hostname"
                    )
                )
            .toString(),
        QStringLiteral(
            "worker-04"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "syslogFormat"
                    )
                )
            .toString(),
        QStringLiteral(
            "RFC 3164"
            )
        );
}

void SyslogImporterTests::
    parsesRfc3164TagAndProcessId()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<11>Aug 12 08:25:01 "
                    "db-01 "
                    "postgres[7214]: "
                    "connection timed out"
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
        QStringLiteral(
            "postgres"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "connection timed out"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "processId"
                    )
                )
            .toString(),
        QStringLiteral(
            "7214"
            )
        );
}

void SyslogImporterTests::
    infersRfc3164YearNearReferenceDate()
{
    SyslogImporter importer(
        basicSyslogProfile(),
        QDate(
            2026,
            1,
            2
            )
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<14>Dec 31 23:59:58 "
                    "gateway-01 "
                    "network: Previous-year event"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.records.first()
            .timestamp.has_value()
        );

    QCOMPARE(
        result.records.first()
            .timestamp->date(),
        QDate(
            2025,
            12,
            31
            )
        );
}

void SyslogImporterTests::
    reportsRfc3164YearInference()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<14>Aug 12 08:26:00 "
                    "api-01 "
                    "orders: Started"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "SYSLOG_LEGACY_TIMESTAMP_INFERRED"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Information
        );

    QCOMPARE(
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral(
                    "timestampOriginal"
                    )
                )
            .toString(),
        QStringLiteral(
            "Aug 12 08:26:00"
            )
        );

    QCOMPARE(
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral(
                    "timestampYearInferred"
                    )
                )
            .toBool(),
        true
        );
}

void SyslogImporterTests::
    malformedPriorityProducesDiagnostic()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<999>1 "
                    "2026-08-12T08:30:00Z "
                    "host app - - - Invalid PRI"
                    )
            }
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
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
            "SYSLOG_RECORD_MALFORMED"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Error
        );
}

void SyslogImporterTests::
    unsupportedRecordProducesDiagnostic()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<14>This is not valid syslog"
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
            "SYSLOG_RECORD_MALFORMED"
            )
        );
}

void SyslogImporterTests::
    blankLinesAreIgnored()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QString(),
                QStringLiteral(
                    "   "
                    ),
                QStringLiteral(
                    "<14>1 "
                    "2026-08-12T08:31:00Z "
                    "host app - - - Valid"
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
}

void SyslogImporterTests::
    preservesPhysicalLineNumbers()
{
    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "<14>1 "
                    "2026-08-12T08:31:00Z "
                    "host app - - - First"
                    ),
                QString(),
                QStringLiteral(
                    "<14>1 "
                    "2026-08-12T08:32:00Z "
                    "host app - - - Second"
                    )
            }
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

void SyslogImporterTests::
    importFileHonorsRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "<14>1 2026-08-12T08:00:00Z "
           "host app - - - First\n"
        << "<14>1 2026-08-12T08:01:00Z "
           "host app - - - Second\n"
        << "<14>1 2026-08-12T08:02:00Z "
           "host app - - - Third\n";

    stream.flush();

    SyslogImporter importer =
        createImporter();

    const ImportResult result =
        importer.importFile(
            file.fileName(),
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

void SyslogImporterTests::
    importFileReportsOpenFailure()
{
    SyslogImporter importer =
        createImporter();

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

QTEST_MAIN(SyslogImporterTests)

#include "SyslogImporterTests.moc"