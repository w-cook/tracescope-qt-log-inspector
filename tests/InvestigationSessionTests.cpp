#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <utility>

#include "../src/importing/JsonLinesImporter.h"
#include "../src/live/LiveSessionFollowCoordinator.h"
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
    void ownsLiveFollowCoordinatorLazily();
    void reloadDiscardsLiveFollowCoordinator();
    void appendsLiveRecordsWithoutDiscardingInvestigationState();
    void ignoresDuplicateLiveRecordIds();
    void followsAppendedJsonLinesIntoExistingSession();
    void rejectsLiveFollowForExistingUnterminatedLine();
    void followsReplacementAsNewSourceGeneration();
    void automaticallyPollsLiveSession();
    void pausePreservesAutomaticPollingBacklog();
    void stopPreventsAutomaticPolling();
    void followsReplacementAfterPausedObservation();
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
    ownsLiveFollowCoordinatorLazily()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live-ownership.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    /*
     * Merely checking support must not allocate
     * live-follow state for an otherwise static
     * investigation session.
     */
    QVERIFY(
        session.supportsLiveFollowing()
        );

    QVERIFY(
        session.liveFollowCoordinator()
        == nullptr
        );

    /*
     * The coordinator is created only when live
     * following is actually requested.
     */
    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    QVERIFY(
        coordinator != nullptr
        );

    QVERIFY(
        session.liveFollowCoordinator()
        == coordinator
        );

    /*
     * Repeated requests must return the one
     * coordinator owned by this session rather
     * than constructing parallel live pipelines.
     */
    QCOMPARE(
        session.ensureLiveFollowCoordinator(),
        coordinator
        );

    /*
     * Structured JSON is a valid TraceScope import
     * format, but it does not yet have incremental
     * structural framing. It therefore remains
     * ineligible for live following at this stage.
     */
    ImportProfile unsupportedProfile;

    unsupportedProfile.name =
        QStringLiteral(
            "Unsupported Test Profile"
            );

    unsupportedProfile.importerId =
        QStringLiteral(
            "unsupported-test-importer"
            );

    ImportResult unsupportedResult;

    InvestigationSession unsupportedSession(
        sourcePath,
        unsupportedProfile,
        std::move(
            unsupportedResult
            )
        );

    QVERIFY(
        !unsupportedSession
             .supportsLiveFollowing()
        );

    QVERIFY(
        unsupportedSession
            .liveFollowCoordinator()
        == nullptr
        );

    QVERIFY(
        unsupportedSession
            .ensureLiveFollowCoordinator()
        == nullptr
        );

    QVERIFY(
        unsupportedSession
            .liveFollowCoordinator()
        == nullptr
        );
}

