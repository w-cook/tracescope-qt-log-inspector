#include "InvestigationReviewPanel.h"

#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>

#include "InvestigationAnalyticsPanel.h"
#include "InvestigationFindingsPanel.h"
#include "InvestigationIssueSummaryPanel.h"

InvestigationReviewPanel::
    InvestigationReviewPanel(
        QWidget *parent
        )
    : QWidget(parent),
    m_tabs(
        new QTabWidget(this)
        ),
    m_issueSummaryPanel(
        new InvestigationIssueSummaryPanel(
            m_tabs
            )
        ),
    m_findingsPanel(
        new InvestigationFindingsPanel(
            m_tabs
            )
        ),
    m_analyticsPanel(
        new InvestigationAnalyticsPanel(
            m_tabs
            )
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

    m_tabs->setDocumentMode(
        true
        );

    m_tabs->addTab(
        m_issueSummaryPanel,
        tr("Issue Summary")
        );

    m_tabs->addTab(
        m_findingsPanel,
        tr("Findings")
        );

    m_tabs->addTab(
        m_analyticsPanel,
        tr("Analytics")
        );

    connect(
        m_tabs,
        &QTabWidget::currentChanged,
        this,
        &InvestigationReviewPanel::
        handleCurrentChanged
        );
}

void InvestigationReviewPanel::setSession(
    InvestigationSession *session
    )
{
    m_session =
        session;

    /*
     * Issue Summary operates directly on the
     * record collection supplied by MainWindow and
     * therefore does not need a session binding.
     *
     * Findings and Analytics both contain
     * session-specific state, so keep those
     * bindings together at this composition
     * boundary.
     */
    m_findingsPanel->setSession(
        session
        );

    m_analyticsPanel->setSession(
        session
        );
}

InvestigationSession *
InvestigationReviewPanel::session() const
{
    return m_session;
}

InvestigationIssueSummaryPanel *
    InvestigationReviewPanel::
    issueSummaryPanel() const
{
    return m_issueSummaryPanel;
}

InvestigationFindingsPanel *
    InvestigationReviewPanel::
    findingsPanel() const
{
    return m_findingsPanel;
}

InvestigationAnalyticsPanel *
    InvestigationReviewPanel::
    analyticsPanel() const
{
    return m_analyticsPanel;
}

void InvestigationReviewPanel::
    setIssueSummaryAvailable(
        bool available
        )
{
    const int index =
        m_tabs->indexOf(
            m_issueSummaryPanel
            );

    if (index < 0
        || m_tabs->isTabVisible(index)
               == available) {
        return;
    }

    /*
     * Capability synchronization must not be
     * interpreted as a user tab selection.
     *
     * This matters while switching between
     * sessions. If the previous session did not
     * expose Issue Summary, for example, changing
     * tab visibility must not overwrite the new
     * session's saved preferred tab before
     * restoreSelectedTab() gets a chance to restore
     * it.
     */
    const QSignalBlocker blocker(
        m_tabs
        );

    m_tabs->setTabVisible(
        index,
        available
        );
}

void InvestigationReviewPanel::
    restoreSelectedTab()
{
    if (m_tabs->count() <= 0) {
        return;
    }

    int targetIndex =
        -1;

    /*
     * Prefer the session's persisted review
     * surface whenever that surface is available
     * for this source.
     */
    if (m_session != nullptr) {
        QWidget *preferredPage =
            pageForTab(
                m_session->reviewTab()
                );

        if (preferredPage != nullptr) {
            const int preferredIndex =
                m_tabs->indexOf(
                    preferredPage
                    );

            if (preferredIndex >= 0
                && m_tabs->isTabVisible(
                    preferredIndex
                    )) {
                targetIndex =
                    preferredIndex;
            }
        }
    }

    /*
     * Some sources do not expose all canonical
     * dimensions. Issue Summary, for example,
     * requires both severity and subsystem data.
     *
     * Fall back deterministically instead of
     * leaving a hidden page selected.
     */
    if (targetIndex < 0) {
        targetIndex =
            firstVisibleTabIndex();
    }

    if (targetIndex < 0) {
        return;
    }

    /*
     * Perform restoration as one state
     * synchronization operation rather than
     * allowing QTabWidget::currentChanged to fire
     * midway through it.
     */
    {
        const QSignalBlocker blocker(
            m_tabs
            );

        m_tabs->setCurrentIndex(
            targetIndex
            );
    }

    const InvestigationReviewTab
        restoredTab =
        currentTab();

    /*
     * If the saved tab was unavailable, persist the
     * deterministic fallback as the session's new
     * current review surface. This matches the
     * behavior of the pre-extraction MainWindow UI.
     */
    if (m_session != nullptr) {
        m_session->setReviewTab(
            restoredTab
            );
    }

    /*
     * Programmatic QTabWidget signals were blocked,
     * but the parent layout still needs to react to
     * the final selected review surface.
     */
    emit currentTabChanged(
        restoredTab
        );
}

InvestigationReviewTab
InvestigationReviewPanel::currentTab() const
{
    QWidget *currentPage =
        m_tabs->currentWidget();

    if (currentPage
        == m_findingsPanel) {
        return InvestigationReviewTab::
            Findings;
    }

    if (currentPage
        == m_analyticsPanel) {
        return InvestigationReviewTab::
            Analytics;
    }

    /*
     * Issue Summary is both a valid page and the
     * deterministic default if QTabWidget does not
     * currently expose another known page.
     */
    return InvestigationReviewTab::
        IssueSummary;
}

QWidget *
InvestigationReviewPanel::pageForTab(
    InvestigationReviewTab tab
    ) const
{
    switch (tab) {
    case InvestigationReviewTab::
        IssueSummary:
        return m_issueSummaryPanel;

    case InvestigationReviewTab::
        Findings:
        return m_findingsPanel;

    case InvestigationReviewTab::
        Analytics:
        return m_analyticsPanel;
    }

    return nullptr;
}

int InvestigationReviewPanel::
    firstVisibleTabIndex() const
{
    for (int index = 0;
         index < m_tabs->count();
         ++index) {
        if (m_tabs->isTabVisible(
                index
                )) {
            return index;
        }
    }

    return -1;
}

void InvestigationReviewPanel::
    handleCurrentChanged(
        int index
        )
{
    if (index < 0
        || !m_tabs->isTabVisible(
            index
            )) {
        return;
    }

    const InvestigationReviewTab tab =
        currentTab();

    /*
     * A normal QTabWidget currentChanged signal
     * represents a genuine visible selection, so
     * persist it to the active investigation.
     */
    if (m_session != nullptr) {
        m_session->setReviewTab(
            tab
            );
    }

    emit currentTabChanged(
        tab
        );
}