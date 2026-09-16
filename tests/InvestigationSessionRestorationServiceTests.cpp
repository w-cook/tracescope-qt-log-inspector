#include <QtTest>

#include <optional>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/domain/RecordIdentity.h"
#include "../src/importing/ImportResultSourceGeneration.h"
#include "../src/importing/JsonLinesImporter.h"
#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/sources/SourcePhysicalIdentity.h"
#include "../src/workspace/InvestigationSessionRestorationService.h"

namespace
{
ImportProfile jsonLinesProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral("json-lines");

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    profile.preserveUnmappedFields =
        true;

    return profile;
}

bool writeFile(
    const QString &filePath,
    const QByteArray &content
    )
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )) {
        return false;
    }

    return file.write(content)
           == content.size();
}

InvestigationSessionSnapshot makeSnapshot(
    const QString &sourcePath,
    const ImportProfile &profile,
    quint64 sourceGeneration = 0
    )
{
    JsonLinesImporter importer(
        profile
        );

    ImportResult result =
        importer.importFile(
            sourcePath
            );

    if (sourceGeneration != 0) {
        rebaseImportResultSourceGeneration(
            result,
            sourceGeneration
            );
    }

    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        result.records;

    snapshot.diagnostics =
        result.diagnostics;

    snapshot.processedRecordCount =
        result.processedRecordCount;

    snapshot.sourceTruncated =
        result.sourceTruncated;

    return snapshot;
}

PersistedInvestigationSession
makeSourceBackedPersistence(
    const QString &sessionId,
    const QString &sourcePath,
    const ImportProfile &profile,
    quint64 sourceGeneration = 0
    )
{
    PersistedInvestigationSession persisted;

    persisted.sessionId =
        sessionId;

    persisted.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    PersistedInvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration =
        sourceGeneration;

    persisted.backing.externalSourceBinding =
        binding;

    persisted.backing.sourceImportProfile =
        profile;

    return persisted;
}

std::optional<SourcePhysicalIdentity>
captureIdentity(
    const QString &sourcePath
    )
{
    const SourcePhysicalIdentityCaptureResult result =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    if (!result.succeeded) {
        return std::nullopt;
    }

    return result.identity;
}

QString absolutePath(
    const QString &path
    )
{
    return QFileInfo(path)
    .absoluteFilePath();
}
}

class InvestigationSessionRestorationServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sourceBackedRestoresPersistedGeneration();

    void sourceBackedKeepsRotatedSourcesIndependent();

    void missingSourceBackedSourceFails();

    void snapshotBackedRestoresRelativeSnapshotWithoutSource();

    void hybridRestoresSnapshotWhenExternalSourceIsMissing();
};

