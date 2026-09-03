#include "InvestigationReportSelectionModel.h"

#include <QSet>

namespace
{

QVector<InvestigationReportSessionSelection>
normalizedSessions(
    QVector<InvestigationReportSessionSelection> sessions
    )
{
    QVector<InvestigationReportSessionSelection> normalized;

    normalized.reserve(
        sessions.size()
        );

    QSet<QString> seenIds;

    for (InvestigationReportSessionSelection &session
         : sessions) {
        const QString sessionId =
            session.sessionId.trimmed();

        if (sessionId.isEmpty()
            || seenIds.contains(sessionId)) {
            continue;
        }

        session.sessionId =
            sessionId;

        /*
         * Selection is established by the origin rules
         * below rather than trusted from caller state.
         */
        session.selected =
            false;

        normalized.append(
            std::move(session)
            );

        seenIds.insert(
            sessionId
            );
    }

    return normalized;
}

QVector<InvestigationReportComparisonSelection>
normalizedComparisons(
    QVector<InvestigationReportComparisonSelection>
        comparisons
    )
{
    QVector<InvestigationReportComparisonSelection>
        normalized;

    normalized.reserve(
        comparisons.size()
        );

    QSet<QString> seenIds;

    for (InvestigationReportComparisonSelection
             &comparison
         : comparisons) {
        const QString comparisonId =
            comparison.comparisonId.trimmed();

        if (comparisonId.isEmpty()
            || seenIds.contains(comparisonId)) {
            continue;
        }

        comparison.comparisonId =
            comparisonId;

        comparison.baselineSessionId =
            comparison
                .baselineSessionId
                .trimmed();

        comparison.comparisonSessionId =
            comparison
                .comparisonSessionId
                .trimmed();

        comparison.selected =
            false;

        normalized.append(
            std::move(comparison)
            );

        seenIds.insert(
            comparisonId
            );
    }

    return normalized;
}

}

InvestigationReportSelectionModel::
    InvestigationReportSelectionModel(
        QVector<InvestigationReportSessionSelection>
            sessions,
        QVector<InvestigationReportComparisonSelection>
            comparisons,
        InvestigationReportSelectionOrigin origin
        )
    : m_sessions(
          normalizedSessions(
              std::move(sessions)
              )
          ),
    m_comparisons(
        normalizedComparisons(
            std::move(comparisons)
            )
        )
{
    applyInitialSelection(
        origin
        );
}

const QVector<InvestigationReportSessionSelection> &
InvestigationReportSelectionModel::sessions() const
{
    return m_sessions;
}

const QVector<InvestigationReportComparisonSelection> &
InvestigationReportSelectionModel::comparisons() const
{
    return m_comparisons;
}

bool InvestigationReportSelectionModel::
    setSessionSelected(
        const QString &sessionId,
        bool selected
        )
{
    for (InvestigationReportSessionSelection &session
         : m_sessions) {
        if (session.sessionId != sessionId) {
            continue;
        }

        session.selected =
            selected;

        return true;
    }

    return false;
}

bool InvestigationReportSelectionModel::
    setComparisonSelected(
        const QString &comparisonId,
        bool selected
        )
{
    for (InvestigationReportComparisonSelection
             &comparison
         : m_comparisons) {
        if (comparison.comparisonId
            != comparisonId) {
            continue;
        }

        comparison.selected =
            selected;

        /*
         * Selecting a comparison pulls in whichever
         * source sessions are currently open and
         * available to the report dialog.
         *
         * Deselecting it deliberately does not remove
         * those sessions: they may have been selected
         * independently or may still be useful report
         * sources.
         */
        if (selected) {
            selectOpenComparisonDependencies(
                comparison
                );
        }

        return true;
    }

    return false;
}

QStringList
    InvestigationReportSelectionModel::
    selectedSessionIds() const
{
    QStringList ids;

    for (const InvestigationReportSessionSelection
             &session
         : m_sessions) {
        if (session.selected) {
            ids.append(
                session.sessionId
                );
        }
    }

    return ids;
}

QStringList
    InvestigationReportSelectionModel::
    selectedComparisonIds() const
{
    QStringList ids;

    for (const InvestigationReportComparisonSelection
             &comparison
         : m_comparisons) {
        if (comparison.selected) {
            ids.append(
                comparison.comparisonId
                );
        }
    }

    return ids;
}

bool InvestigationReportSelectionModel::
    hasSelection() const
{
    for (const InvestigationReportSessionSelection
             &session
         : m_sessions) {
        if (session.selected) {
            return true;
        }
    }

    for (const InvestigationReportComparisonSelection
             &comparison
         : m_comparisons) {
        if (comparison.selected) {
            return true;
        }
    }

    return false;
}

void InvestigationReportSelectionModel::
    applyInitialSelection(
        const InvestigationReportSelectionOrigin &origin
        )
{
    switch (origin.type) {
    case InvestigationReportSelectionOriginType::None:
        return;

    case InvestigationReportSelectionOriginType::Session:
        setSessionSelected(
            origin.documentId,
            true
            );
        return;

    case InvestigationReportSelectionOriginType::Comparison:
        setComparisonSelected(
            origin.documentId,
            true
            );
        return;
    }
}

void InvestigationReportSelectionModel::
    selectOpenComparisonDependencies(
        const InvestigationReportComparisonSelection
            &comparison
        )
{
    if (!comparison
             .baselineSessionId
             .isEmpty()) {
        setSessionSelected(
            comparison.baselineSessionId,
            true
            );
    }

    if (!comparison
             .comparisonSessionId
             .isEmpty()) {
        setSessionSelected(
            comparison.comparisonSessionId,
            true
            );
    }
}