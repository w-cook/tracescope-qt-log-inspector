#include "DetachedWorkspaceDocumentWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

#include "WorkspaceDocument.h"
#include "WorkspaceDocumentHost.h"

DetachedWorkspaceDocumentWindow::
    DetachedWorkspaceDocumentWindow(
        WorkspaceDocumentHost *rootHost,
        QWidget *parent
        )
    : QMainWindow(
          parent,
          Qt::Window
          )
{
    /*
     * Do not expose the old "Detached Workspace"
     * terminology. This window belongs to the same
     * TraceScope workspace as every other application
     * window.
     *
     * Phase 16 workspace-identity work will replace
     * this temporary shared product title with the
     * current .tsw identity.
     */
    setWindowTitle(
        tr(
            "TraceScope — Qt Telemetry Log Inspector"
            )
        );

    resize(
        1100,
        760
        );

    m_documentHost =
        new WorkspaceDocumentHost(
            this,
            rootHost
            );

    setCentralWidget(
        m_documentHost
        );

    createMenus();
}

void DetachedWorkspaceDocumentWindow::
    createMenus()
{
    QMenu *fileMenu =
        menuBar()->addMenu(
            tr("&File")
            );

    m_openAction =
        new QAction(
            tr("&Open Log File..."),
            this
            );

    m_openAction->setShortcut(
        QKeySequence::Open
        );

    m_openAction->setShortcutContext(
        Qt::WindowShortcut
        );

    connect(
        m_openAction,
        &QAction::triggered,
        this,
        [this]() {
            emit openLogRequested(
                m_documentHost
                );
        }
        );

    fileMenu->addAction(
        m_openAction
        );

    m_openSnapshotAction =
        new QAction(
            tr(
                "Open Investigation &Snapshot..."
                ),
            this
            );

    connect(
        m_openSnapshotAction,
        &QAction::triggered,
        this,
        [this]() {
            emit openSnapshotRequested(
                m_documentHost
                );
        }
        );

    fileMenu->addAction(
        m_openSnapshotAction
        );

    m_openWorkspaceAction =
        new QAction(
            tr("Open &Workspace..."),
            this
            );

    connect(
        m_openWorkspaceAction,
        &QAction::triggered,
        this,
        [this]() {
            emit openWorkspaceRequested();
        }
        );

    fileMenu->addAction(
        m_openWorkspaceAction
        );

    m_recentFilesMenu =
        fileMenu->addMenu(
            tr("Recent &Files")
            );

    m_recentFilesMenu->setToolTipsVisible(
        true
        );

    connect(
        m_recentFilesMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            emit recentFilesMenuAboutToShow(
                m_recentFilesMenu,
                m_documentHost
                );
        }
        );

    m_recentWorkspacesMenu =
        fileMenu->addMenu(
            tr("Recent &Workspaces")
            );

    m_recentWorkspacesMenu->setToolTipsVisible(
        true
        );

    connect(
        m_recentWorkspacesMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            emit recentWorkspacesMenuAboutToShow(
                m_recentWorkspacesMenu
                );
        }
        );

    fileMenu->addSeparator();

    m_saveSnapshotAction =
        new QAction(
            tr(
                "Save Investigation &Snapshot..."
                ),
            this
            );

    m_saveSnapshotAction->setEnabled(
        false
        );

    connect(
        m_saveSnapshotAction,
        &QAction::triggered,
        this,
        [this]() {
            emit saveSnapshotRequested(
                m_documentHost
                );
        }
        );

    fileMenu->addAction(
        m_saveSnapshotAction
        );

    m_reloadAction =
        new QAction(
            tr("&Reload Current Session"),
            this
            );

    m_reloadAction->setShortcut(
        QKeySequence::Refresh
        );

    m_reloadAction->setShortcutContext(
        Qt::WindowShortcut
        );

    m_reloadAction->setEnabled(
        false
        );

    connect(
        m_reloadAction,
        &QAction::triggered,
        this,
        [this]() {
            emit reloadRequested(
                m_documentHost
                );
        }
        );

    fileMenu->addAction(
        m_reloadAction
        );

    fileMenu->addSeparator();

    m_saveWorkspaceAction =
        new QAction(
            tr("&Save Workspace"),
            this
            );

    m_saveWorkspaceAction->setShortcut(
        QKeySequence::Save
        );

    m_saveWorkspaceAction->setShortcutContext(
        Qt::WindowShortcut
        );

    connect(
        m_saveWorkspaceAction,
        &QAction::triggered,
        this,
        [this]() {
            emit saveWorkspaceRequested();
        }
        );

    fileMenu->addAction(
        m_saveWorkspaceAction
        );

    m_saveWorkspaceAsAction =
        new QAction(
            tr("Save Workspace &As..."),
            this
            );

    m_saveWorkspaceAsAction->setShortcut(
        QKeySequence::SaveAs
        );

    m_saveWorkspaceAsAction->setShortcutContext(
        Qt::WindowShortcut
        );

    connect(
        m_saveWorkspaceAsAction,
        &QAction::triggered,
        this,
        [this]() {
            emit saveWorkspaceAsRequested();
        }
        );

    fileMenu->addAction(
        m_saveWorkspaceAsAction
        );

    /*
     * Export actions belong to WorkspaceDocument
     * itself, so this menu requires no coordinator
     * routing. It always operates directly on this
     * window's current document.
     */
    QMenu *exportingMenu =
        menuBar()->addMenu(
            tr("&Exporting")
            );

    connect(
        exportingMenu,
        &QMenu::aboutToShow,
        this,
        [this, exportingMenu]() {
            exportingMenu->clear();

            WorkspaceDocument *document =
                m_documentHost != nullptr
                    ? m_documentHost
                          ->currentDocument()
                    : nullptr;

            if (document != nullptr) {
                document->populateExportMenu(
                    exportingMenu
                    );
            }

            if (exportingMenu
                    ->actions()
                    .isEmpty()) {
                QAction *unavailableAction =
                    exportingMenu->addAction(
                        tr(
                            "No export actions available"
                            )
                        );

                unavailableAction->setEnabled(
                    false
                    );
            }
        }
        );

    QMenu *investigationMenu =
        menuBar()->addMenu(
            tr("&Investigation")
            );

    m_compareAction =
        new QAction(
            tr("&Compare Sessions..."),
            this
            );

    m_compareAction->setEnabled(
        false
        );

    connect(
        m_compareAction,
        &QAction::triggered,
        this,
        [this]() {
            emit compareSessionsRequested(
                m_documentHost
                );
        }
        );

    investigationMenu->addAction(
        m_compareAction
        );

    QMenu *helpMenu =
        menuBar()->addMenu(
            tr("&Help")
            );

    QAction *aboutAction =
        new QAction(
            tr("&About TraceScope"),
            this
            );

    connect(
        aboutAction,
        &QAction::triggered,
        this,
        [this]() {
            /*
             * This text intentionally matches the
             * current root-window About dialog.
             * Product-description cleanup belongs to
             * the later v1 identity pass.
             */
            QMessageBox::about(
                this,
                tr("About TraceScope"),
                tr(
                    "TraceScope is a Qt/C++ telemetry "
                    "log inspector for loading, filtering, "
                    "visualizing, and exporting structured "
                    "diagnostic log files."
                    )
                );
        }
        );

    helpMenu->addAction(
        aboutAction
        );
}

