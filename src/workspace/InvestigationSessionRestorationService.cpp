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
#include "InvestigationSessionPersistence.h"

namespace
{
InvestigationSessionRestorationResult failure(
    const QString &errorMessage
    )
{
    InvestigationSessionRestorationResult result;

    result.succeeded = false;
    result.errorMessage = errorMessage;

    return result;
}

InvestigationSessionRestorationResult success(
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
     * Persisted source-family state deliberately keeps
     * only the logical rotation rule. Physical rotated
     * paths must be rediscovered on restoration.
     */
    runtimeConfiguration.rotatedSourcePaths.clear();

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
     * Physical rotated paths are runtime discovery
     * state, not persisted identity.
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

InvestigationSessionRestorationResult
restoreSourceBackedSession(
    const PersistedInvestigationSession
        &persistedSession
    )
{
    const PersistedInvestigationSessionBacking
        &backing =
        persistedSession.backing;

    if (!backing
             .externalSourceBinding
             .has_value()) {
        return failure(
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
        return failure(
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
        return failure(
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
     * Source-backed means exactly that: the external
     * source remains authoritative.
     *
     * An optional snapshot reference attached by a
     * workspace save policy does not silently change
     * the runtime backing mode.
     */
    if (!sourceInfo.exists()
        || !sourceInfo.isFile()) {
        return failure(
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
        return failure(
            QStringLiteral(
                "No importer is available for the "
                "persisted Source-backed import "
                "profile."
                )
            );
    }

    /*
     * Capture the handoff point before static import.
     *
     * If the producer appends while restoration is in
     * progress, later live following may conservatively
     * replay overlap rather than skip unseen bytes.
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
            return failure(
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

    /*
     * Establish a physical identity immediately before
     * import so truncation/replacement during the
     * restoration itself can be rejected.
     */
    const SourcePhysicalIdentityCaptureResult
        identityBeforeImport =
        captureSourcePhysicalIdentity(
            activeSourcePath
            );

    if (!identityBeforeImport.succeeded) {
        return failure(
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
        return failure(
            sourceFamilyError.isEmpty()
                ? QStringLiteral(
                      "Could not rediscover the "
                      "persisted source family."
                      )
                : sourceFamilyError
            );
    }

    SourceFamilyImportService
        importService;

    ImportResult importResult =
        importService.importFiles(
            sourceFamilyConfiguration
                .orderedSourcePaths(
                    activeSourcePath
                    ),
            *importer
            );

    if (importResult.cancelled) {
        return failure(
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
        return failure(
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
     * Reject a reconstruction whose active physical
     * source was truncated or replaced while the
     * candidate was being prepared.
     *
     * Ordinary append-only growth is intentionally not
     * considered replacement.
     */
    if (sourcePhysicalIdentityChanged(
            activeSourcePath,
            identityBeforeImport.identity
            )) {
        return failure(
            QStringLiteral(
                "The external source changed while "
                "the investigation was being "
                "restored. The current investigation "
                "was not replaced."
                )
            );
    }

    /*
     * Static importers describe their input as
     * generation zero.
     *
     * Only the active source belongs to the persisted
     * same-path generation sequence. Rotated physical
     * files remain independently identified sources.
     */
    rebaseImportResultSourceGenerationForPath(
        importResult,
        activeSourcePath,
        sourceGeneration
        );

    auto session =
        std::make_unique<
            InvestigationSession>(
            persistedSession.sessionId,
            activeSourcePath,
            profile,
            std::move(importResult),
            std::move(
                sourceFamilyConfiguration
                )
            );

    session->updateExternalSourceRuntimeState(
        sourceGeneration,
        &identityAfterImport.identity
        );

    session->setInitialLiveFollowByteOffset(
        initialLiveFollowByteOffset
        );

    /*
     * Record/filter/presentation state is restored only
     * after the normalized record set has been
     * established. That lets record-ID validation occur
     * against the reconstructed evidence.
     */
    InvestigationSessionPersistence::
        restoreState(
            persistedSession,
            *session
            );

    return success(
        std::move(session)
        );
}

InvestigationSessionRestorationResult
restoreSnapshotBackedSession(
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
        return failure(
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
        return failure(
            QStringLiteral(
                "The relative investigation snapshot "
                "could not be resolved because the "
                "workspace path is unavailable."
                )
            );
    }

    InvestigationSessionSnapshotFile
        snapshotFile;

    InvestigationSessionSnapshotLoadResult
        loadResult =
        snapshotFile.load(
            snapshotPath
            );

    if (!loadResult.isSuccess()) {
        return failure(
            loadResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The investigation snapshot "
                      "could not be loaded."
                      )
                : loadResult.errorMessage
            );
    }

    auto session =
        InvestigationSession::
        createSnapshotBacked(
            persistedSession.sessionId,
            snapshotPath,
            std::move(
                *loadResult.snapshot
                )
            );

    InvestigationSessionPersistence::
        restoreState(
            persistedSession,
            *session
            );

    return success(
        std::move(session)
        );
}

InvestigationSessionRestorationResult
restoreHybridSession(
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
        return failure(
            QStringLiteral(
                "The Hybrid investigation does not "
                "contain a snapshot reference."
                )
            );
    }

    if (!backing
             .externalSourceBinding
             .has_value()) {
        return failure(
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
        return failure(
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
        return failure(
            QStringLiteral(
                "The relative Hybrid snapshot could "
                "not be resolved because the "
                "workspace path is unavailable."
                )
            );
    }

    InvestigationSessionSnapshotFile
        snapshotFile;

    InvestigationSessionSnapshotLoadResult
        loadResult =
        snapshotFile.load(
            snapshotPath
            );

    if (!loadResult.isSuccess()) {
        return failure(
            loadResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The Hybrid investigation "
                      "snapshot could not be loaded."
                      )
                : loadResult.errorMessage
            );
    }

    const ImportProfile snapshotProfile =
        loadResult.snapshot->importProfile;

    InvestigationExternalSourceBinding
        externalSourceBinding =
        runtimeExternalSourceBinding(
            persistedBinding
            );

    auto session =
        InvestigationSession::createHybrid(
            persistedSession.sessionId,
            snapshotPath,
            std::move(
                *loadResult.snapshot
                ),
            std::move(
                externalSourceBinding
                )
            );

    /*
     * A missing current source is a valid Hybrid
     * recovery case. createHybrid() has already
     * installed the durable snapshot evidence and
     * preserved the external binding for a future
     * continuation.
     *
     * Avoid requiring an importer merely to recover
     * that snapshot-only state.
     */
    const QFileInfo externalSourceInfo(
        session->externalSourcePath()
        );

    if (externalSourceInfo.exists()) {
        const std::shared_ptr<ILogImporter>
            importer =
            createImporter(
                snapshotProfile
                );

        if (!importer) {
            return failure(
                QStringLiteral(
                    "No importer is available for the "
                    "Hybrid investigation's persisted "
                    "snapshot profile."
                    )
                );
        }

        const InvestigationSessionHybridReloadResult
            reloadResult =
            session->reloadHybrid(
                *importer
                );

        if (!reloadResult.succeeded) {
            return failure(
                reloadResult.errorMessage.isEmpty()
                    ? QStringLiteral(
                          "The Hybrid investigation "
                          "could not be reconstructed."
                          )
                    : reloadResult.errorMessage
                );
        }
    }

    InvestigationSessionPersistence::
        restoreState(
            persistedSession,
            *session
            );

    return success(
        std::move(session)
        );
}
}

InvestigationSessionRestorationResult
InvestigationSessionRestorationService::restore(
    const PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath
    ) const
{
    switch (persistedSession.backing.mode) {
    case PersistedInvestigationSessionBackingMode::
        SourceBacked:
        return restoreSourceBackedSession(
            persistedSession
            );

    case PersistedInvestigationSessionBackingMode::
        SnapshotBacked:
        return restoreSnapshotBackedSession(
            persistedSession,
            workspaceFilePath
            );

    case PersistedInvestigationSessionBackingMode::
        Hybrid:
        return restoreHybridSession(
            persistedSession,
            workspaceFilePath
            );
    }

    return failure(
        QStringLiteral(
            "The persisted investigation uses an "
            "unsupported backing mode."
            )
        );
}