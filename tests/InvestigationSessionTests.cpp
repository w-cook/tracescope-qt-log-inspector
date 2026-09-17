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
    void appliesSnapshotReloadWithoutChangingBackingMode();
    void rejectsSnapshotReloadForSourceBackedSession();
    void captureSnapshotPreservesActiveSourceContinuity();
    void captureSnapshotPreservesDormantReconnectContinuity();
    void capturesInitialExternalSourceIdentity();
    void redefinesActiveSourcePathWithoutChangingLogicalIdentity();
    void rejectsUnverifiedSourceRelocationWithoutMutation();
    void redefinesDormantSnapshotReconnectPath();
    void sourceRelocationPreservesLiveFollowBoundary();
    void sourceRelocationSynchronizesPendingGenerationReset();
    void snapshotBackedSessionDerivesReconnectHintFromSnapshot();
    void snapshotBackedSessionWithoutContinuityHasNoReconnectHint();
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

void InvestigationSessionTests::
    appliesSnapshotReloadWithoutChangingBackingMode()
{
    InvestigationSessionSnapshot initialSnapshot;

    initialSnapshot.importProfile.name =
        QStringLiteral("Initial Profile");

    InvestigationRecord retainedRecord;

    retainedRecord.recordId =
        QStringLiteral("retained");

    retainedRecord.message =
        QStringLiteral("old retained value");

    InvestigationRecord removedRecord;

    removedRecord.recordId =
        QStringLiteral("removed");

    initialSnapshot.records = {
        retainedRecord,
        removedRecord
    };

    initialSnapshot.processedRecordCount =
        2;

    auto session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral("snapshot-session"),
            QStringLiteral("snapshot.tsinv"),
            std::move(initialSnapshot)
            );

    QVERIFY(session != nullptr);

    session
        ->investigationStateStore()
        ->setBookmarked(
            QStringLiteral("retained"),
            true
            );

    session
        ->investigationStateStore()
        ->setNote(
            QStringLiteral("retained"),
            QStringLiteral("keep me")
            );

    session
        ->investigationStateStore()
        ->setBookmarked(
            QStringLiteral("removed"),
            true
            );

    InvestigationSessionSnapshot replacement;

    replacement.importProfile.name =
        QStringLiteral("Replacement Profile");

    InvestigationRecord updatedRetained;

    updatedRetained.recordId =
        QStringLiteral("retained");

    updatedRetained.message =
        QStringLiteral("new retained value");

    InvestigationRecord newRecord;

    newRecord.recordId =
        QStringLiteral("new");

    newRecord.message =
        QStringLiteral("new evidence");

    replacement.records = {
        updatedRetained,
        newRecord
    };

    replacement.processedRecordCount =
        2;

    QVERIFY(
        session->applySnapshotReload(
            std::move(replacement)
            )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QCOMPARE(
        session->importedRecordCount(),
        2
        );

    QCOMPARE(
        session->importProfile().name,
        QStringLiteral(
            "Replacement Profile"
            )
        );

    const InvestigationRecordState
        retainedState =
        session
            ->investigationStateStore()
            ->stateForRecord(
                QStringLiteral("retained")
                );

    QVERIFY(
        retainedState.bookmarked
        );

    QCOMPARE(
        retainedState.note,
        QStringLiteral("keep me")
        );

    QVERIFY(
        !session
             ->investigationStateStore()
             ->hasStateForRecord(
                 QStringLiteral("removed")
                 )
        );
}

void InvestigationSessionTests::
    rejectsSnapshotReloadForSourceBackedSession()
{
    ImportProfile profile;
    ImportResult result;

    InvestigationSession session(
        QStringLiteral("source.jsonl"),
        profile,
        std::move(result)
        );

    InvestigationSessionSnapshot snapshot;

    QVERIFY(
        !session.applySnapshotReload(
            std::move(snapshot)
            )
        );

    QCOMPARE(
        session.backing().mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );
}