WorkspaceDocumentHost *
    DetachedWorkspaceDocumentWindow::
    documentHost() const
{
    return m_documentHost;
}

void DetachedWorkspaceDocumentWindow::
    setFileOperationsEnabled(
        bool enabled
        )
{
    if (m_openAction != nullptr) {
        m_openAction->setEnabled(
            enabled
            );
    }

    if (m_openSnapshotAction != nullptr) {
        m_openSnapshotAction->setEnabled(
            enabled
            );
    }

    if (m_openWorkspaceAction != nullptr) {
        m_openWorkspaceAction->setEnabled(
            enabled
            );
    }
}

void DetachedWorkspaceDocumentWindow::
    setSaveSnapshotEnabled(
        bool enabled
        )
{
    if (m_saveSnapshotAction != nullptr) {
        m_saveSnapshotAction->setEnabled(
            enabled
            );
    }
}

void DetachedWorkspaceDocumentWindow::
    setReloadEnabled(
        bool enabled
        )
{
    if (m_reloadAction != nullptr) {
        m_reloadAction->setEnabled(
            enabled
            );
    }
}

void DetachedWorkspaceDocumentWindow::
    setCompareEnabled(
        bool enabled
        )
{
    if (m_compareAction != nullptr) {
        m_compareAction->setEnabled(
            enabled
            );
    }
}

void DetachedWorkspaceDocumentWindow::
    closeEvent(
        QCloseEvent *event
        )
{
    /*
     * Closing the final visible TraceScope window is
     * application closure, regardless of whether the
     * internal coordinator happens to be the hidden
     * root MainWindow.
     *
     * QApplication's normal last-window behavior will
     * terminate the event loop after this peer closes.
     * Dirty-workspace interception will be added later
     * at the shared application-close boundary.
     */
    if (m_documentHost == nullptr
        || !m_documentHost
                ->hasOtherVisibleWorkspaceWindow(
                    this
                    )) {
        QMainWindow::closeEvent(
            event
            );

        return;
    }

    if (m_documentHost
            ->documentCount()
        == 0) {
        QMainWindow::closeEvent(
            event
            );

        return;
    }

    /*
     * Other TraceScope windows remain. Closing this
     * window therefore means closing only the documents
     * in this tab group.
     */
    event->ignore();

    emit closeAllRequested(
        this
        );
}