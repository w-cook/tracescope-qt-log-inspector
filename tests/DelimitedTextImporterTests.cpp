#include <QtTest/QtTest>

#include <optional>
#include <utility>

#include <QFileInfo>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>

#include "../src/importing/DelimitedTextImporter.h"

namespace
{
DelimitedTextImporter createCsvImporter(
    ImportProfile profile = {}
    )
{
    return DelimitedTextImporter(
        QStringLiteral("csv"),
        QStringLiteral("CSV"),
        QLatin1Char(','),
        std::move(profile)
        );
}

DelimitedTextImporter createTsvImporter(
    ImportProfile profile = {}
    )
{
    return DelimitedTextImporter(
        QStringLiteral("tsv"),
        QStringLiteral("TSV"),
        QLatin1Char('\t'),
        std::move(profile)
        );
}

bool writeTemporaryContent(
    QTemporaryFile &file,
    const QString &content
    )
{
    if (!file.open()) {
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    stream.flush();

    file.close();

    return true;
}

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
}

class DelimitedTextImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void csvImporterHasStableIdentity();
    void tsvImporterHasStableIdentity();

    void csvImportsCanonicalFields();
    void tsvImportsCanonicalFields();

    void csvParsesQuotedDelimiter();
    void csvParsesEscapedQuotes();

    void csvPreservesUnmappedColumns();
    void profileMapsExplicitCustomField();
    void profileCanDisableUnmappedPreservation();

    void csvReportsColumnCountMismatch();
    void csvReportsMalformedQuotedRecord();
    void csvRejectsDuplicateHeaders();
    void csvRejectsEmptyHeaders();
    void csvReportsMissingHeader();

    void csvReportsInvalidCanonicalValues();

    void importFilePreservesSourceMetadata();
    void importFileHonorsRecordLimit();
    void importFileReportsOpenFailure();
    void importFileReportsProgress();
    void importFileCanBeCancelled();
};

void DelimitedTextImporterTests::csvImporterHasStableIdentity()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    QCOMPARE(
        importer.id(),
        QStringLiteral("csv")
        );

    QCOMPARE(
        importer.displayName(),
        QStringLiteral("CSV")
        );
}

void DelimitedTextImporterTests::tsvImporterHasStableIdentity()
{
    const DelimitedTextImporter importer =
        createTsvImporter();

    QCOMPARE(
        importer.id(),
        QStringLiteral("tsv")
        );

    QCOMPARE(
        importer.displayName(),
        QStringLiteral("TSV")
        );
}

void DelimitedTextImporterTests::csvImportsCanonicalFields()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "timestamp,level,subsystem,eventCode,entityId,message"
                    ),
                QStringLiteral(
                    "2026-08-11T07:30:00Z,WARN,Tracking,"
                    "TRACK_LOST,TRK-402,Track lost"
                    )
            },
            QStringLiteral(
                "samples/session.csv"
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

    QVERIFY(result.diagnostics.isEmpty());

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.timestamp.has_value());

    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 8, 11)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(7, 30)
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
        record.entityId,
        std::optional<QString>(
            QStringLiteral("TRK-402")
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral("Track lost")
            )
        );

    QCOMPARE(
        record.rawSource,
        QStringLiteral(
            "2026-08-11T07:30:00Z,WARN,Tracking,"
            "TRACK_LOST,TRK-402,Track lost"
            )
        );

    QCOMPARE(
        record.source.sourcePath,
        QStringLiteral(
            "samples/session.csv"
            )
        );

    QCOMPARE(
        record.source.sourceName,
        QStringLiteral("session.csv")
        );

    QCOMPARE(
        record.source.recordNumber,
        qint64(2)
        );

    QVERIFY(!record.recordId.isEmpty());
}

void DelimitedTextImporterTests::tsvImportsCanonicalFields()
{
    const DelimitedTextImporter importer =
        createTsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp\tlevel\tsubsystem\tmessage"
                ),
            QStringLiteral(
                "2026-08-11T07:45:00Z\tERROR\t"
                "Database\tConnection failed"
                )
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(record.severity.has_value());

    QCOMPARE(
        *record.severity,
        RecordSeverity::Error
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral("Database")
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral(
                "Connection failed"
                )
            )
        );
}

