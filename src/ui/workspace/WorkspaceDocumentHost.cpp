#include "WorkspaceDocumentHost.h"

#include <QApplication>
#include <QCursor>
#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPushButton>
#include <QPoint>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

#include "../InterfaceScale.h"

#include "DetachedWorkspaceDocumentWindow.h"
#include "WorkspaceDocument.h"
#include "WorkspaceTabBar.h"

WorkspaceDocumentHost::~WorkspaceDocumentHost()
{
    /*
     * Secondary TraceScope windows are intentionally
     * native top-level peers rather than QWidget
     * children of the root window.
     *
     * The root host therefore owns their application
     * lifetime explicitly even though it does not own
     * them through QObject parenting.
     */
    if (m_rootHost != this) {
        return;
    }

    const QVector<
        DetachedWorkspaceDocumentWindow *>
        windows =
        m_detachedWindows;

    m_detachedWindows.clear();

    for (DetachedWorkspaceDocumentWindow *window
         : windows) {
        if (window == nullptr) {
            continue;
        }

        window->hide();
        delete window;
    }
}

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

    void setEmptyStateWidget(
        QWidget *widget
        )
    {
        m_emptyStateWidget =
            widget;

        if (m_emptyStateWidget == nullptr) {
            return;
        }

        m_emptyStateWidget->setParent(
            this
            );

        updateEmptyStateGeometry();
    }

    void updateEmptyStateGeometry()
    {
        if (m_emptyStateWidget == nullptr) {
            return;
        }

        /*
         * The empty state occupies the complete
         * document surface. The tab bar is explicitly
         * raised above it whenever the empty host is
         * acting as a workspace drop target.
         */
        m_emptyStateWidget->setGeometry(
            rect()
            );

        m_emptyStateWidget->raise();

        if (tabBar() != nullptr) {
            tabBar()->raise();
        }
    }

protected:
    void resizeEvent(
        QResizeEvent *event
        ) override
    {
        QTabWidget::resizeEvent(
            event
            );

        updateEmptyStateGeometry();
    }

