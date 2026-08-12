#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/IisW3cImporter.h"

class IisW3cImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void reportsImporterIdentity();

    void importsTypicalIisRecord();
    void usesFieldsDirectiveOrder();
    void supportsDifferentFieldSets();
    void handlesFieldsDirectiveChanges();

    void combinesDateAndTimeAsUtc();
    void invalidTimestampProducesDiagnostic();
    void buildsRequestMessageWithQuery();

    void treatsHyphenAsMissing();
    void skipsRecordsBeforeFieldsDirective();
    void skipsFieldCountMismatch();
    void preservesCustomW3cFields();

    void honorsProcessedRecordLimit();
    void reportsFileOpenFailure();
};

namespace
{
ImportProfile basicIisProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "IIS W3C Extended Log"
            );

    profile.importerId =
        QStringLiteral(
            "iis-w3c"
            );

    return profile;
}

IisW3cImporter createImporter()
{
    return IisW3cImporter(
        basicIisProfile()
        );
}
}

void IisW3cImporterTests::
    reportsImporterIdentity()
{
    IisW3cImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral(
            "iis-w3c"
            )
        );

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "IIS W3C Extended Log"
            )
        );
}

void IisW3cImporterTests::
    importsTypicalIisRecord()
{
    IisW3cImporter importer =
        createImporter();

    const QString rawSource =
        QStringLiteral(
            "2026-08-12 15:04:03 "
            "192.0.2.10 "
            "GET "
            "/api/orders/5812 "
            "- "
            "443 "
            "- "
            "198.51.100.24 "
            "Mozilla/5.0 "
            "- "
            "200 "
            "0 "
            "0 "
            "42"
            );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Software: Microsoft Internet "
                    "Information Services 10.0"
                    ),
                QStringLiteral(
                    "#Version: 1.0"
                    ),
                QStringLiteral(
                    "#Date: 2026-08-12 15:04:03"
                    ),
                QStringLiteral(
                    "#Fields: date time s-ip "
                    "cs-method cs-uri-stem "
                    "cs-uri-query s-port "
                    "cs-username c-ip "
                    "cs(User-Agent) cs(Referer) "
                    "sc-status sc-substatus "
                    "sc-win32-status time-taken"
                    ),
                rawSource
            },
            QStringLiteral(
                "samples/iis-w3c.log"
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
            15,
            4,
            3
            )
        );

    QCOMPARE(
        record.timestamp->offsetFromUtc(),
        0
        );

    QVERIFY(
        !record.severity.has_value()
        );

    QVERIFY(
        !record.subsystem.has_value()
        );

    QVERIFY(
        !record.eventCode.has_value()
        );

    QVERIFY(
        !record.entityId.has_value()
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "GET /api/orders/5812"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "s-ip"
                    )
                )
            .toString(),
        QStringLiteral(
            "192.0.2.10"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "c-ip"
                    )
                )
            .toString(),
        QStringLiteral(
            "198.51.100.24"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "sc-status"
                    )
                )
            .toString(),
        QStringLiteral(
            "200"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "time-taken"
                    )
                )
            .toString(),
        QStringLiteral(
            "42"
            )
        );

    QCOMPARE(
        record.rawSource,
        rawSource
        );

    QCOMPARE(
        record.source.sourcePath,
        QStringLiteral(
            "samples/iis-w3c.log"
            )
        );

    QCOMPARE(
        record.source.sourceName,
        QStringLiteral(
            "iis-w3c.log"
            )
        );

    QCOMPARE(
        record.source.recordNumber,
        qint64(5)
        );

    QVERIFY(
        !record.recordId.isEmpty()
        );
}

void IisW3cImporterTests::
    usesFieldsDirectiveOrder()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: time date sc-status "
                    "cs-uri-stem cs-method c-ip"
                    ),
                QStringLiteral(
                    "15:10:22 "
                    "2026-08-12 "
                    "201 "
                    "/api/orders "
                    "POST "
                    "198.51.100.30"
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
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(
            15,
            10,
            22
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "POST /api/orders"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "sc-status"
                    )
                )
            .toString(),
        QStringLiteral(
            "201"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "c-ip"
                    )
                )
            .toString(),
        QStringLiteral(
            "198.51.100.30"
            )
        );
}

void IisW3cImporterTests::
    supportsDifferentFieldSets()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem sc-status"
                    ),
                QStringLiteral(
                    "2026-08-12 15:12:00 "
                    "GET /health 200"
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
        record.message.value(),
        QStringLiteral(
            "GET /health"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "sc-status"
                    )
                )
            .toString(),
        QStringLiteral(
            "200"
            )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "c-ip"
                     )
                 )
        );
}