void InvestigationSessionTests::
    captureSnapshotPreservesActiveSourceContinuity()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("active-source.jsonl")
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

    ImportProfile profile;

    profile.name =
        QStringLiteral("Active Source");

    profile.importerId =
        QStringLiteral("json-lines");

    ImportResult result;

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(result)
        );

    const QString logicalSourceKey =
        QStringLiteral(
            "C:/original/location/active-source.jsonl"
            );

    QVERIFY(
        session.updateExternalSourceLogicalKey(
            logicalSourceKey
            )
        );

    SourceFamilyConfiguration
        familyConfiguration;

    familyConfiguration.includeRotatedSources =
        true;

    familyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    familyConfiguration.rotatedSourcePaths = {
        QStringLiteral(
            "C:/temporary/discovered/source.log.2"
            ),
        QStringLiteral(
            "C:/temporary/discovered/source.log.1"
            )
    };

    session.setSourceFamilyConfiguration(
        familyConfiguration
        );

    SourcePhysicalIdentity identity;

    identity.birthTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-17T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    identity.fingerprintLength = 16;

    identity.prefixFingerprint =
        QByteArray(
            32,
            static_cast<char>(0x4a)
            );

    identity.observedSizeBytes = 128;

    session.updateExternalSourceRuntimeState(
        4,
        &identity
        );

    const InvestigationSessionSnapshot
        snapshot =
        session.captureSnapshot();

    QVERIFY(
        snapshot.sourceContinuity.has_value()
        );

    const InvestigationSnapshotSourceContinuity
        &continuity =
        *snapshot.sourceContinuity;

    QCOMPARE(
        continuity.sourcePath,
        QFileInfo(sourcePath)
            .absoluteFilePath()
        );

    QCOMPARE(
        continuity.logicalSourceKey,
        logicalSourceKey
        );

    QCOMPARE(
        continuity.sourceGeneration,
        quint64(4)
        );

    QCOMPARE(
        continuity
            .sourceFamilyConfiguration
            .includeRotatedSources,
        true
        );

    QCOMPARE(
        continuity
            .sourceFamilyConfiguration
            .rotationRule
            .namingScheme,
        RotatedSourceNamingScheme::
        NumericSuffix
        );

    QVERIFY(
        continuity
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(
        continuity.sourceIdentity.has_value()
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->birthTime,
        identity.birthTime
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->fingerprintLength,
        identity.fingerprintLength
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->prefixFingerprint,
        identity.prefixFingerprint
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->observedSizeBytes,
        identity.observedSizeBytes
        );
}

void InvestigationSessionTests::
    captureSnapshotPreservesDormantReconnectContinuity()
{
    InvestigationSessionSnapshot
        initialSnapshot;

    initialSnapshot.importProfile.name =
        QStringLiteral(
            "Snapshot Profile"
            );

    initialSnapshot.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    InvestigationExternalSourceBinding
        reconnectHint;

    reconnectHint.sourcePath =
        QStringLiteral(
            "C:/moved/service.jsonl"
            );

    reconnectHint.logicalSourceKey =
        QStringLiteral(
            "C:/original/service.jsonl"
            );

    reconnectHint.sourceGeneration = 6;

    reconnectHint
        .sourceFamilyConfiguration
        .includeRotatedSources = true;

    reconnectHint
        .sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericBeforeExtension;

    reconnectHint
        .sourceFamilyConfiguration
        .rotatedSourcePaths = {
        QStringLiteral(
            "C:/moved/service.2.jsonl"
            ),
        QStringLiteral(
            "C:/moved/service.1.jsonl"
            )
    };

    SourcePhysicalIdentity identity;

    identity.birthTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-17T13:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    identity.fingerprintLength = 8;

    identity.prefixFingerprint =
        QByteArray(
            32,
            static_cast<char>(0x2b)
            );

    identity.observedSizeBytes = 512;

    reconnectHint.sourceIdentity =
        identity;

    auto session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-session"
                ),
            QStringLiteral(
                "saved-investigation.tsinv"
                ),
            std::move(initialSnapshot),
            reconnectHint
            );

    QVERIFY(session != nullptr);

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        session->backing().externalSource()
        == nullptr
        );

    QVERIFY(
        session->reconnectSourceHint()
        != nullptr
        );

    const InvestigationSessionSnapshot
        captured =
        session->captureSnapshot();

    QVERIFY(
        captured.sourceContinuity.has_value()
        );

    const InvestigationSnapshotSourceContinuity
        &continuity =
        *captured.sourceContinuity;

    QCOMPARE(
        continuity.sourcePath,
        QStringLiteral(
            "C:/moved/service.jsonl"
            )
        );

    QCOMPARE(
        continuity.logicalSourceKey,
        QStringLiteral(
            "C:/original/service.jsonl"
            )
        );

    QCOMPARE(
        continuity.sourceGeneration,
        quint64(6)
        );

    QCOMPARE(
        continuity
            .sourceFamilyConfiguration
            .includeRotatedSources,
        true
        );

    QCOMPARE(
        continuity
            .sourceFamilyConfiguration
            .rotationRule
            .namingScheme,
        RotatedSourceNamingScheme::
        NumericBeforeExtension
        );

    QVERIFY(
        continuity
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(
        continuity.sourceIdentity.has_value()
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->birthTime,
        identity.birthTime
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->fingerprintLength,
        identity.fingerprintLength
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->prefixFingerprint,
        identity.prefixFingerprint
        );

    QCOMPARE(
        continuity
            .sourceIdentity
            ->observedSizeBytes,
        identity.observedSizeBytes
        );
}

