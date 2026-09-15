#include <QtTest>

#include <QFileInfo>

#include <memory>
#include <utility>

#include "../src/workspace/InvestigationWorkspace.h"

class InvestigationWorkspaceTests : public QObject
{
    Q_OBJECT

private slots:
    void startsEmpty();
    void addsAndActivatesSessions();
    void switchesActiveSession();
    void rejectsInvalidSessionIndexes();
    void closesInactiveSession();
    void closesActiveSession();
    void closesFinalSession();
    void reloadsSessionById();
    void newSessionUsesExplicitSourceBacking();
    void sourceFamilyConfigurationUsesExternalBinding();
    void capturesNormalizedInvestigationSnapshot();
    void createsSnapshotBackedSession();
};

static std::unique_ptr<InvestigationSession>
createSession(const QString &sourcePath)
{
    ImportProfile profile;
    ImportResult result;

    return std::make_unique<
        InvestigationSession>(
        sourcePath,
        std::move(profile),
        std::move(result)
        );
}

void InvestigationWorkspaceTests::startsEmpty()
{
    InvestigationWorkspace workspace;

    QCOMPARE(workspace.sessionCount(), 0);
    QCOMPARE(workspace.activeSessionIndex(), -1);

    QVERIFY(
        workspace.activeSession()
        == nullptr
        );
}

void InvestigationWorkspaceTests::
    addsAndActivatesSessions()
{
    InvestigationWorkspace workspace;

    const int firstIndex =
        workspace.addSession(
            createSession(
                QStringLiteral("first.jsonl")
                )
            );

    QCOMPARE(firstIndex, 0);
    QCOMPARE(workspace.sessionCount(), 1);
    QCOMPARE(workspace.activeSessionIndex(), 0);

    QVERIFY(
        workspace.activeSession()
        != nullptr
        );

    const int secondIndex =
        workspace.addSession(
            createSession(
                QStringLiteral("second.jsonl")
                )
            );

    QCOMPARE(secondIndex, 1);
    QCOMPARE(workspace.sessionCount(), 2);

    /*
     * Newly imported sessions become active.
     */
    QCOMPARE(workspace.activeSessionIndex(), 1);
}

void InvestigationWorkspaceTests::
    switchesActiveSession()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("first.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("second.jsonl")
            )
        );

    QVERIFY(
        workspace.setActiveSession(0)
        );

    QCOMPARE(
        workspace.activeSessionIndex(),
        0
        );

    QCOMPARE(
        workspace.activeSession()
            ->sourceMetadata()
            .sourceName,
        QStringLiteral("first.jsonl")
        );
}

void InvestigationWorkspaceTests::
    rejectsInvalidSessionIndexes()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("first.jsonl")
            )
        );

    QVERIFY(
        !workspace.setActiveSession(-1)
        );

    QVERIFY(
        !workspace.setActiveSession(1)
        );

    QVERIFY(
        !workspace.closeSession(-1)
        );

    QVERIFY(
        !workspace.closeSession(1)
        );
}

void InvestigationWorkspaceTests::
    closesInactiveSession()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("first.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("second.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("third.jsonl")
            )
        );

    QVERIFY(
        workspace.setActiveSession(1)
        );

    QVERIFY(
        workspace.closeSession(0)
        );

    QCOMPARE(workspace.sessionCount(), 2);

    /*
     * The same logical session remains active,
     * but its index moves down by one.
     */
    QCOMPARE(
        workspace.activeSessionIndex(),
        0
        );

    QCOMPARE(
        workspace.activeSession()
            ->sourceMetadata()
            .sourceName,
        QStringLiteral("second.jsonl")
        );
}

void InvestigationWorkspaceTests::
    closesActiveSession()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("first.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("second.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("third.jsonl")
            )
        );

    QVERIFY(
        workspace.setActiveSession(1)
        );

    QVERIFY(
        workspace.closeSession(1)
        );

    QCOMPARE(workspace.sessionCount(), 2);
    QCOMPARE(workspace.activeSessionIndex(), 1);

    QCOMPARE(
        workspace.activeSession()
            ->sourceMetadata()
            .sourceName,
        QStringLiteral("third.jsonl")
        );
}

void InvestigationWorkspaceTests::
    closesFinalSession()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("only.jsonl")
            )
        );

    QVERIFY(
        workspace.closeSession(0)
        );

    QCOMPARE(workspace.sessionCount(), 0);
    QCOMPARE(workspace.activeSessionIndex(), -1);

    QVERIFY(
        workspace.activeSession()
        == nullptr
        );
}