void IisW3cImporterTests::
    handlesFieldsDirectiveChanges()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem sc-status"
                    ),
                QStringLiteral(
                    "2026-08-12 15:20:00 "
                    "GET /first 200"
                    ),
                QStringLiteral(
                    "#Fields: date time c-ip "
                    "sc-status cs-uri-stem cs-method"
                    ),
                QStringLiteral(
                    "2026-08-12 15:21:00 "
                    "203.0.113.44 "
                    "404 "
                    "/second "
                    "GET"
                    )
            }
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
        QStringLiteral(
            "GET /first"
            )
        );

    QCOMPARE(
        result.records.at(1)
            .message.value(),
        QStringLiteral(
            "GET /second"
            )
        );

    QCOMPARE(
        result.records.at(1)
            .customAttributes
            .value(
                QStringLiteral(
                    "c-ip"
                    )
                )
            .toString(),
        QStringLiteral(
            "203.0.113.44"
            )
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(4)
        );
}

void IisW3cImporterTests::
    combinesDateAndTimeAsUtc()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time cs-method "
                    "cs-uri-stem"
                    ),
                QStringLiteral(
                    "2026-12-31 23:59:58 "
                    "GET /year-end"
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
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.timestamp->date(),
        QDate(
            2026,
            12,
            31
            )
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(
            23,
            59,
            58
            )
        );

    QCOMPARE(
        record.timestamp->offsetFromUtc(),
        0
        );
}

void IisW3cImporterTests::
    invalidTimestampProducesDiagnostic()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem"
                    ),
                QStringLiteral(
                    "2026-99-99 25:61:61 "
                    "GET /invalid-time"
                    )
            }
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        !result.records.first()
             .timestamp.has_value()
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "INVALID_TIMESTAMP"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Warning
        );
}

void IisW3cImporterTests::
    buildsRequestMessageWithQuery()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem "
                    "cs-uri-query"
                    ),
                QStringLiteral(
                    "2026-08-12 15:30:00 "
                    "GET /search q=widget&page=2"
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
            "GET /search?q=widget&page=2"
            )
        );
}

void IisW3cImporterTests::
    treatsHyphenAsMissing()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem "
                    "cs-uri-query cs-username "
                    "cs(Referer) sc-status"
                    ),
                QStringLiteral(
                    "2026-08-12 15:40:00 "
                    "GET /orders "
                    "- - - 200"
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
        record.message.value(),
        QStringLiteral(
            "GET /orders"
            )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "cs-uri-query"
                     )
                 )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "cs-username"
                     )
                 )
        );

    QVERIFY(
        !record.customAttributes
             .contains(
                 QStringLiteral(
                     "cs(Referer)"
                     )
                 )
        );
}

void IisW3cImporterTests::
    skipsRecordsBeforeFieldsDirective()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "2026-08-12 15:00:00 "
                    "GET /unmapped"
                    ),
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem"
                    ),
                QStringLiteral(
                    "2026-08-12 15:01:00 "
                    "GET /mapped"
                    )
            }
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "GET /mapped"
            )
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first()
            .code,
        QStringLiteral(
            "IIS_W3C_FIELDS_REQUIRED"
            )
        );
}

void IisW3cImporterTests::
    skipsFieldCountMismatch()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem sc-status"
                    ),
                QStringLiteral(
                    "2026-08-12 15:50:00 "
                    "GET /missing-status"
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
            "IIS_W3C_FIELD_COUNT_MISMATCH"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .severity,
        ImportDiagnosticSeverity::Error
        );
}

void IisW3cImporterTests::
    preservesCustomW3cFields()
{
    IisW3cImporter importer =
        createImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "#Fields: date time "
                    "cs-method cs-uri-stem "
                    "x-request-id x-region"
                    ),
                QStringLiteral(
                    "2026-08-12 16:00:00 "
                    "GET /orders "
                    "REQ-9201 us-east"
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
                          "x-request-id"
                          )
                      )
            .toString(),
        QStringLiteral(
            "REQ-9201"
            )
        );

    QCOMPARE(
        attributes.value(
                      QStringLiteral(
                          "x-region"
                          )
                      )
            .toString(),
        QStringLiteral(
            "us-east"
            )
        );
}

void IisW3cImporterTests::
    honorsProcessedRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "#Software: Microsoft Internet "
           "Information Services 10.0\n"
        << "#Version: 1.0\n"
        << "#Fields: date time cs-method "
           "cs-uri-stem sc-status\n"
        << "2026-08-12 16:10:00 "
           "GET /first 200\n"
        << "2026-08-12 16:11:00 "
           "GET /second 200\n"
        << "2026-08-12 16:12:00 "
           "GET /third 200\n";

    stream.flush();

    IisW3cImporter importer =
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

    QCOMPARE(
        result.records.at(0)
            .message.value(),
        QStringLiteral(
            "GET /first"
            )
        );

    QCOMPARE(
        result.records.at(1)
            .message.value(),
        QStringLiteral(
            "GET /second"
            )
        );
}

void IisW3cImporterTests::
    reportsFileOpenFailure()
{
    IisW3cImporter importer =
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

QTEST_MAIN(IisW3cImporterTests)

#include "IisW3cImporterTests.moc"