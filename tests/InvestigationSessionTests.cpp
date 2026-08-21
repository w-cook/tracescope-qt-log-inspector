#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <utility>

#include "../src/workspace/InvestigationSession.h"

class InvestigationSessionTests : public QObject
{
    Q_OBJECT

private slots:
    void preservesImportedSessionContext();
    void assignsDistinctSessionIds();
    void reloadsContentWithoutChangingSessionIdentity();
    void retainsInvestigationStateForRecordsThatSurviveReload();
};

void InvestigationSessionTests::
    preservesImportedSessionContext()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("session.jsonl")
            );

    QFile sourceFile(sourcePath);

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Text
            )
        );

    sourceFile.write(
        "{\"message\":\"test\"}\n"
        );

    sourceFile.close();

    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-1");

    record.message =
        QStringLiteral("Test record");

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral("TEST_WARNING");

    diagnostic.message =
        QStringLiteral("Test diagnostic");

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    ImportResult result;

    result.records.append(record);
    result.diagnostics.append(diagnostic);
    result.processedRecordCount = 2;

    ImportProfile profile;

    profile.name =
        QStringLiteral("Test Profile");

    profile.importerId =
        QStringLiteral("json-lines");

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(result)
        );

    QVERIFY(!session.id().isEmpty());

    QCOMPARE(
        session.sourceMetadata().sourcePath,
        QFileInfo(sourcePath)
            .absoluteFilePath()
        );

    QCOMPARE(
        session.sourceMetadata().sourceName,
        QStringLiteral("session.jsonl")
        );

    QVERIFY(
        session.sourceMetadata()
            .sourceSizeBytes > 0
        );

    QVERIFY(
        session.sourceMetadata()
            .importedAtUtc.isValid()
        );

    QCOMPARE(
        session.importProfile().name,
        QStringLiteral("Test Profile")
        );

    QCOMPARE(
        session.diagnostics().size(),
        1
        );

    QCOMPARE(
        session.processedRecordCount(),
        2
        );

    QCOMPARE(
        session.importedRecordCount(),
        1
        );

    QCOMPARE(
        session.skippedRecordCount(),
        1
        );

    QCOMPARE(
        session
            .investigationController()
            ->totalRecordCount(),
        1
        );
}

void InvestigationSessionTests::
    assignsDistinctSessionIds()
{
    ImportProfile profile;

    ImportResult firstResult;
    ImportResult secondResult;

    InvestigationSession first(
        QStringLiteral("first.jsonl"),
        profile,
        std::move(firstResult)
        );

    InvestigationSession second(
        QStringLiteral("second.jsonl"),
        profile,
        std::move(secondResult)
        );

    QVERIFY(!first.id().isEmpty());
    QVERIFY(!second.id().isEmpty());

    QVERIFY(
        first.id()
        != second.id()
        );
}

void InvestigationSessionTests::
    reloadsContentWithoutChangingSessionIdentity()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Test Profile");

    ImportResult initialResult;

    InvestigationRecord firstRecord;

    firstRecord.recordId =
        QStringLiteral("first");

    initialResult.records.append(
        firstRecord
        );

    InvestigationSession session(
        QStringLiteral("session.jsonl"),
        profile,
        std::move(initialResult)
        );

    const QString originalId =
        session.id();

    session
        .investigationController()
        ->setFilters(
            QStringLiteral("ERROR"),
            QString(),
            QStringLiteral("failure")
            );

    session.setColumnWidths({
        120,
        240
    });

    ImportResult reloadedResult;

    InvestigationRecord secondRecord;

    secondRecord.recordId =
        QStringLiteral("second");

    reloadedResult.records.append(
        secondRecord
        );

    reloadedResult.processedRecordCount = 1;

    session.reload(
        std::move(reloadedResult)
        );

    QCOMPARE(
        session.id(),
        originalId
        );

    QVERIFY(
        session.columnWidths().isEmpty()
        );

    QCOMPARE(
        session.importedRecordCount(),
        1
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .first()
            .recordId,
        QStringLiteral("second")
        );

    QCOMPARE(
        session
            .investigationController()
            ->proxyModel()
            ->severityFilter(),
        QStringLiteral("ERROR")
        );

    QCOMPARE(
        session
            .investigationController()
            ->proxyModel()
            ->searchText(),
        QStringLiteral("failure")
        );
}

void InvestigationSessionTests::
    retainsInvestigationStateForRecordsThatSurviveReload()
{
    ImportProfile profile;

    ImportResult initialResult;

    InvestigationRecord retainedRecord;

    retainedRecord.recordId =
        QStringLiteral("retained");

    InvestigationRecord removedRecord;

    removedRecord.recordId =
        QStringLiteral("removed");

    initialResult.records.append(
        retainedRecord
        );

    initialResult.records.append(
        removedRecord
        );

    InvestigationSession session(
        QStringLiteral("session.jsonl"),
        profile,
        std::move(initialResult)
        );

    InvestigationStateStore *stateStore =
        session.investigationStateStore();

    stateStore->setBookmarked(
        QStringLiteral("retained"),
        true
        );

    stateStore->setNote(
        QStringLiteral("removed"),
        QStringLiteral("Old finding")
        );

    QVERIFY(
        stateStore->hasStateForRecord(
            QStringLiteral("retained")
            )
        );

    QVERIFY(
        stateStore->hasStateForRecord(
            QStringLiteral("removed")
            )
        );

    ImportResult reloadedResult;

    InvestigationRecord retainedReloadedRecord;

    retainedReloadedRecord.recordId =
        QStringLiteral("retained");

    InvestigationRecord newRecord;

    newRecord.recordId =
        QStringLiteral("new");

    reloadedResult.records.append(
        retainedReloadedRecord
        );

    reloadedResult.records.append(
        newRecord
        );

    session.reload(
        std::move(reloadedResult)
        );

    QVERIFY(
        stateStore->hasStateForRecord(
            QStringLiteral("retained")
            )
        );

    QVERIFY(
        stateStore
            ->stateForRecord(
                QStringLiteral("retained")
                )
            .bookmarked
        );

    QVERIFY(
        !stateStore->hasStateForRecord(
            QStringLiteral("removed")
            )
        );

    QVERIFY(
        !stateStore->hasStateForRecord(
            QStringLiteral("new")
            )
        );
}

QTEST_MAIN(InvestigationSessionTests)

#include "InvestigationSessionTests.moc"