void InvestigationSessionTests::
    capturesInitialExternalSourceIdentity()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("source.jsonl")
            );

    QFile sourceFile(sourcePath);

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray content(
        "{\"message\":\"existing\"}\n"
        );

    QCOMPARE(
        sourceFile.write(content),
        qint64(content.size())
        );

    sourceFile.close();

    ImportProfile profile;
    ImportResult result;

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(result)
        );

    const InvestigationExternalSourceBinding
        *binding =
        session
            .backing()
            .externalSource();

    if (binding == nullptr) {
        QFAIL(
            "Expected an active external source binding."
            );

        return;
    }

    QVERIFY(
        binding
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        binding
            ->sourceIdentity
            ->observedSizeBytes,
        qint64(content.size())
        );
}

void InvestigationSessionTests::
    redefinesActiveSourcePathWithoutChangingLogicalIdentity()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("source.log")
            );

    const QString relocatedPath =
        directory.filePath(
            QStringLiteral("relocated.log")
            );

    const QByteArray content(
        6000,
        'A'
        );

    QFile sourceFile(sourcePath);

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        sourceFile.write(content),
        qint64(content.size())
        );

    sourceFile.close();

    ImportProfile profile;
    ImportResult importResult;

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(importResult)
        );

    const QString logicalSourceKey =
        QStringLiteral(
            "stable-logical-source"
            );

    QVERIFY(
        session.updateExternalSourceLogicalKey(
            logicalSourceKey
            )
        );

    const InvestigationExternalSourceBinding
        *initialBinding =
        session
            .backing()
            .externalSource();

    if (initialBinding == nullptr) {
        QFAIL(
            "Expected an active external source binding."
            );

        return;
    }

    QVERIFY(
        initialBinding
            ->sourceIdentity
            .has_value()
        );

    const SourcePhysicalIdentity identity =
        *initialBinding->sourceIdentity;

    session.updateExternalSourceRuntimeState(
        4,
        &identity
        );

    SourceFamilyConfiguration
        familyConfiguration;

    familyConfiguration.includeRotatedSources =
        true;

    familyConfiguration.rotatedSourcePaths = {
        QStringLiteral("old-rotation-1.log"),
        QStringLiteral("old-rotation-2.log")
    };

    session.setSourceFamilyConfiguration(
        familyConfiguration
        );

    QVERIFY(
        QFile::copy(
            sourcePath,
            relocatedPath
            )
        );

    const InvestigationSessionSourceRelocationResult
        relocation =
        session.redefineSourcePath(
            relocatedPath
            );

    QVERIFY2(
        relocation.succeeded,
        qPrintable(
            relocation.errorMessage
            )
        );

    const InvestigationExternalSourceBinding
        *binding =
        session
            .backing()
            .externalSource();

    QVERIFY(binding != nullptr);

    QCOMPARE(
        binding->sourcePath,
        QFileInfo(relocatedPath)
            .absoluteFilePath()
        );

    QCOMPARE(
        binding->logicalSourceKey,
        logicalSourceKey
        );

    QCOMPARE(
        binding->sourceGeneration,
        quint64(4)
        );

    QVERIFY(
        binding
            ->sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(binding->sourceIdentity);

    QCOMPARE(
        session
            .sourceMetadata()
            .sourcePath,
        QFileInfo(relocatedPath)
            .absoluteFilePath()
        );
}

