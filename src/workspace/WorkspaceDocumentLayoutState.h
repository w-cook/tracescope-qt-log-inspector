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
    /*
     * Store normal geometry even when the main
     * window is maximized so restoring/unmaximizing
     * returns to the saved useful size and position.
     */
    QRect mainWindowGeometry;

    bool mainWindowMaximized = false;

    WorkspaceDocumentGroupLayoutState dockedGroup;

    QVector<DetachedWorkspaceWindowLayoutState>
        detachedWindows;

    QString activeDocumentId;
};