void InvestigationWorkspaceTests::
    reloadsSessionById()
{
    InvestigationWorkspace workspace;

    workspace.addSession(
        createSession(
            QStringLiteral("first.jsonl")
            )
        );

    workspace.addSession(
        createSession(
            QStringLiteral("second.jsonl")
            )
        );

    InvestigationSession *first =
        workspace.sessionAt(0);

    QVERIFY(first != nullptr);

    const QString firstId =
        first->id();

    ImportResult result;

    InvestigationRecord record;

    record.recordId =
        QStringLiteral("reloaded");

    result.records.append(record);

    QVERIFY(
        workspace.reloadSession(
            firstId,
            std::move(result)
            )
        );

    QCOMPARE(
        workspace.sessionAt(0)
            ->id(),
        firstId
        );

    QCOMPARE(
        workspace.sessionAt(0)
            ->investigationController()
            ->allRecords()
            .first()
            .recordId,
        QStringLiteral("reloaded")
        );

    QCOMPARE(
        workspace.activeSessionIndex(),
        1
        );
}

void InvestigationWorkspaceTests::
    newSessionUsesExplicitSourceBacking()
{
    const QString sourcePath =
        QStringLiteral(
            "gateway.jsonl"
            );

    std::unique_ptr<InvestigationSession>
        session =
        createSession(
            sourcePath
            );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        session->backing()
            .hasExternalSource()
        );

    QVERIFY(
        !session->backing()
             .hasSnapshot()
        );

    const InvestigationExternalSourceBinding
        *externalSource =
        session->backing()
            .externalSource();

    QVERIFY(
        externalSource != nullptr
        );

    const QString expectedPath =
        QFileInfo(
            sourcePath
            ).absoluteFilePath();

    QCOMPARE(
        externalSource->sourcePath,
        expectedPath
        );

    QCOMPARE(
        session->externalSourcePath(),
        expectedPath
        );

    /*
     * Existing Source-backed behavior remains
     * compatible: descriptive source metadata still
     * identifies the same active external file.
     */
    QCOMPARE(
        session
            ->sourceMetadata()
            .sourcePath,
        expectedPath
        );
}

void InvestigationWorkspaceTests::
    sourceFamilyConfigurationUsesExternalBinding()
{
    ImportProfile profile;
    ImportResult result;

    SourceFamilyConfiguration
        configuration;

    configuration.includeRotatedSources =
        true;

    InvestigationSession session(
        QStringLiteral(
            "gateway.jsonl"
            ),
        std::move(profile),
        std::move(result),
        configuration
        );

    const InvestigationExternalSourceBinding
        *externalSource =
        session.backing()
            .externalSource();

    QVERIFY(
        externalSource != nullptr
        );

    if (!externalSource) {
        return;
    }

    QVERIFY(
        externalSource
            ->sourceFamilyConfiguration
            .includeRotatedSources
        );

    SourceFamilyConfiguration
        replacement;

    replacement.includeRotatedSources =
        false;

    session.setSourceFamilyConfiguration(
        replacement
        );

    QVERIFY(
        !session
             .sourceFamilyConfiguration()
             .includeRotatedSources
        );

    QVERIFY(
        !session
             .backing()
             .externalSource()
             ->sourceFamilyConfiguration
             .includeRotatedSources
        );
}