void InvestigationSessionTests::
    rejectsUnverifiedSourceRelocationWithoutMutation()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("source.log")
            );

    const QString unrelatedPath =
        directory.filePath(
            QStringLiteral("unrelated.log")
            );

    QFile sourceFile(sourcePath);

    QVERIFY(
        sourceFile.open(
            QIODevice::WriteOnly
            )
        );

    sourceFile.write(
        QByteArray(
            5000,
            'A'
            )
        );

    sourceFile.close();

    QFile unrelatedFile(unrelatedPath);

    QVERIFY(
        unrelatedFile.open(
            QIODevice::WriteOnly
            )
        );

    unrelatedFile.write(
        QByteArray(
            5000,
            'B'
            )
        );

    unrelatedFile.close();

    ImportProfile profile;
    ImportResult importResult;

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(importResult)
        );

    const QString originalPath =
        session.externalSourcePath();

    const InvestigationSessionSourceRelocationResult
        relocation =
        session.redefineSourcePath(
            unrelatedPath
            );

    QVERIFY(!relocation.succeeded);

    QCOMPARE(
        session.externalSourcePath(),
        originalPath
        );
}

void InvestigationSessionTests::
    redefinesDormantSnapshotReconnectPath()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral("original.log")
            );

    const QString relocatedPath =
        directory.filePath(
            QStringLiteral("relocated.log")
            );

    QFile originalFile(originalPath);

    QVERIFY(
        originalFile.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray content(
        5000,
        'A'
        );

    QCOMPARE(
        originalFile.write(content),
        qint64(content.size())
        );

    originalFile.close();

    const SourcePhysicalIdentityCaptureResult
        identity =
        captureSourcePhysicalIdentity(
            originalPath
            );

    QVERIFY(identity.succeeded);

    QVERIFY(
        QFile::copy(
            originalPath,
            relocatedPath
            )
        );

    InvestigationExternalSourceBinding
        reconnectHint;

    reconnectHint.sourcePath =
        QFileInfo(originalPath)
            .absoluteFilePath();

    reconnectHint.logicalSourceKey =
        QStringLiteral(
            "stable-logical-source"
            );

    reconnectHint.sourceGeneration = 3;

    reconnectHint.sourceIdentity =
        identity.identity;

    InvestigationSessionSnapshot snapshot;

    auto session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-session"
                ),
            QStringLiteral(
                "investigation.tsinv"
                ),
            std::move(snapshot),
            reconnectHint
            );

    QVERIFY(session != nullptr);

    const InvestigationSessionSourceRelocationResult
        relocation =
        session->redefineSourcePath(
            relocatedPath
            );

    QVERIFY2(
        relocation.succeeded,
        qPrintable(
            relocation.errorMessage
            )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        session
            ->backing()
            .externalSource()
        == nullptr
        );

    const InvestigationExternalSourceBinding
        *updatedHint =
        session->reconnectSourceHint();

    QVERIFY(updatedHint != nullptr);

    QCOMPARE(
        updatedHint->sourcePath,
        QFileInfo(relocatedPath)
            .absoluteFilePath()
        );

    QCOMPARE(
        updatedHint->logicalSourceKey,
        QStringLiteral(
            "stable-logical-source"
            )
        );

    QCOMPARE(
        updatedHint->sourceGeneration,
        quint64(3)
        );
}

