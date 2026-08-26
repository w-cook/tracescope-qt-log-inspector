#include "WorkspaceDocumentHost.h"

#include <QApplication>
#include <QMenu>
#include <QPoint>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QCursor>
#include <QDrag>

#include <utility>

#include "DetachedWorkspaceDocumentWindow.h"
#include "WorkspaceDocument.h"
#include "WorkspaceTabBar.h"

class WorkspaceTabWidget
    : public QTabWidget
{
public:
    explicit WorkspaceTabWidget(
        QWidget *parent = nullptr
        )
        : QTabWidget(parent)
    {
        setTabBar(
            new WorkspaceTabBar(this)
            );

        setTabBarAutoHide(
            false
            );
    }

    WorkspaceTabBar *workspaceTabBar()
        const
    {
        return static_cast<WorkspaceTabBar *>(
            tabBar()
            );
    }
};

WorkspaceDocumentHost::WorkspaceDocumentHost(
    QWidget *parent,
    WorkspaceDocumentHost *rootHost
    )
    : QWidget(parent),
    m_rootHost(
        rootHost != nullptr
            ? rootHost
            : this
        ),
    m_tabs(
        new WorkspaceTabWidget(this)
        )
{
    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->addWidget(
        m_tabs
        );

    m_tabs->setTabsClosable(
        true
        );

    m_tabs->setMovable(
        true
        );

    m_tabs->setDocumentMode(
        true
        );

    m_tabs->setUsesScrollButtons(
        true
        );

    WorkspaceTabBar *tabBar =
        m_tabs->workspaceTabBar();

    tabBar->setElideMode(
        Qt::ElideMiddle
        );

    tabBar->setContextMenuPolicy(
        Qt::CustomContextMenu
        );

    connect(
        m_tabs,
        &QTabWidget::currentChanged,
        this,
        [this](int index) {
            WorkspaceDocument *document =
                documentAt(index);

            emit currentDocumentChanged(
                document != nullptr
                    ? document->documentId()
                    : QString()
                );
        }
        );

    connect(
        m_tabs,
        &QTabWidget::tabCloseRequested,
        this,
        [this](int index) {
            WorkspaceDocument *document =
                documentAt(index);

            if (document == nullptr) {
                return;
            }

            emit documentCloseRequested(
                document->documentId()
                );
        }
        );

    /*
     * Child tab groups forward their public
     * document signals to the root host. MainWindow
     * therefore only needs to observe the root.
     */
    if (m_rootHost != this) {
        connect(
            this,
            &WorkspaceDocumentHost::
            currentDocumentChanged,
            m_rootHost,
            &WorkspaceDocumentHost::
            currentDocumentChanged
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            documentCloseRequested,
            m_rootHost,
            &WorkspaceDocumentHost::
            documentCloseRequested
            );
    }

    connect(
        tabBar,
        &WorkspaceTabBar::
        documentDragStarted,
        this,
        [this](
            const QString &documentId
            ) {
            m_rootHost
                ->beginDocumentDrag(
                    this,
                    documentId
                    );
        }
        );

    connect(
        tabBar,
        &WorkspaceTabBar::
        documentDropRequested,
        this,
        [this](
            const QString &documentId,
            int targetIndex
            ) {
            m_rootHost
                ->handleDocumentDrop(
                    this,
                    documentId,
                    targetIndex
                    );
        }
        );

    connect(
        tabBar,
        &WorkspaceTabBar::
        documentTearOutRequested,
        this,
        [this](
            const QString &documentId,
            const QPoint &globalPosition
            ) {
            m_rootHost
                ->handleDocumentTearOut(
                    documentId,
                    globalPosition
                    );
        }
        );

    connect(
        tabBar,
        &WorkspaceTabBar::
        externalDragCompleted,
        this,
        [this]() {
            m_rootHost
                ->cleanupEmptyDetachedHost(
                    this
                    );
        }
        );

    /*
     * Keep a discoverable fallback in addition to
     * drag-out behavior.
     */
    connect(
        tabBar,
        &QWidget::
        customContextMenuRequested,
        this,
        [this, tabBar](
            const QPoint &position
            ) {
            const int index =
                tabBar->tabAt(
                    position
                    );

            WorkspaceDocument *document =
                documentAt(index);

            if (document == nullptr) {
                return;
            }

            QMenu menu(tabBar);

            QAction *detachAction =
                menu.addAction(
                    tr(
                        "Detach to New Window"
                        )
                    );

            QAction *selected =
                menu.exec(
                    tabBar->mapToGlobal(
                        position
                        )
                    );

            if (selected
                != detachAction) {
                return;
            }

            m_rootHost
                ->detachDocument(
                    document->documentId()
                    );
        }
        );
}

