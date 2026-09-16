#include <QtTest>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/workspace/WorkspaceSavePackageService.h"
#include "../src/workspace/WorkspaceSerialization.h"

namespace
{
ImportProfile makeProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Workspace Save Test");

    profile.importerId =
        QStringLiteral("json-lines");

    return profile;
}

InvestigationSessionSnapshot makeSnapshot(
    const QString &recordId,
    const QString &message
    )
{
    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile =
        makeProfile();

    InvestigationRecord record;

    record.recordId =
        recordId;

    record.message =
        message;

    record.rawSource =
        message;

    snapshot.records.append(
        std::move(record)
        );

    snapshot.processedRecordCount =
        1;

    return snapshot;
}

PersistedInvestigationSession
makeSourceBackedSession(
    const QString &sessionId,
    const QString &sourcePath
    )
{
    PersistedInvestigationSession session;

    session.sessionId =
        sessionId;

    session.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    PersistedInvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    session.backing.externalSourceBinding =
        binding;

    session.backing.sourceImportProfile =
        makeProfile();

    return session;
}

PersistedInvestigationSession
makeSnapshotBackedSession(
    const QString &sessionId
    )
{
    PersistedInvestigationSession session;

    session.sessionId =
        sessionId;

    session.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SnapshotBacked;

    /*
     * No snapshotReference yet.
     *
     * WorkspaceSavePackageService owns assigning the
     * newly captured durable snapshot reference before
     * serializing the manifest.
     */
    return session;
}

PersistedInvestigationSession
makeHybridSession(
    const QString &sessionId,
    const QString &sourcePath
    )
{
    PersistedInvestigationSession session;

    session.sessionId =
        sessionId;

    session.backing.mode =
        PersistedInvestigationSessionBackingMode::
        Hybrid;

    PersistedInvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration =
        3;

    session.backing.externalSourceBinding =
        binding;

    /*
     * As with SnapshotBacked, the new durable snapshot
     * reference is supplied by save packaging.
     */
    return session;
}

WorkspaceSessionSnapshotSaveItem
makeSnapshotItem(
    const QString &sessionId,
    const QString &recordId,
    const QString &message
    )
{
    WorkspaceSessionSnapshotSaveItem item;

    item.sessionId =
        sessionId;

    item.snapshot =
        makeSnapshot(
            recordId,
            message
            );

    return item;
}

QString resolveReference(
    const QString &workspacePath,
    const QString &reference
    )
{
    return QDir(
               QFileInfo(workspacePath)
                   .absolutePath()
               )
        .absoluteFilePath(
            reference
            );
}

WorkspacePersistenceState
loadWorkspaceState(
    const QString &workspacePath
    )
{
    QFile file(
        workspacePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        return {};
    }

    const WorkspaceDeserializationResult result =
        WorkspaceSerializer()
            .deserialize(
                file.readAll()
                );

    if (!result.isSuccess()) {
        return {};
    }

    return *result.workspace;
}
}

class WorkspaceSavePackageServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void savesSelfContainedWorkspacePackage();
    void repeatedSaveUsesNewImmutableSnapshots();
    void invalidSnapshotSetWritesNothing();
};

