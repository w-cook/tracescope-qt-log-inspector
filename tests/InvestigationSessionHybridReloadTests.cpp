#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/domain/RecordIdentity.h"
#include "../src/importing/JsonLinesImporter.h"
#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/sources/SourcePhysicalIdentity.h"
#include "../src/workspace/HybridInvestigationReconstructionService.h"
#include "../src/workspace/InvestigationSession.h"
#include "../src/workspace/InvestigationSessionPersistence.h"

namespace
{
ImportProfile jsonLinesProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "message"
            );

    profile.preserveUnmappedFields =
        true;

    return profile;
}

void writeFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}

void appendFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}

void rebaseRecords(
    QVector<InvestigationRecord> &records,
    quint64 sourceGeneration
    )
{
    for (InvestigationRecord &record
         : records) {
        record.source.sourceGeneration =
            sourceGeneration;

        record.recordId =
            createStableRecordIdentity(
                record.source,
                record.rawSource
                );
    }
}

InvestigationExternalSourceBinding
makeExternalBinding(
    const QString &sourcePath,
    quint64 sourceGeneration
    )
{
    InvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration =
        sourceGeneration;

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    if (identityResult.succeeded) {
        binding.sourceIdentity =
            identityResult.identity;
    }

    return binding;
}
}

class InvestigationSessionHybridReloadTests
    : public QObject
{
    Q_OBJECT

private slots:
    void reloadPreservesSnapshotEvidenceAndRecordState();
    void failedReloadLeavesSessionUntouched();

    void capturesSourceBackedPersistence();
    void capturesSnapshotBackedPersistence();
    void capturesHybridPersistence();

    void reconnectsSnapshotBackedSessionAsHybrid();
    void failedReconnectLeavesSnapshotBackedSessionUntouched();

    void sourceBackedTransitionsToFreshSnapshotBacking();
    void hybridTransitionsToFreshSnapshotBacking();
};

void InvestigationSessionHybridReloadTests::
    reloadPreservesSnapshotEvidenceAndRecordState()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral(
                "session.tsinv"
                )
            );

    const QByteArray baseline(
        "{\"message\":\"one\"}\n"
        "{\"message\":\"two\"}\n"
        );

    writeFile(
        sourcePath,
        baseline
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        2
        );

    constexpr quint64
        SavedGeneration = 2;

    rebaseRecords(
        initialResult.records,
        SavedGeneration
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        initialResult.records;

    snapshot.diagnostics =
        initialResult.diagnostics;

    snapshot.processedRecordCount =
        initialResult.processedRecordCount;

    snapshot.sourceTruncated =
        initialResult.sourceTruncated;

    const QString firstRecordId =
        snapshot.records
            .first()
            .recordId;

    InvestigationSessionSnapshotFile
        snapshotFile;

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        snapshotFile.save(
            snapshotPath,
            snapshot
            );

    QVERIFY2(
        saveResult.isSuccess(),
        qPrintable(
            saveResult.errorMessage
            )
        );

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            SavedGeneration
            );

    QVERIFY(
        binding.sourceIdentity.has_value()
        );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::createHybrid(
            QStringLiteral(
                "hybrid-session"
                ),
            snapshotPath,
            snapshot,
            binding
            );

    QVERIFY(
        session != nullptr
        );

    QCOMPARE(
        session
            ->backing()
            .mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QCOMPARE(
        session->importedRecordCount(),
        qint64(2)
        );

    /*
     * This is the investigation state that the old
     * replacement-style reload path could destroy if
     * its record disappeared from the replacement
     * result.
     */
    session
        ->investigationStateStore()
        ->setBookmarked(
            firstRecordId,
            true
            );

    session
        ->investigationStateStore()
        ->setNote(
            firstRecordId,
            QStringLiteral(
                "Preserve this evidence."
                )
            );

    session->setSelectedRecordId(
        firstRecordId
        );

    appendFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"three\"}\n"
            )
        );

    const InvestigationSessionHybridReloadResult
        reloadResult =
        session->reloadHybrid(
            importer
            );

    QVERIFY2(
        reloadResult.succeeded,
        qPrintable(
            reloadResult.errorMessage
            )
        );

    QVERIFY(
        reloadResult.externalSourceAvailable
        );

    QVERIFY(
        !reloadResult.sourceGenerationChanged
        );

    QCOMPARE(
        reloadResult.duplicateReplayRecordCount,
        qint64(2)
        );

    QCOMPARE(
        reloadResult.appendedReplayRecordCount,
        qint64(1)
        );

    QCOMPARE(
        session->importedRecordCount(),
        qint64(3)
        );

    QCOMPARE(
        session->processedRecordCount(),
        qint64(3)
        );

    const QVector<InvestigationRecord>
        records =
        session
            ->investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        3
        );

    QCOMPARE(
        records.at(0).recordId,
        firstRecordId
        );

    QCOMPARE(
        records.at(2).message,
        std::optional<QString>(
            QStringLiteral(
                "three"
                )
            )
        );

    QCOMPARE(
        records
            .at(2)
            .source
            .sourceGeneration,
        SavedGeneration
        );

    QVERIFY(
        session
            ->investigationStateStore()
            ->hasStateForRecord(
                firstRecordId
                )
        );

    QVERIFY(
        session
            ->investigationStateStore()
            ->bookmarkedRecordIds()
            .contains(
                firstRecordId
                )
        );

    QCOMPARE(
        session->selectedRecordId(),
        firstRecordId
        );

    QCOMPARE(
        session
            ->backing()
            .externalSource()
            ->sourceGeneration,
        SavedGeneration
        );

    QVERIFY(
        session
            ->backing()
            .externalSource()
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        session->initialLiveFollowByteOffset(),
        qint64(0)
        );
}