int WorkspaceDocumentHost::documentCount()
    const
{
    return m_tabs->count();
}

WorkspaceDocument *
WorkspaceDocumentHost::documentAt(
    int index
    ) const
{
    if (index < 0
        || index >= m_tabs->count()) {
        return nullptr;
    }

    return qobject_cast<WorkspaceDocument *>(
        m_tabs->widget(index)
        );
}

WorkspaceDocument *
WorkspaceDocumentHost::currentDocument()
    const
{
    return documentAt(
        m_tabs->currentIndex()
        );
}

int WorkspaceDocumentHost::indexOfDocument(
    const QString &documentId
    ) const
{
    for (int index = 0;
         index < m_tabs->count();
         ++index) {
        WorkspaceDocument *document =
            documentAt(index);

        if (document != nullptr
            && document->documentId()
                   == documentId) {
            return index;
        }
    }

    return -1;
}

WorkspaceDocument *
WorkspaceDocumentHost::localDocumentById(
    const QString &documentId
    ) const
{
    const int index =
        indexOfDocument(
            documentId
            );

    return index >= 0
               ? documentAt(index)
               : nullptr;
}

QVector<WorkspaceDocument *>
WorkspaceDocumentHost::localDocuments()
    const
{
    QVector<WorkspaceDocument *> result;

    result.reserve(
        m_tabs->count()
        );

    for (int index = 0;
         index < m_tabs->count();
         ++index) {
        WorkspaceDocument *document =
            documentAt(index);

        if (document != nullptr) {
            result.push_back(
                document
                );
        }
    }

    return result;
}

WorkspaceDocument *
WorkspaceDocumentHost::documentById(
    const QString &documentId
    ) const
{
    const WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocument *document =
        root->localDocumentById(
            documentId
            );

    if (document != nullptr) {
        return document;
    }

    for (DetachedWorkspaceDocumentWindow *window
         : root->m_detachedWindows) {
        if (window == nullptr
            || window->documentHost()
                   == nullptr) {
            continue;
        }

        document =
            window->documentHost()
                ->localDocumentById(
                    documentId
                    );

        if (document != nullptr) {
            return document;
        }
    }

    return nullptr;
}

QVector<WorkspaceDocument *>
WorkspaceDocumentHost::documents()
    const
{
    const WorkspaceDocumentHost *root =
        m_rootHost;

    QVector<WorkspaceDocument *> result =
        root->localDocuments();

    for (DetachedWorkspaceDocumentWindow *window
         : root->m_detachedWindows) {
        if (window == nullptr
            || window->documentHost()
                   == nullptr) {
            continue;
        }

        const QVector<WorkspaceDocument *>
            detachedDocuments =
            window->documentHost()
                ->localDocuments();

        for (WorkspaceDocument *document
             : detachedDocuments) {
            result.push_back(
                document
                );
        }
    }

    return result;
}

WorkspaceDocumentHost *
WorkspaceDocumentHost::owningHost(
    const QString &documentId
    ) const
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (root->localDocumentById(
            documentId
            )
        != nullptr) {
        return root;
    }

    for (DetachedWorkspaceDocumentWindow *window
         : std::as_const(
             root->m_detachedWindows
             )) {
        if (window == nullptr) {
            continue;
        }

        WorkspaceDocumentHost *host =
            window->documentHost();

        if (host != nullptr
            && host->localDocumentById(
                   documentId
                   )
                   != nullptr) {
            return host;
        }
    }

    return nullptr;
}

