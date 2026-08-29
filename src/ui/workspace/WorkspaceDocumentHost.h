#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include "../../workspace/WorkspaceDocumentLayoutState.h"

class QMenu;
class DetachedWorkspaceDocumentWindow;
class WorkspaceDocument;
class WorkspaceTabWidget;

class WorkspaceDocumentHost
    : public QWidget
{
    Q_OBJECT

public:
    explicit WorkspaceDocumentHost(
        QWidget *parent = nullptr,
        WorkspaceDocumentHost *rootHost = nullptr
        );

    /*
     * These describe this particular tab group.
     */
    int documentCount() const;

    WorkspaceDocument *documentAt(
        int index
        ) const;

    WorkspaceDocument *currentDocument()
        const;

    int indexOfDocument(
        const QString &documentId
        ) const;

    /*
     * These operate across the complete workspace:
     * the main tab group plus all detached groups.
     */
    WorkspaceDocument *documentById(
        const QString &documentId
        ) const;

    QVector<WorkspaceDocument *>
    documents() const;

    bool isDocumentDetached(
        const QString &documentId
        ) const;

    bool addDocument(
        WorkspaceDocument *document,
        bool makeCurrent = true
        );

    WorkspaceDocument *removeDocument(
        const QString &documentId
        );

    bool setCurrentDocument(
        const QString &documentId
        );

    /*
     * Explicit fallback action. Dragging is the
     * primary interaction.
     */
    bool detachDocument(
        const QString &documentId
        );

    bool redockDocument(
        const QString &documentId,
        int targetIndex = -1
        );

    WorkspaceDocumentLayoutState
    captureLayoutState() const;

    void restoreLayoutState(
        const WorkspaceDocumentLayoutState &state
        );

signals:
    void currentDocumentChanged(
        const QString &documentId
        );

    void documentCloseRequested(
        const QString &documentId
        );

    void documentDetached(
        const QString &documentId
        );

    void documentRedocked(
        const QString &documentId
        );

    void documentContextMenuAboutToShow(
        const QString &documentId,
        QMenu *menu
        );

private slots:
    void updateDocumentTitle(
        const QString &title
        );

private:
    struct PendingDocumentDrag
    {
        WorkspaceDocument *document =
            nullptr;

        WorkspaceDocumentHost *sourceHost =
            nullptr;

        int sourceIndex =
            -1;

        bool active() const
        {
            return document != nullptr;
        }

        void clear()
        {
            document = nullptr;
            sourceHost = nullptr;
            sourceIndex = -1;
        }
    };

    WorkspaceDocument *
    localDocumentById(
        const QString &documentId
        ) const;

    QVector<WorkspaceDocument *>
    localDocuments() const;

    WorkspaceDocument *
    takeLocalDocument(
        const QString &documentId
        );

    bool insertLocalDocument(
        WorkspaceDocument *document,
        int index,
        bool makeCurrent
        );

    WorkspaceDocumentHost *
    owningHost(
        const QString &documentId
        ) const;

    void handleDocumentDrop(
        WorkspaceDocumentHost *targetHost,
        const QString &documentId,
        int targetIndex
        );

    void handleDocumentTearOut(
        const QString &documentId,
        const QPoint &globalPosition
        );

    bool transferDocument(
        WorkspaceDocumentHost *sourceHost,
        const QString &documentId,
        WorkspaceDocumentHost *targetHost,
        int targetIndex,
        bool makeCurrent = true,
        bool cleanupEmptySource = true
        );

    DetachedWorkspaceDocumentWindow *
    createDetachedWindow(
        const QPoint &globalPosition
        );

    DetachedWorkspaceDocumentWindow *
    windowForHost(
        WorkspaceDocumentHost *host
        ) const;

    void cleanupEmptyDetachedHost(
        WorkspaceDocumentHost *host
        );

    void redockDetachedWindow(
        DetachedWorkspaceDocumentWindow *window
        );

    void beginDocumentDrag(
        WorkspaceDocumentHost *sourceHost,
        const QString &documentId
        );

    void ensureLocalCurrentDocument(
        int preferredIndex = -1
        );

    bool moveLocalDocumentToIndex(
        const QString &documentId,
        int targetIndex
        );

    WorkspaceDocumentHost *m_rootHost =
        nullptr;

    WorkspaceTabWidget *m_tabs =
        nullptr;

    /*
     * Only meaningful on the root host.
     */
    QVector<DetachedWorkspaceDocumentWindow *>
        m_detachedWindows;

    PendingDocumentDrag m_pendingDocumentDrag;

    QString m_activeDocumentId;
};