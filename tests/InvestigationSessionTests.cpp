#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <utility>

#include "../src/workspace/InvestigationSession.h"
#include "../src/workspace/InvestigationSessionPersistence.h"

class InvestigationSessionTests : public QObject
{
    Q_OBJECT

private slots:
    void preservesImportedSessionContext();
    void assignsDistinctSessionIds();
    void reloadsContentWithoutChangingSessionIdentity();
    void retainsInvestigationStateForRecordsThatSurviveReload();
    void restoresExplicitSessionIdentity();
    void capturesAndRestoresPersistedInvestigationState();
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

void InvestigationSessionTests::
    restoresExplicitSessionIdentity()
{
    ImportProfile profile;
    ImportResult result;

    const QString persistedId =
        QStringLiteral(
            "persisted-session-id"
            );

    InvestigationSession session(
        persistedId,
        QStringLiteral(
            "restored-session.jsonl"
            ),
        profile,
        std::move(result)
        );

    QCOMPARE(
        session.id(),
        persistedId
        );
}

void InvestigationSessionTests::
    capturesAndRestoresPersistedInvestigationState()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Persistence Test");

    ImportResult originalResult;

    InvestigationSession original(
        QStringLiteral("persisted-session"),
        QStringLiteral("source.jsonl"),
        profile,
        std::move(originalResult)
        );

    InvestigationStateStore *stateStore =
        original.investigationStateStore();

    stateStore->setBookmarked(
        QStringLiteral("record-1"),
        true
        );

    stateStore->setNote(
        QStringLiteral("record-1"),
        QStringLiteral(
            "Investigate this event"
            )
        );

    stateStore->setFindingStatus(
        QStringLiteral("record-1"),
        FindingStatus::Open
        );

    stateStore->setNote(
        QStringLiteral("record-2"),
        QStringLiteral(
            "Related observation"
            )
        );

    CustomFieldFilterMap customFilters;

    customFilters.insert(
        QStringLiteral("requestId"),
        {
            QStringLiteral("req-123"),
            QStringLiteral("req-456")
        }
        );

    const QDateTime startTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00Z"
                ),
            Qt::ISODate
            );

    const QDateTime endTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T13:00:00Z"
                ),
            Qt::ISODate
            );

    original.investigationController()
        ->setFilterState(
            {
                QStringLiteral("Warning"),
                QStringLiteral("Error")
            },
            {
                QStringLiteral("Backend")
            },
            QStringLiteral("timeout"),
            {
                QStringLiteral("EVT-100")
            },
            {
                QStringLiteral("node-4")
            },
            startTime,
            endTime,
            customFilters,
            {
                QStringLiteral("Open")
            },
            true
            );

    const PersistedInvestigationSession
        persisted =
        InvestigationSessionPersistence::
        capture(original);

    QCOMPARE(
        persisted.sessionId,
        QStringLiteral(
            "persisted-session"
            )
        );

    QCOMPARE(
        persisted.recordStates.size(),
        2
        );

    QCOMPARE(
        persisted.filterState.searchText,
        QStringLiteral("timeout")
        );

    QVERIFY(
        persisted.filterState.bookmarkedOnly
        );

    ImportResult restoredResult;

    InvestigationSession restored(
        persisted.sessionId,
        persisted.sourcePath,
        persisted.importProfile,
        std::move(restoredResult)
        );

    InvestigationSessionPersistence::
        restoreState(
            persisted,
            restored
            );

    const InvestigationStateStore
        *restoredStateStore =
        restored.investigationStateStore();

    const InvestigationRecordState
        restoredRecordOne =
        restoredStateStore->stateForRecord(
            QStringLiteral("record-1")
            );

    QVERIFY(
        restoredRecordOne.bookmarked
        );

    QCOMPARE(
        restoredRecordOne.note,
        QStringLiteral(
            "Investigate this event"
            )
        );

    QVERIFY(
        restoredRecordOne.findingStatus
        == FindingStatus::Open
        );

    const InvestigationRecordState
        restoredRecordTwo =
        restoredStateStore->stateForRecord(
            QStringLiteral("record-2")
            );

    QCOMPARE(
        restoredRecordTwo.note,
        QStringLiteral(
            "Related observation"
            )
        );

    const InvestigationFilterProxyModel
        *restoredProxy =
        restored
            .investigationController()
            ->proxyModel();

    QCOMPARE(
        restoredProxy->severityFilters(),
        QStringList({
            QStringLiteral("WARNING"),
            QStringLiteral("ERROR")
        })
        );

    QCOMPARE(
        restoredProxy->subsystemFilters(),
        QStringList({
            QStringLiteral("Backend")
        })
        );

    QCOMPARE(
        restoredProxy->searchText(),
        QStringLiteral("timeout")
        );

    QCOMPARE(
        restoredProxy->eventCodeFilters(),
        QStringList({
            QStringLiteral("EVT-100")
        })
        );

    QCOMPARE(
        restoredProxy->entityFilters(),
        QStringList({
            QStringLiteral("node-4")
        })
        );

    QCOMPARE(
        restoredProxy->timeRangeStart(),
        std::optional<QDateTime>(
            startTime
            )
        );

    QCOMPARE(
        restoredProxy->timeRangeEnd(),
        std::optional<QDateTime>(
            endTime
            )
        );

    QCOMPARE(
        restoredProxy->customFieldFilters(),
        customFilters
        );

    QCOMPARE(
        restoredProxy->findingStatusFilters(),
        QStringList({
            QStringLiteral("OPEN")
        })
        );

    QVERIFY(
        restoredProxy->bookmarkedOnly()
        );
}

QTEST_MAIN(InvestigationSessionTests)

#include "InvestigationSessionTests.moc"