#include <QtTest/QtTest>

#include <QFileInfo>
#include <QTemporaryFile>
#include <QTextStream>

#include "../src/importing/RegexTextImporter.h"

class RegexTextImporterTests : public QObject
{
    Q_OBJECT

private slots:
    void importerHasStableId();
    void importerHasDisplayName();

    void namedCapturesMapCanonicalFields();
    void unmatchedLinesProduceDiagnostic();
    void blankLinesAreIgnored();

    void severityAliasesAreApplied();
    void qtTimestampRulesAreApplied();

    void explicitCustomFieldsAreMapped();
    void unmappedCapturesArePreserved();
    void unmappedCapturePreservationCanBeDisabled();
    void optionalUnmatchedCaptureIsOmitted();

    void invalidPatternProducesDiagnostic();

    void importFilePreservesPhysicalLineNumbers();
    void importFileHonorsRecordLimit();
    void importFileReportsOpenFailure();
    void importFileReportsProgress();
    void importFileCanBeCancelled();
};

namespace
{
ImportProfile basicRegexProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Application Log"
            );

    profile.importerId =
        QStringLiteral(
            "regex-text"
            );

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+(?<message>.*)$)"
            );

    profile.canonicalFields.severityPath =
        QStringLiteral("severity");

    return profile;
}
}

void RegexTextImporterTests::
    importerHasStableId()
{
    RegexTextImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral(
            "regex-text"
            )
        );
}

void RegexTextImporterTests::
    importerHasDisplayName()
{
    RegexTextImporter importer;

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "Regex Plain Text"
            )
        );
}

void RegexTextImporterTests::
    namedCapturesMapCanonicalFields()
{
    RegexTextImporter importer(
        basicRegexProfile()
        );

    const QString rawSource =
        QStringLiteral(
            "2026-08-11T12:03:14.125Z "
            "[WARN] [Orders] "
            "Supplier response delayed"
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
        QDate(2026, 8, 11)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(12, 3, 14, 125)
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

    QVERIFY(
        !record.eventCode.has_value()
        );

    QVERIFY(
        !record.entityId.has_value()
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Supplier response delayed"
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

    QVERIFY(
        record.customAttributes.isEmpty()
        );
}

void RegexTextImporterTests::
    unmatchedLinesProduceDiagnostic()
{
    RegexTextImporter importer(
        basicRegexProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QStringLiteral(
                    "2026-08-11T12:03:14Z "
                    "[INFO] [Orders] Valid"
                    ),
                QStringLiteral(
                    "this line does not match"
                    )
            }
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

    const ImportDiagnostic &diagnostic =
        result.diagnostics.first();

    QCOMPARE(
        diagnostic.code,
        QStringLiteral(
            "REGEX_RECORD_NO_MATCH"
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
        diagnostic.source->recordNumber,
        qint64(2)
        );
}

void RegexTextImporterTests::
    blankLinesAreIgnored()
{
    RegexTextImporter importer(
        basicRegexProfile()
        );

    const ImportResult result =
        importer.importLines(
            {
                QString(),
                QStringLiteral(
                    "   "
                    ),
                QStringLiteral(
                    "2026-08-11T12:00:00Z "
                    "[INFO] [Orders] First"
                    ),
                QString(),
                QStringLiteral(
                    "2026-08-11T12:01:00Z "
                    "[ERROR] [Orders] Second"
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

    QVERIFY(
        result.diagnostics.isEmpty()
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(3)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(5)
        );
}

void RegexTextImporterTests::
    severityAliasesAreApplied()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    profile.severityAliases.insert(
        QStringLiteral("FAIL"),
        RecordSeverity::Error
        );

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026-08-11T12:00:00Z "
                "[NOTICE] [Gateway] Accepted"
                ),
            QStringLiteral(
                "2026-08-11T12:01:00Z "
                "[FAIL] [Gateway] Rejected"
                )
        });

    QCOMPARE(
        result.records.size(),
        2
        );

    QVERIFY(
        result.diagnostics.isEmpty()
        );

    QCOMPARE(
        result.records.at(0)
            .severity.value(),
        RecordSeverity::Info
        );

    QCOMPARE(
        result.records.at(1)
            .severity.value(),
        RecordSeverity::Error
        );
}

void RegexTextImporterTests::
    qtTimestampRulesAreApplied()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+(?<message>.*)$)"
            );

    profile.timestampRules.clear();

    profile.timestampRules.append({
        TimestampRuleType::QtFormat,
        QStringLiteral(
            "yyyy/MM/dd HH:mm:ss.zzz"
            )
    });

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026/08/11 12:14:31.275 "
                "[INFO] [Worker] "
                "Background job started"
                )
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

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.timestamp->date(),
        QDate(2026, 8, 11)
        );

    QCOMPARE(
        record.timestamp->time(),
        QTime(12, 14, 31, 275)
        );
}

