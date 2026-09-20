#pragma once

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QMenu;
class WorkspaceDocumentHost;

class DetachedWorkspaceDocumentWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit DetachedWorkspaceDocumentWindow(
        WorkspaceDocumentHost *rootHost,
        QWidget *parent = nullptr
        );

    WorkspaceDocumentHost *
    documentHost() const;

    void setFileOperationsEnabled(
        bool enabled
        );

    void setSaveSnapshotEnabled(
        bool enabled
        );

    void setReloadEnabled(
        bool enabled
        );

    void setCompareEnabled(
        bool enabled
        );

    void setWorkspaceWindowTitle(
        const QString &title
        );

signals:
    /*
     * Window-local commands carry this window's
     * document host so the central application
     * coordinator can target the invoking window
     * explicitly.
     */
    void openLogRequested(
        WorkspaceDocumentHost *targetHost
        );

    void openSnapshotRequested(
        WorkspaceDocumentHost *targetHost
        );

    void newWorkspaceRequested();

    /*
     * Workspace open/save operations remain global
     * because one .tsw workspace spans every visible
     * TraceScope window.
     */
    void openWorkspaceRequested();

    void saveWorkspaceRequested();

    void saveWorkspaceAsRequested();

    void recentFilesMenuAboutToShow(
        QMenu *menu,
        WorkspaceDocumentHost *targetHost
        );

    void recentWorkspacesMenuAboutToShow(
        QMenu *menu
        );

    void saveSnapshotRequested(
        WorkspaceDocumentHost *targetHost
        );

    void reloadRequested(
        WorkspaceDocumentHost *targetHost
        );

    void compareSessionsRequested(
        WorkspaceDocumentHost *targetHost
        );

    void closeAllRequested(
        DetachedWorkspaceDocumentWindow *window
        );

    void applicationCloseRequested();

protected:
    void closeEvent(
        QCloseEvent *event
        ) override;

private:
    void createMenus();

    WorkspaceDocumentHost *m_documentHost =
        nullptr;

    QAction *m_openAction =
        nullptr;

    QAction *m_openSnapshotAction =
        nullptr;

    QAction *m_openWorkspaceAction =
        nullptr;

    QAction *m_saveSnapshotAction =
        nullptr;

    QAction *m_reloadAction =
        nullptr;

    QAction *m_newWorkspaceAction =
        nullptr;

    QAction *m_saveWorkspaceAction =
        nullptr;

    QAction *m_saveWorkspaceAsAction =
        nullptr;

    QAction *m_compareAction =
        nullptr;

    QMenu *m_recentFilesMenu =
        nullptr;

    QMenu *m_recentWorkspacesMenu =
        nullptr;
};