void WorkspaceSavePackageServiceTests::
    savesSelfContainedWorkspacePackage()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString workspacePath =
        directory.filePath(
            QStringLiteral(
                "incident-review.tsw"
                )
            );

    WorkspacePersistenceState state;

    state.sessions.append(
        makeSourceBackedSession(
            QStringLiteral("source"),
            directory.filePath(
                QStringLiteral("source.jsonl")
                )
            )
        );

    state.sessions.append(
        makeSnapshotBackedSession(
            QStringLiteral("snapshot")
            )
        );

    state.sessions.append(
        makeHybridSession(
            QStringLiteral("hybrid"),
            directory.filePath(
                QStringLiteral("live.jsonl")
                )
            )
        );

    QVector<WorkspaceSessionSnapshotSaveItem>
        snapshots;

    snapshots.append(
        makeSnapshotItem(
            QStringLiteral("source"),
            QStringLiteral("record-source"),
            QStringLiteral(
                "source evidence"
                )
            )
        );

    snapshots.append(
        makeSnapshotItem(
            QStringLiteral("snapshot"),
            QStringLiteral("record-snapshot"),
            QStringLiteral(
                "snapshot evidence"
                )
            )
        );

    snapshots.append(
        makeSnapshotItem(
            QStringLiteral("hybrid"),
            QStringLiteral("record-hybrid"),
            QStringLiteral(
                "hybrid evidence"
                )
            )
        );

    const WorkspaceSavePackageResult result =
        WorkspaceSavePackageService()
            .save(
                workspacePath,
                state,
                std::move(snapshots)
                );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        QFileInfo::exists(
            workspacePath
            )
        );

    const QString snapshotDirectoryPath =
        directory.filePath(
            QStringLiteral(
                "incident-review.sessions"
                )
            );

    QVERIFY(
        QFileInfo(
            snapshotDirectoryPath
            )
            .isDir()
        );

    QCOMPARE(
        result.savedState.sessions.size(),
        3
        );

    /*
     * Save policy must not silently change runtime /
     * persisted backing semantics.
     */
    QCOMPARE(
        result.savedState
            .sessions[0]
            .backing
            .mode,
        PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QCOMPARE(
        result.savedState
            .sessions[1]
            .backing
            .mode,
        PersistedInvestigationSessionBackingMode::
        SnapshotBacked
        );

    QCOMPARE(
        result.savedState
            .sessions[2]
            .backing
            .mode,
        PersistedInvestigationSessionBackingMode::
        Hybrid
        );

    const WorkspacePersistenceState
        loadedState =
        loadWorkspaceState(
            workspacePath
            );

    QCOMPARE(
        loadedState.sessions.size(),
        3
        );

    const QStringList expectedMessages {
        QStringLiteral("source evidence"),
        QStringLiteral("snapshot evidence"),
        QStringLiteral("hybrid evidence")
    };

    InvestigationSessionSnapshotFile
        snapshotFile;

    for (int index = 0;
         index < loadedState.sessions.size();
         ++index) {
        const PersistedInvestigationSession
            &session =
            loadedState.sessions[index];

        QVERIFY(
            session
                .backing
                .snapshotReference
                .has_value()
            );

        const QString reference =
            *session
                 .backing
                 .snapshotReference;

        QVERIFY(
            !reference.isEmpty()
            );

        /*
         * Workspace manifests must remain movable with
         * their sidecar directory.
         */
        QVERIFY(
            !QFileInfo(reference)
                 .isAbsolute()
            );

        QVERIFY(
            reference.startsWith(
                QStringLiteral(
                    "incident-review.sessions/"
                    )
                )
            );

        const QString snapshotPath =
            resolveReference(
                workspacePath,
                reference
                );

        QVERIFY(
            QFileInfo::exists(
                snapshotPath
                )
            );

        QVERIFY(
            result
                .snapshotPathsBySessionId
                .contains(
                    session.sessionId
                    )
            );

        QCOMPARE(
            QFileInfo(
                result
                    .snapshotPathsBySessionId
                    .value(
                        session.sessionId
                        )
                )
                .absoluteFilePath(),
            QFileInfo(snapshotPath)
                .absoluteFilePath()
            );

        const InvestigationSessionSnapshotLoadResult
            snapshotLoad =
            snapshotFile.load(
                snapshotPath
                );

        QVERIFY2(
            snapshotLoad.isSuccess(),
            qPrintable(
                snapshotLoad.errorMessage
                )
            );

        QCOMPARE(
            snapshotLoad
                .snapshot
                ->records
                .size(),
            1
            );

        QVERIFY(
            snapshotLoad
                .snapshot
                ->records
                .first()
                .message
                .has_value()
            );

        QCOMPARE(
            *snapshotLoad
                 .snapshot
                 ->records
                 .first()
                 .message,
            expectedMessages[index]
            );
    }

    /*
     * SourceBacked gains a durable fallback without
     * losing its external source authority.
     */
    QVERIFY(
        loadedState
            .sessions[0]
            .backing
            .externalSourceBinding
            .has_value()
        );

    QVERIFY(
        loadedState
            .sessions[0]
            .backing
            .sourceImportProfile
            .has_value()
        );

    /*
     * Hybrid likewise retains its external continuation
     * binding while gaining the newly saved snapshot.
     */
    QVERIFY(
        loadedState
            .sessions[2]
            .backing
            .externalSourceBinding
            .has_value()
        );
}

