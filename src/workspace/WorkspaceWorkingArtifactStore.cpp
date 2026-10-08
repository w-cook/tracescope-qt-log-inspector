#include "WorkspaceWorkingArtifactStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUuid>

#include "../persistence/InvestigationSessionSnapshotFile.h"

WorkspaceWorkingArtifactStore::
    WorkspaceWorkingArtifactStore() =
    default;

WorkspaceWorkingArtifactStore::
    ~WorkspaceWorkingArtifactStore() =
    default;

bool WorkspaceWorkingArtifactStore::
    hasWorkingDirectory() const
{
    return m_workingDirectory != nullptr
           && m_workingDirectory->isValid();
}

QString WorkspaceWorkingArtifactStore::
    workingDirectoryPath() const
{
    if (!hasWorkingDirectory()) {
        return {};
    }

    return m_workingDirectory->path();
}

bool WorkspaceWorkingArtifactStore::
    ensureWorkingDirectory(
        QString *errorMessage
        )
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    if (hasWorkingDirectory()) {
        return true;
    }

    const QString templatePath =
        QDir(
            QDir::tempPath()
            )
            .filePath(
                QStringLiteral(
                    "TraceScope-workspace-XXXXXX"
                    )
                );

    auto workingDirectory =
        std::make_unique<QTemporaryDir>(
            templatePath
            );

    /*
     * This is the final safety net for normal object
     * destruction. Explicit workspace lifecycle
     * operations will also call reset().
     */
    workingDirectory->setAutoRemove(
        true
        );

    if (!workingDirectory->isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage =
                workingDirectory
                        ->errorString()
                        .trimmed()
                        .isEmpty()
                    ? QStringLiteral(
                          "TraceScope could not create "
                          "temporary workspace storage."
                          )
                    : workingDirectory
                          ->errorString();
        }

        return false;
    }

    m_workingDirectory =
        std::move(
            workingDirectory
            );

    return true;
}

WorkspaceWorkingSnapshotSaveResult
    WorkspaceWorkingArtifactStore::
    saveSnapshot(
        const InvestigationSessionSnapshot &snapshot
        )
{
    WorkspaceWorkingSnapshotSaveResult result;

    QString directoryError;

    if (!ensureWorkingDirectory(
            &directoryError
            )) {
        result.errorMessage =
            directoryError;

        return result;
    }

    const QString snapshotFileName =
        QStringLiteral(
            "snapshot-%1.tsinv"
            )
            .arg(
                QUuid::createUuid()
                    .toString(
                        QUuid::WithoutBraces
                        )
                );

    const QString snapshotPath =
        QDir(
            m_workingDirectory->path()
            )
            .absoluteFilePath(
                snapshotFileName
                );

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        InvestigationSessionSnapshotFile()
            .save(
                snapshotPath,
                snapshot
                );

    if (!saveResult.isSuccess()) {
        /*
         * QSaveFile already protects against a partial
         * committed destination, but remove defensively
         * in case the underlying implementation ever
         * changes.
         */
        QFile::remove(
            snapshotPath
            );

        result.errorMessage =
            saveResult
                    .errorMessage
                    .trimmed()
                    .isEmpty()
                ? QStringLiteral(
                      "TraceScope could not create a "
                      "temporary investigation snapshot."
                      )
                : saveResult.errorMessage;

        return result;
    }

    result.succeeded = true;

    result.snapshotPath =
        QFileInfo(
            snapshotPath
            ).absoluteFilePath();

    return result;
}

bool WorkspaceWorkingArtifactStore::
    reset(
        QString *errorMessage
        )
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    if (m_workingDirectory == nullptr) {
        return true;
    }

    const QString directoryPath =
        m_workingDirectory->path();

    if (!m_workingDirectory->remove()) {
        if (errorMessage != nullptr) {
            const QString underlyingError =
                m_workingDirectory
                    ->errorString()
                    .trimmed();

            *errorMessage =
                underlyingError.isEmpty()
                    ? QStringLiteral(
                          "TraceScope could not remove "
                          "temporary workspace storage: "
                          "%1"
                          )
                          .arg(
                              directoryPath
                              )
                    : underlyingError;
        }

        /*
         * Keep ownership so cleanup can be retried.
         */
        return false;
    }

    m_workingDirectory.reset();

    return true;
}