void InvestigationSessionTests::
    sourceRelocationPreservesLiveFollowBoundary()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral(
                "original-live.jsonl"
                )
            );

    const QString relocatedPath =
        directory.filePath(
            QStringLiteral(
                "relocated-live.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Initial record"})"
        "\n"
        );

    QFile originalFile(
        originalPath
        );

    QVERIFY(
        originalFile.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        originalFile.write(
            initialRecord
            ),
        qint64(
            initialRecord.size()
            )
        );

    originalFile.close();

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
            originalPath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        originalPath,
        profile,
        std::move(
            initialResult
            )
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

    QCOMPARE(
        startResult.baselineByteCount,
        qint64(
            initialRecord.size()
            )
        );

    const QByteArray followedRecord(
        R"({"message":"Followed before relocation"})"
        "\n"
        );

    QVERIFY(
        originalFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        originalFile.write(
            followedRecord
            ),
        qint64(
            followedRecord.size()
            )
        );

    originalFile.close();

    const LiveSessionFollowPollResult
        firstPoll =
        coordinator->pollOnce();

    QVERIFY2(
        firstPoll.succeeded,
        qPrintable(
            firstPoll.errorMessage
            )
        );

    QCOMPARE(
        firstPoll.importedRecordCount,
        qint64(1)
        );

    const qint64 consumedByteCount =
        qint64(
            initialRecord.size()
            + followedRecord.size()
            );

    QCOMPARE(
        coordinator
            ->state()
            .readOffset(),
        consumedByteCount
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    QVERIFY(
        QFile::copy(
            originalPath,
            relocatedPath
            )
        );

    const InvestigationSessionSourceRelocationResult
        relocation =
        session.redefineSourcePath(
            relocatedPath
            );

    QVERIFY2(
        relocation.succeeded,
        qPrintable(
            relocation.errorMessage
            )
        );

    /*
     * redefineSourcePath() destroys the coordinator
     * that belonged to the previous physical path.
     */
    QVERIFY(
        session.liveFollowCoordinator()
        == nullptr
        );

    QCOMPARE(
        session.initialLiveFollowByteOffset(),
        consumedByteCount
        );

    QCOMPARE(
        session.externalSourcePath(),
        QFileInfo(
            relocatedPath
            ).absoluteFilePath()
        );

    LiveSessionFollowCoordinator
        *relocatedCoordinator =
        session.ensureLiveFollowCoordinator();

    QVERIFY(
        relocatedCoordinator != nullptr
        );

    const LiveSessionFollowStartResult
        relocatedStart =
        relocatedCoordinator->start();

    QVERIFY2(
        relocatedStart.succeeded,
        qPrintable(
            relocatedStart.errorMessage
            )
        );

    QCOMPARE(
        relocatedStart.baselineByteCount,
        consumedByteCount
        );

    QCOMPARE(
        relocatedCoordinator
            ->state()
            .readOffset(),
        consumedByteCount
        );

    const QByteArray postRelocationRecord(
        R"({"message":"Followed after relocation"})"
        "\n"
        );

    QFile relocatedFile(
        relocatedPath
        );

    QVERIFY(
        relocatedFile.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        relocatedFile.write(
            postRelocationRecord
            ),
        qint64(
            postRelocationRecord.size()
            )
        );

    relocatedFile.close();

    const LiveSessionFollowPollResult
        secondPoll =
        relocatedCoordinator->pollOnce();

    QVERIFY2(
        secondPoll.succeeded,
        qPrintable(
            secondPoll.errorMessage
            )
        );

    QCOMPARE(
        secondPoll.importedRecordCount,
        qint64(1)
        );

    QVERIFY(
        !secondPoll.sourceGenerationChanged
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(3)
        );

    const QVector<InvestigationRecord> &records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        3
        );

    QCOMPARE(
        records.at(2).message,
        std::optional<QString>(
            QStringLiteral(
                "Followed after relocation"
                )
            )
        );

    /*
     * Relocating the same logical source must not
     * create a new source generation.
     */
    QCOMPARE(
        records.at(2)
            .source
            .sourceGeneration,
        quint64(0)
        );
}

