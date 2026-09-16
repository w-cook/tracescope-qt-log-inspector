#include "InvestigationSessionRestorationService.h"

#include <limits>
#include <memory>
#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "../importing/BuiltInImporterRegistry.h"
#include "../importing/ILogImporter.h"
#include "../importing/ImporterRegistry.h"
#include "../importing/ImportResultSourceGeneration.h"
#include "../importing/SourceFamilyImportService.h"
#include "../live/LiveSessionFollowCoordinator.h"
#include "../persistence/InvestigationSessionSnapshotFile.h"
#include "../sources/RotatedSourceDiscoveryService.h"
#include "../sources/SourcePhysicalIdentity.h"
#include "HybridInvestigationReconstructionService.h"
#include "InvestigationSessionPersistence.h"

namespace
{
InvestigationSessionRestorationPreparationResult
preparationFailure(
    const QString &errorMessage
    )
{
    InvestigationSessionRestorationPreparationResult
        result;

    result.succeeded = false;
    result.errorMessage = errorMessage;

    return result;
}

InvestigationSessionRestorationPreparationResult
preparationSuccess(
    InvestigationSessionRestorationPreparedData
        preparedData
    )
{
    InvestigationSessionRestorationPreparationResult
        result;

    result.succeeded = true;

    result.preparedData =
        std::move(preparedData);

    return result;
}

InvestigationSessionRestorationResult
restorationFailure(
    const QString &errorMessage
    )
{
    InvestigationSessionRestorationResult result;

    result.succeeded = false;
    result.errorMessage = errorMessage;

    return result;
}

InvestigationSessionRestorationResult
restorationSuccess(
    std::unique_ptr<InvestigationSession> session
    )
{
    InvestigationSessionRestorationResult result;

    result.succeeded = true;
    result.session = std::move(session);

    return result;
}

QString resolveWorkspaceReference(
    const QString &workspaceFilePath,
    const QString &reference
    )
{
    const QFileInfo referenceInfo(
        reference
        );

    if (referenceInfo.isAbsolute()) {
        return QDir::cleanPath(
            referenceInfo.absoluteFilePath()
            );
    }

    if (workspaceFilePath.trimmed().isEmpty()) {
        return {};
    }

    const QDir workspaceDirectory(
        QFileInfo(workspaceFilePath)
            .absolutePath()
        );

    return QDir::cleanPath(
        workspaceDirectory.absoluteFilePath(
            reference
            )
        );
}

std::shared_ptr<ILogImporter> createImporter(
    const ImportProfile &profile
    )
{
    ImporterRegistry registry =
        createBuiltInImporterRegistry(
            profile
            );

    return registry.importerById(
        profile.importerId
        );
}

bool prepareSourceFamilyConfiguration(
    const QString &activeSourcePath,
    const SourceFamilyConfiguration
        &persistedConfiguration,
    SourceFamilyConfiguration
        &runtimeConfiguration,
    QString &errorMessage
    )
{
    runtimeConfiguration =
        persistedConfiguration;

    /*
     * Physical rotated paths are runtime discovery
     * results, not durable source-family configuration.
     */
    runtimeConfiguration
        .rotatedSourcePaths
        .clear();

    if (!runtimeConfiguration
             .includeRotatedSources) {
        return true;
    }

    const RotatedSourceDiscoveryResult
        discoveryResult =
        RotatedSourceDiscoveryService::discover(
            activeSourcePath,
            runtimeConfiguration.rotationRule
            );

    if (!discoveryResult.succeeded) {
        errorMessage =
            discoveryResult.errorMessage;

        return false;
    }

    runtimeConfiguration
        .rotatedSourcePaths
        .reserve(
            discoveryResult
                .rotatedSources
                .size()
            );

    for (const RotatedSourceMatch &match
         : discoveryResult.rotatedSources) {
        runtimeConfiguration
            .rotatedSourcePaths
            .append(
                match.filePath
                );
    }

    return true;
}

InvestigationExternalSourceBinding
runtimeExternalSourceBinding(
    const PersistedInvestigationExternalSourceBinding
        &persistedBinding
    )
{
    InvestigationExternalSourceBinding binding;

    binding.sourcePath =
        QFileInfo(
            persistedBinding.sourcePath
            ).absoluteFilePath();

    binding.sourceFamilyConfiguration =
        persistedBinding.sourceFamilyConfiguration;

    /*
     * Physical rotated members must be rediscovered
     * rather than treated as persisted identity.
     */
    binding
        .sourceFamilyConfiguration
        .rotatedSourcePaths
        .clear();

    binding.sourceGeneration =
        persistedBinding.sourceGeneration;

    binding.sourceIdentity =
        persistedBinding.sourceIdentity;

    return binding;
}

InvestigationSessionRestorationPreparationResult
prepareSourceBackedSession(
    const PersistedInvestigationSession
        &persistedSession,
    const ImportExecutionContext
        &executionContext
    )
{
    const PersistedInvestigationSessionBacking
        &backing =
        persistedSession.backing;

    if (!backing
             .externalSourceBinding
             .has_value()) {
        return preparationFailure(
            QStringLiteral(
                "The Source-backed investigation "
                "does not contain an external source "
                "binding."
                )
            );
    }

    if (!backing
             .sourceImportProfile
             .has_value()) {
        return preparationFailure(
            QStringLiteral(
                "The Source-backed investigation "
                "does not contain an import profile."
                )
            );
    }

    const PersistedInvestigationExternalSourceBinding
        &persistedBinding =
        *backing.externalSourceBinding;

    const ImportProfile profile =
        *backing.sourceImportProfile;

    if (persistedBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The Source-backed investigation "
                "does not contain a source path."
                )
            );
    }

    const QString activeSourcePath =
        QFileInfo(
            persistedBinding.sourcePath
            ).absoluteFilePath();

    const QFileInfo sourceInfo(
        activeSourcePath
        );

    /*
     * SourceBacked means the external source remains
     * authoritative.
     *
     * A workspace-save fallback snapshot does not
     * silently change that runtime backing mode.
     */
    if (!sourceInfo.exists()
        || !sourceInfo.isFile()) {
        return preparationFailure(
            QStringLiteral(
                "The external source file for the "
                "Source-backed investigation is not "
                "available."
                )
            );
    }

    const std::shared_ptr<ILogImporter>
        importer =
        createImporter(
            profile
            );

    if (!importer) {
        return preparationFailure(
            QStringLiteral(
                "No importer is available for the "
                "persisted Source-backed import "
                "profile."
                )
            );
    }

    /*
     * Capture the live-follow handoff before import.
     *
     * Producer growth during preparation therefore
     * causes overlap rather than a gap.
     */
    const qint64 initialLiveFollowByteOffset =
        LiveSessionFollowCoordinator::
        captureInitialReadOffset(
            activeSourcePath,
            profile
            );

    quint64 sourceGeneration =
        persistedBinding.sourceGeneration;

    if (persistedBinding
            .sourceIdentity
            .has_value()
        && sourcePhysicalIdentityChanged(
            activeSourcePath,
            *persistedBinding.sourceIdentity
            )) {
        if (sourceGeneration
            == std::numeric_limits<
                quint64>::max()) {
            return preparationFailure(
                QStringLiteral(
                    "The external source appears to "
                    "have been replaced, but the "
                    "persisted source generation "
                    "cannot be advanced."
                    )
                );
        }

        ++sourceGeneration;
    }

    const SourcePhysicalIdentityCaptureResult
        identityBeforeImport =
        captureSourcePhysicalIdentity(
            activeSourcePath
            );

    if (!identityBeforeImport.succeeded) {
        return preparationFailure(
            identityBeforeImport.errorMessage
                    .isEmpty()
                ? QStringLiteral(
                      "Could not capture the external "
                      "source identity before "
                      "restoration."
                      )
                : identityBeforeImport.errorMessage
            );
    }

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    QString sourceFamilyError;

    if (!prepareSourceFamilyConfiguration(
            activeSourcePath,
            persistedBinding
                .sourceFamilyConfiguration,
            sourceFamilyConfiguration,
            sourceFamilyError
            )) {
        return preparationFailure(
            sourceFamilyError.isEmpty()
                ? QStringLiteral(
                      "Could not rediscover the "
                      "persisted source family."
                      )
                : sourceFamilyError
            );
    }

    SourceFamilyImportOptions
        importOptions;

    importOptions.maxProcessedRecords =
        ILogImporter::UnlimitedRecordLimit;

    importOptions.recordLimitMode =
        SourceFamilyRecordLimitMode::
        Sequential;

    ImportResult importResult =
        SourceFamilyImportService()
            .importFiles(
                sourceFamilyConfiguration
                    .orderedSourcePaths(
                        activeSourcePath
                        ),
                *importer,
                importOptions,
                executionContext
                );

    if (importResult.cancelled) {
        return preparationFailure(
            QStringLiteral(
                "Source-backed restoration was "
                "cancelled before import completed."
                )
            );
    }

    const SourcePhysicalIdentityCaptureResult
        identityAfterImport =
        captureSourcePhysicalIdentity(
            activeSourcePath
            );

    if (!identityAfterImport.succeeded) {
        return preparationFailure(
            identityAfterImport.errorMessage
                    .isEmpty()
                ? QStringLiteral(
                      "Could not capture the external "
                      "source identity after "
                      "restoration."
                      )
                : identityAfterImport.errorMessage
            );
    }

    /*
     * Append-only growth is allowed. Replacement or
     * truncation during preparation invalidates the
     * candidate.
     */
    if (sourcePhysicalIdentityChanged(
            activeSourcePath,
            identityBeforeImport.identity
            )) {
        return preparationFailure(
            QStringLiteral(
                "The external source changed while "
                "the investigation was being "
                "restored. The current investigation "
                "was not replaced."
                )
            );
    }

    /*
     * Static importers produce generation zero.
     *
     * Only the current active path participates in
     * same-path generation tracking. Rotated physical
     * files retain their own independent identity.
     */
    rebaseImportResultSourceGenerationForPath(
        importResult,
        activeSourcePath,
        sourceGeneration
        );

    PreparedSourceBackedInvestigationSession
        prepared;

    prepared.sourcePath =
        activeSourcePath;

    prepared.importProfile =
        profile;

    prepared.importResult =
        std::move(importResult);

    prepared.sourceFamilyConfiguration =
        std::move(
            sourceFamilyConfiguration
            );

    prepared.sourceGeneration =
        sourceGeneration;

    prepared.sourceIdentity =
        identityAfterImport.identity;

    prepared.initialLiveFollowByteOffset =
        initialLiveFollowByteOffset;

    return preparationSuccess(
        InvestigationSessionRestorationPreparedData(
            std::move(prepared)
            )
        );
}

