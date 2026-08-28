#include "InvestigationSessionPersistence.h"

#include <utility>

PersistedInvestigationSession
InvestigationSessionPersistence::capture(
    const InvestigationSession &session
    )
{
    PersistedInvestigationSession persisted;

    persisted.sessionId =
        session.id();

    persisted.sourcePath =
        session.sourceMetadata().sourcePath;

    persisted.importProfile =
        session.importProfile();

    const InvestigationStateStore *stateStore =
        session.investigationStateStore();

    QStringList recordIds =
        stateStore->statefulRecordIds();

    recordIds.sort(
        Qt::CaseSensitive
        );

    for (const QString &recordId : std::as_const(recordIds)) {
        const InvestigationRecordState state =
            stateStore->stateForRecord(
                recordId
                );

        PersistedInvestigationRecordState
            persistedState;

        persistedState.recordId =
            recordId;

        persistedState.bookmarked =
            state.bookmarked;

        persistedState.note =
            state.note;

        persistedState.findingStatus =
            state.findingStatus;

        persisted.recordStates.append(
            std::move(persistedState)
            );
    }

    const InvestigationController *controller =
        session.investigationController();

    const InvestigationFilterProxyModel *proxy =
        controller->proxyModel();

    persisted.filterState.severities =
        proxy->severityFilters();

    persisted.filterState.subsystems =
        proxy->subsystemFilters();

    persisted.filterState.searchText =
        proxy->searchText();

    persisted.filterState.eventCodes =
        proxy->eventCodeFilters();

    persisted.filterState.entityIds =
        proxy->entityFilters();

    persisted.filterState.startTime =
        proxy->timeRangeStart();

    persisted.filterState.endTime =
        proxy->timeRangeEnd();

    persisted.filterState.customFieldFilters =
        proxy->customFieldFilters();

    persisted.filterState.findingStatuses =
        proxy->findingStatusFilters();

    persisted.filterState.bookmarkedOnly =
        proxy->bookmarkedOnly();

    return persisted;
}

void InvestigationSessionPersistence::restoreState(
    const PersistedInvestigationSession
        &persistedSession,
    InvestigationSession &session
    )
{
    InvestigationStateStore *stateStore =
        session.investigationStateStore();

    for (
        const PersistedInvestigationRecordState &state
        : persistedSession.recordStates
        ) {
        if (state.recordId.isEmpty()) {
            continue;
        }

        stateStore->setBookmarked(
            state.recordId,
            state.bookmarked
            );

        stateStore->setNote(
            state.recordId,
            state.note
            );

        stateStore->setFindingStatus(
            state.recordId,
            state.findingStatus
            );
    }

    const PersistedInvestigationFilterState
        &filters =
        persistedSession.filterState;

    session.investigationController()
        ->setFilterState(
            filters.severities,
            filters.subsystems,
            filters.searchText,
            filters.eventCodes,
            filters.entityIds,
            filters.startTime,
            filters.endTime,
            filters.customFieldFilters,
            filters.findingStatuses,
            filters.bookmarkedOnly
            );
}