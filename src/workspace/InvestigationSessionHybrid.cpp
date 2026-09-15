#include "InvestigationSession.h"

#include <QFileInfo>

#include <utility>

#include "../importing/ILogImporter.h"
#include "../live/LiveSessionFollowCoordinator.h"
#include "../persistence/InvestigationSessionSnapshotFile.h"
#include "HybridInvestigationReconstructionService.h"

std::unique_ptr<InvestigationSession>
InvestigationSession::createHybrid(
    QString sessionId,
    const QString &snapshotPath,
    InvestigationSessionSnapshot snapshot,
    InvestigationExternalSourceBinding
        externalSourceBinding
    )
{
    if (!externalSourceBinding
             .sourcePath
             .trimmed()
             .isEmpty()) {
        externalSourceBinding.sourcePath =
            QFileInfo(
                externalSourceBinding.sourcePath
                ).absoluteFilePath();
    }

    InvestigationSnapshotBinding
        snapshotBinding;

    snapshotBinding.snapshotPath =
        QFileInfo(
            snapshotPath
            ).absoluteFilePath();

    snapshotBinding.sourceFidelity =
        snapshot.sourceFidelity;

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::hybrid(
            std::move(
                externalSourceBinding
                ),
            std::move(
                snapshotBinding
                )
            );

    InvestigationSessionSourceMetadata
        sourceMetadata;

    ImportProfile profile =
        snapshot.importProfile;

    ImportResult result;

    result.records =
        std::move(
            snapshot.records
            );

    result.diagnostics =
        std::move(
            snapshot.diagnostics
            );

    result.processedRecordCount =
        snapshot.processedRecordCount;

    result.sourceTruncated =
        snapshot.sourceTruncated;

    auto session =
        std::unique_ptr<
            InvestigationSession>(
            new InvestigationSession(
                std::move(sessionId),
                std::move(backing),
                std::move(sourceMetadata),
                std::move(profile),
                std::move(result)
                )
            );

    /*
     * Hybrid sessions retain original record
     * provenance from the snapshot while also having
     * an active external dependency. Current
     * filesystem metadata therefore comes from that
     * external binding.
     */
    session->refreshSourceMetadata();

    /*
     * Hybrid continuation deliberately begins from a
     * conservative replay boundary.
     *
     * We do not persist parser/framer state in Phase
     * 15. Starting future live following at byte zero
     * allows the existing incremental path to rebuild
     * that state and rely on stable record-ID
     * deduplication for already captured evidence.
     */
    session->m_initialLiveFollowByteOffset =
        0;

    return session;
}

InvestigationSessionHybridReloadResult
InvestigationSession::reloadHybrid(
    const ILogImporter &importer
    )
{
    InvestigationSessionHybridReloadResult
        result;

    if (m_backing.mode()
        != InvestigationSessionBackingMode::
        Hybrid) {
        result.errorMessage =
            QStringLiteral(
                "Hybrid reload requires a Hybrid "
                "investigation session."
                );

        return result;
    }

    const InvestigationSnapshotBinding
        *snapshotBinding =
        m_backing.snapshot();

    const InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (!snapshotBinding
        || !externalSource) {
        result.errorMessage =
            QStringLiteral(
                "The Hybrid investigation backing is "
                "incomplete."
                );

        return result;
    }

    if (snapshotBinding
            ->snapshotPath
            .trimmed()
            .isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The Hybrid investigation has no "
                "snapshot path."
                );

        return result;
    }

    /*
     * Everything above this mutation boundary is
     * preparation only.
     *
     * A load or reconstruction failure must leave the
     * currently open investigation, annotations,
     * selection, live coordinator, and backing state
     * untouched.
     */
    InvestigationSessionSnapshotFile
        snapshotFile;

    InvestigationSessionSnapshotLoadResult
        loadResult =
        snapshotFile.load(
            snapshotBinding->snapshotPath
            );

    if (!loadResult.isSuccess()) {
        result.errorMessage =
            loadResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The investigation snapshot "
                      "could not be loaded."
                      )
                : loadResult.errorMessage;

        return result;
    }

    const InvestigationSessionSnapshot
        &snapshot =
        *loadResult.snapshot;

    HybridInvestigationReconstructionService
        reconstructionService;

    HybridInvestigationReconstructionResult
        reconstruction =
        reconstructionService.reconstruct(
            snapshot,
            *externalSource,
            importer
            );

    if (!reconstruction.succeeded) {
        result.errorMessage =
            reconstruction.errorMessage;

        return result;
    }

    /*
     * Reconstruction is now complete.
     *
     * From this point forward we commit the candidate
     * to the live session as one replacement.
     */
    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    result.externalSourceAvailable =
        reconstruction.externalSourceAvailable;

    result.sourceGenerationChanged =
        reconstruction.sourceGenerationChanged;

    result.duplicateReplayRecordCount =
        reconstruction
            .duplicateReplayRecordCount;

    result.appendedReplayRecordCount =
        reconstruction
            .appendedReplayRecordCount;

    result.replaySkippedRecordCount =
        reconstruction
            .replaySkippedRecordCount;

    m_backing.connectExternalSource(
        std::move(
            reconstruction
                .externalSourceBinding
            )
        );

    InvestigationSnapshotBinding
        *mutableSnapshotBinding =
        m_backing.snapshot();

    if (mutableSnapshotBinding) {
        mutableSnapshotBinding
            ->sourceFidelity =
            snapshot.sourceFidelity;
    }

    /*
     * The saved snapshot owns the interpretation used
     * to produce its normalized evidence.
     */
    m_importProfile =
        snapshot.importProfile;

    /*
     * Phase 15 does not persist incremental
     * parser/framer state. Any later live-follow start
     * therefore conservatively replays the active file
     * from zero and lets stable record identity remove
     * overlap.
     */
    m_initialLiveFollowByteOffset =
        0;

    refreshSourceMetadata();

    /*
     * The reconstruction result already contains the
     * full snapshot baseline plus any newly admitted
     * replay records. installImportResult() therefore
     * sees the complete candidate exactly once.
     */
    installImportResult(
        std::move(
            reconstruction.importResult
            )
        );

    result.succeeded = true;

    return result;
}