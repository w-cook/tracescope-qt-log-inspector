#include "InvestigationComparisonPersistence.h"

#include <utility>

namespace
{
PersistedInvestigationComparisonSource
captureSource(
    const InvestigationComparisonSourceSnapshot
        &source
    )
{
    PersistedInvestigationComparisonSource
        persisted;

    persisted.sessionId =
        source.sessionId;

    persisted.sourcePath =
        source.sourceMetadata.sourcePath;

    persisted.sourceName =
        source.sourceMetadata.sourceName;

    persisted.sourceSizeBytes =
        source.sourceMetadata.sourceSizeBytes;

    persisted.sourceLastModified =
        source.sourceMetadata.sourceLastModified;

    persisted.importedAtUtc =
        source.sourceMetadata.importedAtUtc;

    return persisted;
}

InvestigationComparisonSourceSnapshot
restoreSource(
    const PersistedInvestigationComparisonSource
        &persisted
    )
{
    InvestigationComparisonSourceSnapshot source;

    source.sessionId =
        persisted.sessionId;

    source.sourceMetadata.sourcePath =
        persisted.sourcePath;

    source.sourceMetadata.sourceName =
        persisted.sourceName;

    source.sourceMetadata.sourceSizeBytes =
        persisted.sourceSizeBytes;

    source.sourceMetadata.sourceLastModified =
        persisted.sourceLastModified;

    source.sourceMetadata.importedAtUtc =
        persisted.importedAtUtc;

    return source;
}
}

PersistedInvestigationComparison
InvestigationComparisonPersistence::capture(
    const InvestigationComparisonSnapshot
        &snapshot
    )
{
    PersistedInvestigationComparison persisted;

    persisted.comparisonId =
        snapshot.id();

    persisted.baselineSource =
        captureSource(
            snapshot.baselineSource()
            );

    persisted.comparisonSource =
        captureSource(
            snapshot.comparisonSource()
            );

    persisted.requestedBurstSettings =
        snapshot.requestedBurstSettings();

    persisted.analysis =
        snapshot.analysis();

    return persisted;
}

InvestigationComparisonSnapshot
InvestigationComparisonPersistence::restore(
    const PersistedInvestigationComparison
        &persistedComparison
    )
{
    InvestigationComparisonSourceSnapshot
        baselineSource =
        restoreSource(
            persistedComparison.baselineSource
            );

    InvestigationComparisonSourceSnapshot
        comparisonSource =
        restoreSource(
            persistedComparison.comparisonSource
            );

    return InvestigationComparisonSnapshot(
        persistedComparison.comparisonId,
        std::move(baselineSource),
        std::move(comparisonSource),
        persistedComparison.requestedBurstSettings,
        persistedComparison.analysis
        );
}