void DelimitedTextImporterTests::csvParsesQuotedDelimiter()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "\"Request completed, with warnings\""
                )
        });

    QCOMPARE(result.records.size(), 1);

    QCOMPARE(
        result.records.first().message,
        std::optional<QString>(
            QStringLiteral(
                "Request completed, with warnings"
                )
            )
        );
}

void DelimitedTextImporterTests::csvParsesEscapedQuotes()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "\"Service reported \"\"ready\"\" state\""
                )
        });

    QCOMPARE(result.records.size(), 1);

    QCOMPARE(
        result.records.first().message,
        std::optional<QString>(
            QStringLiteral(
                "Service reported \"ready\" state"
                )
            )
        );
}

void DelimitedTextImporterTests::csvPreservesUnmappedColumns()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message,"
                "durationMs,requestId"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "Request completed,184,REQ-204"
                )
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.size(),
        2
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("durationMs")
                )
            .toString(),
        QStringLiteral("184")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("requestId")
                )
            .toString(),
        QStringLiteral("REQ-204")
        );
}

void DelimitedTextImporterTests::profileMapsExplicitCustomField()
{
    ImportProfile profile;

    profile.customFields.append({
        QStringLiteral("Duration"),
        QStringLiteral("duration_ms")
    });

    const DelimitedTextImporter importer =
        createCsvImporter(profile);

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message,duration_ms"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "Request completed,245"
                )
        });

    QCOMPARE(result.records.size(), 1);

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        record.customAttributes.contains(
            QStringLiteral("Duration")
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("Duration")
                )
            .toString(),
        QStringLiteral("245")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("duration_ms")
            )
        );
}

void DelimitedTextImporterTests::profileCanDisableUnmappedPreservation()
{
    ImportProfile profile;

    profile.preserveUnmappedFields = false;

    const DelimitedTextImporter importer =
        createCsvImporter(profile);

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message,requestId"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "Request completed,REQ-204"
                )
        });

    QCOMPARE(result.records.size(), 1);

    QVERIFY(
        result.records
            .first()
            .customAttributes
            .isEmpty()
        );
}

void DelimitedTextImporterTests::csvReportsColumnCountMismatch()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO"
                )
        });

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

    const ImportDiagnostic *diagnostic =
        findDiagnostic(
            result,
            QStringLiteral(
                "DELIMITED_COLUMN_COUNT_MISMATCH"
                )
            );

    QVERIFY(diagnostic != nullptr);

    QCOMPARE(
        diagnostic->severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(diagnostic->source.has_value());

    QCOMPARE(
        diagnostic->source->recordNumber,
        qint64(2)
        );
}

void DelimitedTextImporterTests::csvReportsMalformedQuotedRecord()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "\"Unterminated message"
                )
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(0)
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "MALFORMED_DELIMITED_RECORD"
                )
            )
        != nullptr
        );
}

void DelimitedTextImporterTests::csvRejectsDuplicateHeaders()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message,Message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "First,Second"
                )
        });

    QVERIFY(result.records.isEmpty());

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "DUPLICATE_DELIMITED_HEADER"
                )
            )
        != nullptr
        );
}

void DelimitedTextImporterTests::csvRejectsEmptyHeaders()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,,message"
                ),
            QStringLiteral(
                "2026-08-11T08:00:00Z,INFO,"
                "Request completed"
                )
        });

    QVERIFY(result.records.isEmpty());

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "EMPTY_DELIMITED_HEADER"
                )
            )
        != nullptr
        );
}

void DelimitedTextImporterTests::csvReportsMissingHeader()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QString(),
            QStringLiteral("   ")
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(0)
        );

    QVERIFY(result.records.isEmpty());

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "DELIMITED_HEADER_REQUIRED"
                )
            )
        != nullptr
        );
}