bool WorkspaceDocumentHost::
    isDocumentDetached(
        const QString &documentId
        ) const
{
    WorkspaceDocumentHost *host =
        owningHost(
            documentId
            );

    return host != nullptr
           && host != m_rootHost;
}

bool WorkspaceDocumentHost::addDocument(
    WorkspaceDocument *document,
    bool makeCurrent
    )
{
    if (document == nullptr
        || document->documentId()
               .trimmed()
               .isEmpty()
        || m_rootHost
                   ->documentById(
                       document->documentId()
                       )
               != nullptr) {
        return false;
    }

    return insertLocalDocument(
        document,
        m_tabs->count(),
        makeCurrent
        );
}

WorkspaceDocument *
    WorkspaceDocumentHost::
    takeLocalDocument(
        const QString &documentId
        )
{
    const int index =
        indexOfDocument(
            documentId
            );

    if (index < 0) {
        return nullptr;
    }

    WorkspaceDocument *document =
        documentAt(
            index
            );

    if (document == nullptr) {
        return nullptr;
    }

    WorkspaceDocument *previousCurrent =
        currentDocument();

    const bool removedWasCurrent =
        previousCurrent == document
        || m_tabs->tabBar()
                   ->currentIndex()
               == index;

    disconnect(
        document,
        &WorkspaceDocument::
        documentTitleChanged,
        this,
        &WorkspaceDocumentHost::
        updateDocumentTitle
        );

    m_tabs->removeTab(
        index
        );

    document->hide();

    document->setParent(
        nullptr
        );

    /*
     * If tabs remain, establish a deterministic
     * local current tab immediately.
     */
    if (m_tabs->count() > 0) {
        int nextIndex = -1;

        if (!removedWasCurrent
            && previousCurrent != nullptr) {
            nextIndex =
                m_tabs->indexOf(
                    previousCurrent
                    );
        }

        if (nextIndex < 0) {
            /*
             * Prefer the tab that collapsed into the
             * removed tab's position. If the removed
             * tab was last, use the new final tab.
             */
            nextIndex =
                qMin(
                    index,
                    m_tabs->count() - 1
                    );
        }

        ensureLocalCurrentDocument(
            nextIndex
            );
    }

    return document;
}

bool WorkspaceDocumentHost::
    insertLocalDocument(
        WorkspaceDocument *document,
        int index,
        bool makeCurrent
        )
{
    if (document == nullptr
        || document->documentId()
               .trimmed()
               .isEmpty()
        || m_rootHost
                   ->documentById(
                       document->documentId()
                       )
               != nullptr) {
        return false;
    }

    WorkspaceDocument *previousCurrent =
        currentDocument();

    const int boundedIndex =
        index < 0
            ? m_tabs->count()
            : qBound(
                  0,
                  index,
                  m_tabs->count()
                  );

    const int insertedIndex =
        m_tabs->insertTab(
            boundedIndex,
            document,
            document->documentTitle()
            );

    if (insertedIndex < 0) {
        return false;
    }

    m_tabs->tabBar()
        ->setTabData(
            insertedIndex,
            document->documentId()
            );

    m_tabs->setTabToolTip(
        insertedIndex,
        document->toolTip()
        );

    connect(
        document,
        &WorkspaceDocument::
        documentTitleChanged,
        this,
        &WorkspaceDocumentHost::
        updateDocumentTitle,
        Qt::UniqueConnection
        );

    if (makeCurrent) {
        /*
         * A dropped/inserted document explicitly
         * becomes this tab group's current document.
         */
        ensureLocalCurrentDocument(
            insertedIndex
            );
    } else if (
        previousCurrent != nullptr
        ) {
        /*
         * Preserve the previous local current
         * document when insertion is non-activating.
         * Its numeric index may have shifted.
         */
        const int previousIndex =
            m_tabs->indexOf(
                previousCurrent
                );

        if (previousIndex >= 0) {
            ensureLocalCurrentDocument(
                previousIndex
                );
        }
    } else {
        /*
         * A non-empty group must still have a valid
         * local current tab.
         */
        ensureLocalCurrentDocument(
            insertedIndex
            );
    }

    return true;
}

