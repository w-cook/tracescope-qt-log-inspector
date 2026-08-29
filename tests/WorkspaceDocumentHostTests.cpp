#include <QtTest/QtTest>

#include <QPointer>
#include <QSet>

#include <memory>

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
    void workspaceLayoutRoundTrips();
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

void WorkspaceDocumentHostTests::
    workspaceLayoutRoundTrips()
{
    WorkspaceDocumentHost originalHost;

    originalHost.resize(
        900,
        600
        );

    originalHost.show();

    QCoreApplication::processEvents();

    auto session1Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    WorkspaceDocument *session1 =
        session1Owner.get();

    QVERIFY(
        originalHost.addDocument(
            session1
            )
        );

    session1Owner.release();

    auto session2Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-2"),
            QStringLiteral("Session Two")
            );

    WorkspaceDocument *session2 =
        session2Owner.get();

    QVERIFY(
        originalHost.addDocument(
            session2
            )
        );

    session2Owner.release();

    auto session3Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-3"),
            QStringLiteral("Session Three")
            );

    WorkspaceDocument *session3 =
        session3Owner.get();

    QVERIFY(
        originalHost.addDocument(
            session3
            )
        );

    session3Owner.release();

    auto comparison1Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("comparison-1"),
            QStringLiteral("Comparison One")
            );

    WorkspaceDocument *comparison1 =
        comparison1Owner.get();

    QVERIFY(
        originalHost.addDocument(
            comparison1
            )
        );

    comparison1Owner.release();

    /*
     * Root group:
     *
     * session-1
     * session-2
     *
     * with session-2 as its local current tab.
     */
    QVERIFY(
        originalHost.setCurrentDocument(
            QStringLiteral("session-2")
            )
        );

    /*
     * Create one detached workspace containing
     * session-3.
     */
    QVERIFY(
        originalHost.detachDocument(
            QStringLiteral("session-3")
            )
        );

    QCoreApplication::processEvents();

    const auto detachedWindows =
        originalHost.findChildren<
            DetachedWorkspaceDocumentWindow *>();

    QCOMPARE(
        detachedWindows.size(),
        1
        );

    if (detachedWindows.isEmpty()) {
        QFAIL(
            "Expected one detached workspace window."
            );

        return;
    }

    DetachedWorkspaceDocumentWindow
        *detachedWindow =
        detachedWindows.first();

    if (detachedWindow == nullptr) {
        QFAIL(
            "Detached workspace window was null."
            );

        return;
    }

    WorkspaceDocumentHost *detachedHost =
        detachedWindow->documentHost();

    if (detachedHost == nullptr) {
        QFAIL(
            "Detached workspace window had no "
            "document host."
            );

        return;
    }

    /*
     * Move comparison-1 into the existing detached
     * group so that the detached window contains:
     *
     * session-3
     * comparison-1
     */
    WorkspaceDocument *removedComparison =
        originalHost.removeDocument(
            QStringLiteral("comparison-1")
            );

    if (removedComparison == nullptr) {
        QFAIL(
            "Expected comparison-1 to be removed "
            "from the root workspace."
            );

        return;
    }

    QCOMPARE(
        removedComparison,
        comparison1
        );

    std::unique_ptr<WorkspaceDocument>
        removedComparisonOwner(
            removedComparison
            );

    if (detachedHost == nullptr) {
        QFAIL(
            "Expected the detached workspace window "
            "to contain a document host."
            );

        return;
    }

    QVERIFY(
        detachedHost->addDocument(
            removedComparison,
            false
            )
        );

    removedComparisonOwner.release();

    QCOMPARE(
        detachedHost->documentCount(),
        2
        );

    QCOMPARE(
        detachedHost
            ->documentAt(0)
            ->documentId(),
        QStringLiteral("session-3")
        );

    QCOMPARE(
        detachedHost
            ->documentAt(1)
            ->documentId(),
        QStringLiteral("comparison-1")
        );

    /*
     * Preserve session-2 as the root group's local
     * current tab.
     */
    QVERIFY(
        originalHost.setCurrentDocument(
            QStringLiteral("session-2")
            )
        );

    /*
     * Make comparison-1 current in its detached
     * group and therefore globally active.
     */
    QVERIFY(
        originalHost.setCurrentDocument(
            QStringLiteral("comparison-1")
            )
        );

    detachedWindow->setGeometry(
        120,
        140,
        720,
        480
        );

    detachedWindow->show();

    QCoreApplication::processEvents();

    const WorkspaceDocumentLayoutState saved =
        originalHost.captureLayoutState();

    /*
     * Verify that the source fixture represents the
     * workspace configuration we intended to save.
     */
    QCOMPARE(
        saved.dockedGroup.documentIds,
        QStringList({
            QStringLiteral("session-1"),
            QStringLiteral("session-2")
        })
        );

    QCOMPARE(
        saved.dockedGroup.currentDocumentId,
        QStringLiteral("session-2")
        );

    QCOMPARE(
        saved.detachedWindows.size(),
        1
        );

    const DetachedWorkspaceWindowLayoutState
        &savedDetached =
        saved.detachedWindows.first();

    QCOMPARE(
        savedDetached.group.documentIds,
        QStringList({
            QStringLiteral("session-3"),
            QStringLiteral("comparison-1")
        })
        );

    QCOMPARE(
        savedDetached.group.currentDocumentId,
        QStringLiteral("comparison-1")
        );

    QCOMPARE(
        saved.activeDocumentId,
        QStringLiteral("comparison-1")
        );

    QVERIFY(
        savedDetached.geometry.isValid()
        );

    QVERIFY(
        !savedDetached.maximized
        );

    /*
     * Model workspace reopening: reconstruct every
     * document in the root host first, deliberately
     * using an order that does not match the saved
     * layout.
     */
    WorkspaceDocumentHost restoredHost;

    restoredHost.resize(
        900,
        600
        );

    restoredHost.show();

    QCoreApplication::processEvents();

    auto restoredComparisonOwner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("comparison-1"),
            QStringLiteral("Comparison One")
            );

    WorkspaceDocument *restoredComparison =
        restoredComparisonOwner.get();

    QVERIFY(
        restoredHost.addDocument(
            restoredComparison
            )
        );

    restoredComparisonOwner.release();

    auto restoredSession3Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-3"),
            QStringLiteral("Session Three")
            );

    WorkspaceDocument *restoredSession3 =
        restoredSession3Owner.get();

    QVERIFY(
        restoredHost.addDocument(
            restoredSession3
            )
        );

    restoredSession3Owner.release();

    auto restoredSession2Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-2"),
            QStringLiteral("Session Two")
            );

    WorkspaceDocument *restoredSession2 =
        restoredSession2Owner.get();

    QVERIFY(
        restoredHost.addDocument(
            restoredSession2
            )
        );

    restoredSession2Owner.release();

    auto restoredSession1Owner =
        std::make_unique<WorkspaceDocument>(
            QStringLiteral("session-1"),
            QStringLiteral("Session One")
            );

    WorkspaceDocument *restoredSession1 =
        restoredSession1Owner.get();

    QVERIFY(
        restoredHost.addDocument(
            restoredSession1
            )
        );

    restoredSession1Owner.release();

    QCOMPARE(
        restoredHost.documentCount(),
        4
        );

    restoredHost.restoreLayoutState(
        saved
        );

    QCoreApplication::processEvents();

    const WorkspaceDocumentLayoutState restored =
        restoredHost.captureLayoutState();

    /*
     * Docked group order and local current tab.
     */
    QCOMPARE(
        restored.dockedGroup.documentIds,
        saved.dockedGroup.documentIds
        );

    QCOMPARE(
        restored.dockedGroup.currentDocumentId,
        saved.dockedGroup.currentDocumentId
        );

    /*
     * Detached group membership, order, and local
     * current tab.
     */
    QCOMPARE(
        restored.detachedWindows.size(),
        1
        );

    const DetachedWorkspaceWindowLayoutState
        &restoredDetached =
        restored.detachedWindows.first();

    QCOMPARE(
        restoredDetached.group.documentIds,
        savedDetached.group.documentIds
        );

    QCOMPARE(
        restoredDetached.group.currentDocumentId,
        savedDetached.group.currentDocumentId
        );

    /*
     * Global active document remains independent of
     * the root group's local current tab.
     */
    QCOMPARE(
        restored.activeDocumentId,
        saved.activeDocumentId
        );

    QCOMPARE(
        restored.activeDocumentId,
        QStringLiteral("comparison-1")
        );

    QCOMPARE(
        restored.dockedGroup.currentDocumentId,
        QStringLiteral("session-2")
        );

    QCOMPARE(
        restoredDetached.maximized,
        savedDetached.maximized
        );

    /*
     * Window systems may normalize screen
     * coordinates. Verify useful effective geometry
     * without requiring an exact top-left position.
     */
    QVERIFY(
        restoredDetached.geometry.isValid()
        );

    QCOMPARE(
        restoredDetached.geometry.size(),
        savedDetached.geometry.size()
        );

    /*
     * Confirm actual host membership agrees with the
     * restored persisted representation.
     */
    QCOMPARE(
        restoredHost.documentCount(),
        2
        );

    QCOMPARE(
        restoredHost
            .documentAt(0)
            ->documentId(),
        QStringLiteral("session-1")
        );

    QCOMPARE(
        restoredHost
            .documentAt(1)
            ->documentId(),
        QStringLiteral("session-2")
        );

    QVERIFY(
        restoredHost.isDocumentDetached(
            QStringLiteral("session-3")
            )
        );

    QVERIFY(
        restoredHost.isDocumentDetached(
            QStringLiteral("comparison-1")
            )
        );

    QCOMPARE(
        restoredHost.documents().size(),
        4
        );
}

QTEST_MAIN(
    WorkspaceDocumentHostTests
    )

#include "WorkspaceDocumentHostTests.moc"