#include "WorkspaceDocument.h"

#include <utility>

#include <QAction>
#include <QMenu>

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
    if (menu == nullptr) {
        return;
    }

    menu->setToolTipsVisible(
        true
        );

    QAction *reportAction =
        menu->addAction(
            tr(
                "Export Investigation Report..."
                )
            );

    reportAction->setToolTip(
        tr(
            "Create a self-contained HTML report "
            "from this investigation and other "
            "selected workspace documents"
            )
        );

    connect(
        reportAction,
        &QAction::triggered,
        this,
        [this]() {
            emit investigationReportExportRequested(
                documentId()
                );
        }
        );
}