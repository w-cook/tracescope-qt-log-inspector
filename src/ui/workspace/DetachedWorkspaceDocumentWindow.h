#pragma once

#include <QMainWindow>

class QCloseEvent;
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

signals:
    void redockAllRequested(
        DetachedWorkspaceDocumentWindow *window
        );

    void closeAllRequested(
        DetachedWorkspaceDocumentWindow *window
        );

protected:
    void closeEvent(
        QCloseEvent *event
        ) override;

private:
    WorkspaceDocumentHost *m_documentHost =
        nullptr;
};