WorkspaceDocument *
WorkspaceDocumentHost::removeDocument(
    const QString &documentId
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocumentHost *host =
        root->owningHost(
            documentId
            );

    if (host == nullptr) {
        return nullptr;
    }

    WorkspaceDocument *document =
        host->takeLocalDocument(
            documentId
            );

    root->cleanupEmptyDetachedHost(
        host
        );

    return document;
}

bool WorkspaceDocumentHost::setCurrentDocument(
    const QString &documentId
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocumentHost *host =
        root->owningHost(
            documentId
            );

    if (host == nullptr) {
        return false;
    }

    const int index =
        host->indexOfDocument(
            documentId
            );

    if (index < 0) {
        return false;
    }

    host->m_tabs->setCurrentIndex(
        index
        );

    if (host != root) {
        DetachedWorkspaceDocumentWindow *window =
            root->windowForHost(
                host
                );

        if (window != nullptr) {
            window->show();
            window->raise();
            window->activateWindow();
        }
    }

    return true;
}

bool WorkspaceDocumentHost::transferDocument(
    WorkspaceDocumentHost *sourceHost,
    const QString &documentId,
    WorkspaceDocumentHost *targetHost,
    int targetIndex,
    bool makeCurrent,
    bool cleanupEmptySource
    )
{
    if (sourceHost == nullptr
        || targetHost == nullptr
        || sourceHost == targetHost) {
        return false;
    }

    const int originalIndex =
        sourceHost->indexOfDocument(
            documentId
            );

    if (originalIndex < 0) {
        return false;
    }

    WorkspaceDocument *document =
        sourceHost
            ->takeLocalDocument(
                documentId
                );

    if (document == nullptr) {
        return false;
    }

    if (!targetHost
             ->insertLocalDocument(
                 document,
                 targetIndex,
                 makeCurrent
                 )) {
        sourceHost
            ->insertLocalDocument(
                document,
                originalIndex,
                true
                );

        return false;
    }

    if (cleanupEmptySource) {
        m_rootHost
            ->cleanupEmptyDetachedHost(
                sourceHost
                );
    }

    return true;
}

DetachedWorkspaceDocumentWindow *
WorkspaceDocumentHost::createDetachedWindow(
    const QPoint &globalPosition
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    auto *window =
        new DetachedWorkspaceDocumentWindow(
            root,
            root
            );

    root->m_detachedWindows
        .push_back(
            window
            );

    connect(
        window,
        &DetachedWorkspaceDocumentWindow::
        redockAllRequested,
        root,
        [root](
            DetachedWorkspaceDocumentWindow
                *requestedWindow
            ) {
            root->redockDetachedWindow(
                requestedWindow
                );
        }
        );

    connect(
        window,
        &DetachedWorkspaceDocumentWindow::
        closeAllRequested,
        root,
        [root](
            DetachedWorkspaceDocumentWindow
                *requestedWindow
            ) {
            if (requestedWindow == nullptr
                || requestedWindow
                    ->documentHost()
                        == nullptr) {
                return;
            }

            const QVector<WorkspaceDocument *>
                documents =
                requestedWindow
                    ->documentHost()
                    ->localDocuments();

            /*
             * Snapshot IDs before emitting anything.
             * Each close request may synchronously cause
             * MainWindow/InvestigationWorkspace to remove
             * that document from this tab group.
             */
            QVector<QString> documentIds;

            documentIds.reserve(
                documents.size()
                );

            for (WorkspaceDocument *document
                 : documents) {
                if (document != nullptr) {
                    documentIds.push_back(
                        document->documentId()
                        );
                }
            }

            for (const QString &documentId
                 : documentIds) {
                emit root->documentCloseRequested(
                    documentId
                    );
            }
        }
        );

    /*
     * Position the new workspace near the user's
     * release point without hiding the tab beneath
     * the cursor.
     */
    window->move(
        globalPosition
        - QPoint(
            80,
            20
            )
        );

    window->show();
    window->raise();
    window->activateWindow();

    return window;
}