void WorkspaceSavePackageServiceTests::
    repeatedSaveUsesNewImmutableSnapshots()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString workspacePath =
        directory.filePath(
            QStringLiteral(
                "repeat-save.tsw"
                )
            );

    WorkspacePersistenceState firstState;

    firstState.sessions.append(
        makeSourceBackedSession(
            QStringLiteral("session"),
            directory.filePath(
                QStringLiteral("source.jsonl")
                )
            )
        );

    QVector<WorkspaceSessionSnapshotSaveItem>
        firstSnapshots;

    firstSnapshots.append(
        makeSnapshotItem(
            QStringLiteral("session"),
            QStringLiteral("record-1"),
            QStringLiteral("first save")
            )
        );

    const WorkspaceSavePackageResult
        firstResult =
        WorkspaceSavePackageService()
            .save(
                workspacePath,
                firstState,
                std::move(
                    firstSnapshots
                    )
                );

    QVERIFY2(
        firstResult.succeeded,
        qPrintable(
            firstResult.errorMessage
            )
        );

    const QString firstSnapshotPath =
        firstResult
            .snapshotPathsBySessionId
            .value(
                QStringLiteral("session")
                );

    QVERIFY(
        !firstSnapshotPath.isEmpty()
        );

    QVERIFY(
        QFileInfo::exists(
            firstSnapshotPath
            )
        );

    const QString firstReference =
        *firstResult
             .savedState
             .sessions
             .first()
             .backing
             .snapshotReference;

    WorkspacePersistenceState secondState =
        firstState;

    QVector<WorkspaceSessionSnapshotSaveItem>
        secondSnapshots;

    secondSnapshots.append(
        makeSnapshotItem(
            QStringLiteral("session"),
            QStringLiteral("record-2"),
            QStringLiteral("second save")
            )
        );

    const WorkspaceSavePackageResult
        secondResult =
        WorkspaceSavePackageService()
            .save(
                workspacePath,
                secondState,
                std::move(
                    secondSnapshots
                    )
                );

    QVERIFY2(
        secondResult.succeeded,
        qPrintable(
            secondResult.errorMessage
            )
        );

    const QString secondSnapshotPath =
        secondResult
            .snapshotPathsBySessionId
            .value(
                QStringLiteral("session")
                );

    QVERIFY(
        !secondSnapshotPath.isEmpty()
        );

    QVERIFY(
        firstSnapshotPath
        != secondSnapshotPath
        );

    /*
     * The previous durable target is intentionally not
     * overwritten or removed by the next save.
     */
    QVERIFY(
        QFileInfo::exists(
            firstSnapshotPath
            )
        );

    QVERIFY(
        QFileInfo::exists(
            secondSnapshotPath
            )
        );

    const QString secondReference =
        *secondResult
             .savedState
             .sessions
             .first()
             .backing
             .snapshotReference;

    QVERIFY(
        firstReference
        != secondReference
        );

    InvestigationSessionSnapshotFile
        snapshotFile;

    const auto firstLoad =
        snapshotFile.load(
            firstSnapshotPath
            );

    const auto secondLoad =
        snapshotFile.load(
            secondSnapshotPath
            );

    QVERIFY(firstLoad.isSuccess());
    QVERIFY(secondLoad.isSuccess());

    QCOMPARE(
        *firstLoad
             .snapshot
             ->records
             .first()
             .message,
        QStringLiteral("first save")
        );

    QCOMPARE(
        *secondLoad
             .snapshot
             ->records
             .first()
             .message,
        QStringLiteral("second save")
        );

    /*
     * The workspace manifest committed by the second
     * save must reference only the second immutable
     * snapshot target.
     */
    const WorkspacePersistenceState
        loadedState =
        loadWorkspaceState(
            workspacePath
            );

    QCOMPARE(
        loadedState.sessions.size(),
        1
        );

    QVERIFY(
        loadedState
            .sessions
            .first()
            .backing
            .snapshotReference
            .has_value()
        );

    QCOMPARE(
        *loadedState
             .sessions
             .first()
             .backing
             .snapshotReference,
        secondReference
        );
}

void WorkspaceSavePackageServiceTests::
    invalidSnapshotSetWritesNothing()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString workspacePath =
        directory.filePath(
            QStringLiteral(
                "invalid-save.tsw"
                )
            );

    WorkspacePersistenceState state;

    state.sessions.append(
        makeSourceBackedSession(
            QStringLiteral("session"),
            directory.filePath(
                QStringLiteral("source.jsonl")
                )
            )
        );

    /*
     * Intentionally omit the required snapshot item.
     * Validation should fail before any package files
     * are created.
     */
    QVector<WorkspaceSessionSnapshotSaveItem>
        snapshots;

    const WorkspaceSavePackageResult result =
        WorkspaceSavePackageService()
            .save(
                workspacePath,
                state,
                std::move(snapshots)
                );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        !QFileInfo::exists(
            workspacePath
            )
        );

    QVERIFY(
        !QFileInfo::exists(
            directory.filePath(
                QStringLiteral(
                    "invalid-save.sessions"
                    )
                )
            )
        );
}

QTEST_APPLESS_MAIN(
    WorkspaceSavePackageServiceTests
    )

#include "WorkspaceSavePackageServiceTests.moc"