void InvestigationWorkspaceTests::
    capturesNormalizedInvestigationSnapshot()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Gateway Profile"
            );

    ImportResult result;

    InvestigationRecord first;

    first.recordId =
        QStringLiteral("record-1");

    first.rawSource =
        QStringLiteral("first");

    first.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    first.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    first.source.recordNumber = 1;

    InvestigationRecord second;

    second.recordId =
        QStringLiteral("record-2");

    second.rawSource =
        QStringLiteral("second");

    second.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    second.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    second.source.recordNumber = 2;
    second.source.sourceGeneration = 3;

    result.records = {
        first,
        second
    };

    result.processedRecordCount = 3;
    result.sourceTruncated = true;

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral(
            "TEST_WARNING"
            );

    diagnostic.message =
        QStringLiteral(
            "Skipped test record"
            );

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    result.diagnostics.append(
        diagnostic
        );

    InvestigationSession session(
        QStringLiteral(
            "gateway.jsonl"
            ),
        std::move(profile),
        std::move(result)
        );

    /*
     * Workspace annotations deliberately do not
     * belong to the snapshot.
     */
    session.investigationStateStore()
        ->setBookmarked(
            QStringLiteral("record-1"),
            true
            );

    session.investigationStateStore()
        ->setNote(
            QStringLiteral("record-2"),
            QStringLiteral(
                "Investigate this"
                )
            );

    const InvestigationSessionSnapshot
        snapshot =
        session.captureSnapshot();

    QCOMPARE(
        snapshot.sourceFidelity,
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly
        );

    QCOMPARE(
        snapshot.importProfile.name,
        QStringLiteral(
            "Gateway Profile"
            )
        );

    QCOMPARE(
        snapshot.records.size(),
        2
        );

    QCOMPARE(
        snapshot.records.at(0).recordId,
        QStringLiteral("record-1")
        );

    QCOMPARE(
        snapshot.records.at(1).recordId,
        QStringLiteral("record-2")
        );

    QCOMPARE(
        snapshot
            .records
            .at(1)
            .source
            .sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        snapshot.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        snapshot.sourceTruncated,
        true
        );

    QCOMPARE(
        snapshot.diagnostics.size(),
        1
        );

    /*
     * Capture must not mutate or consume workspace
     * annotation state.
     */
    QVERIFY(
        session.investigationStateStore()
            ->stateForRecord(
                QStringLiteral("record-1")
                )
            .bookmarked
        );

    QCOMPARE(
        session.investigationStateStore()
            ->stateForRecord(
                QStringLiteral("record-2")
                )
            .note,
        QStringLiteral(
            "Investigate this"
            )
        );
}

void InvestigationWorkspaceTests::
    createsSnapshotBackedSession()
{
    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile.name =
        QStringLiteral(
            "Saved Profile"
            );

    snapshot.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    InvestigationRecord original;

    original.recordId =
        QStringLiteral(
            "historical-record"
            );

    original.rawSource =
        QStringLiteral(
            "historical"
            );

    original.source.sourcePath =
        QStringLiteral(
            "C:/logs/warehouse-live.jsonl"
            );

    original.source.sourceName =
        QStringLiteral(
            "warehouse-live.jsonl"
            );

    original.source.recordNumber = 8;
    original.source.sourceGeneration = 3;

    snapshot.records.append(
        original
        );

    snapshot.processedRecordCount = 1;

    const QString snapshotPath =
        QStringLiteral(
            "warehouse.tsinv"
            );

    std::unique_ptr<InvestigationSession>
        session =
        InvestigationSession::
        createSnapshotBacked(
            QStringLiteral(
                "restored-session"
                ),
            snapshotPath,
            std::move(snapshot)
            );

    QVERIFY(
        session != nullptr
        );

    QCOMPARE(
        session->id(),
        QStringLiteral(
            "restored-session"
            )
        );

    QCOMPARE(
        session->backing().mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        !session->backing()
             .hasExternalSource()
        );

    QVERIFY(
        session->backing()
            .hasSnapshot()
        );

    const InvestigationSnapshotBinding
        *snapshotBinding =
        session->backing()
            .snapshot();

    QVERIFY(
        snapshotBinding != nullptr
        );

    if (!snapshotBinding) {
        return;
    }

    QCOMPARE(
        snapshotBinding->snapshotPath,
        QFileInfo(
            snapshotPath
            ).absoluteFilePath()
        );

    QCOMPARE(
        snapshotBinding->sourceFidelity,
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly
        );

    QCOMPARE(
        session->externalSourcePath(),
        QString()
        );

    QVERIFY(
        !session->supportsLiveFollowing()
        );

    QCOMPARE(
        session->importProfile().name,
        QStringLiteral(
            "Saved Profile"
            )
        );

    QCOMPARE(
        session->processedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session
            ->investigationController()
            ->allRecords()
            .size(),
        1
        );

    const InvestigationRecord
        &restored =
        session
            ->investigationController()
            ->allRecords()
            .first();

    QCOMPARE(
        restored.recordId,
        QStringLiteral(
            "historical-record"
            )
        );

    QCOMPARE(
        restored.source.sourceGeneration,
        quint64(3)
        );

    /*
     * Descriptive provenance survives without
     * becoming an external dependency.
     */
    QCOMPARE(
        session
            ->sourceMetadata()
            .sourcePath,
        QStringLiteral(
            "C:/logs/warehouse-live.jsonl"
            )
        );

    QCOMPARE(
        session
            ->sourceMetadata()
            .sourceName,
        QStringLiteral(
            "warehouse-live.jsonl"
            )
        );
}

QTEST_MAIN(InvestigationWorkspaceTests)

#include "InvestigationWorkspaceTests.moc"