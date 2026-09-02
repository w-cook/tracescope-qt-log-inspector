#include "InvestigationFindingExportSnapshotBuilder.h"

#include "../workspace/InvestigationSession.h"

QVector<InvestigationFindingExport>
InvestigationFindingExportSnapshotBuilder::build(
    const InvestigationSession &session,
    InvestigationFindingExportScope scope
    ) const
{
    QVector<InvestigationFindingExport>
        findings;

    const InvestigationController *controller =
        session.investigationController();

    const InvestigationStateStore *stateStore =
        session.investigationStateStore();

    const QVector<InvestigationRecord> &records =
        controller->allRecords();

    for (const InvestigationRecord &record : records) {
        const InvestigationRecordState state =
            stateStore->stateForRecord(
                record.recordId
                );

        if (state.findingStatus
            == FindingStatus::None) {
            continue;
        }

        bool include = false;

        switch (scope) {
        case InvestigationFindingExportScope::All:
            include = true;
            break;

        case InvestigationFindingExportScope::Filtered:
            /*
             * Membership comes from the active proxy
             * filter state, but iteration remains over
             * source records so export order remains
             * deterministic and independent of the
             * event table's current sort order.
             */
            include =
                controller->proxyRowForRecordId(
                    record.recordId
                    )
                >= 0;
            break;

        case InvestigationFindingExportScope::Bookmarked:
            /*
             * Bookmark scope is deliberately independent
             * of the current investigation filters.
             */
            include =
                state.bookmarked;
            break;
        }

        if (!include) {
            continue;
        }

        InvestigationFindingExport finding;

        finding.record = record;
        finding.status =
            state.findingStatus;
        finding.note =
            state.note;
        finding.bookmarked =
            state.bookmarked;

        findings.append(finding);
    }

    return findings;
}