void InvestigationSessionTests::
    reloadDiscardsLiveFollowCoordinator()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live-reload.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    QCOMPARE(
        initialResult.processedRecordCount,
        qint64(1)
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    QVERIFY(
        coordinator != nullptr
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator->start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QVERIFY(
        coordinator
            ->state()
            .isFollowing()
        );

    /*
     * A destructive static reload establishes a
     * completely new session baseline. Any follower,
     * byte offset, framing state, and incremental
     * importer state associated with the old baseline
     * must therefore be discarded.
     */
    ImportResult reloadedResult;

    InvestigationRecord reloadedRecord;

    reloadedRecord.recordId =
        QStringLiteral(
            "reloaded-record"
            );

    reloadedRecord.message =
        QStringLiteral(
            "Reloaded record"
            );

    reloadedResult.records.append(
        reloadedRecord
        );

    reloadedResult.processedRecordCount = 1;

    session.reload(
        std::move(
            reloadedResult
            )
        );

    /*
     * Do not inspect `coordinator` below this point.
     * The reload intentionally destroyed the object
     * it previously referred to.
     */
    QVERIFY(
        session.liveFollowCoordinator()
        == nullptr
        );

    /*
     * Reloading removes the old live-follow runtime
     * state, but does not change the session's import
     * profile or its eligibility to begin following
     * again later.
     */
    QVERIFY(
        session.supportsLiveFollowing()
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.skippedRecordCount(),
        qint64(0)
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        1
        );

    QCOMPARE(
        records.first().recordId,
        QStringLiteral(
            "reloaded-record"
            )
        );

    QCOMPARE(
        records.first().message,
        std::optional<QString>(
            QStringLiteral(
                "Reloaded record"
                )
            )
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

void InvestigationSessionTests::
    followsAppendedJsonLinesIntoExistingSession()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live-session.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"timestamp":"2026-09-10T14:00:00Z","level":"INFO","subsystem":"Gateway","eventCode":"STARTED","entityId":"REQ-1","message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    QCOMPARE(
        initialResult.processedRecordCount,
        qint64(1)
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    /*
     * The coordinator now owns the complete live
     * ingestion path:
     *
     * physical source
     * -> bounded read
     * -> line framing
     * -> importer
     * -> InvestigationSession append
     */
    LiveSessionFollowCoordinator coordinator(
        session
        );

    QVERIFY(
        coordinator.isSupported()
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator.start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    /*
     * The existing static contents establish the
     * fixed live-follow baseline. Those bytes must
     * never be imported a second time.
     */
    QCOMPARE(
        startResult.baselineByteCount,
        qint64(
            initialRecord.size()
            )
        );

    QCOMPARE(
        coordinator
            .state()
            .sourceGeneration(),
        quint64(0)
        );

    const QByteArray appendedRecord(
        R"({"timestamp":"2026-09-10T14:00:01Z","level":"WARN","subsystem":"Gateway","eventCode":"SLOW_REQUEST","entityId":"REQ-2","message":"Live appended record"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            appendedRecord
            ),
        qint64(
            appendedRecord.size()
            )
        );

    sourceFile.close();

    /*
     * A single coordinator poll should consume the
     * new physical bytes, frame the completed line,
     * reuse the configured JSON Lines importer, and
     * append the resulting record to the existing
     * investigation session.
     */
    const LiveSessionFollowPollResult
        pollResult =
        coordinator.pollOnce();

    QVERIFY2(
        pollResult.succeeded,
        qPrintable(
            pollResult.errorMessage
            )
        );

    QCOMPARE(
        pollResult.observation.kind,
        LiveFileObservationKind::Appended
        );

    QCOMPARE(
        pollResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(1)
        );

    QVERIFY(
        !pollResult.sourceGenerationChanged
        );

    /*
     * The original static evidence remains in place
     * and the newly followed record has been appended
     * to the same investigation.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0).message,
        std::optional<QString>(
            QStringLiteral(
                "Existing record"
                )
            )
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Live appended record"
                )
            )
        );

    /*
     * The live record continues the original physical
     * source, so it is line two of generation zero.
     */
    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(0)
        );

    QVERIFY(
        records.at(0).recordId
        != records.at(1).recordId
        );
}

void InvestigationSessionTests::
    rejectsLiveFollowForExistingUnterminatedLine()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "unterminated.jsonl"
                )
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray content(
        R"({"message":"Existing"})"
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();

    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult importResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        importResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(importResult)
        );

    LiveSessionFollowCoordinator coordinator(
        session
        );

    const LiveSessionFollowStartResult result =
        coordinator.start();

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        coordinator.state().isStopped()
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );
}

