#include <QtTest>

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

QTEST_MAIN(InvestigationWorkspaceTests)

#include "InvestigationWorkspaceTests.moc"