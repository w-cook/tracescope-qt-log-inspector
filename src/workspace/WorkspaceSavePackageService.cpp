#include "WorkspaceSavePackageService.h"

#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QUuid>

#include "../persistence/InvestigationSessionSnapshotFile.h"
#include "WorkspaceSerialization.h"

namespace
{
WorkspaceSavePackageResult failure(
    const QString &errorMessage
    )
{
    WorkspaceSavePackageResult result;

    result.succeeded = false;
    result.errorMessage = errorMessage;

    return result;
}

void removeWrittenSnapshots(
    const QHash<QString, QString>
        &snapshotPathsBySessionId
    )
{
    for (const QString &snapshotPath
         : snapshotPathsBySessionId) {
        QFile::remove(
            snapshotPath
            );
    }
}

QString workspaceSnapshotDirectoryName(
    const QFileInfo &workspaceInfo
    )
{
    QString baseName =
        workspaceInfo.completeBaseName();

    if (baseName.trimmed().isEmpty()) {
        baseName =
            workspaceInfo.fileName();
    }

    if (baseName.trimmed().isEmpty()) {
        baseName =
            QStringLiteral("workspace");
    }

    return baseName
           + QStringLiteral(".sessions");
}

QString createSnapshotFileName(
    int sessionIndex
    )
{
    return QStringLiteral(
               "snapshot-%1-%2.tsinv"
               )
        .arg(sessionIndex)
        .arg(
            QUuid::createUuid()
                .toString(
                    QUuid::WithoutBraces
                    )
            );
}

QString relativeWorkspaceReference(
    const QDir &workspaceDirectory,
    const QString &absolutePath
    )
{
    /*
     * Persist slash-separated relative references even
     * on Windows. QDir resolves them portably when the
     * workspace is reopened.
     */
    return QDir::fromNativeSeparators(
        workspaceDirectory.relativeFilePath(
            absolutePath
            )
        );
}

void removeSupersededSnapshots(
    const QDir &snapshotDirectory,
    const QHash<QString, QString>
        &currentSnapshotPaths
    )
{
    QSet<QString> retainedPaths;

    retainedPaths.reserve(
        currentSnapshotPaths.size()
        );

    for (const QString &path
         : currentSnapshotPaths) {
        retainedPaths.insert(
            QFileInfo(path)
                .absoluteFilePath()
            );
    }

    const QFileInfoList existingSnapshots =
        snapshotDirectory.entryInfoList(
            {
                QStringLiteral("*.tsinv")
            },
            QDir::Files
                | QDir::NoSymLinks
            );

    for (const QFileInfo &snapshotInfo
         : existingSnapshots) {
        const QString absolutePath =
            snapshotInfo.absoluteFilePath();

        if (retainedPaths.contains(
                absolutePath
                )) {
            continue;
        }

        /*
         * Cleanup happens only after the new manifest
         * commits successfully.
         *
         * A failed deletion leaves a harmless orphan;
         * it must never invalidate the newly committed
         * workspace package.
         */
        QFile::remove(
            absolutePath
            );
    }
}
}