InvestigationSessionRestorationPreparationResult
prepareSnapshotBackedSession(
    const PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath
    )
{
    const PersistedInvestigationSessionBacking
        &backing =
        persistedSession.backing;

    if (!backing.snapshotReference.has_value()
        || backing
               .snapshotReference
               ->trimmed()
               .isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The Snapshot-backed investigation "
                "does not contain a snapshot "
                "reference."
                )
            );
    }

    const QString snapshotPath =
        resolveWorkspaceReference(
            workspaceFilePath,
            *backing.snapshotReference
            );

    if (snapshotPath.isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The relative investigation snapshot "
                "could not be resolved because the "
                "workspace path is unavailable."
                )
            );
    }

    InvestigationSessionSnapshotLoadResult
        loadResult =
        InvestigationSessionSnapshotFile()
            .load(
                snapshotPath
                );

    if (!loadResult.isSuccess()) {
        return preparationFailure(
            loadResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The investigation snapshot "
                      "could not be loaded."
                      )
                : loadResult.errorMessage
            );
    }

    PreparedSnapshotBackedInvestigationSession
        prepared;

    prepared.snapshotPath =
        snapshotPath;

    prepared.snapshot =
        std::move(
            *loadResult.snapshot
            );

    if (backing
            .externalSourceBinding
            .has_value()) {
        prepared.reconnectSourceHint =
            runtimeExternalSourceBinding(
                *backing.externalSourceBinding
                );
    }

    return preparationSuccess(
        InvestigationSessionRestorationPreparedData(
            std::move(prepared)
            )
        );
}

