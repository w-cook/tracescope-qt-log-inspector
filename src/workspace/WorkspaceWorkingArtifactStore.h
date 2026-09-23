#pragma once

#include <memory>

#include <QString>

#include "../persistence/InvestigationSessionSnapshot.h"

class QTemporaryDir;

struct WorkspaceWorkingSnapshotSaveResult
{
    bool succeeded = false;

    QString snapshotPath;
    QString errorMessage;

    bool isSuccess() const
    {
        return succeeded;
    }
};

class WorkspaceWorkingArtifactStore
{
public:
    WorkspaceWorkingArtifactStore();

    ~WorkspaceWorkingArtifactStore();

    WorkspaceWorkingArtifactStore(
        const WorkspaceWorkingArtifactStore &
        ) = delete;

    WorkspaceWorkingArtifactStore &
    operator=(
        const WorkspaceWorkingArtifactStore &
        ) = delete;

    bool hasWorkingDirectory() const;

    QString workingDirectoryPath() const;

    WorkspaceWorkingSnapshotSaveResult
    saveSnapshot(
        const InvestigationSessionSnapshot &snapshot
        );

    /*
     * Remove every artifact belonging to the current
     * uncommitted working workspace.
     *
     * On failure, retain the QTemporaryDir object so a
     * later cleanup attempt or destruction can retry.
     */
    bool reset(
        QString *errorMessage = nullptr
        );

private:
    bool ensureWorkingDirectory(
        QString *errorMessage
        );

    std::unique_ptr<QTemporaryDir>
        m_workingDirectory;
};