bool WorkspaceDocumentHost::detachDocument(
    const QString &documentId
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocumentHost *sourceHost =
        root->owningHost(
            documentId
            );

    if (sourceHost == nullptr) {
        return false;
    }

    const QPoint position =
        QCursor::pos();

    DetachedWorkspaceDocumentWindow *window =
        root->createDetachedWindow(
            position
            );

    if (window == nullptr
        || window->documentHost()
               == nullptr) {
        return false;
    }

    const bool wasDocked =
        sourceHost == root;

    const bool moved =
        root->transferDocument(
            sourceHost,
            documentId,
            window->documentHost(),
            0,
            true
            );

    if (!moved) {
        window->deleteLater();
        return false;
    }

    if (wasDocked) {
        emit root->documentDetached(
            documentId
            );
    }

    return true;
}

bool WorkspaceDocumentHost::redockDocument(
    const QString &documentId,
    int targetIndex
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocumentHost *sourceHost =
        root->owningHost(
            documentId
            );

    if (sourceHost == nullptr
        || sourceHost == root) {
        return false;
    }

    const bool moved =
        root->transferDocument(
            sourceHost,
            documentId,
            root,
            targetIndex,
            true
            );

    if (moved) {
        emit root->documentRedocked(
            documentId
            );
    }

    return moved;
}

void WorkspaceDocumentHost::
    handleDocumentDrop(
        WorkspaceDocumentHost *targetHost,
        const QString &documentId,
        int targetIndex
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (targetHost == nullptr
        || documentId.isEmpty()) {
        return;
    }

    PendingDocumentDrag &pending =
        root->m_pendingDocumentDrag;

    if (!pending.active()
        || pending.document
                   ->documentId()
               != documentId) {
        return;
    }

    WorkspaceDocument *document =
        pending.document;

    WorkspaceDocumentHost *sourceHost =
        pending.sourceHost;

    const int sourceIndex =
        pending.sourceIndex;

    const bool sourceWasRoot =
        sourceHost == root;

    const bool targetIsRoot =
        targetHost == root;

    /*
     * The target index is based on a bar that does
     * not contain the dragged tab, so it is already
     * the correct insertion point even when returning
     * to the original group.
     */
    if (!targetHost
             ->insertLocalDocument(
                 document,
                 targetIndex,
                 true
                 )) {
        if (sourceHost != nullptr) {
            sourceHost
                ->insertLocalDocument(
                    document,
                    sourceIndex,
                    true
                    );
        }

        pending.clear();

        return;
    }

    pending.clear();

    if (sourceWasRoot
        && !targetIsRoot) {
        emit root->documentDetached(
            documentId
            );
    } else if (
        !sourceWasRoot
        && targetIsRoot
        ) {
        emit root->documentRedocked(
            documentId
            );
    }
}

void WorkspaceDocumentHost::
    handleDocumentTearOut(
        const QString &documentId,
        const QPoint &globalPosition
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    PendingDocumentDrag &pending =
        root->m_pendingDocumentDrag;

    if (!pending.active()
        || pending.document
                   ->documentId()
               != documentId) {
        return;
    }

    WorkspaceDocument *document =
        pending.document;

    WorkspaceDocumentHost *sourceHost =
        pending.sourceHost;

    const int sourceIndex =
        pending.sourceIndex;

    const bool sourceWasRoot =
        sourceHost == root;

    DetachedWorkspaceDocumentWindow *window =
        root->createDetachedWindow(
            globalPosition
            );

    if (window == nullptr
        || window->documentHost()
               == nullptr) {
        if (sourceHost != nullptr) {
            sourceHost
                ->insertLocalDocument(
                    document,
                    sourceIndex,
                    true
                    );
        }

        pending.clear();

        return;
    }

    WorkspaceDocumentHost *targetHost =
        window->documentHost();

    if (!targetHost
             ->insertLocalDocument(
                 document,
                 0,
                 true
                 )) {
        window->hide();
        window->deleteLater();

        if (sourceHost != nullptr) {
            sourceHost
                ->insertLocalDocument(
                    document,
                    sourceIndex,
                    true
                    );
        }

        pending.clear();

        return;
    }

    pending.clear();

    if (sourceWasRoot) {
        emit root->documentDetached(
            documentId
            );
    }
}