void DelimitedTextImporterTests::csvReportsInvalidCanonicalValues()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "timestamp,level,message"
                ),
            QStringLiteral(
                "not-a-time,UNKNOWN,"
                "Invalid canonical values"
                )
        });

    QCOMPARE(result.records.size(), 1);

    QVERIFY(
        !result.records
             .first()
             .timestamp
             .has_value()
        );

    QVERIFY(
        !result.records
             .first()
             .severity
             .has_value()
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "INVALID_TIMESTAMP"
                )
            )
        != nullptr
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "UNMAPPED_SEVERITY"
                )
            )
        != nullptr
        );
}

void DelimitedTextImporterTests::importFilePreservesSourceMetadata()
{
    QTemporaryFile file;

    QVERIFY(
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp,level,message\n"
                "2026-08-11T08:00:00Z,INFO,"
                "First record\n"
                "2026-08-11T08:01:00Z,WARN,"
                "Second record\n"
                )
            )
        );

    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importFile(
            file.fileName()
            );

    QCOMPARE(result.records.size(), 2);

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(3)
        );

    QCOMPARE(
        result.records.at(0)
            .source.sourcePath,
        file.fileName()
        );

    QCOMPARE(
        result.records.at(0)
            .source.sourceName,
        QFileInfo(file.fileName())
            .fileName()
        );
}

void DelimitedTextImporterTests::importFileHonorsRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp,level,message\n"
                "2026-08-11T08:00:00Z,INFO,One\n"
                "2026-08-11T08:01:00Z,INFO,Two\n"
                "2026-08-11T08:02:00Z,INFO,Three\n"
                )
            )
        );

    const DelimitedTextImporter importer =
        createCsvImporter();

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
        result.importedRecordCount(),
        qint64(2)
        );

    QVERIFY(result.sourceTruncated);

    QCOMPARE(
        result.records.at(0).message,
        std::optional<QString>(
            QStringLiteral("One")
            )
        );

    QCOMPARE(
        result.records.at(1).message,
        std::optional<QString>(
            QStringLiteral("Two")
            )
        );
}

void DelimitedTextImporterTests::importFileReportsOpenFailure()
{
    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importFile(
            QStringLiteral(
                "this-file-does-not-exist.csv"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(0)
        );

    QVERIFY(result.records.isEmpty());

    const ImportDiagnostic *diagnostic =
        findDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                )
            );

    QVERIFY(diagnostic != nullptr);

    QCOMPARE(
        diagnostic->severity,
        ImportDiagnosticSeverity::Error
        );
}

void DelimitedTextImporterTests::
    importFileReportsProgress()
{
    QTemporaryFile file;

    QVERIFY(
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp,level,message\n"
                "2026-08-11T08:00:00Z,INFO,One\n"
                "2026-08-11T08:01:00Z,WARN,Two\n"
                )
            )
        );

    QVector<ImportProgress> progress;

    ImportExecutionContext context;

    context.reportProgress =
        [&progress](
            const ImportProgress &value
            ) {
            progress.append(value);
        };

    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importFile(
            file.fileName(),
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QVERIFY(!progress.isEmpty());

    QCOMPARE(
        progress.first().bytesProcessed,
        qint64(0)
        );

    QCOMPARE(
        progress.last().bytesProcessed,
        QFileInfo(file.fileName()).size()
        );

    QCOMPARE(
        progress.last().processedRecordCount,
        qint64(2)
        );
}

void DelimitedTextImporterTests::
    importFileCanBeCancelled()
{
    QTemporaryFile file;

    QVERIFY(
        writeTemporaryContent(
            file,
            QStringLiteral(
                "timestamp,level,message\n"
                "2026-08-11T08:00:00Z,INFO,One\n"
                "2026-08-11T08:01:00Z,INFO,Two\n"
                "2026-08-11T08:02:00Z,INFO,Three\n"
                )
            )
        );

    int cancellationChecks = 0;

    ImportExecutionContext context;

    context.isCancellationRequested =
        [&cancellationChecks]() {
            ++cancellationChecks;

            /*
             * First check: header.
             * Second check: first data record.
             * Cancel before the second data record.
             */
            return cancellationChecks > 2;
        };

    const DelimitedTextImporter importer =
        createCsvImporter();

    const ImportResult result =
        importer.importFile(
            file.fileName(),
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

QTEST_MAIN(DelimitedTextImporterTests)

#include "DelimitedTextImporterTests.moc"