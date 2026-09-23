#include <QtTest>

#include <QFileInfo>

#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/workspace/WorkspaceWorkingArtifactStore.h"

namespace
{
InvestigationSessionSnapshot makeSnapshot(
    const QString &recordId,
    const QString &message
    )
{
    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile.name =
        QStringLiteral(
            "Working Artifact Test"
            );

    snapshot.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    InvestigationRecord record;

    record.recordId =
        recordId;

    record.message =
        message;

    record.rawSource =
        message;

    snapshot.records.append(
        std::move(
            record
            )
        );

    snapshot.processedRecordCount =
        1;

    return snapshot;
}
}

class WorkspaceWorkingArtifactStoreTests
    : public QObject
{
    Q_OBJECT

private slots:
    void directoryIsCreatedLazily();

    void snapshotIsWrittenIntoWorkingDirectory();

    void resetRemovesEntireWorkingDirectory();

    void destructionRemovesEntireWorkingDirectory();
};

void WorkspaceWorkingArtifactStoreTests::
    directoryIsCreatedLazily()
{
    WorkspaceWorkingArtifactStore store;

    QVERIFY(
        !store.hasWorkingDirectory()
        );

    QVERIFY(
        store
            .workingDirectoryPath()
            .isEmpty()
        );
}

void WorkspaceWorkingArtifactStoreTests::
    snapshotIsWrittenIntoWorkingDirectory()
{
    WorkspaceWorkingArtifactStore store;

    const WorkspaceWorkingSnapshotSaveResult
        saveResult =
        store.saveSnapshot(
            makeSnapshot(
                QStringLiteral("record-1"),
                QStringLiteral("temporary evidence")
                )
            );

    QVERIFY2(
        saveResult.isSuccess(),
        qPrintable(
            saveResult.errorMessage
            )
        );

    QVERIFY(
        store.hasWorkingDirectory()
        );

    QVERIFY(
        !store
             .workingDirectoryPath()
             .isEmpty()
        );

    QVERIFY(
        QFileInfo::exists(
            saveResult.snapshotPath
            )
        );

    QCOMPARE(
        QFileInfo(
            saveResult.snapshotPath
            )
            .absolutePath(),
        QFileInfo(
            store.workingDirectoryPath()
            )
            .absoluteFilePath()
        );

    const InvestigationSessionSnapshotLoadResult
        loadResult =
        InvestigationSessionSnapshotFile()
            .load(
                saveResult.snapshotPath
                );

    QVERIFY2(
        loadResult.isSuccess(),
        qPrintable(
            loadResult.errorMessage
            )
        );

    QCOMPARE(
        loadResult
            .snapshot
            ->records
            .size(),
        1
        );

    QVERIFY(
        loadResult
            .snapshot
            ->records
            .first()
            .message
            .has_value()
        );

    QCOMPARE(
        *loadResult
             .snapshot
             ->records
             .first()
             .message,
        QStringLiteral(
            "temporary evidence"
            )
        );
}

void WorkspaceWorkingArtifactStoreTests::
    resetRemovesEntireWorkingDirectory()
{
    WorkspaceWorkingArtifactStore store;

    const WorkspaceWorkingSnapshotSaveResult
        firstSave =
        store.saveSnapshot(
            makeSnapshot(
                QStringLiteral("record-1"),
                QStringLiteral("first")
                )
            );

    QVERIFY2(
        firstSave.isSuccess(),
        qPrintable(
            firstSave.errorMessage
            )
        );

    const WorkspaceWorkingSnapshotSaveResult
        secondSave =
        store.saveSnapshot(
            makeSnapshot(
                QStringLiteral("record-2"),
                QStringLiteral("second")
                )
            );

    QVERIFY2(
        secondSave.isSuccess(),
        qPrintable(
            secondSave.errorMessage
            )
        );

    const QString workingDirectoryPath =
        store.workingDirectoryPath();

    QVERIFY(
        QFileInfo::exists(
            workingDirectoryPath
            )
        );

    QVERIFY(
        QFileInfo::exists(
            firstSave.snapshotPath
            )
        );

    QVERIFY(
        QFileInfo::exists(
            secondSave.snapshotPath
            )
        );

    QString cleanupError;

    QVERIFY2(
        store.reset(
            &cleanupError
            ),
        qPrintable(
            cleanupError
            )
        );

    QVERIFY(
        !store.hasWorkingDirectory()
        );

    QVERIFY(
        store
            .workingDirectoryPath()
            .isEmpty()
        );

    QVERIFY(
        !QFileInfo::exists(
            workingDirectoryPath
            )
        );

    QVERIFY(
        !QFileInfo::exists(
            firstSave.snapshotPath
            )
        );

    QVERIFY(
        !QFileInfo::exists(
            secondSave.snapshotPath
            )
        );

    /*
     * The store remains reusable for the next working
     * workspace after cleanup.
     */
    const WorkspaceWorkingSnapshotSaveResult
        thirdSave =
        store.saveSnapshot(
            makeSnapshot(
                QStringLiteral("record-3"),
                QStringLiteral("third")
                )
            );

    QVERIFY2(
        thirdSave.isSuccess(),
        qPrintable(
            thirdSave.errorMessage
            )
        );

    QVERIFY(
        store.hasWorkingDirectory()
        );

    QVERIFY(
        QFileInfo::exists(
            thirdSave.snapshotPath
            )
        );
}

void WorkspaceWorkingArtifactStoreTests::
    destructionRemovesEntireWorkingDirectory()
{
    QString workingDirectoryPath;
    QString snapshotPath;

    {
        WorkspaceWorkingArtifactStore store;

        const WorkspaceWorkingSnapshotSaveResult
            saveResult =
            store.saveSnapshot(
                makeSnapshot(
                    QStringLiteral("record-1"),
                    QStringLiteral("destructor cleanup")
                    )
                );

        QVERIFY2(
            saveResult.isSuccess(),
            qPrintable(
                saveResult.errorMessage
                )
            );

        workingDirectoryPath =
            store.workingDirectoryPath();

        snapshotPath =
            saveResult.snapshotPath;

        QVERIFY(
            QFileInfo::exists(
                workingDirectoryPath
                )
            );

        QVERIFY(
            QFileInfo::exists(
                snapshotPath
                )
            );
    }

    /*
     * QTemporaryDir auto-removal is the safety net for
     * normal application destruction.
     */
    QVERIFY(
        !QFileInfo::exists(
            workingDirectoryPath
            )
        );

    QVERIFY(
        !QFileInfo::exists(
            snapshotPath
            )
        );
}

QTEST_MAIN(
    WorkspaceWorkingArtifactStoreTests
    )

#include "WorkspaceWorkingArtifactStoreTests.moc"