void InvestigationSessionTests::
    sourceRelocationSynchronizesPendingGenerationReset()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral(
                "original-live.jsonl"
                )
            );

    const QString relocatedPath =
        directory.filePath(
            QStringLiteral(
                "relocated-live.jsonl"
                )
            );

    const QByteArray initialRecord(
        R"({"message":"Generation zero"})"
        "\n"
        );

    QFile sourceFile(
        originalPath
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

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            originalPath
            );

    InvestigationSession session(
        originalPath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator *coordinator =
        session.ensureLiveFollowCoordinator();

    QVERIFY(coordinator != nullptr);

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
            .sourceGeneration(),
        quint64(0)
        );

    /*
     * Simulate the generator's in-place truncate.
     * Do not poll TraceScope afterward: relocation
     * itself must discover this pending generation.
     */
    const QByteArray replacementRecord(
        R"({"message":"Generation one"})"
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

    QVERIFY(
        QFile::copy(
            originalPath,
            relocatedPath
            )
        );

    const InvestigationSessionSourceRelocationResult
        relocation =
        session.redefineSourcePath(
            relocatedPath
            );

    QVERIFY2(
        relocation.succeeded,
        qPrintable(
            relocation.errorMessage
            )
        );

    const InvestigationExternalSourceBinding
        *binding =
        session
            .backing()
            .externalSource();

    if (binding == nullptr) {
        QFAIL(
            "Expected an active external source binding."
            );

        return;
    }

    QCOMPARE(
        binding->sourceGeneration,
        quint64(1)
        );

    QCOMPARE(
        session.initialLiveFollowByteOffset(),
        qint64(0)
        );

    QVERIFY(
        session.liveFollowCoordinator()
        == nullptr
        );

    LiveSessionFollowCoordinator
        *relocatedCoordinator =
        session.ensureLiveFollowCoordinator();

    QVERIFY(
        relocatedCoordinator != nullptr
        );

    const LiveSessionFollowStartResult
        relocatedStart =
        relocatedCoordinator->start();

    QVERIFY2(
        relocatedStart.succeeded,
        qPrintable(
            relocatedStart.errorMessage
            )
        );

    QCOMPARE(
        relocatedCoordinator
            ->state()
            .sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        relocatedStart.baselineByteCount,
        qint64(0)
        );

    const LiveSessionFollowPollResult
        pollResult =
        relocatedCoordinator->pollOnce();

    QVERIFY2(
        pollResult.succeeded,
        qPrintable(
            pollResult.errorMessage
            )
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(1)
        );

    QVERIFY(
        !pollResult.sourceGenerationChanged
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
        records.last()
            .source
            .sourceGeneration,
        quint64(1)
        );
}

void InvestigationSessionTests::
    snapshotBackedSessionDerivesReconnectHintFromSnapshot()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral(
                "standalone.tsinv"
                )
            );

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile.name =
        QStringLiteral(
            "Snapshot Profile"
            );

    snapshot.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    InvestigationSnapshotSourceContinuity
        continuity;

    continuity.sourcePath =
        QStringLiteral(
            "C:/logs/field-gateway.jsonl"
            );

    continuity.logicalSourceKey =
        QStringLiteral(
            "logical-field-gateway"
            );

    continuity
        .sourceFamilyConfiguration
        .includeRotatedSources = true;

    continuity
        .sourceFamilyConfiguration
        .rotatedSourcePaths = {
        QStringLiteral(
            "C:/logs/field-gateway.jsonl.1"
            )
    };

    continuity.sourceGeneration = 4;

    SourcePhysicalIdentity identity;

    identity.observedSizeBytes = 128;
    identity.fingerprintLength = 64;
    identity.prefixFingerprint =
        QByteArray(
            32,
            'a'
            );

    continuity.sourceIdentity =
        identity;

    snapshot.sourceContinuity =
        continuity;

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-session"
                ),
            snapshotPath,
            std::move(snapshot)
            );

    QVERIFY(session != nullptr);

    QCOMPARE(
        session
            ->backing()
            .mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    const InvestigationExternalSourceBinding
        *reconnectHint =
        session->reconnectSourceHint();

    if (reconnectHint == nullptr) {
        QFAIL(
            "Expected schema-v2 source continuity "
            "to produce a reconnect hint."
            );

        return;
    }

    QCOMPARE(
        reconnectHint->sourcePath,
        QStringLiteral(
            "C:/logs/field-gateway.jsonl"
            )
        );

    QCOMPARE(
        reconnectHint->logicalSourceKey,
        QStringLiteral(
            "logical-field-gateway"
            )
        );

    QCOMPARE(
        reconnectHint->sourceGeneration,
        quint64(4)
        );

    QVERIFY(
        reconnectHint
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        reconnectHint
            ->sourceIdentity
            ->observedSizeBytes,
        qint64(128)
        );

    QVERIFY(
        reconnectHint
            ->sourceFamilyConfiguration
            .includeRotatedSources
        );

    QVERIFY(
        reconnectHint
            ->sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );
}

void InvestigationSessionTests::
    snapshotBackedSessionWithoutContinuityHasNoReconnectHint()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "legacy-snapshot-session"
                ),
            directory.filePath(
                QStringLiteral(
                    "legacy.tsinv"
                    )
                ),
            std::move(snapshot)
            );

    QVERIFY(session != nullptr);

    QCOMPARE(
        session
            ->backing()
            .mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        session->reconnectSourceHint()
        == nullptr
        );
}

QTEST_MAIN(InvestigationSessionTests)

#include "InvestigationSessionTests.moc"