void InvestigationSessionHybridReloadTests::
    failedReloadLeavesSessionUntouched()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral(
                "session.tsinv"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"saved\"}\n"
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    constexpr quint64
        SavedGeneration = 5;

    rebaseRecords(
        initialResult.records,
        SavedGeneration
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        initialResult.records;

    snapshot.processedRecordCount =
        initialResult.processedRecordCount;

    const QString recordId =
        snapshot.records
            .first()
            .recordId;

    InvestigationSessionSnapshotFile
        snapshotFile;

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        snapshotFile.save(
            snapshotPath,
            snapshot
            );

    QVERIFY(
        saveResult.isSuccess()
        );

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            SavedGeneration
            );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::createHybrid(
            QStringLiteral(
                "hybrid-session"
                ),
            snapshotPath,
            snapshot,
            binding
            );

    QVERIFY(
        session != nullptr
        );

    session
        ->investigationStateStore()
        ->setBookmarked(
            recordId,
            true
            );

    session->setSelectedRecordId(
        recordId
        );

    const qint64 originalRecordCount =
        session->importedRecordCount();

    const quint64 originalGeneration =
        session
            ->backing()
            .externalSource()
            ->sourceGeneration;

    /*
     * Corrupt the backing snapshot after the session
     * is already open.
     */
    writeFile(
        snapshotPath,
        QByteArray(
            "not a valid tsinv file"
            )
        );

    const InvestigationSessionHybridReloadResult
        reloadResult =
        session->reloadHybrid(
            importer
            );

    QVERIFY(
        !reloadResult.succeeded
        );

    QVERIFY(
        !reloadResult.errorMessage.isEmpty()
        );

    /*
     * Atomic failure invariant:
     * nothing currently installed in the session was
     * replaced or pruned.
     */
    QCOMPARE(
        session->importedRecordCount(),
        originalRecordCount
        );

    QCOMPARE(
        session->selectedRecordId(),
        recordId
        );

    QVERIFY(
        session
            ->investigationStateStore()
            ->bookmarkedRecordIds()
            .contains(
                recordId
                )
        );

    QCOMPARE(
        session
            ->backing()
            .mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QCOMPARE(
        session
            ->backing()
            .externalSource()
            ->sourceGeneration,
        originalGeneration
        );
}

void InvestigationSessionHybridReloadTests::
    capturesSourceBackedPersistence()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "source-backed.jsonl"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"record\"}\n"
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult importResult =
        importer.importFile(
            sourcePath
            );

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    sourceFamilyConfiguration
        .includeRotatedSources = true;

    sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    /*
     * These are current discovery results and must
     * not be carried into durable persistence.
     */
    sourceFamilyConfiguration
        .rotatedSourcePaths = {
        directory.filePath(
            QStringLiteral(
                "source-backed.jsonl.2"
                )
            ),
        directory.filePath(
            QStringLiteral(
                "source-backed.jsonl.1"
                )
            )
    };

    InvestigationSession session(
        QStringLiteral(
            "source-backed-session"
            ),
        sourcePath,
        profile,
        std::move(
            importResult
            ),
        sourceFamilyConfiguration
        );

    constexpr quint64
        ExpectedGeneration = 7;

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        identityResult.succeeded,
        qPrintable(
            identityResult.errorMessage
            )
        );

    session.updateExternalSourceRuntimeState(
        ExpectedGeneration,
        &identityResult.identity
        );

    const PersistedInvestigationSession
        persisted =
        InvestigationSessionPersistence::
        capture(
            session
            );

    QVERIFY(
        persisted.backing.mode
        == PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        persisted
            .backing
            .externalSourceBinding
            .has_value()
        );

    QVERIFY(
        persisted
            .backing
            .sourceImportProfile
            .has_value()
        );

    QVERIFY(
        !persisted
             .backing
             .snapshotReference
             .has_value()
        );

    const PersistedInvestigationExternalSourceBinding
        &externalSource =
        *persisted
             .backing
             .externalSourceBinding;

    QCOMPARE(
        externalSource.sourceGeneration,
        ExpectedGeneration
        );

    QCOMPARE(
        externalSource.sourcePath,
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QVERIFY(
        externalSource
            .sourceIdentity
            .has_value()
        );

    QCOMPARE(
        externalSource
            .sourceIdentity
            ->prefixFingerprint,
        identityResult
            .identity
            .prefixFingerprint
        );

    QVERIFY(
        externalSource
            .sourceFamilyConfiguration
            .includeRotatedSources
        );

    QVERIFY(
        externalSource
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(
        persisted
            .backing
            .sourceImportProfile
            ->importerId
        == profile.importerId
        );

    /*
     * Compatibility mirrors remain populated until
     * schema-v2 restoration no longer needs them.
     */
    QCOMPARE(
        persisted.sourcePath,
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QCOMPARE(
        persisted.importProfile.importerId,
        profile.importerId
        );

    QVERIFY(
        persisted
            .sourceFamilyConfiguration
            .includeRotatedSources
        );
}

void InvestigationSessionHybridReloadTests::
    capturesSnapshotBackedPersistence()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral(
                "snapshot-only.tsinv"
                )
            );

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        jsonLinesProfile();

    InvestigationRecord record;

    record.recordId =
        QStringLiteral(
            "snapshot-record"
            );

    record.message =
        QStringLiteral(
            "Saved snapshot record"
            );

    record.source.sourcePath =
        QStringLiteral(
            "C:/original/source.jsonl"
            );

    record.source.sourceName =
        QStringLiteral(
            "source.jsonl"
            );

    record.source.recordNumber = 1;

    snapshot.records.append(
        record
        );

    snapshot.processedRecordCount = 1;

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-backed-session"
                ),
            snapshotPath,
            snapshot
            );

    QVERIFY(
        session != nullptr
        );

    const PersistedInvestigationSession
        persisted =
        InvestigationSessionPersistence::
        capture(
            *session
            );

    QVERIFY(
        persisted.backing.mode
        == PersistedInvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        persisted
            .backing
            .snapshotReference
            .has_value()
        );

    QVERIFY(
        !persisted
             .backing
             .externalSourceBinding
             .has_value()
        );

    QVERIFY(
        !persisted
             .backing
             .sourceImportProfile
             .has_value()
        );

    QCOMPARE(
        *persisted
             .backing
             .snapshotReference,
        QFileInfo(
            snapshotPath
            ).absoluteFilePath()
        );

    /*
     * Snapshot-backed provenance can still populate
     * the temporary compatibility sourcePath, but it
     * must not become an external-source dependency.
     */
    QCOMPARE(
        persisted.sourcePath,
        QStringLiteral(
            "C:/original/source.jsonl"
            )
        );

    QCOMPARE(
        persisted.importProfile.importerId,
        QStringLiteral(
            "json-lines"
            )
        );
}

