#include <QtTest/QtTest>

#include <QPointer>

#include "../src/ui/workspace/WorkspaceDocument.h"
#include "../src/ui/workspace/WorkspaceDocumentHost.h"

class WorkspaceDocumentHostTests
    : public QObject
{
    Q_OBJECT

private slots:
    void addsAndSelectsDocument();
    void rejectsDuplicateDocumentId();
    void switchesCurrentDocumentById();
    void documentTitleChangesUpdateDocumentState();
    void removeDocumentTransfersOwnershipWithoutDeleting();
    void closeRequestDoesNotRemoveDocument();
};

void WorkspaceDocumentHostTests::
    addsAndSelectsDocument()
{
    WorkspaceDocumentHost host;

    auto *document =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    QVERIFY(
        host.addDocument(
            document
            )
        );

    QCOMPARE(
        host.documentCount(),
        1
        );

    QCOMPARE(
        host.documentAt(0),
        document
        );

    QCOMPARE(
        host.currentDocument(),
        document
        );

    QCOMPARE(
        host.indexOfDocument(
            QStringLiteral("session-1")
            ),
        0
        );
}

void WorkspaceDocumentHostTests::
    rejectsDuplicateDocumentId()
{
    WorkspaceDocumentHost host;

    auto *first =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("First")
            );

    auto *duplicate =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Duplicate")
            );

    QVERIFY(
        host.addDocument(
            first
            )
        );

    QVERIFY(
        !host.addDocument(
            duplicate
            )
        );

    QCOMPARE(
        host.documentCount(),
        1
        );

    /*
     * The rejected document was never adopted by
     * the host, so the caller still owns it.
     */
    delete duplicate;
}

void WorkspaceDocumentHostTests::
    switchesCurrentDocumentById()
{
    WorkspaceDocumentHost host;

    auto *first =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("First")
            );

    auto *second =
        new WorkspaceDocument(
            QStringLiteral("session-2"),
            QStringLiteral("Second")
            );

    QVERIFY(
        host.addDocument(
            first
            )
        );

    QVERIFY(
        host.addDocument(
            second
            )
        );

    QCOMPARE(
        host.currentDocument(),
        second
        );

    QVERIFY(
        host.setCurrentDocument(
            QStringLiteral("session-1")
            )
        );

    QCOMPARE(
        host.currentDocument(),
        first
        );

    QVERIFY(
        !host.setCurrentDocument(
            QStringLiteral("missing")
            )
        );

    QCOMPARE(
        host.currentDocument(),
        first
        );
}

void WorkspaceDocumentHostTests::
    documentTitleChangesUpdateDocumentState()
{
    WorkspaceDocumentHost host;

    auto *document =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Original")
            );

    QVERIFY(
        host.addDocument(
            document
            )
        );

    QSignalSpy titleSpy(
        document,
        &WorkspaceDocument::
        documentTitleChanged
        );

    document->setDocumentTitle(
        QStringLiteral("Updated")
        );

    QCOMPARE(
        document->documentTitle(),
        QStringLiteral("Updated")
        );

    QCOMPARE(
        document->windowTitle(),
        QStringLiteral("Updated")
        );

    QCOMPARE(
        titleSpy.count(),
        1
        );

    /*
     * Reapplying the same title is not a state
     * change and should not emit another signal.
     */
    document->setDocumentTitle(
        QStringLiteral("Updated")
        );

    QCOMPARE(
        titleSpy.count(),
        1
        );
}

void WorkspaceDocumentHostTests::
    removeDocumentTransfersOwnershipWithoutDeleting()
{
    WorkspaceDocumentHost host;

    auto *document =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    QPointer<WorkspaceDocument>
        guardedDocument(
            document
            );

    QVERIFY(
        host.addDocument(
            document
            )
        );

    WorkspaceDocument *removed =
        host.removeDocument(
            QStringLiteral("session-1")
            );

    QCOMPARE(
        removed,
        document
        );

    QCOMPARE(
        host.documentCount(),
        0
        );

    QVERIFY(
        guardedDocument
        );

    QCOMPARE(
        document->parentWidget(),
        nullptr
        );

    delete document;

    QVERIFY(
        guardedDocument.isNull()
        );
}

void WorkspaceDocumentHostTests::
    closeRequestDoesNotRemoveDocument()
{
    WorkspaceDocumentHost host;

    auto *document =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    QVERIFY(
        host.addDocument(
            document
            )
        );

    QSignalSpy closeSpy(
        &host,
        &WorkspaceDocumentHost::
        documentCloseRequested
        );

    /*
     * The host intentionally exposes close intent
     * rather than owning the lifetime policy.
     *
     * Exercise that public signal contract directly;
     * actual tab-button interaction is Qt behavior,
     * not logic this unit test needs to reproduce.
     */
    emit host.documentCloseRequested(
        document->documentId()
        );

    QCOMPARE(
        closeSpy.count(),
        1
        );

    QCOMPARE(
        closeSpy.at(0)
            .at(0)
            .toString(),
        QStringLiteral("session-1")
        );

    QCOMPARE(
        host.documentCount(),
        1
        );

    QCOMPARE(
        host.currentDocument(),
        document
        );
}

QTEST_MAIN(
    WorkspaceDocumentHostTests
    )

#include "WorkspaceDocumentHostTests.moc"