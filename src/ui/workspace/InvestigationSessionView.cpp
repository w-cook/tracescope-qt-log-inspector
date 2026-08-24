#include "InvestigationSessionView.h"

#include <QVBoxLayout>

#include "../investigation/InvestigationSessionSummaryPanel.h"
#include "../../workspace/InvestigationSession.h"

namespace
{
QString documentIdFor(
    const InvestigationSession *session
    )
{
    return session != nullptr
               ? session->id()
               : QString();
}

QString documentTitleFor(
    const InvestigationSession *session
    )
{
    if (session == nullptr) {
        return QString();
    }

    return session
        ->sourceMetadata()
        .sourceName;
}
}

InvestigationSessionView::InvestigationSessionView(
    InvestigationSession *session,
    QWidget *parent
    )
    : WorkspaceDocument(
          documentIdFor(session),
          documentTitleFor(session),
          parent
          ),
    m_session(session),
    m_summaryPanel(
        new InvestigationSessionSummaryPanel(
            session,
            this
            )
        ),
    m_layout(
        new QVBoxLayout(this)
        )
{
    m_layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    m_layout->addWidget(
        m_summaryPanel
        );

    if (m_session != nullptr) {
        setToolTip(
            m_session
                ->sourceMetadata()
                .sourcePath
            );
    }
}

InvestigationSession *
InvestigationSessionView::session() const
{
    return m_session;
}

InvestigationSessionSummaryPanel *
InvestigationSessionView::summaryPanel() const
{
    return m_summaryPanel;
}

bool InvestigationSessionView::attachContent(
    QWidget *content
    )
{
    if (content == nullptr) {
        return false;
    }

    if (m_content == content) {
        return true;
    }

    /*
     * Do not silently orphan an existing content
     * widget. The caller must explicitly take it
     * before attaching something else.
     */
    if (m_content != nullptr) {
        return false;
    }

    m_content =
        content;

    m_content->setParent(
        this
        );

    m_layout->addWidget(
        m_content
        );

    m_content->show();

    return true;
}

QWidget *
InvestigationSessionView::takeContent()
{
    if (m_content == nullptr) {
        return nullptr;
    }

    QWidget *content =
        m_content;

    m_layout->removeWidget(
        content
        );

    m_content =
        nullptr;

    /*
     * Taking the content transfers presentation
     * ownership without destroying it. This is the
     * same principle used by WorkspaceDocumentHost
     * when documents are later detached.
     */
    content->hide();
    content->setParent(
        nullptr
        );

    return content;
}

QWidget *
InvestigationSessionView::content() const
{
    return m_content;
}