void InvestigationSessionRestorationServiceTests::
    sourceBackedRestoresPersistedGeneration()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("active.jsonl")
            );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArrayLiteral(
                "{\"message\":\"one\"}\n"
                "{\"message\":\"two\"}\n"
                )
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    constexpr quint64
        PersistedGeneration = 4;

    PersistedInvestigationSession persisted =
        makeSourceBackedPersistence(
            QStringLiteral("source-session"),
            sourcePath,
            profile,
            PersistedGeneration
            );

    const std::optional<SourcePhysicalIdentity>
        identity =
        captureIdentity(
            sourcePath
            );

    QVERIFY(identity.has_value());

    persisted
        .backing
        .externalSourceBinding
        ->sourceIdentity =
        *identity;

    InvestigationSessionRestorationService
        service;

    InvestigationSessionRestorationResult result =
        service.restore(
            persisted,
            directory.filePath(
                QStringLiteral("workspace.tsw")
                )
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(result.errorMessage)
        );

    QVERIFY(result.session != nullptr);

    QCOMPARE(
        result.session->backing().mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QCOMPARE(
        result.session->importedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord> records =
        result.session
            ->investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    for (const InvestigationRecord &record
         : records) {
        QCOMPARE(
            record.source.sourceGeneration,
            PersistedGeneration
            );

        QCOMPARE(
            record.recordId,
            createStableRecordIdentity(
                record.source,
                record.rawSource
                )
            );
    }

    const InvestigationExternalSourceBinding
        *restoredBinding =
        result.session
            ->backing()
            .externalSource();

    QVERIFY(restoredBinding != nullptr);

    QCOMPARE(
        restoredBinding->sourceGeneration,
        PersistedGeneration
        );

    QVERIFY(
        restoredBinding
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        result.session
            ->initialLiveFollowByteOffset(),
        QFileInfo(sourcePath).size()
        );
}

void InvestigationSessionRestorationServiceTests::
    sourceBackedKeepsRotatedSourcesIndependent()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("service.log")
            );

    const QString rotatedPath =
        sourcePath
        + QStringLiteral(".1");

    QVERIFY(
        writeFile(
            rotatedPath,
            QByteArrayLiteral(
                "{\"message\":\"rotated\"}\n"
                )
            )
        );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArrayLiteral(
                "{\"message\":\"active\"}\n"
                )
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    constexpr quint64
        ActiveGeneration = 7;

    PersistedInvestigationSession persisted =
        makeSourceBackedPersistence(
            QStringLiteral("rotated-session"),
            sourcePath,
            profile,
            ActiveGeneration
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

    persisted
        .backing
        .externalSourceBinding
        ->sourceFamilyConfiguration =
        familyConfiguration;

    const std::optional<SourcePhysicalIdentity>
        identity =
        captureIdentity(
            sourcePath
            );

    QVERIFY(identity.has_value());

    persisted
        .backing
        .externalSourceBinding
        ->sourceIdentity =
        *identity;

    InvestigationSessionRestorationService
        service;

    InvestigationSessionRestorationResult result =
        service.restore(
            persisted,
            directory.filePath(
                QStringLiteral("workspace.tsw")
                )
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(result.errorMessage)
        );

    QCOMPARE(
        result.session->importedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord> records =
        result.session
            ->investigationController()
            ->allRecords();

    const InvestigationRecord *rotatedRecord =
        nullptr;

    const InvestigationRecord *activeRecord =
        nullptr;

    for (const InvestigationRecord &record
         : records) {
        if (record.message
            == std::optional<QString>(
                QStringLiteral("rotated")
                )) {
            rotatedRecord =
                &record;
        }

        if (record.message
            == std::optional<QString>(
                QStringLiteral("active")
                )) {
            activeRecord =
                &record;
        }
    }

    QVERIFY(rotatedRecord != nullptr);
    QVERIFY(activeRecord != nullptr);

    /*
     * Rotated files are distinct physical sources.
     * They are not historical generations of the
     * current active path.
     */
    QCOMPARE(
        rotatedRecord
            ->source
            .sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        activeRecord
            ->source
            .sourceGeneration,
        ActiveGeneration
        );

    QCOMPARE(
        absolutePath(
            rotatedRecord
                ->source
                .sourcePath
            ),
        absolutePath(
            rotatedPath
            )
        );

    QCOMPARE(
        absolutePath(
            activeRecord
                ->source
                .sourcePath
            ),
        absolutePath(
            sourcePath
            )
        );

    const SourceFamilyConfiguration
        &restoredFamily =
        result.session
            ->sourceFamilyConfiguration();

    QVERIFY(
        restoredFamily.includeRotatedSources
        );

    QCOMPARE(
        restoredFamily
            .rotatedSourcePaths
            .size(),
        1
        );

    QCOMPARE(
        absolutePath(
            restoredFamily
                .rotatedSourcePaths
                .first()
            ),
        absolutePath(
            rotatedPath
            )
        );
}

void InvestigationSessionRestorationServiceTests::
    missingSourceBackedSourceFails()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString missingPath =
        directory.filePath(
            QStringLiteral("missing.jsonl")
            );

    PersistedInvestigationSession persisted =
        makeSourceBackedPersistence(
            QStringLiteral("missing-session"),
            missingPath,
            jsonLinesProfile()
            );

    InvestigationSessionRestorationService
        service;

    InvestigationSessionRestorationResult result =
        service.restore(
            persisted,
            directory.filePath(
                QStringLiteral("workspace.tsw")
                )
            );

    QVERIFY(!result.succeeded);

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(result.session == nullptr);
}

void InvestigationSessionRestorationServiceTests::
    snapshotBackedRestoresRelativeSnapshotWithoutSource()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("original.jsonl")
            );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArrayLiteral(
                "{\"message\":\"durable\"}\n"
                )
            )
        );

    const ImportProfile profile =
        jsonLinesProfile();

    const InvestigationSessionSnapshot snapshot =
        makeSnapshot(
            sourcePath,
            profile
            );

    QCOMPARE(
        snapshot.records.size(),
        1
        );

    const QString recordId =
        snapshot.records
            .first()
            .recordId;

    const QString sessionsDirectory =
        directory.filePath(
            QStringLiteral("sessions")
            );

    QVERIFY(
        QDir().mkpath(
            sessionsDirectory
            )
        );

    const QString snapshotPath =
        QDir(sessionsDirectory)
            .filePath(
                QStringLiteral(
                    "snapshot-session.tsinv"
                    )
                );

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

    /*
     * Snapshot-backed restoration must not depend on
     * the original source still existing.
     */
    QVERIFY(
        QFile::remove(
            sourcePath
            )
        );

    PersistedInvestigationSession persisted;

    persisted.sessionId =
        QStringLiteral(
            "snapshot-session"
            );

    persisted.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SnapshotBacked;

    persisted.backing.snapshotReference =
        QStringLiteral(
            "sessions/snapshot-session.tsinv"
            );

    PersistedInvestigationRecordState
        recordState;

    recordState.recordId =
        recordId;

    recordState.bookmarked =
        true;

    recordState.note =
        QStringLiteral(
            "Persisted evidence."
            );

    persisted.recordStates.append(
        recordState
        );

    InvestigationSessionRestorationService
        service;

    InvestigationSessionRestorationResult result =
        service.restore(
            persisted,
            directory.filePath(
                QStringLiteral("workspace.tsw")
                )
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(result.errorMessage)
        );

    QVERIFY(result.session != nullptr);

    QCOMPARE(
        result.session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QCOMPARE(
        result.session->importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        result.session
            ->investigationController()
            ->allRecords()
            .first()
            .recordId,
        recordId
        );

    const InvestigationRecordState
        restoredState =
        result.session
            ->investigationStateStore()
            ->stateForRecord(
                recordId
                );

    QVERIFY(
        restoredState.bookmarked
        );

    QCOMPARE(
        restoredState.note,
        QStringLiteral(
            "Persisted evidence."
            )
        );

    QVERIFY(
        !result.session
             ->backing()
             .hasExternalSource()
        );

    QVERIFY(
        result.session
            ->backing()
            .snapshot()
        != nullptr
        );

    QCOMPARE(
        absolutePath(
            result.session
                ->backing()
                .snapshot()
                ->snapshotPath
            ),
        absolutePath(
            snapshotPath
            )
        );
}