void InvestigationSessionTests::
    followsReplacementAsNewSourceGeneration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live-replacement.jsonl"
                )
            );

    /*
     * Keep the original source deliberately larger
     * than the replacement so the same-path overwrite
     * is unambiguously observable as a size regression.
     */
    const QByteArray initialRecord(
        R"({"timestamp":"2026-09-10T15:00:00Z","level":"INFO","subsystem":"Gateway","eventCode":"ORIGINAL_GENERATION","entityId":"REQ-ORIGINAL","message":"Original generation record with deliberately longer physical contents"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    LiveSessionFollowCoordinator coordinator(
        session
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator.start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QCOMPARE(
        coordinator
            .state()
            .sourceGeneration(),
        quint64(0)
        );

    /*
     * Replace the contents at the same path.
     *
     * Opening WriteOnly without Append truncates the
     * existing file before the new generation is
     * written.
     */
    const QByteArray replacementRecord(
        R"({"message":"Replacement generation"})"
        "\n"
        );

    QVERIFY(
        replacementRecord.size()
        < initialRecord.size()
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            replacementRecord
            ),
        qint64(
            replacementRecord.size()
            )
        );

    sourceFile.close();

    /*
     * The coordinator should detect the size
     * regression, advance the physical generation,
     * reset line/import state, read the new source
     * from byte zero, and append its evidence.
     */
    const LiveSessionFollowPollResult
        pollResult =
        coordinator.pollOnce();

    QVERIFY2(
        pollResult.succeeded,
        qPrintable(
            pollResult.errorMessage
            )
        );

    QCOMPARE(
        pollResult.observation.kind,
        LiveFileObservationKind::
        SourceReset
        );

    QVERIFY(
        pollResult.sourceGenerationChanged
        );

    QCOMPARE(
        coordinator
            .state()
            .sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        pollResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(1)
        );

    /*
     * A physical reset must not behave like static
     * reload. Generation-zero evidence remains part
     * of the investigation.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0).message,
        std::optional<QString>(
            QStringLiteral(
                "Original generation record "
                "with deliberately longer "
                "physical contents"
                )
            )
        );

    QCOMPARE(
        records.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        records.at(0)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Replacement generation"
                )
            )
        );

    /*
     * The replacement is a new physical source, so
     * numbering restarts at line one and identity
     * carries generation one.
     */
    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(1)
        );

    QVERIFY(
        records.at(0).recordId
        != records.at(1).recordId
        );

    QVERIFY(
        coordinator.stop()
        );

    const LiveSessionFollowStartResult
        restartResult =
        coordinator.start();

    QVERIFY2(
        restartResult.succeeded,
        qPrintable(
            restartResult.errorMessage
            )
        );

    QCOMPARE(
        coordinator
            .state()
            .sourceGeneration(),
        quint64(1)
        );

    const QByteArray restartedRecord(
        R"({"message":"After live restart"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            restartedRecord
            ),
        qint64(
            restartedRecord.size()
            )
        );

    sourceFile.close();

    const LiveSessionFollowPollResult
        restartedPollResult =
        coordinator.pollOnce();

    QVERIFY2(
        restartedPollResult.succeeded,
        qPrintable(
            restartedPollResult.errorMessage
            )
        );

    QCOMPARE(
        restartedPollResult.observation.kind,
        LiveFileObservationKind::Appended
        );

    const QVector<InvestigationRecord>
        &restartedRecords =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        restartedRecords.size(),
        3
        );

    QCOMPARE(
        restartedRecords.at(2).message,
        std::optional<QString>(
            QStringLiteral(
                "After live restart"
                )
            )
        );

    QCOMPARE(
        restartedRecords.at(2)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        restartedRecords.at(2)
            .source.sourceGeneration,
        quint64(1)
        );
}

void InvestigationSessionTests::
    automaticallyPollsLiveSession()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "automatic-live-session.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    if (coordinator == nullptr) {
        QFAIL(
            "Expected the session to create a "
            "live-follow coordinator."
            );

        return;
    }

    QVERIFY(
        coordinator
            ->setPollIntervalMilliseconds(
                10
                )
        );

    QCOMPARE(
        coordinator
            ->pollIntervalMilliseconds(),
        10
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator->start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QVERIFY(
        coordinator
            ->state()
            .isFollowing()
        );

    const QByteArray appendedRecord(
        R"({"message":"Automatically followed record"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            appendedRecord
            ),
        qint64(
            appendedRecord.size()
            )
        );

    sourceFile.close();

    /*
     * No explicit pollOnce() call is made here.
     * The coordinator's timer must observe, frame,
     * import, and append the new physical record.
     */
    QTRY_COMPARE_WITH_TIMEOUT(
        session.importedRecordCount(),
        qint64(2),
        1000
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Automatically followed record"
                )
            )
        );

    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(0)
        );
}

void InvestigationSessionTests::
    pausePreservesAutomaticPollingBacklog()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "paused-live-session.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    if (coordinator == nullptr) {
        QFAIL(
            "Expected the session to create a "
            "live-follow coordinator."
            );

        return;
    }

    QVERIFY(
        coordinator
            ->setPollIntervalMilliseconds(
                10
                )
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator->start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QCOMPARE(
        coordinator
            ->state()
            .readOffset(),
        startResult.baselineByteCount
        );

    QVERIFY(
        coordinator->pause()
        );

    QVERIFY(
        coordinator
            ->state()
            .isPaused()
        );

    const qint64 pausedReadOffset =
        coordinator
            ->state()
            .readOffset();

    const QByteArray appendedRecord(
        R"({"message":"Backlogged while paused"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            appendedRecord
            ),
        qint64(
            appendedRecord.size()
            )
        );

    sourceFile.close();

    /*
     * The timer continues polling while paused, but
     * the follower must not consume the newly
     * available bytes.
     */
    QTest::qWait(
        75
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        coordinator
            ->state()
            .readOffset(),
        pausedReadOffset
        );

    QVERIFY(
        coordinator
            ->state()
            .isPaused()
        );

    /*
     * Resume must consume the unread backlog rather
     * than establishing a new EOF baseline.
     */
    QVERIFY(
        coordinator->resume()
        );

    QVERIFY(
        coordinator
            ->state()
            .isFollowing()
        );

    QTRY_COMPARE_WITH_TIMEOUT(
        session.importedRecordCount(),
        qint64(2),
        1000
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Backlogged while paused"
                )
            )
        );

    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(0)
        );
}

