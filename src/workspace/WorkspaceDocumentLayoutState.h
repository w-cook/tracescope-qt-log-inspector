#pragma once

#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector>

struct WorkspaceDocumentGroupLayoutState
{
    QStringList documentIds;
    QString currentDocumentId;
};

struct DetachedWorkspaceWindowLayoutState
{
    WorkspaceDocumentGroupLayoutState group;

    /*
     * Store normal geometry even when maximized so
     * restoring/unmaximizing returns to the saved
     * useful window size and position.
     */
    QRect geometry;

    bool maximized = false;
};

struct WorkspaceDocumentLayoutState
{
    WorkspaceDocumentGroupLayoutState dockedGroup;

    QVector<DetachedWorkspaceWindowLayoutState>
        detachedWindows;

    /*
     * The current tab of each group is independent
     * from which workspace document/window was most
     * recently active.
     */
    QString activeDocumentId;
};