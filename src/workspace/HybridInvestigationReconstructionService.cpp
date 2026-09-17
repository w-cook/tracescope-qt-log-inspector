#include "HybridInvestigationReconstructionService.h"

#include <limits>
#include <utility>

#include <QFileInfo>

#include "../importing/ImportResultSourceGeneration.h"
#include "../sources/SourcePhysicalIdentity.h"
#include "HybridInvestigationCandidateBuilder.h"

namespace
{
HybridInvestigationReconstructionResult
failureResult(
    const InvestigationExternalSourceBinding
        &externalSourceBinding,
    const QString &message
    )
{
    HybridInvestigationReconstructionResult result;

    result.succeeded = false;

    result.errorMessage =
        message;

    result.externalSourceBinding =
        externalSourceBinding;

    return result;
}

void copyCandidateResult(
    HybridInvestigationReconstructionResult &result,
    HybridInvestigationCandidateBuildResult candidate
    )
{
    result.importResult =
        std::move(
            candidate.importResult
            );

    result.duplicateReplayRecordCount =
        candidate.duplicateReplayRecordCount;

    result.appendedReplayRecordCount =
        candidate.appendedReplayRecordCount;

    result.replaySkippedRecordCount =
        candidate.replaySkippedRecordCount;
}
}

HybridInvestigationReconstructionResult
    HybridInvestigationReconstructionService::
    reconstruct(
        const InvestigationSessionSnapshot &snapshot,
        const InvestigationExternalSourceBinding
            &externalSourceBinding,
        const ILogImporter &importer,
        const ImportExecutionContext
            &executionContext
        ) const
{
    if (externalSourceBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        return failureResult(
            externalSourceBinding,
            QStringLiteral(
                "The Hybrid investigation has no "
                "external source path."
                )
            );
    }

    HybridInvestigationReconstructionResult
        result;

    result.externalSourceBinding =
        externalSourceBinding;

    const QFileInfo sourceInfo(
        externalSourceBinding.sourcePath
        );

    HybridInvestigationCandidateBuilder
        candidateBuilder;

    /*
     * Missing external source:
     *
     * Hybrid semantics treat the snapshot as the
     * durable evidence floor. Reconstruct it exactly
     * and preserve the external binding for a later
     * retry/reconnection.
     */
    if (!sourceInfo.exists()
        || !sourceInfo.isFile()) {
        ImportResult emptyReplay;

        HybridInvestigationCandidateBuildResult
            candidate =
            candidateBuilder.build(
                snapshot,
                std::move(emptyReplay),
                externalSourceBinding
                    .sourceGeneration
                );

        if (!candidate.succeeded) {
            return failureResult(
                externalSourceBinding,
                candidate.errorMessage
                );
        }

        copyCandidateResult(
            result,
            std::move(candidate)
            );

        result.externalSourceAvailable =
            false;

        return result;
    }

    /*
     * The replay importer must represent the same
     * interpretation that produced the snapshot.
     */
    if (importer.id()
        != snapshot.importProfile.importerId) {
        return failureResult(
            externalSourceBinding,
            QStringLiteral(
                "The replay importer does not match "
                "the snapshot import profile."
                )
            );
    }

    quint64 replaySourceGeneration =
        externalSourceBinding.sourceGeneration;

    /*
     * When we have a persisted physical identity,
     * compare it with the current active source before
     * replay begins.
     *
     * Append-only growth remains the same generation.
     * Replacement/truncation advances the logical
     * same-path generation.
     */
    if (externalSourceBinding
            .sourceIdentity
            .has_value()
        && sourcePhysicalIdentityChanged(
            externalSourceBinding.sourcePath,
            *externalSourceBinding.sourceIdentity
            )) {
        if (replaySourceGeneration
            == std::numeric_limits<
                quint64>::max()) {
            return failureResult(
                externalSourceBinding,
                QStringLiteral(
                    "The external source generation "
                    "cannot be advanced further."
                    )
                );
        }

        ++replaySourceGeneration;

        result.sourceGenerationChanged =
            true;
    }

    /*
     * Capture the physical file that we are about to
     * replay.
     *
     * A second identity capture after import lets us
     * reject a candidate if the producer replaced or
     * truncated the source during reconstruction.
     */
    const SourcePhysicalIdentityCaptureResult
        identityBefore =
        captureSourcePhysicalIdentity(
            externalSourceBinding.sourcePath
            );

    if (!identityBefore.succeeded) {
        return failureResult(
            externalSourceBinding,
            identityBefore.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The external source identity "
                      "could not be captured before "
                      "replay."
                      )
                : identityBefore.errorMessage
            );
    }

    ImportResult replayResult =
        importer.importFile(
            externalSourceBinding.sourcePath,
            ILogImporter::UnlimitedRecordLimit,
            executionContext
            );

    const QString logicalSourceKey =
        externalSourceBinding
                .logicalSourceKey
                .trimmed()
                .isEmpty()
            ? externalSourceBinding.sourcePath
            : externalSourceBinding.logicalSourceKey;

    rebaseImportResultLogicalSourceKeyForPath(
        replayResult,
        externalSourceBinding.sourcePath,
        logicalSourceKey
        );

    /*
     * The candidate builder rejects cancellation, but
     * do the physical-source validation before
     * returning any reconstruction result.
     */
    const SourcePhysicalIdentityCaptureResult
        identityAfter =
        captureSourcePhysicalIdentity(
            externalSourceBinding.sourcePath
            );

    if (!identityAfter.succeeded) {
        return failureResult(
            externalSourceBinding,
            QStringLiteral(
                "The external source became "
                "unavailable while it was being "
                "replayed."
                )
            );
    }

    if (sourcePhysicalIdentityChanged(
            externalSourceBinding.sourcePath,
            identityBefore.identity
            )) {
        return failureResult(
            externalSourceBinding,
            QStringLiteral(
                "The external source was replaced or "
                "truncated while reconstruction was "
                "in progress."
                )
            );
    }

    HybridInvestigationCandidateBuildResult
        candidate =
        candidateBuilder.build(
            snapshot,
            std::move(replayResult),
            replaySourceGeneration
            );

    if (!candidate.succeeded) {
        return failureResult(
            externalSourceBinding,
            candidate.errorMessage
            );
    }

    copyCandidateResult(
        result,
        std::move(candidate)
        );

    result.externalSourceAvailable =
        true;

    result.externalSourceBinding
        .sourceGeneration =
        replaySourceGeneration;

    result.externalSourceBinding
        .sourceIdentity =
        identityAfter.identity;

    return result;
}