#include "DetachedWorkspaceDocumentWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QToolBar>

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
    setWindowTitle(
        tr(
            "TraceScope — Detached Workspace"
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

    auto *toolBar =
        addToolBar(
            tr("Workspace")
            );

    toolBar->setMovable(
        false
        );

    QAction *redockAction =
        toolBar->addAction(
            tr("Re-dock Window")
            );

    redockAction->setToolTip(
        tr(
            "Return all tabs in this window "
            "to the main workspace."
            )
        );

    connect(
        redockAction,
        &QAction::triggered,
        this,
        [this]() {
            emit redockAllRequested(
                this
                );
        }
        );
}

WorkspaceDocumentHost *
    DetachedWorkspaceDocumentWindow::
    documentHost() const
{
    return m_documentHost;
}

void DetachedWorkspaceDocumentWindow::
    closeEvent(
        QCloseEvent *event
        )
{
    if (m_documentHost == nullptr
        || m_documentHost
            ->documentCount()
                == 0) {
        event->accept();
        return;
    }

    /*
     * A detached workspace window owns a group of
     * open document presentations. Closing the
     * window means closing every document currently
     * contained in that group.
     *
     * Actual document lifetime remains controlled
     * by the root workspace close contract.
     */
    event->ignore();

    emit closeAllRequested(
        this
        );
}