void InvestigationSessionHybridReloadTests::
    capturesHybridPersistence()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "hybrid.jsonl"
                )
            );

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral(
                "hybrid.tsinv"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"current\"}\n"
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult imported =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        imported.records.size(),
        1
        );

    constexpr quint64
        ExpectedGeneration = 4;

    rebaseRecords(
        imported.records,
        ExpectedGeneration
        );

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        imported.records;

    snapshot.processedRecordCount =
        imported.processedRecordCount;

    InvestigationExternalSourceBinding
        externalSource =
        makeExternalBinding(
            sourcePath,
            ExpectedGeneration
            );

    QVERIFY(
        externalSource
            .sourceIdentity
            .has_value()
        );

    externalSource
        .sourceFamilyConfiguration
        .includeRotatedSources = true;

    externalSource
        .sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    /*
     * Physical family members must not survive the
     * persistence capture.
     */
    externalSource
        .sourceFamilyConfiguration
        .rotatedSourcePaths = {
        directory.filePath(
            QStringLiteral(
                "hybrid.jsonl.2"
                )
            ),
        directory.filePath(
            QStringLiteral(
                "hybrid.jsonl.1"
                )
            )
    };

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::createHybrid(
            QStringLiteral(
                "hybrid-session"
                ),
            snapshotPath,
            snapshot,
            externalSource
            );

    QVERIFY(
        session != nullptr
        );

    const PersistedInvestigationSession
        persisted =
        InvestigationSessionPersistence::
        capture(
            *session
            );

    QVERIFY(
        persisted.backing.mode
        == PersistedInvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        persisted
            .backing
            .snapshotReference
            .has_value()
        );

    QVERIFY(
        persisted
            .backing
            .externalSourceBinding
            .has_value()
        );

    QVERIFY(
        !persisted
             .backing
             .sourceImportProfile
             .has_value()
        );

    QCOMPARE(
        *persisted
             .backing
             .snapshotReference,
        QFileInfo(
            snapshotPath
            ).absoluteFilePath()
        );

    const PersistedInvestigationExternalSourceBinding
        &persistedExternalSource =
        *persisted
             .backing
             .externalSourceBinding;

    QCOMPARE(
        persistedExternalSource.sourceGeneration,
        ExpectedGeneration
        );

    QCOMPARE(
        persistedExternalSource.sourcePath,
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QVERIFY(
        persistedExternalSource
            .sourceIdentity
            .has_value()
        );

    QVERIFY(
        persistedExternalSource
            .sourceFamilyConfiguration
            .includeRotatedSources
        );

    QVERIFY(
        persistedExternalSource
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QCOMPARE(
        persisted.importProfile.importerId,
        profile.importerId
        );
}

void InvestigationSessionHybridReloadTests::
    reconnectsSnapshotBackedSessionAsHybrid()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    const QString snapshotPath =
        directory.filePath(
            QStringLiteral("session.tsinv")
            );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    /*
     * First create the evidence that will become the
     * durable Snapshot-backed investigation.
     */
    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"one\"}\n"
            "{\"message\":\"two\"}\n"
            )
        );

    ImportResult snapshotImport =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        snapshotImport.records.size(),
        2
        );

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        snapshotImport.records;

    snapshot.diagnostics =
        snapshotImport.diagnostics;

    snapshot.processedRecordCount =
        snapshotImport.processedRecordCount;

    snapshot.sourceTruncated =
        snapshotImport.sourceTruncated;

    const QString retainedRecordId =
        snapshot.records
            .first()
            .recordId;

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        InvestigationSessionSnapshotFile()
            .save(
                snapshotPath,
                snapshot
                );

    QVERIFY2(
        saveResult.isSuccess(),
        qPrintable(
            saveResult.errorMessage
            )
        );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-session"
                ),
            snapshotPath,
            snapshot
            );

    QVERIFY(
        session != nullptr
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    /*
     * User-created investigation state must survive
     * the reconnect candidate replacement.
     */
    session
        ->investigationStateStore()
        ->setBookmarked(
            retainedRecordId,
            true
            );

    session
        ->investigationStateStore()
        ->setNote(
            retainedRecordId,
            QStringLiteral(
                "preserve this note"
                )
            );

    /*
     * The source continues while TraceScope is
     * Snapshot-backed.
     */
    appendFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"three\"}\n"
            )
        );

    InvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    HybridInvestigationReconstructionResult
        reconstruction =
        HybridInvestigationReconstructionService()
            .reconstruct(
                snapshot,
                binding,
                importer
                );

    QVERIFY2(
        reconstruction.succeeded,
        qPrintable(
            reconstruction.errorMessage
            )
        );

    QVERIFY(
        reconstruction
            .externalSourceAvailable
        );

    QCOMPARE(
        reconstruction
            .duplicateReplayRecordCount,
        qint64(2)
        );

    QCOMPARE(
        reconstruction
            .appendedReplayRecordCount,
        qint64(1)
        );

    InvestigationSessionHybridReloadResult
        reconnectResult =
        session->applyHybridReconnect(
            snapshot,
            std::move(
                reconstruction
                )
            );

    QVERIFY2(
        reconnectResult.succeeded,
        qPrintable(
            reconnectResult.errorMessage
            )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        session->backing()
            .hasSnapshot()
        );

    QVERIFY(
        session->backing()
            .hasExternalSource()
        );

    QCOMPARE(
        session->externalSourcePath(),
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QCOMPARE(
        session->importedRecordCount(),
        3
        );

    const InvestigationRecordState
        retainedState =
        session
            ->investigationStateStore()
            ->stateForRecord(
                retainedRecordId
                );

    QVERIFY(
        retainedState.bookmarked
        );

    QCOMPARE(
        retainedState.note,
        QStringLiteral(
            "preserve this note"
            )
        );
}

