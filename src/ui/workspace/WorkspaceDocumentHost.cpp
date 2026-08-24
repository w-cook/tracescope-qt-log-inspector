#include "WorkspaceDocumentHost.h"

#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "WorkspaceDocument.h"

WorkspaceDocumentHost::WorkspaceDocumentHost(
    QWidget *parent
    )
    : QWidget(parent),
    m_tabs(
        new QTabWidget(this)
        )
{
    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->addWidget(
        m_tabs
        );

    m_tabs->setTabsClosable(
        true
        );

    m_tabs->setMovable(
        true
        );

    m_tabs->setDocumentMode(
        true
        );

    m_tabs->setUsesScrollButtons(
        true
        );

    m_tabs->tabBar()->setElideMode(
        Qt::ElideMiddle
        );

    connect(
        m_tabs,
        &QTabWidget::currentChanged,
        this,
        [this](int index) {
            WorkspaceDocument *document =
                documentAt(index);

            emit currentDocumentChanged(
                document != nullptr
                    ? document->documentId()
                    : QString()
                );
        }
        );

    connect(
        m_tabs,
        &QTabWidget::tabCloseRequested,
        this,
        [this](int index) {
            WorkspaceDocument *document =
                documentAt(index);

            if (document == nullptr) {
                return;
            }

            emit documentCloseRequested(
                document->documentId()
                );
        }
        );
}

int WorkspaceDocumentHost::documentCount()
    const
{
    return m_tabs->count();
}

WorkspaceDocument *
WorkspaceDocumentHost::documentAt(
    int index
    ) const
{
    if (index < 0
        || index >= m_tabs->count()) {
        return nullptr;
    }

    return qobject_cast<WorkspaceDocument *>(
        m_tabs->widget(index)
        );
}

WorkspaceDocument *
WorkspaceDocumentHost::currentDocument()
    const
{
    return documentAt(
        m_tabs->currentIndex()
        );
}

int WorkspaceDocumentHost::indexOfDocument(
    const QString &documentId
    ) const
{
    for (int index = 0;
         index < m_tabs->count();
         ++index) {
        WorkspaceDocument *document =
            documentAt(index);

        if (document != nullptr
            && document->documentId()
                   == documentId) {
            return index;
        }
    }

    return -1;
}

bool WorkspaceDocumentHost::addDocument(
    WorkspaceDocument *document,
    bool makeCurrent
    )
{
    if (document == nullptr
        || document->documentId()
               .trimmed()
               .isEmpty()
        || indexOfDocument(
               document->documentId()
               )
               >= 0) {
        return false;
    }

    const int index =
        m_tabs->addTab(
            document,
            document->documentTitle()
            );

    m_tabs->setTabToolTip(
        index,
        document->toolTip()
        );

    connect(
        document,
        &WorkspaceDocument::
        documentTitleChanged,
        this,
        &WorkspaceDocumentHost::
        updateDocumentTitle,
        Qt::UniqueConnection
        );

    if (makeCurrent) {
        m_tabs->setCurrentIndex(
            index
            );
    }

    return true;
}

WorkspaceDocument *
WorkspaceDocumentHost::removeDocument(
    const QString &documentId
    )
{
    const int index =
        indexOfDocument(
            documentId
            );

    if (index < 0) {
        return nullptr;
    }

    WorkspaceDocument *document =
        documentAt(index);

    m_tabs->removeTab(
        index
        );

    if (document != nullptr) {
        /*
         * Removal means "no longer hosted here,"
         * not "destroy this document."
         *
         * That distinction is what will later allow
         * the same widget instance to move between
         * the tab host and a detached window.
         */
        document->hide();
        document->setParent(
            nullptr
            );
    }

    return document;
}

bool WorkspaceDocumentHost::setCurrentDocument(
    const QString &documentId
    )
{
    const int index =
        indexOfDocument(
            documentId
            );

    if (index < 0) {
        return false;
    }

    m_tabs->setCurrentIndex(
        index
        );

    return true;
}

void WorkspaceDocumentHost::updateDocumentTitle(
    const QString &title
    )
{
    auto *document =
        qobject_cast<WorkspaceDocument *>(
            sender()
            );

    if (document == nullptr) {
        return;
    }

    const int index =
        m_tabs->indexOf(
            document
            );

    if (index < 0) {
        return;
    }

    m_tabs->setTabText(
        index,
        title
        );
}