InvestigationSessionRestorationPreparationResult
prepareHybridSession(
    const PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath,
    const ImportExecutionContext
        &executionContext
    )
{
    const PersistedInvestigationSessionBacking
        &backing =
        persistedSession.backing;

    if (!backing.snapshotReference.has_value()
        || backing
               .snapshotReference
               ->trimmed()
               .isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The Hybrid investigation does not "
                "contain a snapshot reference."
                )
            );
    }

    if (!backing
             .externalSourceBinding
             .has_value()) {
        return preparationFailure(
            QStringLiteral(
                "The Hybrid investigation does not "
                "contain an external source binding."
                )
            );
    }

    const PersistedInvestigationExternalSourceBinding
        &persistedBinding =
        *backing.externalSourceBinding;

    if (persistedBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The Hybrid investigation does not "
                "contain an external source path."
                )
            );
    }

    const QString snapshotPath =
        resolveWorkspaceReference(
            workspaceFilePath,
            *backing.snapshotReference
            );

    if (snapshotPath.isEmpty()) {
        return preparationFailure(
            QStringLiteral(
                "The relative Hybrid snapshot could "
                "not be resolved because the "
                "workspace path is unavailable."
                )
            );
    }

    InvestigationSessionSnapshotLoadResult
        loadResult =
        InvestigationSessionSnapshotFile()
            .load(
                snapshotPath
                );

    if (!loadResult.isSuccess()) {
        return preparationFailure(
            loadResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The Hybrid investigation "
                      "snapshot could not be loaded."
                      )
                : loadResult.errorMessage
            );
    }

    InvestigationSessionSnapshot snapshot =
        std::move(
            *loadResult.snapshot
            );

    InvestigationExternalSourceBinding
        externalSourceBinding =
        runtimeExternalSourceBinding(
            persistedBinding
            );

    std::optional<ImportResult>
        reconstructedImportResult;

    const QFileInfo externalSourceInfo(
        externalSourceBinding.sourcePath
        );

    /*
     * Missing external source is a valid Hybrid
     * restoration case. The snapshot remains the
     * durable evidence floor, and the binding is kept
     * for a later continuation attempt.
     */
    if (externalSourceInfo.exists()
        && externalSourceInfo.isFile()) {
        const std::shared_ptr<ILogImporter>
            importer =
            createImporter(
                snapshot.importProfile
                );

        if (!importer) {
            return preparationFailure(
                QStringLiteral(
                    "No importer is available for the "
                    "Hybrid investigation's persisted "
                    "snapshot profile."
                    )
                );
        }

        HybridInvestigationReconstructionResult
            reconstruction =
            HybridInvestigationReconstructionService()
                .reconstruct(
                    snapshot,
                    externalSourceBinding,
                    *importer,
                    executionContext
                    );

        if (!reconstruction.succeeded) {
            return preparationFailure(
                reconstruction.errorMessage.isEmpty()
                    ? QStringLiteral(
                          "The Hybrid investigation "
                          "could not be reconstructed."
                          )
                    : reconstruction.errorMessage
                );
        }

        externalSourceBinding =
            std::move(
                reconstruction
                    .externalSourceBinding
                );

        reconstructedImportResult =
            std::move(
                reconstruction.importResult
                );
    }

    PreparedHybridInvestigationSession
        prepared;

    prepared.snapshotPath =
        snapshotPath;

    prepared.snapshot =
        std::move(snapshot);

    prepared.externalSourceBinding =
        std::move(
            externalSourceBinding
            );

    prepared.reconstructedImportResult =
        std::move(
            reconstructedImportResult
            );

    return preparationSuccess(
        InvestigationSessionRestorationPreparedData(
            std::move(prepared)
            )
        );
}
}