DetachedWorkspaceDocumentWindow *
WorkspaceDocumentHost::windowForHost(
    WorkspaceDocumentHost *host
    ) const
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    for (DetachedWorkspaceDocumentWindow *window
        : std::as_const(
            root->m_detachedWindows
            )) {
        if (window != nullptr
            && window->documentHost()
                   == host) {
            return window;
        }
    }

    return nullptr;
}

void WorkspaceDocumentHost::
    cleanupEmptyDetachedHost(
        WorkspaceDocumentHost *host
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (host == nullptr
        || host == root
        || host->documentCount() > 0) {
        return;
    }

    DetachedWorkspaceDocumentWindow *window =
        root->windowForHost(
            host
            );

    if (window == nullptr) {
        return;
    }

    root->m_detachedWindows
        .removeOne(
            window
            );

    window->hide();
    window->deleteLater();
}

void WorkspaceDocumentHost::
    redockDetachedWindow(
        DetachedWorkspaceDocumentWindow *window
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (window == nullptr
        || window->documentHost()
               == nullptr) {
        return;
    }

    WorkspaceDocumentHost *sourceHost =
        window->documentHost();

    const QVector<WorkspaceDocument *>
        sourceDocuments =
        sourceHost
            ->localDocuments();

    QString lastDocumentId;

    for (WorkspaceDocument *document
         : sourceDocuments) {
        if (document == nullptr) {
            continue;
        }

        lastDocumentId =
            document->documentId();

        if (root->transferDocument(
                sourceHost,
                document->documentId(),
                root,
                root->documentCount(),
                false,
                false
                )) {
            emit root->documentRedocked(
                document->documentId()
                );
        }
    }

    root->m_detachedWindows
        .removeOne(
            window
            );

    window->hide();
    window->deleteLater();

    if (!lastDocumentId.isEmpty()) {
        root->setCurrentDocument(
            lastDocumentId
            );
    }
}

void WorkspaceDocumentHost::updateDocumentTitle(
    const QString &title
    )
{
    auto *document =
        qobject_cast<WorkspaceDocument *>(
            sender()
            );

    if (document == nullptr) {
        return;
    }

    const int index =
        m_tabs->indexOf(
            document
            );

    if (index < 0) {
        return;
    }

    m_tabs->setTabText(
        index,
        title
        );
}

void WorkspaceDocumentHost::
    beginDocumentDrag(
        WorkspaceDocumentHost *sourceHost,
        const QString &documentId
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (sourceHost == nullptr
        || documentId.isEmpty()
        || root->m_pendingDocumentDrag
               .active()) {
        return;
    }

    const int sourceIndex =
        sourceHost
            ->indexOfDocument(
                documentId
                );

    if (sourceIndex < 0) {
        return;
    }

    WorkspaceDocument *document =
        sourceHost
            ->takeLocalDocument(
                documentId
                );

    if (document == nullptr) {
        return;
    }

    root->m_pendingDocumentDrag
        .document =
        document;

    root->m_pendingDocumentDrag
        .sourceHost =
        sourceHost;

    root->m_pendingDocumentDrag
        .sourceIndex =
        sourceIndex;

    /*
     * Do NOT clean up an empty detached source
     * window yet. Its tab bar is still executing
     * the QDrag operation.
     */
}

void WorkspaceDocumentHost::
    ensureLocalCurrentDocument(
        int preferredIndex
        )
{
    const int count =
        m_tabs->count();

    if (count <= 0) {
        return;
    }

    int index =
        preferredIndex;

    if (index < 0
        || index >= count) {
        index =
            m_tabs->currentIndex();
    }

    if (index < 0
        || index >= count) {
        index =
            m_tabs->tabBar()
                ->currentIndex();
    }

    if (index < 0
        || index >= count) {
        index = 0;
    }

    /*
     * Keep both sides of QTabWidget's tab/stack
     * relationship explicit after our custom
     * extraction and insertion operations.
     */
    m_tabs->setCurrentIndex(
        index
        );

    m_tabs->tabBar()
        ->setCurrentIndex(
            index
            );

    m_tabs->tabBar()->update();
    m_tabs->update();
}