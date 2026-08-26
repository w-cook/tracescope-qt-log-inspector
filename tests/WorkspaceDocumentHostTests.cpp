#include <QtTest/QtTest>

#include <QPointer>
#include <QSet>

#include "../src/ui/workspace/DetachedWorkspaceDocumentWindow.h"
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
    void detachesAndRedocksSameDocument();
    void rejectsDuplicateDocumentIdWhileDetached();
    void detachedWindowCanContainMultipleDocuments();
    void removeDetachedDocumentTransfersOwnershipWithoutDeleting();
    void detachedWindowCloseRequestsAllDocuments();
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

    QVERIFY(
        host.addDocument(
            first
            )
        );

    /*
     * Create this only after the first document has
     * been successfully adopted by the host. QTest
     * assertions return immediately on failure, so
     * allocating it earlier would create a possible
     * leak path.
     */
    auto *duplicate =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Duplicate")
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

    QVERIFY(
        host.addDocument(
            first
            )
        );

    auto *second =
        new WorkspaceDocument(
            QStringLiteral("session-2"),
            QStringLiteral("Second")
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

void WorkspaceDocumentHostTests::
    detachesAndRedocksSameDocument()
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

    QSignalSpy detachedSpy(
        &host,
        &WorkspaceDocumentHost::
        documentDetached
        );

    QSignalSpy redockedSpy(
        &host,
        &WorkspaceDocumentHost::
        documentRedocked
        );

    QVERIFY(
        host.detachDocument(
            document->documentId()
            )
        );

    QCOMPARE(
        host.documentCount(),
        0
        );

    QCOMPARE(
        host.documentById(
            document->documentId()
            ),
        document
        );

    QVERIFY(
        host.isDocumentDetached(
            document->documentId()
            )
        );

    const auto windows =
        host.findChildren<
            DetachedWorkspaceDocumentWindow *>();

    QCOMPARE(
        windows.size(),
        1
        );

    if (windows.isEmpty()) {
        QFAIL(
            "Expected a detached workspace window."
            );

        return;
    }

    DetachedWorkspaceDocumentWindow *window =
        windows.first();

    if (window == nullptr) {
        QFAIL(
            "Detached workspace window was null."
            );

        return;
    }

    WorkspaceDocumentHost *detachedHost =
        window->documentHost();

    if (detachedHost == nullptr) {
        QFAIL(
            "Detached workspace window had no document host."
            );

        return;
    }

    QCOMPARE(
        detachedHost->documentCount(),
        1
        );

    QCOMPARE(
        detachedHost->documentAt(0),
        document
        );

    QCOMPARE(
        detachedSpy.count(),
        1
        );

    QVERIFY(
        host.redockDocument(
            document->documentId()
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
        host.documentById(
            document->documentId()
            ),
        document
        );

    QVERIFY(
        !host.isDocumentDetached(
            document->documentId()
            )
        );

    QCOMPARE(
        redockedSpy.count(),
        1
        );
}

void WorkspaceDocumentHostTests::
    rejectsDuplicateDocumentIdWhileDetached()
{
    WorkspaceDocumentHost host;

    auto *original =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Original")
            );

    QVERIFY(
        host.addDocument(
            original
            )
        );

    QVERIFY(
        host.detachDocument(
            original->documentId()
            )
        );

    auto *duplicate =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Duplicate")
            );

    QVERIFY(
        !host.addDocument(
            duplicate
            )
        );

    delete duplicate;

    QCOMPARE(
        host.documentById(
            QStringLiteral("session-1")
            ),
        original
        );

    QVERIFY(
        host.isDocumentDetached(
            QStringLiteral("session-1")
            )
        );
}