void RegexTextImporterTests::
    explicitCustomFieldsAreMapped()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+\[(?<thread>[^\]]+)\]\s+(?<message>.*)$)"
            );

    profile.customFields.append({
        QStringLiteral(
            "Thread"
            ),
        QStringLiteral(
            "thread"
            )
    });

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026-08-11T12:20:00Z "
                "[INFO] [Orders] [worker-7] "
                "Order accepted"
                )
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("Thread")
                                   ).toString(),
        QStringLiteral("worker-7")
        );

    QVERIFY(
        !record.customAttributes.contains(
            QStringLiteral("thread")
            )
        );
}

void RegexTextImporterTests::
    unmappedCapturesArePreserved()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+\[(?<thread>[^\]]+)\]\s+(?<message>.*)$)"
            );

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026-08-11T12:20:00Z "
                "[INFO] [Orders] [worker-7] "
                "Order accepted"
                )
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes.value(
                                   QStringLiteral("thread")
                                   ).toString(),
        QStringLiteral("worker-7")
        );

    QCOMPARE(
        record.customAttributes.size(),
        1
        );
}

void RegexTextImporterTests::
    unmappedCapturePreservationCanBeDisabled()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\]\s+\[(?<thread>[^\]]+)\]\s+(?<message>.*)$)"
            );

    profile.preserveUnmappedFields =
        false;

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026-08-11T12:20:00Z "
                "[INFO] [Orders] [worker-7] "
                "Order accepted"
                )
        });

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

void RegexTextImporterTests::
    optionalUnmatchedCaptureIsOmitted()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+\[(?<severity>\w+)\]\s+\[(?<subsystem>[^\]]+)\](?:\s+\[(?<thread>[^\]]+)\])?\s+(?<message>.*)$)"
            );

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "2026-08-11T12:25:00Z "
                "[INFO] [Orders] "
                "No thread identifier"
                )
        });

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        !result.records.first()
             .customAttributes
             .contains(
                 QStringLiteral("thread")
                 )
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "No thread identifier"
            )
        );
}

void RegexTextImporterTests::
    invalidPatternProducesDiagnostic()
{
    ImportProfile profile =
        basicRegexProfile();

    profile.regexPattern =
        QStringLiteral(
            R"((?<timestamp>\S+)"
            );

    RegexTextImporter importer(
        profile
        );

    const ImportResult result =
        importer.importLines({
            QStringLiteral(
                "anything"
                )
        });

    QCOMPARE(
        result.processedRecordCount,
        qint64(0)
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
        result.diagnostics.first().code,
        QStringLiteral(
            "INVALID_REGEX_PATTERN"
            )
        );

    QCOMPARE(
        result.diagnostics.first().severity,
        ImportDiagnosticSeverity::Error
        );
}