void InvestigationSessionTests::
    stopPreventsAutomaticPolling()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "stopped-live-session.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Existing record"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    if (coordinator == nullptr) {
        QFAIL(
            "Expected the session to create a "
            "live-follow coordinator."
            );

        return;
    }

    QVERIFY(
        coordinator
            ->setPollIntervalMilliseconds(
                10
                )
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator->start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    /*
     * First prove that automatic polling is active
     * before testing the stop boundary.
     */
    const QByteArray followedRecord(
        R"({"message":"Followed before stop"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            followedRecord
            ),
        qint64(
            followedRecord.size()
            )
        );

    sourceFile.close();

    QTRY_COMPARE_WITH_TIMEOUT(
        session.importedRecordCount(),
        qint64(2),
        1000
        );

    QVERIFY(
        coordinator->stop()
        );

    QVERIFY(
        coordinator
            ->state()
            .isStopped()
        );

    const qint64 stoppedReadOffset =
        coordinator
            ->state()
            .readOffset();

    const QByteArray afterStopRecord(
        R"({"message":"Must not be followed after stop"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        sourceFile.write(
            afterStopRecord
            ),
        qint64(
            afterStopRecord.size()
            )
        );

    sourceFile.close();

    /*
     * Multiple former timer intervals pass here.
     * Once stopped, no further automatic observation
     * or ingestion should advance the live session.
     */
    QTest::qWait(
        75
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        coordinator
            ->state()
            .readOffset(),
        stoppedReadOffset
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .size(),
        2
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .last()
            .message,
        std::optional<QString>(
            QStringLiteral(
                "Followed before stop"
                )
            )
        );
}

void InvestigationSessionTests::
    followsReplacementAfterPausedObservation()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "paused-replacement.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Original generation"})"
        "\n"
        );

    QFile sourceFile(
        sourcePath
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    sourceFile.close();

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Live JSON Lines"
            );

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    if (coordinator == nullptr) {
        QFAIL(
            "Expected the session to create a "
            "live-follow coordinator."
            );

        return;
    }

    QVERIFY(
        coordinator
            ->setPollIntervalMilliseconds(
                10
                )
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator->start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QVERIFY(
        coordinator->pause()
        );

    QVERIFY(
        coordinator
            ->state()
            .isPaused()
        );

    QCOMPARE(
        coordinator
            ->state()
            .sourceGeneration(),
        quint64(0)
        );

    /*
     * Replace the physical source while following
     * remains paused. The timer should still observe
     * the replacement, but must not consume it.
     */
    const QByteArray replacementRecord(
        R"({"message":"Replacement generation"})"
        "\n"
        );

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        sourceFile.write(
            replacementRecord
            ),
        qint64(
            replacementRecord.size()
            )
        );

    sourceFile.close();

    /*
     * Wait for the timer to observe the replacement
     * and advance the physical source generation.
     */
    QTRY_COMPARE_WITH_TIMEOUT(
        coordinator
            ->state()
            .sourceGeneration(),
        quint64(1),
        1000
        );

    QVERIFY(
        coordinator
            ->state()
            .isPaused()
        );

    QCOMPARE(
        coordinator
            ->state()
            .readOffset(),
        qint64(0)
        );

    /*
     * Observation while paused must not import the
     * replacement source yet.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .size(),
        1
        );

    /*
     * Resuming should consume the replacement source
     * from byte zero as generation 1.
     */
    QVERIFY(
        coordinator->resume()
        );

    QTRY_COMPARE_WITH_TIMEOUT(
        session.importedRecordCount(),
        qint64(2),
        1000
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0).message,
        std::optional<QString>(
            QStringLiteral(
                "Original generation"
                )
            )
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Replacement generation"
                )
            )
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(1)
        );

    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(1)
        );

    QVERIFY(
        records.at(0).recordId
        != records.at(1).recordId
        );
}

QTEST_MAIN(InvestigationSessionTests)

#include "InvestigationSessionTests.moc"