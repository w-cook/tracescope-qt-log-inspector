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
    void appendsLiveRecordsWithoutDiscardingInvestigationState();
    void ignoresDuplicateLiveRecordIds();
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

void InvestigationSessionTests::
    appendsLiveRecordsWithoutDiscardingInvestigationState()
{
    ImportProfile profile;

    ImportResult initialResult;

    InvestigationRecord first;

    first.recordId =
        QStringLiteral("first");

    first.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-10T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    first.severity =
        RecordSeverity::Info;

    first.subsystem =
        QStringLiteral("Gateway");

    first.eventCode =
        QStringLiteral("STARTED");

    first.entityId =
        QStringLiteral("gateway-1");

    first.message =
        QStringLiteral("Gateway started");

    first.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("edge-01")
        );

    initialResult.records.append(
        first
        );

    initialResult.processedRecordCount = 1;

    InvestigationSession session(
        QStringLiteral("session.jsonl"),
        profile,
        std::move(initialResult)
        );

    session.setSelectedRecordId(
        QStringLiteral("first")
        );

    session.setColumnWidths({
        120,
        180,
        240
    });

    InvestigationStateStore *stateStore =
        session.investigationStateStore();

    stateStore->setBookmarked(
        QStringLiteral("first"),
        true
        );

    stateStore->setNote(
        QStringLiteral("first"),
        QStringLiteral("Keep this context")
        );

    stateStore->setFindingStatus(
        QStringLiteral("first"),
        FindingStatus::Open
        );

    session
        .investigationController()
        ->setFilters(
            QStringLiteral("INFO"),
            QString(),
            QString()
            );

    ImportResult liveResult;

    InvestigationRecord second;

    second.recordId =
        QStringLiteral("second");

    second.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-10T10:05:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    second.severity =
        RecordSeverity::Error;

    second.subsystem =
        QStringLiteral("Payments");

    second.eventCode =
        QStringLiteral("PAYMENT_TIMEOUT");

    second.entityId =
        QStringLiteral("order-42");

    second.message =
        QStringLiteral("Payment timed out");

    second.customAttributes.insert(
        QStringLiteral("attempt"),
        3
        );

    liveResult.records.append(
        second
        );

    liveResult.processedRecordCount = 2;

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral("LIVE_TEST_DIAGNOSTIC");

    diagnostic.message =
        QStringLiteral(
            "One live record was skipped"
            );

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    liveResult.diagnostics.append(
        diagnostic
        );

    session.appendLiveImportResult(
        std::move(liveResult)
        );

    QCOMPARE(
        session.importedRecordCount(),
        2
        );

    QCOMPARE(
        session.processedRecordCount(),
        3
        );

    QCOMPARE(
        session.skippedRecordCount(),
        1
        );

    QCOMPARE(
        session.selectedRecordId(),
        QStringLiteral("first")
        );

    QCOMPARE(
        session.columnWidths(),
        QVector<int>({
            120,
            180,
            240
        })
        );

    QVERIFY(
        stateStore
            ->stateForRecord(
                QStringLiteral("first")
                )
            .bookmarked
        );

    QCOMPARE(
        stateStore
            ->stateForRecord(
                QStringLiteral("first")
                )
            .note,
        QStringLiteral("Keep this context")
        );

    QVERIFY(
        stateStore
            ->stateForRecord(
                QStringLiteral("first")
                )
            .findingStatus
        == FindingStatus::Open
        );

    QCOMPARE(
        session
            .investigationController()
            ->proxyModel()
            ->severityFilter(),
        QStringLiteral("INFO")
        );

    QCOMPARE(
        session
            .investigationController()
            ->proxyModel()
            ->rowCount(),
        1
        );

    QVERIFY(
        session.hasSeverityData()
        );

    QVERIFY(
        session.hasSubsystemData()
        );

    QVERIFY(
        session.availableSubsystems().contains(
            QStringLiteral("Gateway")
            )
        );

    QVERIFY(
        session.availableSubsystems().contains(
            QStringLiteral("Payments")
            )
        );

    QVERIFY(
        session.hasEventCodeData()
        );

    QVERIFY(
        session.availableEventCodes().contains(
            QStringLiteral("PAYMENT_TIMEOUT")
            )
        );

    QVERIFY(
        session.hasEntityData()
        );

    QVERIFY(
        session.availableEntities().contains(
            QStringLiteral("order-42")
            )
        );

    QVERIFY(
        session.hasCustomFieldData()
        );

    QVERIFY(
        session.availableCustomFields().contains(
            QStringLiteral("host")
            )
        );

    QVERIFY(
        session.availableCustomFields().contains(
            QStringLiteral("attempt")
            )
        );

    QCOMPARE(
        session.firstTimestamp().value(),
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-10T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            )
        );

    QCOMPARE(
        session.lastTimestamp().value(),
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-10T10:05:00.000Z"
                ),
            Qt::ISODateWithMs
            )
        );

    QCOMPARE(
        session.diagnostics().size(),
        1
        );
}

void InvestigationSessionTests::
    ignoresDuplicateLiveRecordIds()
{
    ImportProfile profile;

    ImportResult initialResult;

    InvestigationRecord first;

    first.recordId =
        QStringLiteral("first");

    first.subsystem =
        QStringLiteral("Gateway");

    initialResult.records.append(
        first
        );

    initialResult.processedRecordCount = 1;

    InvestigationSession session(
        QStringLiteral("session.jsonl"),
        profile,
        std::move(initialResult)
        );

    ImportResult liveResult;

    /*
     * Simulate defensive redelivery of a record
     * already incorporated into the investigation.
     * Its changed content must not affect session
     * state because the stable record identity has
     * already been observed.
     */
    InvestigationRecord duplicate;

    duplicate.recordId =
        QStringLiteral("first");

    duplicate.subsystem =
        QStringLiteral("DuplicateSubsystem");

    InvestigationRecord second;

    second.recordId =
        QStringLiteral("second");

    second.subsystem =
        QStringLiteral("Payments");

    liveResult.records.append(
        duplicate
        );

    liveResult.records.append(
        second
        );

    liveResult.processedRecordCount = 2;

    session.appendLiveImportResult(
        std::move(liveResult)
        );

    QCOMPARE(
        session.importedRecordCount(),
        2
        );

    QCOMPARE(
        session.processedRecordCount(),
        2
        );

    QCOMPARE(
        session.skippedRecordCount(),
        0
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .at(0)
            .recordId,
        QStringLiteral("first")
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .at(1)
            .recordId,
        QStringLiteral("second")
        );

    QVERIFY(
        session.availableSubsystems().contains(
            QStringLiteral("Gateway")
            )
        );

    QVERIFY(
        session.availableSubsystems().contains(
            QStringLiteral("Payments")
            )
        );

    QVERIFY(
        !session.availableSubsystems().contains(
            QStringLiteral(
                "DuplicateSubsystem"
                )
            )
        );
}

QTEST_MAIN(InvestigationSessionTests)

#include "InvestigationSessionTests.moc"