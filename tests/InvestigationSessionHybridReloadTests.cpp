#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/domain/RecordIdentity.h"
#include "../src/importing/JsonLinesImporter.h"
#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/sources/SourcePhysicalIdentity.h"
#include "../src/workspace/InvestigationSession.h"

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

QTEST_MAIN(
    InvestigationSessionHybridReloadTests
    )

#include "InvestigationSessionHybridReloadTests.moc"