#include "InvestigationSessionPersistence.h"

#include <utility>

namespace
{
PersistedInvestigationExternalSourceBinding
captureExternalSourceBinding(
    const InvestigationExternalSourceBinding &source
    )
{
    PersistedInvestigationExternalSourceBinding
        persisted;

    persisted.sourcePath =
        source.sourcePath;

    persisted.sourceFamilyConfiguration =
        source.sourceFamilyConfiguration;

    /*
     * Physical rotated members are discovery results,
     * not durable configuration.
     */
    persisted
        .sourceFamilyConfiguration
        .rotatedSourcePaths
        .clear();

    persisted.sourceGeneration =
        source.sourceGeneration;

    persisted.sourceIdentity =
        source.sourceIdentity;

    return persisted;
}

PersistedSourceFamilyConfiguration
captureLegacySourceFamilyConfiguration(
    const SourceFamilyConfiguration &configuration
    )
{
    PersistedSourceFamilyConfiguration
        persisted;

    persisted.includeRotatedSources =
        configuration.includeRotatedSources;

    persisted.rotationRule =
        configuration.rotationRule;

    return persisted;
}
}

PersistedInvestigationSession
InvestigationSessionPersistence::capture(
    const InvestigationSession &session,
    const InvestigationSessionPresentationState
        &presentationState
    )
{
    PersistedInvestigationSession persisted;

    persisted.sessionId =
        session.id();

    const InvestigationSessionBacking
        &sessionBacking =
        session.backing();

    const InvestigationExternalSourceBinding
        *externalSource =
        sessionBacking.externalSource();

    const InvestigationSnapshotBinding
        *snapshot =
        sessionBacking.snapshot();

    /*
     * Populate the normalized schema-v2 backing model.
     */
    switch (sessionBacking.mode()) {
    case InvestigationSessionBackingMode::
        SourceBacked:
        persisted.backing.mode =
            PersistedInvestigationSessionBackingMode::
            SourceBacked;

        if (externalSource) {
            persisted
                .backing
                .externalSourceBinding =
                captureExternalSourceBinding(
                    *externalSource
                    );
        }

        persisted.backing.sourceImportProfile =
            session.importProfile();
        break;

    case InvestigationSessionBackingMode::
        SnapshotBacked:
        persisted.backing.mode =
            PersistedInvestigationSessionBackingMode::
            SnapshotBacked;

        if (snapshot) {
            persisted.backing.snapshotReference =
                snapshot->snapshotPath;
        }
        break;

    case InvestigationSessionBackingMode::
        Hybrid:
        persisted.backing.mode =
            PersistedInvestigationSessionBackingMode::
            Hybrid;

        if (externalSource) {
            persisted
                .backing
                .externalSourceBinding =
                captureExternalSourceBinding(
                    *externalSource
                    );
        }

        if (snapshot) {
            persisted.backing.snapshotReference =
                snapshot->snapshotPath;
        }
        break;
    }

    /*
     * Temporary compatibility mirrors for the current
     * workspace-open implementation.
     *
     * These disappear once restoration reads backing
     * directly.
     */
    persisted.importProfile =
        session.importProfile();

    if (externalSource) {
        persisted.sourcePath =
            externalSource->sourcePath;

        persisted.sourceFamilyConfiguration =
            captureLegacySourceFamilyConfiguration(
                externalSource
                    ->sourceFamilyConfiguration
                );
    } else {
        persisted.sourcePath =
            session.sourceMetadata().sourcePath;
    }

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

    persisted.presentationState =
        presentationState;

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