WorkspaceSavePackageResult
WorkspaceSavePackageService::save(
    const QString &workspaceFilePath,
    WorkspacePersistenceState state,
    QVector<WorkspaceSessionSnapshotSaveItem>
        sessionSnapshots
    ) const
{
    if (workspaceFilePath.trimmed().isEmpty()) {
        return failure(
            QStringLiteral(
                "The workspace save path is empty."
                )
            );
    }

    const QFileInfo workspaceInfo(
        workspaceFilePath
        );

    if (workspaceInfo.exists()
        && workspaceInfo.isDir()) {
        return failure(
            QStringLiteral(
                "The selected workspace path refers "
                "to a directory rather than a file."
                )
            );
    }

    if (state.sessions.size()
        != sessionSnapshots.size()) {
        return failure(
            QStringLiteral(
                "Workspace snapshot capture does not "
                "match the persisted session count."
                )
            );
    }

    /*
     * Validate the snapshot set completely before
     * writing anything.
     */
    QHash<QString, int>
        snapshotIndexBySessionId;

    snapshotIndexBySessionId.reserve(
        sessionSnapshots.size()
        );

    for (int index = 0;
         index < sessionSnapshots.size();
         ++index) {
        const QString sessionId =
            sessionSnapshots[index]
                .sessionId
                .trimmed();

        if (sessionId.isEmpty()) {
            return failure(
                QStringLiteral(
                    "A workspace session snapshot has "
                    "no session ID."
                    )
                );
        }

        if (snapshotIndexBySessionId.contains(
                sessionId
                )) {
            return failure(
                QStringLiteral(
                    "The workspace contains duplicate "
                    "session snapshot IDs."
                    )
                );
        }

        snapshotIndexBySessionId.insert(
            sessionId,
            index
            );
    }

    QSet<QString>
        persistedSessionIds;

    persistedSessionIds.reserve(
        state.sessions.size()
        );

    for (const PersistedInvestigationSession
             &persistedSession
         : std::as_const(state.sessions)) {
        const QString sessionId =
            persistedSession
                .sessionId
                .trimmed();

        if (sessionId.isEmpty()) {
            return failure(
                QStringLiteral(
                    "A persisted workspace session has "
                    "no session ID."
                    )
                );
        }

        if (persistedSessionIds.contains(
                sessionId
                )) {
            return failure(
                QStringLiteral(
                    "The workspace contains duplicate "
                    "persisted session IDs."
                    )
                );
        }

        persistedSessionIds.insert(
            sessionId
            );

        if (!snapshotIndexBySessionId.contains(
                sessionId
                )) {
            return failure(
                QStringLiteral(
                    "No durable snapshot was captured "
                    "for persisted session '%1'."
                    )
                    .arg(sessionId)
                );
        }
    }

    const QDir workspaceDirectory(
        workspaceInfo.absolutePath()
        );

    /*
     * Keep every workspace's sidecar snapshots isolated
     * from other workspaces saved in the same folder.
     */
    const QString snapshotDirectoryPath =
        workspaceDirectory.absoluteFilePath(
            workspaceSnapshotDirectoryName(
                workspaceInfo
                )
            );

    if (!QDir().mkpath(
            snapshotDirectoryPath
            )) {
        return failure(
            QStringLiteral(
                "TraceScope could not create the "
                "workspace snapshot directory."
                )
            );
    }

    const QDir snapshotDirectory(
        snapshotDirectoryPath
        );

    InvestigationSessionSnapshotFile
        snapshotFile;

    QHash<QString, QString>
        writtenSnapshotPaths;

    /*
     * Snapshots are always written before the manifest.
     *
     * Each target is unique. Therefore an existing
     * workspace manifest can never begin referring to
     * newly modified contents merely because a save
     * partially succeeded.
     */
    for (int sessionIndex = 0;
         sessionIndex < state.sessions.size();
         ++sessionIndex) {
        PersistedInvestigationSession
            &persistedSession =
            state.sessions[
                sessionIndex
        ];

        const int snapshotIndex =
            snapshotIndexBySessionId.value(
                persistedSession.sessionId,
                -1
                );

        if (snapshotIndex < 0
            || snapshotIndex
                   >= sessionSnapshots.size()) {
            removeWrittenSnapshots(
                writtenSnapshotPaths
                );

            return failure(
                QStringLiteral(
                    "TraceScope could not match a "
                    "workspace session to its durable "
                    "snapshot."
                    )
                );
        }

        const QString snapshotPath =
            snapshotDirectory.absoluteFilePath(
                createSnapshotFileName(
                    sessionIndex
                    )
                );

        const InvestigationSessionSnapshotSaveResult
            snapshotSaveResult =
            snapshotFile.save(
                snapshotPath,
                sessionSnapshots[
                    snapshotIndex
        ].snapshot
                );

        if (!snapshotSaveResult.isSuccess()) {
            removeWrittenSnapshots(
                writtenSnapshotPaths
                );

            QFile::remove(
                snapshotPath
                );

            return failure(
                snapshotSaveResult
                        .errorMessage
                        .isEmpty()
                    ? QStringLiteral(
                          "TraceScope could not save "
                          "one of the investigation "
                          "snapshots."
                          )
                    : snapshotSaveResult
                          .errorMessage
                );
        }

        writtenSnapshotPaths.insert(
            persistedSession.sessionId,
            QFileInfo(snapshotPath)
                .absoluteFilePath()
            );

        /*
         * This is save policy, not runtime mode.
         *
         * SourceBacked remains SourceBacked while
         * gaining an explicit snapshot fallback.
         * SnapshotBacked and Hybrid retain their modes
         * and simply point the saved manifest at the
         * newly captured durable snapshot.
         */
        persistedSession
            .backing
            .snapshotReference =
            relativeWorkspaceReference(
                workspaceDirectory,
                snapshotPath
                );
    }

    const QByteArray manifestJson =
        WorkspaceSerializer().serialize(
            state
            );

    if (manifestJson.isEmpty()) {
        removeWrittenSnapshots(
            writtenSnapshotPaths
            );

        return failure(
            QStringLiteral(
                "TraceScope could not serialize the "
                "workspace manifest."
                )
            );
    }

    /*
     * Commit the manifest last.
     *
     * QSaveFile ensures that failure cannot partially
     * replace an already valid workspace manifest.
     */
    QSaveFile manifestFile(
        workspaceInfo.absoluteFilePath()
        );

    if (!manifestFile.open(
            QIODevice::WriteOnly
            )) {
        removeWrittenSnapshots(
            writtenSnapshotPaths
            );

        return failure(
            QStringLiteral(
                "TraceScope could not open the "
                "workspace manifest for writing."
                )
            );
    }

    if (manifestFile.write(
            manifestJson
            )
        != manifestJson.size()) {
        manifestFile.cancelWriting();

        removeWrittenSnapshots(
            writtenSnapshotPaths
            );

        return failure(
            QStringLiteral(
                "TraceScope could not write the "
                "complete workspace manifest."
                )
            );
    }

    if (!manifestFile.commit()) {
        removeWrittenSnapshots(
            writtenSnapshotPaths
            );

        return failure(
            QStringLiteral(
                "TraceScope could not atomically "
                "commit the workspace manifest."
                )
            );
    }

    /*
     * The manifest now references only the newly written
     * immutable snapshots. Older .tsinv files in this
     * workspace's dedicated sidecar directory are no
     * longer authoritative and can safely be removed.
     *
     * If TraceScope crashes before this cleanup, the old
     * files are merely orphans. A later successful save
     * will remove them.
     */
    removeSupersededSnapshots(
        snapshotDirectory,
        writtenSnapshotPaths
        );

    WorkspaceSavePackageResult result;

    result.succeeded = true;

    result.savedState =
        std::move(state);

    result.snapshotPathsBySessionId =
        std::move(
            writtenSnapshotPaths
            );

    return result;
}