void WorkspaceDocumentHostTests::
    detachedWindowCanContainMultipleDocuments()
{
    WorkspaceDocumentHost host;

    auto *first =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    QVERIFY(
        host.addDocument(
            first
            )
        );

    QVERIFY(
        host.detachDocument(
            first->documentId()
            )
        );

    const auto windows =
        host.findChildren<
            DetachedWorkspaceDocumentWindow *>();

    QCOMPARE(
        windows.size(),
        1
        );

    if (windows.isEmpty()) {
        QFAIL(
            "Expected a detached workspace window."
            );

        return;
    }

    DetachedWorkspaceDocumentWindow *window =
        windows.first();

    if (window == nullptr) {
        QFAIL(
            "Detached workspace window was null."
            );

        return;
    }

    WorkspaceDocumentHost *detachedHost =
        window->documentHost();

    if (detachedHost == nullptr) {
        QFAIL(
            "Detached workspace window had no document host."
            );

        return;
    }

    auto *second =
        new WorkspaceDocument(
            QStringLiteral("session-2"),
            QStringLiteral("Session Two")
            );

    QVERIFY(
        detachedHost->addDocument(
            second
            )
        );

    QCOMPARE(
        detachedHost->documentCount(),
        2
        );

    QCOMPARE(
        detachedHost->documentAt(0),
        first
        );

    QCOMPARE(
        detachedHost->documentAt(1),
        second
        );

    QCOMPARE(
        host.documentById(
            first->documentId()
            ),
        first
        );

    QCOMPARE(
        host.documentById(
            second->documentId()
            ),
        second
        );

    QVERIFY(
        host.isDocumentDetached(
            first->documentId()
            )
        );

    QVERIFY(
        host.isDocumentDetached(
            second->documentId()
            )
        );

    const QVector<WorkspaceDocument *>
        allDocuments =
        host.documents();

    QCOMPARE(
        allDocuments.size(),
        2
        );

    QVERIFY(
        allDocuments.contains(
            first
            )
        );

    QVERIFY(
        allDocuments.contains(
            second
            )
        );
}

void WorkspaceDocumentHostTests::
    removeDetachedDocumentTransfersOwnershipWithoutDeleting()
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

    QVERIFY(
        host.detachDocument(
            document->documentId()
            )
        );

    WorkspaceDocument *removed =
        host.removeDocument(
            document->documentId()
            );

    QCOMPARE(
        removed,
        document
        );

    QCOMPARE(
        host.documentById(
            document->documentId()
            ),
        nullptr
        );

    QVERIFY(
        !host.isDocumentDetached(
            document->documentId()
            )
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
    detachedWindowCloseRequestsAllDocuments()
{
    WorkspaceDocumentHost host;

    auto *first =
        new WorkspaceDocument(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    QVERIFY(
        host.addDocument(
            first
            )
        );

    QVERIFY(
        host.detachDocument(
            first->documentId()
            )
        );

    const auto windows =
        host.findChildren<
            DetachedWorkspaceDocumentWindow *>();

    QCOMPARE(
        windows.size(),
        1
        );

    if (windows.isEmpty()) {
        QFAIL(
            "Expected a detached workspace window."
            );

        return;
    }

    DetachedWorkspaceDocumentWindow *window =
        windows.first();

    if (window == nullptr) {
        QFAIL(
            "Detached workspace window was null."
            );

        return;
    }

    WorkspaceDocumentHost *detachedHost =
        window->documentHost();

    if (detachedHost == nullptr) {
        QFAIL(
            "Detached workspace window had no document host."
            );

        return;
    }

    auto *second =
        new WorkspaceDocument(
            QStringLiteral("session-2"),
            QStringLiteral("Session Two")
            );

    QVERIFY(
        detachedHost->addDocument(
            second
            )
        );

    QSignalSpy closeSpy(
        &host,
        &WorkspaceDocumentHost::
        documentCloseRequested
        );

    window->close();

    QCOMPARE(
        closeSpy.count(),
        2
        );

    QSet<QString> requestedIds;

    for (int index = 0;
         index < closeSpy.count();
         ++index) {
        requestedIds.insert(
            closeSpy.at(index)
                .at(0)
                .toString()
            );
    }

    QSet<QString> expectedIds;

    expectedIds.insert(
        QStringLiteral("session-1")
        );

    expectedIds.insert(
        QStringLiteral("session-2")
        );

    QCOMPARE(
        requestedIds,
        expectedIds
        );

    /*
     * This test has no workspace owner responding
     * to documentCloseRequested, so close intent
     * alone must not remove or destroy anything.
     */
    QCOMPARE(
        detachedHost->documentCount(),
        2
        );

    QCOMPARE(
        host.documentById(
            QStringLiteral("session-1")
            ),
        first
        );

    QCOMPARE(
        host.documentById(
            QStringLiteral("session-2")
            ),
        second
        );
}

QTEST_MAIN(
    WorkspaceDocumentHostTests
    )

#include "WorkspaceDocumentHostTests.moc"