#pragma once

#include "WorkspaceDocument.h"

class InvestigationSession;
class QVBoxLayout;

class InvestigationSessionView
    : public WorkspaceDocument
{
    Q_OBJECT

public:
    explicit InvestigationSessionView(
        InvestigationSession *session,
        QWidget *parent = nullptr
        );

    InvestigationSession *session() const;

    /*
     * During the incremental migration from the
     * legacy MainWindow-owned investigation UI,
     * the active session view hosts that shared
     * surface.
     *
     * Later this same content boundary can contain
     * a fully independent per-session investigation
     * surface without changing document hosting.
     */
    bool attachContent(
        QWidget *content
        );

    QWidget *takeContent();

    QWidget *content() const;

private:
    InvestigationSession *m_session = nullptr;

    QVBoxLayout *m_layout = nullptr;

    QWidget *m_content = nullptr;
};