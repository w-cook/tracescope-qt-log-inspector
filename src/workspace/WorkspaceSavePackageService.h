#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include "../persistence/InvestigationSessionSnapshot.h"
#include "WorkspacePersistenceState.h"

struct WorkspaceSessionSnapshotSaveItem
{
    QString sessionId;

    InvestigationSessionSnapshot snapshot;
};

struct WorkspaceSavePackageResult
{
    bool succeeded = false;

    QString errorMessage;

    /*
     * Exact persistence state written to the workspace
     * manifest, including the newly generated relative
     * snapshot references.
     */
    WorkspacePersistenceState savedState;

    /*
     * Absolute snapshot paths written by this save,
     * indexed by investigation session ID.
     *
     * MainWindow will later use this to update runtime
     * SnapshotBacked/Hybrid bindings after the manifest
     * commits successfully.
     */
    QHash<QString, QString>
        snapshotPathsBySessionId;
};

class WorkspaceSavePackageService
{
public:
    WorkspaceSavePackageResult save(
        const QString &workspaceFilePath,
        WorkspacePersistenceState state,
        QVector<WorkspaceSessionSnapshotSaveItem>
            sessionSnapshots
        ) const;
};