void InvestigationSessionRestorationServiceTests::
    hybridRestoresSnapshotWhenExternalSourceIsMissing()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.jsonl")
            );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArrayLiteral(
                "{\"message\":\"retained\"}\n"
                )
            )
        );

    constexpr quint64
        SavedGeneration = 3;

    const ImportProfile profile =
        jsonLinesProfile();

    const InvestigationSessionSnapshot snapshot =
        makeSnapshot(
            sourcePath,
            profile,
            SavedGeneration
            );

    QCOMPARE(
        snapshot.records.size(),
        1
        );

    const QString recordId =
        snapshot.records
            .first()
            .recordId;

    const std::optional<SourcePhysicalIdentity>
        identity =
        captureIdentity(
            sourcePath
            );

    QVERIFY(identity.has_value());

    const QString sessionsDirectory =
        directory.filePath(
            QStringLiteral("sessions")
            );

    QVERIFY(
        QDir().mkpath(
            sessionsDirectory
            )
        );

    const QString snapshotPath =
        QDir(sessionsDirectory)
            .filePath(
                QStringLiteral(
                    "hybrid-session.tsinv"
                    )
                );

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

    /*
     * The producer/source is gone, but durable
     * investigation evidence still exists.
     */
    QVERIFY(
        QFile::remove(
            sourcePath
            )
        );

    PersistedInvestigationSession persisted;

    persisted.sessionId =
        QStringLiteral(
            "hybrid-session"
            );

    persisted.backing.mode =
        PersistedInvestigationSessionBackingMode::
        Hybrid;

    persisted.backing.snapshotReference =
        QStringLiteral(
            "sessions/hybrid-session.tsinv"
            );

    PersistedInvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration =
        SavedGeneration;

    binding.sourceIdentity =
        *identity;

    persisted.backing.externalSourceBinding =
        binding;

    PersistedInvestigationRecordState
        recordState;

    recordState.recordId =
        recordId;

    recordState.bookmarked =
        true;

    persisted.recordStates.append(
        recordState
        );

    InvestigationSessionRestorationService
        service;

    InvestigationSessionRestorationResult result =
        service.restore(
            persisted,
            directory.filePath(
                QStringLiteral("workspace.tsw")
                )
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(result.errorMessage)
        );

    QVERIFY(result.session != nullptr);

    QCOMPARE(
        result.session->backing().mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QCOMPARE(
        result.session->importedRecordCount(),
        qint64(1)
        );

    const QVector<InvestigationRecord> records =
        result.session
            ->investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        1
        );

    QCOMPARE(
        records.first().recordId,
        recordId
        );

    QCOMPARE(
        records
            .first()
            .source
            .sourceGeneration,
        SavedGeneration
        );

    const InvestigationExternalSourceBinding
        *restoredBinding =
        result.session
            ->backing()
            .externalSource();

    QVERIFY(restoredBinding != nullptr);

    QCOMPARE(
        restoredBinding->sourceGeneration,
        SavedGeneration
        );

    QCOMPARE(
        absolutePath(
            restoredBinding->sourcePath
            ),
        absolutePath(
            sourcePath
            )
        );

    QVERIFY(
        restoredBinding
            ->sourceIdentity
            .has_value()
        );

    QVERIFY(
        result.session
            ->investigationStateStore()
            ->bookmarkedRecordIds()
            .contains(
                recordId
                )
        );

    QCOMPARE(
        result.session
            ->initialLiveFollowByteOffset(),
        qint64(0)
        );
}

QTEST_MAIN(
    InvestigationSessionRestorationServiceTests
    )

#include "InvestigationSessionRestorationServiceTests.moc"