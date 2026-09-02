#include "WorkspaceDocument.h"

#include <utility>

WorkspaceDocument::WorkspaceDocument(
    QString documentId,
    QString documentTitle,
    QWidget *parent
    )
    : QWidget(parent),
    m_documentId(
        std::move(documentId)
        ),
    m_documentTitle(
        std::move(documentTitle)
        )
{
    setWindowTitle(
        m_documentTitle
        );
}

const QString &
WorkspaceDocument::documentId() const
{
    return m_documentId;
}

const QString &
WorkspaceDocument::documentTitle() const
{
    return m_documentTitle;
}

void WorkspaceDocument::setDocumentTitle(
    const QString &title
    )
{
    if (m_documentTitle == title) {
        return;
    }

    m_documentTitle =
        title;

    /*
     * Keep the QWidget window title synchronized
     * so this same document can later move into a
     * detached top-level window without requiring
     * a second title source.
     */
    setWindowTitle(
        m_documentTitle
        );

    emit documentTitleChanged(
        m_documentTitle
        );
}

void WorkspaceDocument::populateExportMenu(
    QMenu *menu
    )
{
    Q_UNUSED(menu);
}