void InvestigationSessionHybridReloadTests::
    failedReconnectLeavesSnapshotBackedSessionUntouched()
{
    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        jsonLinesProfile();

    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-1");

    record.message =
        QStringLiteral("saved evidence");

    snapshot.records.append(
        record
        );

    snapshot.processedRecordCount =
        1;

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "snapshot-session"
                ),
            QStringLiteral(
                "session.tsinv"
                ),
            snapshot
            );

    QVERIFY(
        session != nullptr
        );

    session
        ->investigationStateStore()
        ->setBookmarked(
            QStringLiteral("record-1"),
            true
            );

    HybridInvestigationReconstructionResult
        failedReconstruction;

    failedReconstruction.succeeded =
        false;

    failedReconstruction.errorMessage =
        QStringLiteral(
            "prepared reconnect failed"
            );

    InvestigationSessionHybridReloadResult
        reconnectResult =
        session->applyHybridReconnect(
            snapshot,
            std::move(
                failedReconstruction
                )
            );

    QVERIFY(
        !reconnectResult.succeeded
        );

    QCOMPARE(
        reconnectResult.errorMessage,
        QStringLiteral(
            "prepared reconnect failed"
            )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QCOMPARE(
        session->importedRecordCount(),
        1
        );

    QVERIFY(
        session
            ->investigationStateStore()
            ->stateForRecord(
                QStringLiteral("record-1")
                )
            .bookmarked
        );
}

