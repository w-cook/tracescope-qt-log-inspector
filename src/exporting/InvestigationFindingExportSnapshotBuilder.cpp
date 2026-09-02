#include "InvestigationFindingExportSnapshotBuilder.h"

#include "../workspace/InvestigationSession.h"

QVector<InvestigationFindingExport>
InvestigationFindingExportSnapshotBuilder::build(
    const InvestigationSession &session
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