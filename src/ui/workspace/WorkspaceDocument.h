#pragma once

#include <QString>
#include <QWidget>

class WorkspaceDocument
    : public QWidget
{
    Q_OBJECT

public:
    explicit WorkspaceDocument(
        QString documentId,
        QString documentTitle,
        QWidget *parent = nullptr
        );

    const QString &documentId() const;

    const QString &documentTitle() const;

    void setDocumentTitle(
        const QString &title
        );

signals:
    void documentTitleChanged(
        const QString &title
        );

private:
    QString m_documentId;
    QString m_documentTitle;
};