void InvestigationSessionHybridReloadTests::
    sourceBackedTransitionsToFreshSnapshotBacking()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "source.jsonl"
                )
            );

    const QString newSnapshotPath =
        directory.filePath(
            QStringLiteral(
                "fresh-snapshot.tsinv"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"one\"}\n"
            "{\"message\":\"two\"}\n"
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult importResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        importResult.records.size(),
        2
        );

    const QString retainedRecordId =
        importResult.records
            .first()
            .recordId;

    InvestigationSession session(
        QStringLiteral(
            "source-session"
            ),
        sourcePath,
        profile,
        std::move(
            importResult
            )
        );

    constexpr quint64
        SourceGeneration = 4;

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        identityResult.succeeded,
        qPrintable(
            identityResult.errorMessage
            )
        );

    session.updateExternalSourceRuntimeState(
        SourceGeneration,
        &identityResult.identity
        );

    session
        .investigationStateStore()
        ->setBookmarked(
            retainedRecordId,
            true
            );

    session
        .investigationStateStore()
        ->setNote(
            retainedRecordId,
            QStringLiteral(
                "keep this state"
                )
            );

    session.setSelectedRecordId(
        retainedRecordId
        );

    QVERIFY(
        session.applySnapshotOnlyTransition(
            newSnapshotPath,
            InvestigationSnapshotSourceFidelity::
            NormalizedOnly
            )
        );

    QCOMPARE(
        session.backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        session.backing().hasSnapshot()
        );

    QVERIFY(
        !session.backing()
             .hasExternalSource()
        );

    QCOMPARE(
        session.backing()
            .snapshot()
            ->snapshotPath,
        QFileInfo(
            newSnapshotPath
            ).absoluteFilePath()
        );

    QVERIFY(
        !session.supportsLiveFollowing()
        );

    /*
     * The removed operational binding becomes dormant
     * reconnect metadata.
     */
    const InvestigationExternalSourceBinding
        *reconnectHint =
        session.reconnectSourceHint();

    QVERIFY(
        reconnectHint != nullptr
        );

    QCOMPARE(
        reconnectHint->sourcePath,
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QCOMPARE(
        reconnectHint->sourceGeneration,
        SourceGeneration
        );

    QVERIFY(
        reconnectHint
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        reconnectHint
            ->sourceIdentity
            ->prefixFingerprint,
        identityResult
            .identity
            .prefixFingerprint
        );

    /*
     * Runtime investigation evidence/state is not
     * rebuilt by the backing transition.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        session.selectedRecordId(),
        retainedRecordId
        );

    const InvestigationRecordState
        retainedState =
        session
            .investigationStateStore()
            ->stateForRecord(
                retainedRecordId
                );

    QVERIFY(
        retainedState.bookmarked
        );

    QCOMPARE(
        retainedState.note,
        QStringLiteral(
            "keep this state"
            )
        );
}

void InvestigationSessionHybridReloadTests::
    hybridTransitionsToFreshSnapshotBacking()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    const QString oldSnapshotPath =
        directory.filePath(
            QStringLiteral(
                "old-snapshot.tsinv"
                )
            );

    const QString newSnapshotPath =
        directory.filePath(
            QStringLiteral(
                "new-snapshot.tsinv"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"saved\"}\n"
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    constexpr quint64
        SourceGeneration = 6;

    rebaseRecords(
        initialResult.records,
        SourceGeneration
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        initialResult.records;

    snapshot.diagnostics =
        initialResult.diagnostics;

    snapshot.processedRecordCount =
        initialResult.processedRecordCount;

    snapshot.sourceTruncated =
        initialResult.sourceTruncated;

    const QString retainedRecordId =
        snapshot.records
            .first()
            .recordId;

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            SourceGeneration
            );

    QVERIFY(
        binding.sourceIdentity.has_value()
        );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createHybrid(
            QStringLiteral(
                "hybrid-session"
                ),
            oldSnapshotPath,
            snapshot,
            binding
            );

    QVERIFY(
        session != nullptr
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    session
        ->investigationStateStore()
        ->setBookmarked(
            retainedRecordId,
            true
            );

    QVERIFY(
        session
            ->applySnapshotOnlyTransition(
                newSnapshotPath,
                InvestigationSnapshotSourceFidelity::
                NormalizedOnly
                )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        session->backing().hasSnapshot()
        );

    QVERIFY(
        !session->backing()
             .hasExternalSource()
        );

    /*
     * The previous Hybrid evidence floor is replaced by
     * the freshly captured snapshot path supplied by
     * the persistence transaction.
     */
    QCOMPARE(
        session->backing()
            .snapshot()
            ->snapshotPath,
        QFileInfo(
            newSnapshotPath
            ).absoluteFilePath()
        );

    QVERIFY(
        session->backing()
            .snapshot()
            ->snapshotPath
        != QFileInfo(
               oldSnapshotPath
               ).absoluteFilePath()
        );

    const InvestigationExternalSourceBinding
        *reconnectHint =
        session->reconnectSourceHint();

    QVERIFY(
        reconnectHint != nullptr
        );

    QCOMPARE(
        reconnectHint->sourceGeneration,
        SourceGeneration
        );

    QCOMPARE(
        reconnectHint->sourcePath,
        QFileInfo(
            sourcePath
            ).absoluteFilePath()
        );

    QVERIFY(
        reconnectHint
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        session->importedRecordCount(),
        qint64(1)
        );

    QVERIFY(
        session
            ->investigationStateStore()
            ->stateForRecord(
                retainedRecordId
                )
            .bookmarked
        );

    QVERIFY(
        !session->supportsLiveFollowing()
        );
}

QTEST_MAIN(
    InvestigationSessionHybridReloadTests
    )

#include "InvestigationSessionHybridReloadTests.moc"