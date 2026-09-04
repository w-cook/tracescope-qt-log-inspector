#include "InvestigationFindingExportSnapshotBuilder.h"

#include <QSet>

#include "../workspace/InvestigationSession.h"

namespace
{
QSet<QString> visibleRecordIds(
    const InvestigationController *controller
    )
{
    QSet<QString> recordIds;

    if (controller == nullptr) {
        return recordIds;
    }

    const InvestigationFilterProxyModel *proxyModel =
        controller->proxyModel();

    if (proxyModel == nullptr) {
        return recordIds;
    }

    recordIds.reserve(
        proxyModel->rowCount()
        );

    for (
        int proxyRow = 0;
        proxyRow < proxyModel->rowCount();
        ++proxyRow
        ) {
        const QModelIndex proxyIndex =
            proxyModel->index(
                proxyRow,
                0
                );

        const InvestigationRecord *record =
            controller->recordForProxyIndex(
                proxyIndex
                );

        if (record != nullptr
            && !record->recordId.isEmpty()) {
            recordIds.insert(
                record->recordId
                );
        }
    }

    return recordIds;
}
}

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

    const QSet<QString> filteredRecordIds =
        scope
                == InvestigationFindingExportScope::
                Filtered
            ? visibleRecordIds(controller)
            : QSet<QString>();

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
            include =
                filteredRecordIds.contains(
                    record.recordId
                    );
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

InvestigationFindingExportCounts
InvestigationFindingExportSnapshotBuilder::counts(
    const InvestigationSession &session
    ) const
{
    InvestigationFindingExportCounts counts;

    const InvestigationController *controller =
        session.investigationController();

    const InvestigationStateStore *stateStore =
        session.investigationStateStore();

    if (controller == nullptr
        || stateStore == nullptr) {
        return counts;
    }

    const QSet<QString> filteredRecordIds =
        visibleRecordIds(controller);

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

        ++counts.all;

        if (filteredRecordIds.contains(
                record.recordId
                )) {
            ++counts.filtered;
        }

        if (state.bookmarked) {
            ++counts.bookmarked;
        }
    }

    return counts;
}