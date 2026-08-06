#include <QtTest/QtTest>

#include "../src/importing/ImportDiagnostic.h"
#include "../src/importing/ImportResult.h"

class ImportResultTests : public QObject
{
    Q_OBJECT

private slots:
    void defaultResultIsEmpty();
    void resultCalculatesImportedAndSkippedCounts();
    void resultDetectsWarningDiagnostics();
    void resultDetectsErrorDiagnostics();
    void diagnosticPreservesSourceInformation();
    void diagnosticSeverityToStringReturnsCanonicalLabels();
};

void ImportResultTests::defaultResultIsEmpty()
{
    const ImportResult result;

    QCOMPARE(result.processedRecordCount, qint64(0));
    QCOMPARE(result.importedRecordCount(), qint64(0));
    QCOMPARE(result.skippedRecordCount(), qint64(0));

    QVERIFY(result.records.isEmpty());
    QVERIFY(result.diagnostics.isEmpty());
    QVERIFY(!result.hasWarnings());
    QVERIFY(!result.hasErrors());
}

void ImportResultTests::resultCalculatesImportedAndSkippedCounts()
{
    ImportResult result;

    result.processedRecordCount = 4;
    result.records.append(InvestigationRecord {});
    result.records.append(InvestigationRecord {});

    QCOMPARE(result.importedRecordCount(), qint64(2));
    QCOMPARE(result.skippedRecordCount(), qint64(2));
}

void ImportResultTests::resultDetectsWarningDiagnostics()
{
    ImportResult result;

    ImportDiagnostic diagnostic;
    diagnostic.code = QStringLiteral("UNMAPPED_SEVERITY");
    diagnostic.message = QStringLiteral(
        "The severity value could not be mapped."
        );
    diagnostic.severity = ImportDiagnosticSeverity::Warning;

    result.diagnostics.append(diagnostic);

    QVERIFY(result.hasWarnings());
    QVERIFY(!result.hasErrors());
}

void ImportResultTests::resultDetectsErrorDiagnostics()
{
    ImportResult result;

    ImportDiagnostic diagnostic;
    diagnostic.code = QStringLiteral("MALFORMED_RECORD");
    diagnostic.message = QStringLiteral(
        "The source record is not valid JSON."
        );
    diagnostic.severity = ImportDiagnosticSeverity::Error;

    result.diagnostics.append(diagnostic);

    QVERIFY(!result.hasWarnings());
    QVERIFY(result.hasErrors());
}

void ImportResultTests::diagnosticPreservesSourceInformation()
{
    RecordSourceMetadata source;
    source.sourcePath = QStringLiteral("samples/session.jsonl");
    source.sourceName = QStringLiteral("session.jsonl");
    source.recordNumber = 7;

    ImportDiagnostic diagnostic;
    diagnostic.code = QStringLiteral("INVALID_TIMESTAMP");
    diagnostic.message = QStringLiteral(
        "The timestamp value is invalid."
        );
    diagnostic.severity = ImportDiagnosticSeverity::Warning;
    diagnostic.source = source;

    QVERIFY(diagnostic.source.has_value());
    QCOMPARE(
        diagnostic.source->sourcePath,
        QStringLiteral("samples/session.jsonl")
        );
    QCOMPARE(
        diagnostic.source->sourceName,
        QStringLiteral("session.jsonl")
        );
    QCOMPARE(diagnostic.source->recordNumber, qint64(7));
}

void ImportResultTests::
    diagnosticSeverityToStringReturnsCanonicalLabels()
{
    QCOMPARE(
        importDiagnosticSeverityToString(
            ImportDiagnosticSeverity::Information
            ),
        QStringLiteral("INFO")
        );

    QCOMPARE(
        importDiagnosticSeverityToString(
            ImportDiagnosticSeverity::Warning
            ),
        QStringLiteral("WARNING")
        );

    QCOMPARE(
        importDiagnosticSeverityToString(
            ImportDiagnosticSeverity::Error
            ),
        QStringLiteral("ERROR")
        );
}

QTEST_MAIN(ImportResultTests)

#include "ImportResultTests.moc"