#pragma once

#include <QString>
#include <QWidget>

class QTabWidget;
class WorkspaceDocument;

class WorkspaceDocumentHost
    : public QWidget
{
    Q_OBJECT

public:
    explicit WorkspaceDocumentHost(
        QWidget *parent = nullptr
        );

    int documentCount() const;

    WorkspaceDocument *documentAt(
        int index
        ) const;

    WorkspaceDocument *currentDocument()
        const;

    int indexOfDocument(
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

signals:
    void currentDocumentChanged(
        const QString &documentId
        );

    /*
     * The host reports close intent but does not
     * decide whether the underlying investigation
     * session/comparison should actually be
     * destroyed.
     */
    void documentCloseRequested(
        const QString &documentId
        );

private slots:
    void updateDocumentTitle(
        const QString &title
        );

private:
    QTabWidget *m_tabs;
};