private:
    QWidget *m_emptyStateWidget =
        nullptr;
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

    /*
     * The tab widget always owns the complete host
     * surface. The empty-state presentation is
     * overlaid inside it rather than occupying a
     * separate layout row.
     *
     * This allows an empty tab bar to temporarily
     * expand at the top during a workspace-document
     * drag without moving the drop target into the
     * middle of the window.
     */
    layout->addWidget(
        m_tabs,
        1
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

    /*
     * Empty-workspace presentation.
     */
    m_emptyStateWidget =
        new QWidget(m_tabs);

    auto *emptyStateLayout =
        new QVBoxLayout(
            m_emptyStateWidget
            );

    emptyStateLayout->setContentsMargins(
        InterfaceScale::margins(
            20,
            24,
            20,
            24,
            m_emptyStateWidget
            )
        );

    emptyStateLayout->addStretch(
        1
        );


    m_emptyStateTitleLabel =
        new QLabel(
            tr("TraceScope"),
            m_emptyStateWidget
            );

    refreshEmptyStateTitleFont();

    m_emptyStateTitleLabel->setAlignment(
        Qt::AlignHCenter
        );

    emptyStateLayout->addWidget(
        m_emptyStateTitleLabel
        );

    auto *instructionLabel =
        new QLabel(
            tr(
                "Open a log file, Investigation Snapshot, "
                "or Investigation Workspace to get started."
                ),
            m_emptyStateWidget
            );

    instructionLabel->setAlignment(
        Qt::AlignHCenter
        );

    instructionLabel->setWordWrap(
        true
        );

    QSizePolicy instructionPolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Minimum
        );

    instructionPolicy.setHeightForWidth(
        true
        );

    instructionLabel->setSizePolicy(
        instructionPolicy
        );

    instructionLabel->setMinimumWidth(
        0
        );

    emptyStateLayout->addWidget(
        instructionLabel
        );

    m_emptyStateActionSpacing =
        new QSpacerItem(
            0,
            InterfaceScale::pixels(
                14,
                m_emptyStateWidget
                ),
            QSizePolicy::Minimum,
            QSizePolicy::Fixed
            );

    emptyStateLayout->addItem(
        m_emptyStateActionSpacing
        );

    /*
     * Empty-workspace actions deliberately emit intent
     * rather than invoking application workflows directly.
     *
     * WorkspaceDocumentHost remains reusable presentation
     * infrastructure while MainWindow continues to own
     * file/workspace orchestration.
     */
    m_emptyStateOpenLogButton =
        new QPushButton(
            tr("Open Log File..."),
            m_emptyStateWidget
            );

    m_emptyStateOpenSnapshotButton =
        new QPushButton(
            tr("Open Snapshot..."),
            m_emptyStateWidget
            );

    m_emptyStateOpenWorkspaceButton =
        new QPushButton(
            tr("Open Workspace..."),
            m_emptyStateWidget
            );

    m_emptyStateRecentFilesButton =
        new QPushButton(
            tr("Recent Files"),
            m_emptyStateWidget
            );

    m_emptyStateRecentWorkspacesButton =
        new QPushButton(
            tr("Recent Workspaces"),
            m_emptyStateWidget
            );


    /*
     * Primary open actions.
     *
     * Keeping these controls on one centered row reduces
     * the start surface's required vertical height.
     */
    auto *openActionsLayout =
        new QHBoxLayout();

    openActionsLayout->addStretch(1);

    openActionsLayout->addWidget(
        m_emptyStateOpenLogButton
        );

    openActionsLayout->addWidget(
        m_emptyStateOpenSnapshotButton
        );

    openActionsLayout->addWidget(
        m_emptyStateOpenWorkspaceButton
        );

    openActionsLayout->addStretch(1);

    emptyStateLayout->addLayout(
        openActionsLayout
        );

    /*
     * Recent-item actions use a separate centered row.
     *
     * These retain their existing shared menu population
     * and enabled-state behavior.
     */
    auto *recentActionsLayout =
        new QHBoxLayout();

    recentActionsLayout->addStretch(1);

    recentActionsLayout->addWidget(
        m_emptyStateRecentFilesButton
        );

    recentActionsLayout->addWidget(
        m_emptyStateRecentWorkspacesButton
        );

    recentActionsLayout->addStretch(1);

    emptyStateLayout->addLayout(
        recentActionsLayout
        );

    emptyStateLayout->addStretch(
        1
        );

    connect(
        m_emptyStateOpenLogButton,
        &QPushButton::clicked,
        this,
        [this]() {
            emit openLogRequested(
                this
                );
        }
        );

    connect(
        m_emptyStateOpenSnapshotButton,
        &QPushButton::clicked,
        this,
        [this]() {
            emit openSnapshotRequested(
                this
                );
        }
        );

    connect(
        m_emptyStateOpenWorkspaceButton,
        &QPushButton::clicked,
        this,
        [this]() {
            emit openWorkspaceRequested();
        }
        );

    connect(
        m_emptyStateRecentFilesButton,
        &QPushButton::clicked,
        this,
        [this]() {
            QMenu menu(
                m_emptyStateRecentFilesButton
                );

            menu.setToolTipsVisible(
                true
                );

            emit recentFilesMenuAboutToShow(
                &menu,
                this
                );

            if (!menu.isEnabled()
                || menu.actions().isEmpty()) {
                return;
            }

            menu.exec(
                m_emptyStateRecentFilesButton
                    ->mapToGlobal(
                        QPoint(
                            0,
                            m_emptyStateRecentFilesButton
                                ->height()
                            )
                        )
                );
        }
        );

    connect(
        m_emptyStateRecentWorkspacesButton,
        &QPushButton::clicked,
        this,
        [this]() {
            QMenu menu(
                m_emptyStateRecentWorkspacesButton
                );

            menu.setToolTipsVisible(
                true
                );

            emit recentWorkspacesMenuAboutToShow(
                &menu
                );

            if (!menu.isEnabled()
                || menu.actions().isEmpty()) {
                return;
            }

            menu.exec(
                m_emptyStateRecentWorkspacesButton
                    ->mapToGlobal(
                        QPoint(
                            0,
                            m_emptyStateRecentWorkspacesButton
                                ->height()
                            )
                        )
                );
        }
        );

    m_tabs->setEmptyStateWidget(
        m_emptyStateWidget
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

            const QString documentId =
                document != nullptr
                    ? document->documentId()
                    : QString();

            if (!documentId.isEmpty()) {
                m_rootHost->m_activeDocumentId =
                    documentId;
            }

            emit currentDocumentChanged(
                documentId
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

        connect(
            this,
            &WorkspaceDocumentHost::
            documentContextMenuAboutToShow,
            m_rootHost,
            &WorkspaceDocumentHost::
            documentContextMenuAboutToShow
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            openLogRequested,
            m_rootHost,
            &WorkspaceDocumentHost::
            openLogRequested
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            openSnapshotRequested,
            m_rootHost,
            &WorkspaceDocumentHost::
            openSnapshotRequested
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            openWorkspaceRequested,
            m_rootHost,
            &WorkspaceDocumentHost::
            openWorkspaceRequested
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            recentFilesMenuAboutToShow,
            m_rootHost,
            &WorkspaceDocumentHost::
            recentFilesMenuAboutToShow
            );

        connect(
            this,
            &WorkspaceDocumentHost::
            recentWorkspacesMenuAboutToShow,
            m_rootHost,
            &WorkspaceDocumentHost::
            recentWorkspacesMenuAboutToShow
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
                ->setWorkspaceDocumentDragActive(
                    false
                    );

            m_rootHost
                ->cleanupEmptyHost(
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

            emit documentContextMenuAboutToShow(
                document->documentId(),
                &menu
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

    connect(
        tabBar,
        &QTabBar::tabMoved,
        this,
        [this](
            int,
            int
            ) {
            emit m_rootHost
                ->workspaceLayoutChanged();
        }
        );

    connect(
        InterfaceScale::instance(),
        &InterfaceScale::
        userFactorChanged,
        this,
        [this](qreal) {
            refreshInterfaceScale();
        }
        );

    updateEmptyStatePresentation();
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
WorkspaceDocumentHost::documentHostForId(
    const QString &documentId
    ) const
{
    return m_rootHost->owningHost(
        documentId
        );
}

QVector<DetachedWorkspaceDocumentWindow *>
WorkspaceDocumentHost::detachedWindows() const
{
    return m_rootHost
        ->m_detachedWindows;
}

bool WorkspaceDocumentHost::
    hasOtherVisibleWorkspaceWindow(
        const QWidget *excludedWindow
        ) const
{
    const WorkspaceDocumentHost *root =
        m_rootHost;

    QWidget *rootWindow =
        root->window();

    if (rootWindow != nullptr
        && rootWindow != excludedWindow
        && rootWindow->isVisible()) {
        return true;
    }

    for (DetachedWorkspaceDocumentWindow *window
         : root->m_detachedWindows) {
        if (window != nullptr
            && window != excludedWindow
            && window->isVisible()) {
            return true;
        }
    }

    return false;
}

bool WorkspaceDocumentHost::
    moveDocumentsToAnotherVisibleWindow(
        WorkspaceDocumentHost *sourceHost
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (sourceHost == nullptr
        || sourceHost->m_rootHost != root
        || sourceHost->documentCount() <= 0) {
        return false;
    }

    QWidget *sourceWindow =
        sourceHost == root
            ? root->window()
            : static_cast<QWidget *>(
                  root->windowForHost(
                      sourceHost
                      )
                  );

    if (sourceWindow == nullptr) {
        return false;
    }

    /*
     * Destination selection is intentionally simple
     * and deterministic.
     *
     * Prefer the visible root peer when possible.
     * Otherwise use the first other visible detached
     * peer. All documents from the closing window go
     * to the same destination.
     */
    WorkspaceDocumentHost *targetHost =
        nullptr;

    QWidget *rootWindow =
        root->window();

    if (rootWindow != nullptr
        && rootWindow != sourceWindow
        && rootWindow->isVisible()) {
        targetHost =
            root;
    }

    if (targetHost == nullptr) {
        for (
            DetachedWorkspaceDocumentWindow *window
            : std::as_const(
                root->m_detachedWindows
                )
            ) {
            if (window == nullptr
                || window == sourceWindow
                || !window->isVisible()
                || window->documentHost()
                       == nullptr) {
                continue;
            }

            targetHost =
                window->documentHost();

            break;
        }
    }

    if (targetHost == nullptr
        || targetHost == sourceHost) {
        return false;
    }

    QVector<QString> documentIds;

    documentIds.reserve(
        sourceHost->documentCount()
        );

    for (
        WorkspaceDocument *document
        : sourceHost->localDocuments()
        ) {
        if (document == nullptr) {
            return false;
        }

        documentIds.append(
            document->documentId()
            );
    }

    const QString sourceCurrentId =
        sourceHost->currentDocument()
                != nullptr
            ? sourceHost
                  ->currentDocument()
                  ->documentId()
            : QString();

    const QString targetCurrentId =
        targetHost->currentDocument()
                != nullptr
            ? targetHost
                  ->currentDocument()
                  ->documentId()
            : QString();

    QVector<QString> movedDocumentIds;

    /*
     * Suppress QTabWidget selection notifications
     * during the batch operation.
     *
     * Removing the current tab from the source would
     * otherwise walk the global active-session state
     * through each remaining source tab. Likewise,
     * insertion must not disturb the destination's
     * current tab.
     */
    const QSignalBlocker sourceTabBlocker(
        sourceHost->m_tabs
        );

    const QSignalBlocker targetTabBlocker(
        targetHost->m_tabs
        );

    auto restoreOriginalState =
        [&]() {
            /*
             * Put every document that was already moved
             * back into the source host. A failure in
             * this internal move operation should not
             * leave a partially migrated window.
             */
            for (
                const QString &documentId
                : std::as_const(
                    movedDocumentIds
                    )
                ) {
                WorkspaceDocument *document =
                    targetHost
                        ->takeLocalDocument(
                            documentId
                            );

                if (document != nullptr) {
                    sourceHost
                        ->insertLocalDocument(
                            document,
                            sourceHost
                                ->documentCount(),
                            false
                            );
                }
            }

            /*
             * Restore the original source order after
             * reinsertion.
             */
            for (
                int index = 0;
                index < documentIds.size();
                ++index
                ) {
                sourceHost
                    ->moveLocalDocumentToIndex(
                        documentIds.at(index),
                        index
                        );
            }

            const int sourceCurrentIndex =
                sourceHost->indexOfDocument(
                    sourceCurrentId
                    );

            if (sourceCurrentIndex >= 0) {
                sourceHost
                    ->ensureLocalCurrentDocument(
                        sourceCurrentIndex
                        );
            }

            const int targetCurrentIndex =
                targetHost->indexOfDocument(
                    targetCurrentId
                    );

            if (targetCurrentIndex >= 0) {
                targetHost
                    ->ensureLocalCurrentDocument(
                        targetCurrentIndex
                        );
            }
        };

    for (
        const QString &documentId
        : std::as_const(
            documentIds
            )
        ) {
        WorkspaceDocument *document =
            sourceHost
                ->takeLocalDocument(
                    documentId
                    );

        if (document == nullptr) {
            restoreOriginalState();
            return false;
        }

        if (!targetHost
                 ->insertLocalDocument(
                     document,
                     targetHost
                         ->documentCount(),
                     false
                     )) {
            /*
             * The document has already been detached
             * from the source but was not adopted by
             * the destination. Return it before rolling
             * back the documents moved earlier.
             */
            sourceHost
                ->insertLocalDocument(
                    document,
                    sourceHost
                        ->documentCount(),
                    false
                    );

            restoreOriginalState();
            return false;
        }

        movedDocumentIds.append(
            documentId
            );
    }

    /*
     * Explicitly retain the destination's previously
     * selected tab. If the destination was empty, its
     * first inserted document naturally becomes current.
     */
    if (!targetCurrentId.isEmpty()) {
        const int targetCurrentIndex =
            targetHost->indexOfDocument(
                targetCurrentId
                );

        if (targetCurrentIndex >= 0) {
            targetHost
                ->ensureLocalCurrentDocument(
                    targetCurrentIndex
                    );
        }
    }

    /*
     * The source is now empty. Normal workspace-window
     * cleanup hides the root peer or destroys a
     * redundant detached peer as appropriate.
     */
    root->cleanupEmptyHost(
        sourceHost
        );

    /*
     * Treat the entire migration as one user-visible
     * workspace layout change.
     */
    emit root->workspaceLayoutChanged();

    WorkspaceDocument *targetCurrent =
        targetHost->currentDocument();

    if (targetCurrent != nullptr) {
        const QString targetDocumentId =
            targetCurrent->documentId();

        /*
         * Selection signals were intentionally blocked
         * during migration, so synchronize the global
         * workspace selection once with the destination
         * tab that actually remained selected.
         */
        root->m_activeDocumentId =
            targetDocumentId;

        emit root->currentDocumentChanged(
            targetDocumentId
            );

        QWidget *targetWindow =
            targetHost == root
                ? root->window()
                : static_cast<QWidget *>(
                      root->windowForHost(
                          targetHost
                          )
                      );

        if (targetWindow != nullptr) {
            targetWindow->show();
            targetWindow->raise();
            targetWindow->activateWindow();
        }
    }

    return true;
}

void WorkspaceDocumentHost::
    resetWindowLayout()
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (root != this) {
        root->resetWindowLayout();
        return;
    }

    /*
     * Full workspace replacement deliberately discards
     * the previous multi-window presentation.
     *
     * Ordinary document removal preserves one final
     * empty window, but that rule must not leave an old
     * empty peer behind while a different .tsw layout is
     * being installed.
     */
    const QVector<
        DetachedWorkspaceDocumentWindow *>
        windows =
        root->m_detachedWindows;

    root->m_detachedWindows.clear();

    for (DetachedWorkspaceDocumentWindow *window
         : windows) {
        if (window == nullptr) {
            continue;
        }

        window->hide();
        window->deleteLater();
    }

    root->m_activeDocumentId.clear();

    QWidget *rootWindow =
        root->window();

    if (rootWindow != nullptr) {
        rootWindow->show();
    }
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

    const bool added =
        insertLocalDocument(
            document,
            m_tabs->count(),
            makeCurrent
            );

    if (added) {
        emit m_rootHost
            ->workspaceLayoutChanged();
    }

    return added;
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

    disconnect(
        document,
        &WorkspaceDocument::
        documentTabPresentationChanged,
        this,
        &WorkspaceDocumentHost::
        updateDocumentTabPresentation
        );

    m_tabs
        ->workspaceTabBar()
        ->clearDocumentTint(
            documentId
            );

    m_tabs->removeTab(
        index
        );

    updateEmptyStatePresentation();

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

    updateEmptyStatePresentation();

    m_tabs->tabBar()
        ->setTabData(
            insertedIndex,
            document->documentId()
            );

    m_tabs->setTabToolTip(
        insertedIndex,
        document->tabToolTip()
        );

    m_tabs
        ->workspaceTabBar()
        ->setDocumentTint(
            document->documentId(),
            document->tabTint()
            );

    refreshDocumentTabAccessory(
        document,
        insertedIndex
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

    connect(
        document,
        &WorkspaceDocument::
        documentTabPresentationChanged,
        this,
        &WorkspaceDocumentHost::
        updateDocumentTabPresentation,
        Qt::UniqueConnection
        );

    connect(
        document,
        &WorkspaceDocument::
        investigationReportExportRequested,
        m_rootHost,
        &WorkspaceDocumentHost::
        investigationReportExportRequested,
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

    root->cleanupEmptyHost(
        host
        );

    if (document != nullptr) {
        emit root
            ->workspaceLayoutChanged();
    }

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

    /*
     * QTabWidget::currentChanged is not emitted if
     * this document is already the current tab in
     * its local group. An explicit
     * setCurrentDocument() call nevertheless means
     * this document is now the globally active
     * workspace document.
     */
    root->m_activeDocumentId =
        documentId;

    QWidget *targetWindow =
        host == root
            ? root->window()
            : static_cast<QWidget *>(
                  root->windowForHost(
                      host
                      )
                  );

    if (targetWindow != nullptr) {
        targetWindow->show();
        targetWindow->raise();
        targetWindow->activateWindow();
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
            ->cleanupEmptyHost(
                sourceHost
                );
    }

    emit m_rootHost
        ->workspaceLayoutChanged();

    return true;
}

DetachedWorkspaceDocumentWindow *
WorkspaceDocumentHost::createDetachedWindow(
    const QPoint &globalPosition
    )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    /*
     * Visible TraceScope windows are native top-level
     * peers. The root WorkspaceDocumentHost remains the
     * logical workspace coordinator, but it must not be
     * the QWidget owner of secondary windows.
     *
     * Giving a secondary top-level window the root
     * window as its QWidget parent creates an OS-level
     * owned-window relationship on Windows, which forces
     * the secondary window to remain above its owner.
     */
    auto *window =
        new DetachedWorkspaceDocumentWindow(
            root,
            nullptr
            );

    root->m_detachedWindows
        .push_back(
            window
            );

    /*
     * Secondary windows no longer have QObject/QWidget
     * ownership through the root window, so keep the
     * root's non-owning window registry synchronized
     * explicitly.
     */
    connect(
        window,
        &QObject::destroyed,
        root,
        [
            root,
            window
        ]() {
            root->m_detachedWindows
                .removeOne(
                    window
                    );
        }
        );

    /*
     * The root application coordinator can now wire
     * this otherwise ordinary workspace window into
     * shared TraceScope commands and command state.
     */
    emit root->detachedWindowCreated(
        window
        );

    /*
     * Position the new workspace near the user's
     * release point without hiding the tab beneath
     * the cursor.
     */
    window->move(
        globalPosition
        - QPoint(
            InterfaceScale::pixels(
                80,
                this
                ),
            InterfaceScale::pixels(
                20,
                this
                )
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

WorkspaceDocumentLayoutState
    WorkspaceDocumentHost::
    captureLayoutState() const
{
    const WorkspaceDocumentHost *root =
        m_rootHost;

    WorkspaceDocumentLayoutState state;

    for (WorkspaceDocument *document
         : root->localDocuments()) {
        if (document != nullptr) {
            state.dockedGroup.documentIds.append(
                document->documentId()
                );
        }
    }

    if (WorkspaceDocument *current =
        root->currentDocument();
        current != nullptr) {
        state.dockedGroup.currentDocumentId =
            current->documentId();
    }

    for (DetachedWorkspaceDocumentWindow *window
         : root->m_detachedWindows) {
        if (window == nullptr
            || window->documentHost() == nullptr) {
            continue;
        }

        WorkspaceDocumentHost *host =
            window->documentHost();

        if (host->documentCount() <= 0) {
            continue;
        }

        DetachedWorkspaceWindowLayoutState
            windowState;

        for (WorkspaceDocument *document
             : host->localDocuments()) {
            if (document != nullptr) {
                windowState
                    .group
                    .documentIds
                    .append(
                        document->documentId()
                        );
            }
        }

        if (WorkspaceDocument *current =
            host->currentDocument();
            current != nullptr) {
            windowState.group.currentDocumentId =
                current->documentId();
        }

        windowState.maximized =
            window->isMaximized();

        windowState.geometry =
            windowState.maximized
                ? window->normalGeometry()
                : window->geometry();

        state.detachedWindows.append(
            windowState
            );
    }

    state.activeDocumentId =
        root->m_activeDocumentId;

    return state;
}

void WorkspaceDocumentHost::
    restoreLayoutState(
        const WorkspaceDocumentLayoutState &state
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (root != this) {
        root->restoreLayoutState(
            state
            );

        return;
    }

    /*
     * Restore the root tab order first. Missing
     * documents are intentionally ignored so a
     * partially recovered workspace remains usable.
     */
    int targetIndex = 0;

    for (const QString &documentId
         : state.dockedGroup.documentIds) {
        if (root->owningHost(documentId)
            != root) {
            continue;
        }

        if (root->moveLocalDocumentToIndex(
                documentId,
                targetIndex
                )) {
            ++targetIndex;
        }
    }

    /*
     * Recreate detached groups from documents that
     * currently exist in the root host. A group
     * whose documents could not be restored is
     * simply omitted.
     */
    for (const DetachedWorkspaceWindowLayoutState
             &windowState
         : state.detachedWindows) {
        QStringList availableIds;

        for (const QString &documentId
             : windowState.group.documentIds) {
            if (root->owningHost(documentId)
                == root) {
                availableIds.append(
                    documentId
                    );
            }
        }

        if (availableIds.isEmpty()) {
            continue;
        }

        const QPoint initialPosition =
            windowState.geometry.isValid()
                ? windowState.geometry.topLeft()
                      + QPoint(80, 20)
                : QCursor::pos();

        DetachedWorkspaceDocumentWindow *window =
            root->createDetachedWindow(
                initialPosition
                );

        if (window == nullptr
            || window->documentHost()
                   == nullptr) {
            continue;
        }

        WorkspaceDocumentHost *targetHost =
            window->documentHost();

        int detachedIndex = 0;

        for (const QString &documentId
             : availableIds) {
            if (root->transferDocument(
                    root,
                    documentId,
                    targetHost,
                    detachedIndex,
                    false,
                    false
                    )) {
                ++detachedIndex;
            }
        }

        if (targetHost->documentCount() <= 0) {
            root->cleanupEmptyHost(
                targetHost
                );

            continue;
        }

        const int currentIndex =
            targetHost->indexOfDocument(
                windowState
                    .group
                    .currentDocumentId
                );

        targetHost->ensureLocalCurrentDocument(
            currentIndex >= 0
                ? currentIndex
                : 0
            );

        if (windowState.geometry.isValid()) {
            window->setGeometry(
                windowState.geometry
                );
        }

        if (windowState.maximized) {
            window->showMaximized();
        } else {
            window->show();
        }
    }

    /*
     * Restore the root group's current tab without
     * disturbing the independently saved globally
     * active workspace document.
     */
    const int dockedCurrentIndex =
        root->indexOfDocument(
            state.dockedGroup.currentDocumentId
            );

    if (dockedCurrentIndex >= 0) {
        root->ensureLocalCurrentDocument(
            dockedCurrentIndex
            );
    } else if (root->documentCount() > 0) {
        root->ensureLocalCurrentDocument(
            0
            );
    }

    /*
     * Make the globally active document last. This
     * also raises its detached window when needed.
     */
    if (!state.activeDocumentId.isEmpty()
        && root->documentById(
               state.activeDocumentId
               )
               != nullptr) {
        root->setCurrentDocument(
            state.activeDocumentId
            );
    }

    /*
     * A saved workspace may legitimately contain only
     * secondary document groups. Once restoration has
     * recreated those windows, eliminate an empty root
     * presentation just as interactive tab movement would.
     */
    root->cleanupEmptyHost(
        root
        );
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

    /*
     * The document has successfully moved between
     * workspace tab groups.
     */
    emit root->workspaceLayoutChanged();

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

        pending.clear();

        return;
    }

    emit root
        ->workspaceLayoutChanged();

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
    cleanupEmptyHost(
        WorkspaceDocumentHost *host
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    if (host == nullptr
        || host->documentCount() > 0) {
        return;
    }

    /*
     * The internal root window is not privileged in
     * the visible UI.
     *
     * If its last document moved elsewhere, hide it
     * once another workspace window exists. If it is
     * the final window, leave it visible with the
     * empty-workspace start state.
     */
    if (host == root) {
        QWidget *rootWindow =
            root->window();

        if (rootWindow != nullptr
            && root
                   ->hasOtherVisibleWorkspaceWindow(
                       rootWindow
                       )) {
            rootWindow->hide();
        }

        return;
    }

    DetachedWorkspaceDocumentWindow *window =
        root->windowForHost(
            host
            );

    if (window == nullptr) {
        return;
    }

    /*
     * A root host containing documents must itself
     * remain a visible peer. This also covers the
     * programmatic redock path where the root may
     * previously have been hidden.
     */
    QWidget *rootWindow =
        root->window();

    if (root->documentCount() > 0
        && rootWindow != nullptr
        && !rootWindow->isVisible()) {
        rootWindow->show();
    }

    /*
     * Preserve the final visible window even when its
     * last document closes. It becomes the ordinary
     * TraceScope empty-workspace window.
     */
    if (!root
             ->hasOtherVisibleWorkspaceWindow(
                 window
                 )) {
        return;
    }

    /*
     * Another usable TraceScope window remains, so
     * this empty peer is redundant.
     */
    root->m_detachedWindows
        .removeOne(
            window
            );

    window->hide();
    window->deleteLater();
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
    updateDocumentTabPresentation()
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

    m_tabs->setTabToolTip(
        index,
        document->tabToolTip()
        );

    m_tabs
        ->workspaceTabBar()
        ->setDocumentTint(
            document->documentId(),
            document->tabTint()
            );

    refreshDocumentTabAccessory(
        document,
        index
        );
}

void WorkspaceDocumentHost::
    refreshDocumentTabAccessory(
        WorkspaceDocument *document,
        int index
        )
{
    if (document == nullptr
        || index < 0
        || index >= m_tabs->count()) {
        return;
    }

    WorkspaceTabBar *tabBar =
        m_tabs->workspaceTabBar();

    QWidget *existingAccessory =
        tabBar->tabButton(
            index,
            QTabBar::LeftSide
            );

    QWidget *newAccessory =
        document
            ->createTabAccessoryWidget(
                tabBar
                );

    /*
     * Nothing changed for documents that neither had
     * nor now require an accessory.
     */
    if (existingAccessory == nullptr
        && newAccessory == nullptr) {
        return;
    }

    /*
     * Remove the previous accessory explicitly before
     * installing the newly derived presentation.
     *
     * Investigation backing transitions can change
     * whether live-follow controls are supported.
     */
    if (existingAccessory != nullptr) {
        tabBar->setTabButton(
            index,
            QTabBar::LeftSide,
            nullptr
            );

        existingAccessory->deleteLater();
    }

    if (newAccessory != nullptr) {
        tabBar->setTabButton(
            index,
            QTabBar::LeftSide,
            newAccessory
            );
    }
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

    root->setWorkspaceDocumentDragActive(
        true
        );

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

bool WorkspaceDocumentHost::
    moveLocalDocumentToIndex(
        const QString &documentId,
        int targetIndex
        )
{
    const int sourceIndex =
        indexOfDocument(
            documentId
            );

    if (sourceIndex < 0
        || m_tabs == nullptr
        || m_tabs->count() <= 0) {
        return false;
    }

    const int boundedTarget =
        qBound(
            0,
            targetIndex,
            m_tabs->count() - 1
            );

    if (sourceIndex == boundedTarget) {
        return true;
    }

    m_tabs->tabBar()->moveTab(
        sourceIndex,
        boundedTarget
        );

    return true;
}

void WorkspaceDocumentHost::
    setWorkspaceDocumentDragActive(
        bool active
        )
{
    WorkspaceDocumentHost *root =
        m_rootHost;

    root->m_workspaceDocumentDragActive =
        active;

    root->updateEmptyStatePresentation();

    for (DetachedWorkspaceDocumentWindow *window
         : std::as_const(
             root->m_detachedWindows
             )) {
        if (window == nullptr
            || window->documentHost()
                   == nullptr) {
            continue;
        }

        window->documentHost()
            ->updateEmptyStatePresentation();
    }
}

void WorkspaceDocumentHost::
    refreshEmptyStateTitleFont()
{
    if (m_emptyStateTitleLabel == nullptr) {
        return;
    }

    /*
     * Always derive the title from the current
     * application font, never the title's previously
     * scaled font.
     *
     * This reproduces the same result whether the
     * application starts at a particular scale or
     * changes to it while running.
     */
    QFont titleFont =
        QApplication::font();

    titleFont.setBold(true);

    if (titleFont.pointSizeF() > 0.0) {
        titleFont.setPointSizeF(
            titleFont.pointSizeF() * 1.5
            );
    } else if (titleFont.pixelSize() > 0) {
        titleFont.setPixelSize(
            qRound(
                titleFont.pixelSize() * 1.5
                )
            );
    }

    m_emptyStateTitleLabel->setFont(
        titleFont
        );
}

void WorkspaceDocumentHost::
    updateEmptyStatePresentation()
{
    if (m_tabs == nullptr) {
        return;
    }

    const bool empty =
        m_tabs->count() == 0;

    if (m_emptyStateWidget != nullptr) {
        m_emptyStateWidget->setVisible(
            empty
            && !m_rootHost
                    ->m_workspaceDocumentDragActive
            );
    }

    WorkspaceTabBar *tabBar =
        m_tabs->workspaceTabBar();

    if (tabBar != nullptr) {
        tabBar->setEmptyDropTargetActive(
            empty
            && m_rootHost
                   ->m_workspaceDocumentDragActive
            );
    }

    /*
     * The empty page is an overlay, not a normal tab.
     * Qt therefore excludes its content from the
     * tab widget's natural minimum height.
     *
     * Reserve both its actual content requirement
     * and the normal styled tab-row height.
     *
     * Release the explicit constraint when a document
     * opens. Normal document sizing then takes over.
     */
    int minimumHeight = 0;

    if (empty
        && m_emptyStateWidget != nullptr) {
        minimumHeight =
            m_emptyStateWidget
                ->minimumSizeHint()
                .height()
            + InterfaceScale::pixels(
                2,
                m_emptyStateWidget
                );

        if (tabBar != nullptr) {
            minimumHeight +=
                tabBar->normalTabRowHeight();

            minimumHeight += std::max(
                0,
                m_tabs->style()->pixelMetric(
                    QStyle::PM_DefaultFrameWidth,
                    nullptr,
                    m_tabs
                    )
                );
        }
    }

    m_tabs->setMinimumHeight(
        minimumHeight + (empty ? 1 : 0)
        );

    m_tabs->updateEmptyStateGeometry();

    m_tabs->updateGeometry();
    updateGeometry();
}

void WorkspaceDocumentHost::
    setFileOperationsEnabled(
        bool enabled
        )
{
    m_fileOperationsEnabled =
        enabled;

    if (m_emptyStateOpenLogButton != nullptr) {
        m_emptyStateOpenLogButton
            ->setEnabled(
                enabled
                );
    }

    if (m_emptyStateOpenSnapshotButton != nullptr) {
        m_emptyStateOpenSnapshotButton
            ->setEnabled(
                enabled
                );
    }

    if (m_emptyStateOpenWorkspaceButton != nullptr) {
        m_emptyStateOpenWorkspaceButton
            ->setEnabled(
                enabled
                );
    }

    if (m_emptyStateRecentFilesButton
        != nullptr) {
        m_emptyStateRecentFilesButton
            ->setEnabled(
                enabled
                && m_recentFilesAvailable
                );
    }

    if (m_emptyStateRecentWorkspacesButton
        != nullptr) {
        m_emptyStateRecentWorkspacesButton
            ->setEnabled(
                enabled
                && m_recentWorkspacesAvailable
                );
    }
}

void WorkspaceDocumentHost::
    setRecentFilesAvailable(
        bool available
        )
{
    m_recentFilesAvailable =
        available;

    if (m_emptyStateRecentFilesButton
        != nullptr) {
        m_emptyStateRecentFilesButton
            ->setEnabled(
                m_fileOperationsEnabled
                && m_recentFilesAvailable
                );
    }
}

void WorkspaceDocumentHost::
    setRecentWorkspacesAvailable(
        bool available
        )
{
    m_recentWorkspacesAvailable =
        available;

    if (m_emptyStateRecentWorkspacesButton
        != nullptr) {
        m_emptyStateRecentWorkspacesButton
            ->setEnabled(
                m_fileOperationsEnabled
                && m_recentWorkspacesAvailable
                );
    }
}

void WorkspaceDocumentHost::
    refreshInterfaceScale()
{
    refreshEmptyStateTitleFont();

    if (m_emptyStateWidget != nullptr) {
        if (QLayout *emptyLayout =
            m_emptyStateWidget->layout();
            emptyLayout != nullptr) {
            emptyLayout->setContentsMargins(
                InterfaceScale::margins(
                    20,
                    24,
                    20,
                    24,
                    m_emptyStateWidget
                    )
                );

            emptyLayout->invalidate();
        }
    }

    if (m_emptyStateActionSpacing != nullptr) {
        m_emptyStateActionSpacing->changeSize(
            0,
            InterfaceScale::pixels(
                14,
                m_emptyStateWidget
                ),
            QSizePolicy::Minimum,
            QSizePolicy::Fixed
            );
    }

    if (m_tabs != nullptr) {
        WorkspaceTabBar *tabBar =
            m_tabs->workspaceTabBar();

        if (tabBar != nullptr) {
            /*
             * WorkspaceTabBar calculates its scaled
             * empty-target dimensions dynamically in
             * sizeHint()/paintEvent(), so it only needs
             * geometry invalidation and repainting.
             */
            tabBar->updateGeometry();
            tabBar->update();
        }
    }

    for (
        WorkspaceDocument *document
        : localDocuments()
        ) {
        if (document != nullptr) {
            document->refreshInterfaceScale();
        }
    }

    updateGeometry();
    update();


    updateEmptyStatePresentation();

    /*
     * The application has changed its font and style,
     * but Qt may still have pending layout updates.
     *
     * Recalculate the empty overlay's minimum after
     * that layout work has had an opportunity to run.
     * Do not resize the outer window.
     */
    QTimer::singleShot(
        0,
        this,
        [this]() {
            if (
                m_tabs == nullptr
                || m_emptyStateWidget == nullptr
                || m_tabs->count() != 0
                ) {
                return;
            }

            if (QLayout *emptyLayout =
                m_emptyStateWidget->layout();
                emptyLayout != nullptr) {
                emptyLayout->invalidate();
                emptyLayout->activate();
            }

            updateEmptyStatePresentation();
        }
        );
}