InvestigationSessionRestorationPreparationResult
InvestigationSessionRestorationService::prepare(
    const PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath,
    const ImportExecutionContext
        &executionContext
    ) const
{
    switch (persistedSession.backing.mode) {
    case PersistedInvestigationSessionBackingMode::
        SourceBacked:
        return prepareSourceBackedSession(
            persistedSession,
            executionContext
            );

    case PersistedInvestigationSessionBackingMode::
        SnapshotBacked:
        return prepareSnapshotBackedSession(
            persistedSession,
            workspaceFilePath
            );

    case PersistedInvestigationSessionBackingMode::
        Hybrid:
        return prepareHybridSession(
            persistedSession,
            workspaceFilePath,
            executionContext
            );
    }

    return preparationFailure(
        QStringLiteral(
            "The persisted investigation uses an "
            "unsupported backing mode."
            )
        );
}

InvestigationSessionRestorationResult
InvestigationSessionRestorationService::materialize(
    const PersistedInvestigationSession
        &persistedSession,
    InvestigationSessionRestorationPreparationResult
        preparation
    ) const
{
    if (!preparation.succeeded) {
        return restorationFailure(
            preparation.errorMessage.isEmpty()
                ? QStringLiteral(
                      "Investigation restoration "
                      "preparation failed."
                      )
                : preparation.errorMessage
            );
    }

    if (!preparation.preparedData.has_value()) {
        return restorationFailure(
            QStringLiteral(
                "Investigation restoration preparation "
                "did not produce materialization data."
                )
            );
    }

    InvestigationSessionRestorationPreparedData
        preparedData =
        std::move(
            *preparation.preparedData
            );

    if (std::holds_alternative<
            PreparedSourceBackedInvestigationSession>(
            preparedData
            )) {
        PreparedSourceBackedInvestigationSession
            prepared =
            std::get<
                PreparedSourceBackedInvestigationSession>(
                std::move(preparedData)
                );

        auto session =
            std::make_unique<
                InvestigationSession>(
                persistedSession.sessionId,
                prepared.sourcePath,
                std::move(
                    prepared.importProfile
                    ),
                std::move(
                    prepared.importResult
                    ),
                std::move(
                    prepared
                        .sourceFamilyConfiguration
                    )
                );

        session->updateExternalSourceRuntimeState(
            prepared.sourceGeneration,
            &prepared.sourceIdentity
            );

        session->setInitialLiveFollowByteOffset(
            prepared
                .initialLiveFollowByteOffset
            );

        InvestigationSessionPersistence::
            restoreState(
                persistedSession,
                *session
                );

        return restorationSuccess(
            std::move(session)
            );
    }

    if (std::holds_alternative<
            PreparedSnapshotBackedInvestigationSession>(
            preparedData
            )) {
        PreparedSnapshotBackedInvestigationSession
            prepared =
            std::get<
                PreparedSnapshotBackedInvestigationSession>(
                std::move(preparedData)
                );

        auto session =
            InvestigationSession::
            createSnapshotBacked(
                persistedSession.sessionId,
                prepared.snapshotPath,
                std::move(
                    prepared.snapshot
                    ),
                std::move(
                    prepared.reconnectSourceHint
                    )
                );

        if (!session) {
            return restorationFailure(
                QStringLiteral(
                    "The Snapshot-backed investigation "
                    "could not be materialized."
                    )
                );
        }

        InvestigationSessionPersistence::
            restoreState(
                persistedSession,
                *session
                );

        return restorationSuccess(
            std::move(session)
            );
    }

    if (std::holds_alternative<
            PreparedHybridInvestigationSession>(
            preparedData
            )) {
        PreparedHybridInvestigationSession
            prepared =
            std::get<
                PreparedHybridInvestigationSession>(
                std::move(preparedData)
                );

        /*
         * When preparation replayed the current source,
         * place that complete candidate into the
         * in-memory snapshot used for construction.
         *
         * This does not modify the durable .tsinv file.
         * The backing still references the original
         * durable evidence floor, exactly as the old
         * createHybrid() + reloadHybrid() path did.
         */
        if (prepared
                .reconstructedImportResult
                .has_value()) {
            ImportResult candidate =
                std::move(
                    *prepared
                         .reconstructedImportResult
                    );

            prepared.snapshot.records =
                std::move(
                    candidate.records
                    );

            prepared.snapshot.diagnostics =
                std::move(
                    candidate.diagnostics
                    );

            prepared.snapshot.processedRecordCount =
                candidate.processedRecordCount;

            prepared.snapshot.sourceTruncated =
                candidate.sourceTruncated;
        }

        auto session =
            InvestigationSession::
            createHybrid(
                persistedSession.sessionId,
                prepared.snapshotPath,
                std::move(
                    prepared.snapshot
                    ),
                std::move(
                    prepared
                        .externalSourceBinding
                    )
                );

        if (!session) {
            return restorationFailure(
                QStringLiteral(
                    "The Hybrid investigation could "
                    "not be materialized."
                    )
                );
        }

        InvestigationSessionPersistence::
            restoreState(
                persistedSession,
                *session
                );

        return restorationSuccess(
            std::move(session)
            );
    }

    return restorationFailure(
        QStringLiteral(
            "Investigation restoration preparation "
            "contains an unsupported prepared type."
            )
        );
}

InvestigationSessionRestorationResult
InvestigationSessionRestorationService::restore(
    const PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath,
    const ImportExecutionContext
        &executionContext
    ) const
{
    InvestigationSessionRestorationPreparationResult
        preparation =
        prepare(
            persistedSession,
            workspaceFilePath,
            executionContext
            );

    return materialize(
        persistedSession,
        std::move(preparation)
        );
}