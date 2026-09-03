#include "InvestigationReportWorkspaceContext.h"

#include <utility>

#include "InvestigationComparisonDocument.h"
#include "InvestigationSessionView.h"
#include "WorkspaceDocument.h"
#include "WorkspaceDocumentHost.h"

#include "../../workspace/InvestigationComparisonSnapshot.h"
#include "../../workspace/InvestigationSession.h"

namespace
{

QString usableTitle(
    const QString &documentTitle,
    const QString &fallback
    )
{
    const QString title =
        documentTitle.trimmed();

    return title.isEmpty()
               ? fallback
               : title;
}

QString suggestedReportTitle(
    const QString &documentTitle
    )
{
    const QString title =
        documentTitle.trimmed();

    if (title.isEmpty()) {
        return QStringLiteral(
            "Investigation Report"
            );
    }

    return QStringLiteral(
               "%1 — Investigation Report"
               )
        .arg(title);
}

}

InvestigationReportWorkspaceContext
InvestigationReportWorkspaceContextBuilder::build(
    const WorkspaceDocumentHost &host,
    const QString &originDocumentId
    ) const
{
    InvestigationReportWorkspaceContext context;

    const QVector<WorkspaceDocument *> documents =
        host.documents();

    for (WorkspaceDocument *document
         : documents) {
        if (document == nullptr) {
            continue;
        }

        /*
         * -------------------------------------------------
         * Investigation sessions
         * -------------------------------------------------
         */

        if (auto *sessionView =
            qobject_cast<
                InvestigationSessionView *>(
                document
                )) {
            InvestigationSession *session =
                sessionView->session();

            if (session == nullptr) {
                continue;
            }

            const QString sessionId =
                session->id();

            const QString title =
                usableTitle(
                    document->documentTitle(),
                    session->sourceMetadata()
                        .sourceName
                    );

            InvestigationReportSessionSelection
                selection;

            selection.sessionId =
                sessionId;

            selection.documentTitle =
                title;

            context.sessionSelections.append(
                std::move(selection)
                );

            InvestigationReportSessionInput input;

            input.session =
                session;

            input.documentTitle =
                title;

            context.sessionInputs.append(
                std::move(input)
                );

            if (document->documentId()
                == originDocumentId) {
                context.origin.type =
                    InvestigationReportSelectionOriginType::
                    Session;

                context.origin.documentId =
                    sessionId;

                context.suggestedTitle =
                    suggestedReportTitle(
                        title
                        );
            }

            continue;
        }

        /*
         * -------------------------------------------------
         * Immutable comparisons
         * -------------------------------------------------
         */

        if (auto *comparisonDocument =
            qobject_cast<
                InvestigationComparisonDocument *>(
                document
                )) {
            const InvestigationComparisonSnapshot
                &snapshot =
                comparisonDocument->snapshot();

            const QString title =
                usableTitle(
                    document->documentTitle(),
                    snapshot.id()
                    );

            InvestigationReportComparisonSelection
                selection;

            selection.comparisonId =
                snapshot.id();

            selection.documentTitle =
                title;

            selection.baselineSessionId =
                snapshot
                    .baselineSource()
                    .sessionId;

            selection.comparisonSessionId =
                snapshot
                    .comparisonSource()
                    .sessionId;

            context.comparisonSelections.append(
                std::move(selection)
                );

            InvestigationReportComparisonInput input;

            input.snapshot =
                &snapshot;

            input.documentTitle =
                title;

            context.comparisonInputs.append(
                std::move(input)
                );

            if (document->documentId()
                == originDocumentId) {
                context.origin.type =
                    InvestigationReportSelectionOriginType::
                    Comparison;

                context.origin.documentId =
                    snapshot.id();

                context.suggestedTitle =
                    suggestedReportTitle(
                        title
                        );
            }
        }
    }

    /*
     * A caller without a recognized origin can still
     * construct a report manually from the available
     * documents.
     */
    if (context.suggestedTitle.isEmpty()) {
        context.suggestedTitle =
            QStringLiteral(
                "Investigation Report"
                );
    }

    return context;
}