void RegexTextImporterTests::
    importFilePreservesPhysicalLineNumbers()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "\n"
        << "2026-08-11T12:30:00Z "
           "[INFO] [Gateway] First"
        << "\n"
        << "\n"
        << "this line does not match"
        << "\n"
        << "2026-08-11T12:32:00Z "
           "[ERROR] [Gateway] Third"
        << "\n";

    stream.flush();

    const QString filePath =
        file.fileName();

    const QString fileName =
        QFileInfo(
            filePath
            ).fileName();

    file.close();

    RegexTextImporter importer(
        basicRegexProfile()
        );

    const ImportResult result =
        importer.importFile(
            filePath
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
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(5)
        );

    QCOMPARE(
        result.records.at(0)
            .source.sourcePath,
        filePath
        );

    QCOMPARE(
        result.records.at(0)
            .source.sourceName,
        fileName
        );

    QVERIFY(
        result.diagnostics.first()
            .source.has_value()
        );

    QCOMPARE(
        result.diagnostics.first()
            .source->recordNumber,
        qint64(4)
        );

    QCOMPARE(
        result.diagnostics.first()
            .source->sourcePath,
        filePath
        );
}

void RegexTextImporterTests::
    importFileHonorsRecordLimit()
{
    QTemporaryFile file;

    QVERIFY(
        file.open()
        );

    QTextStream stream(&file);

    stream
        << "2026-08-11T12:00:00Z "
           "[INFO] [Orders] One"
        << "\n"
        << "\n"
        << "2026-08-11T12:01:00Z "
           "[WARN] [Orders] Two"
        << "\n"
        << "2026-08-11T12:02:00Z "
           "[ERROR] [Orders] Three"
        << "\n";

    stream.flush();

    const QString filePath =
        file.fileName();

    file.close();

    RegexTextImporter importer(
        basicRegexProfile()
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

    QVERIFY(
        result.sourceTruncated
        );
}

void RegexTextImporterTests::
    importFileReportsOpenFailure()
{
    RegexTextImporter importer(
        basicRegexProfile()
        );

    const QString missingPath =
        QStringLiteral(
            "missing/nonexistent/application.log"
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
        result.records.size(),
        0
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first().code,
        QStringLiteral(
            "FILE_OPEN_FAILED"
            )
        );

    QCOMPARE(
        result.diagnostics.first().severity,
        ImportDiagnosticSeverity::Error
        );

    QVERIFY(
        result.diagnostics.first()
            .source.has_value()
        );

    QCOMPARE(
        result.diagnostics.first()
            .source->sourcePath,
        missingPath
        );

    QCOMPARE(
        result.diagnostics.first()
            .source->sourceName,
        QStringLiteral(
            "application.log"
            )
        );

    QCOMPARE(
        result.diagnostics.first()
            .source->recordNumber,
        qint64(0)
        );
}

void RegexTextImporterTests::
    importFileReportsProgress()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    QTextStream stream(&file);

    stream
        << "2026-08-11T12:00:00Z "
           "[INFO] [Orders] First\n"
        << "2026-08-11T12:01:00Z "
           "[WARN] [Orders] Second\n";

    stream.flush();
    file.close();

    QVector<ImportProgress> progress;

    ImportExecutionContext context;

    context.reportProgress =
        [&progress](
            const ImportProgress &value
            ) {
            progress.append(value);
        };

    RegexTextImporter importer(
        basicRegexProfile()
        );

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

void RegexTextImporterTests::
    importFileCanBeCancelled()
{
    QTemporaryFile file;

    QVERIFY(file.open());

    QTextStream stream(&file);

    stream
        << "2026-08-11T12:00:00Z "
           "[INFO] [Orders] First\n"
        << "2026-08-11T12:01:00Z "
           "[WARN] [Orders] Second\n"
        << "2026-08-11T12:02:00Z "
           "[ERROR] [Orders] Third\n";

    stream.flush();
    file.close();

    int cancellationChecks = 0;

    ImportExecutionContext context;

    context.isCancellationRequested =
        [&cancellationChecks]() {
            ++cancellationChecks;

            return cancellationChecks > 1;
        };

    RegexTextImporter importer(
        basicRegexProfile()
        );

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

QTEST_MAIN(
    RegexTextImporterTests
    )

#include "RegexTextImporterTests.moc"