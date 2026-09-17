#include "InvestigationSession.h"

#include <QFileInfo>
#include <QUuid>
#include <QSet>

#include <algorithm>
#include <utility>

#include "../live/LiveSessionFollowCoordinator.h"

#include "HybridInvestigationReconstructionService.h"

namespace
{
InvestigationSessionSourceMetadata
snapshotSourceMetadata(
    const InvestigationSessionSnapshot &snapshot
    )
{
    InvestigationSessionSourceMetadata
        sourceMetadata;

    /*
     * Snapshot-backed investigations retain descriptive
     * source provenance without retaining an operational
     * dependency on the original source file.
     */
    for (auto iterator =
         snapshot.records.crbegin();
         iterator
         != snapshot.records.crend();
         ++iterator) {
        if (iterator->source.sourcePath.isEmpty()
            && iterator->source.sourceName.isEmpty()) {
            continue;
        }

        sourceMetadata.sourcePath =
            iterator->source.sourcePath;

        sourceMetadata.sourceName =
            iterator->source.sourceName;

        break;
    }

    return sourceMetadata;
}

std::optional<InvestigationExternalSourceBinding>
snapshotReconnectSourceHint(
    const InvestigationSessionSnapshot &snapshot
    )
{
    if (!snapshot.sourceContinuity.has_value()) {
        return std::nullopt;
    }

    const InvestigationSnapshotSourceContinuity
        &continuity =
        *snapshot.sourceContinuity;

    if (continuity
            .sourcePath
            .trimmed()
            .isEmpty()) {
        return std::nullopt;
    }

    InvestigationExternalSourceBinding binding;

    binding.sourcePath =
        continuity.sourcePath;

    binding.logicalSourceKey =
        continuity
                .logicalSourceKey
                .trimmed()
                .isEmpty()
            ? continuity.sourcePath
            : continuity.logicalSourceKey;

    binding.sourceFamilyConfiguration =
        continuity.sourceFamilyConfiguration;

    /*
     * Rotated physical paths are transient discovery
     * results, not durable source-family identity.
     */
    binding
        .sourceFamilyConfiguration
        .rotatedSourcePaths
        .clear();

    binding.sourceGeneration =
        continuity.sourceGeneration;

    binding.sourceIdentity =
        continuity.sourceIdentity;

    return binding;
}

InvestigationExternalSourceBinding
initialExternalSourceBinding(
    const QString &filePath,
    SourceFamilyConfiguration
        sourceFamilyConfiguration
    )
{
    const QString absolutePath =
        QFileInfo(
            filePath
            ).absoluteFilePath();

    InvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        absolutePath;

    binding.logicalSourceKey =
        absolutePath;

    binding.sourceFamilyConfiguration =
        std::move(
            sourceFamilyConfiguration
            );

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            absolutePath
            );

    if (identityResult.succeeded) {
        binding.sourceIdentity =
            identityResult.identity;
    }

    return binding;
}
}

InvestigationSession::InvestigationSession(
    const QString &filePath,
    ImportProfile profile,
    ImportResult result,
    SourceFamilyConfiguration
        sourceFamilyConfiguration
    )
    : InvestigationSession(
          QUuid::createUuid().toString(
              QUuid::WithoutBraces
              ),
          filePath,
          std::move(profile),
          std::move(result),
          std::move(
              sourceFamilyConfiguration
              )
          )
{
}

InvestigationSession::InvestigationSession(
    QString sessionId,
    const QString &filePath,
    ImportProfile profile,
    ImportResult result,
    SourceFamilyConfiguration
        sourceFamilyConfiguration
    )
    : m_id(
          std::move(sessionId)
          ),
    m_backing(
        InvestigationSessionBacking::
        sourceBacked(
            initialExternalSourceBinding(
                filePath,
                std::move(
                    sourceFamilyConfiguration
                    )
                )
            )
        ),
    m_importProfile(
        std::move(profile)
        )
{
    refreshSourceMetadata();

    installImportResult(
        std::move(result)
        );
}

InvestigationSession::InvestigationSession(
    QString sessionId,
    InvestigationSessionBacking backing,
    InvestigationSessionSourceMetadata
        sourceMetadata,
    ImportProfile profile,
    ImportResult result
    )
    : m_id(
          std::move(sessionId)
          ),
    m_sourceMetadata(
        std::move(sourceMetadata)
        ),
    m_backing(
        std::move(backing)
        ),
    m_importProfile(
        std::move(profile)
        )
{
    installImportResult(
        std::move(result)
        );
}

InvestigationSession::~InvestigationSession() =
    default;

const QString &
InvestigationSession::id() const
{
    return m_id;
}

const QString &
InvestigationSession::selectedRecordId() const
{
    return m_selectedRecordId;
}

void InvestigationSession::setSelectedRecordId(
    const QString &recordId
    )
{
    m_selectedRecordId =
        recordId;
}

const InvestigationSessionSourceMetadata &
InvestigationSession::sourceMetadata() const
{
    return m_sourceMetadata;
}

const InvestigationSessionBacking &
InvestigationSession::backing() const
{
    return m_backing;
}

const InvestigationExternalSourceBinding *
InvestigationSession::reconnectSourceHint() const
{
    return m_reconnectSourceHint.has_value()
    ? &*m_reconnectSourceHint
    : nullptr;
}

bool InvestigationSession::
    updateSnapshotBackingPath(
        const QString &snapshotPath
        )
{
    return m_backing.updateSnapshotPath(
        snapshotPath
        );
}

QString InvestigationSession::
    externalSourcePath() const
{
    const InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (!externalSource) {
        return {};
    }

    return externalSource->sourcePath;
}

qint64 InvestigationSession::
    initialLiveFollowByteOffset() const
{
    return m_initialLiveFollowByteOffset;
}

void InvestigationSession::
    setInitialLiveFollowByteOffset(
        qint64 byteOffset
        )
{
    m_initialLiveFollowByteOffset =
        std::max<qint64>(
            0,
            byteOffset
            );
}

const SourceFamilyConfiguration &
    InvestigationSession::
    sourceFamilyConfiguration() const
{
    const InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (externalSource) {
        return externalSource
            ->sourceFamilyConfiguration;
    }

    /*
     * Snapshot-only sessions have no active external
     * source family. Existing callers can still query
     * this API safely while we preserve its current
     * reference-returning contract.
     */
    static const SourceFamilyConfiguration
        emptyConfiguration;

    return emptyConfiguration;
}

void InvestigationSession::
    setSourceFamilyConfiguration(
        SourceFamilyConfiguration configuration
        )
{
    InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (!externalSource) {
        return;
    }

    externalSource
        ->sourceFamilyConfiguration =
        std::move(configuration);
}

const ImportProfile &
InvestigationSession::importProfile() const
{
    return m_importProfile;
}

const QVector<ImportDiagnostic> &
InvestigationSession::diagnostics() const
{
    return m_diagnostics;
}

qint64 InvestigationSession::
    processedRecordCount() const
{
    return m_processedRecordCount;
}

qint64 InvestigationSession::
    importedRecordCount() const
{
    return m_investigationController
        .totalRecordCount();
}

qint64 InvestigationSession::
    skippedRecordCount() const
{
    return std::max<qint64>(
        0,
        m_processedRecordCount
            - importedRecordCount()
        );
}

bool InvestigationSession::
    sourceTruncated() const
{
    return m_sourceTruncated;
}

InvestigationController *
    InvestigationSession::
    investigationController()
{
    return &m_investigationController;
}

const InvestigationController *
    InvestigationSession::
    investigationController() const
{
    return &m_investigationController;
}

InvestigationStateStore *
    InvestigationSession::
    investigationStateStore()
{
    return &m_investigationStateStore;
}

const InvestigationStateStore *
    InvestigationSession::
    investigationStateStore() const
{
    return &m_investigationStateStore;
}

void InvestigationSession::reload(
    ImportResult result
    )
{
    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    refreshSourceMetadata();

    installImportResult(
        std::move(result)
        );
}

void InvestigationSession::
    appendLiveImportResult(
        ImportResult result
        )
{
    const qint64 skippedRecordCount =
        std::max<qint64>(
            0,
            result.processedRecordCount
                - result.records.size()
            );

    if (!result.diagnostics.isEmpty()) {
        m_diagnostics.reserve(
            m_diagnostics.size()
            + result.diagnostics.size()
            );

        for (ImportDiagnostic &diagnostic
             : result.diagnostics) {
            m_diagnostics.append(
                std::move(diagnostic)
                );
        }
    }

    QVector<InvestigationRecord>
        appendedRecords;

    appendedRecords.reserve(
        result.records.size()
        );

    for (InvestigationRecord &record
         : result.records) {
        if (!record.recordId.isEmpty()
            && m_recordIds.contains(
                record.recordId
                )) {
            continue;
        }

        if (!record.recordId.isEmpty()) {
            m_recordIds.insert(
                record.recordId
                );
        }

        appendedRecords.append(
            std::move(record)
            );
    }

    m_processedRecordCount +=
        skippedRecordCount
        + appendedRecords.size();

    updateDerivedDataForAppendedRecords(
        appendedRecords
        );

    m_investigationController.appendRecords(
        std::move(appendedRecords)
        );
}

bool InvestigationSession::
    supportsLiveFollowing() const
{
    return m_backing.hasExternalSource()
        && LiveSessionFollowCoordinator::
            supportsProfile(
                m_importProfile
            );
}

LiveSessionFollowCoordinator *
    InvestigationSession::
    ensureLiveFollowCoordinator()
{
    if (!supportsLiveFollowing()) {
        return nullptr;
    }

    if (!m_liveFollowCoordinator) {
        m_liveFollowCoordinator =
            std::make_unique<
                LiveSessionFollowCoordinator
                >(
                *this
                );
    }

    return m_liveFollowCoordinator.get();
}

LiveSessionFollowCoordinator *
    InvestigationSession::
    liveFollowCoordinator()
{
    return m_liveFollowCoordinator.get();
}

const LiveSessionFollowCoordinator *
    InvestigationSession::
    liveFollowCoordinator() const
{
    return m_liveFollowCoordinator.get();
}

bool InvestigationSession::
    hasSeverityData() const
{
    return m_hasSeverityData;
}

bool InvestigationSession::
    hasSubsystemData() const
{
    return m_hasSubsystemData;
}

const QStringList &
    InvestigationSession::
    availableSubsystems() const
{
    return m_availableSubsystems;
}

bool InvestigationSession::
    hasCustomFieldData() const
{
    return m_hasCustomFieldData;
}

const QStringList &
    InvestigationSession::
    availableCustomFields() const
{
    return m_availableCustomFields;
}

bool InvestigationSession::
    hasEventCodeData() const
{
    return m_hasEventCodeData;
}

bool InvestigationSession::
    hasEntityData() const
{
    return m_hasEntityData;
}

const QStringList &
    InvestigationSession::
    availableEventCodes() const
{
    return m_availableEventCodes;
}

const QStringList &
    InvestigationSession::
    availableEntities() const
{
    return m_availableEntities;
}

const std::optional<QDateTime> &
    InvestigationSession::
    firstTimestamp() const
{
    return m_firstTimestamp;
}

const std::optional<QDateTime> &
    InvestigationSession::
    lastTimestamp() const
{
    return m_lastTimestamp;
}

const QVector<int> &
    InvestigationSession::
    columnWidths() const
{
    return m_columnWidths;
}

void InvestigationSession::setColumnWidths(
    QVector<int> widths
    )
{
    m_columnWidths =
        std::move(widths);
}

InvestigationReviewTab
InvestigationSession::reviewTab() const
{
    return m_reviewTab;
}

void InvestigationSession::setReviewTab(
    InvestigationReviewTab tab
    )
{
    m_reviewTab = tab;
}

InvestigationAnalyticsTab
InvestigationSession::analyticsTab() const
{
    return m_analyticsTab;
}

void InvestigationSession::setAnalyticsTab(
    InvestigationAnalyticsTab tab
    )
{
    m_analyticsTab = tab;
}

InvestigationTimelineBreakdown
InvestigationSession::timelineBreakdown() const
{
    return m_timelineBreakdown;
}

void InvestigationSession::setTimelineBreakdown(
    InvestigationTimelineBreakdown breakdown
    )
{
    m_timelineBreakdown =
        breakdown;
}

int InvestigationSession::subsystemTrendLimit() const
{
    return m_subsystemTrendLimit;
}

void InvestigationSession::setSubsystemTrendLimit(
    int limit
    )
{
    m_subsystemTrendLimit =
        limit;
}

void InvestigationSession::
    refreshSourceMetadata()
{
    const InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (!externalSource) {
        /*
         * Historical source provenance may still exist
         * in a Snapshot-backed investigation, but there
         * is no external file whose current filesystem
         * metadata should replace it.
         */
        return;
    }

    const QFileInfo fileInfo(
        externalSource->sourcePath
        );

    m_sourceMetadata.sourcePath =
        fileInfo.absoluteFilePath();

    m_sourceMetadata.sourceName =
        fileInfo.fileName();

    m_sourceMetadata.sourceSizeBytes =
        fileInfo.exists()
            ? fileInfo.size()
            : 0;

    m_sourceMetadata.sourceLastModified =
        fileInfo.exists()
            ? fileInfo.lastModified()
            : QDateTime();

    m_sourceMetadata.importedAtUtc =
        QDateTime::currentDateTimeUtc();
}

void InvestigationSession::installImportResult(
    ImportResult result
    )
{
    m_diagnostics =
        std::move(result.diagnostics);

    m_processedRecordCount =
        result.processedRecordCount;

    m_sourceTruncated =
        result.sourceTruncated;

    rebuildDerivedData(
        result.records
        );

    m_investigationStateStore.retainOnly(
        m_recordIds
        );

    if (!m_selectedRecordId.isEmpty()
        && !m_recordIds.contains(
            m_selectedRecordId
            )) {
        m_selectedRecordId.clear();
    }

    m_investigationController.setRecords(
        result.records
        );

    /*
     * A reload may change the available columns
     * or their representative content.
     */
    m_columnWidths.clear();
}

void InvestigationSession::rebuildDerivedData(
    const QVector<InvestigationRecord> &records
    )
{
    m_hasSeverityData = false;
    m_hasSubsystemData = false;

    m_availableSubsystems.clear();

    m_hasEventCodeData = false;
    m_hasEntityData = false;

    m_hasCustomFieldData = false;
    m_availableCustomFields.clear();

    m_availableEventCodes.clear();
    m_availableEntities.clear();

    m_recordIds.clear();

    m_knownSubsystems.clear();
    m_knownEventCodes.clear();
    m_knownEntities.clear();
    m_knownCustomFields.clear();

    m_firstTimestamp.reset();
    m_lastTimestamp.reset();

    for (const InvestigationRecord &record
         : records) {
        if (!record.recordId.isEmpty()) {
            m_recordIds.insert(
                record.recordId
                );
        }

        m_hasSeverityData =
            m_hasSeverityData
            || record.severity.has_value();

        if (record.subsystem.has_value()
            && !record.subsystem->isEmpty()) {
            m_hasSubsystemData = true;

            m_knownSubsystems.insert(
                record.subsystem.value()
                );
        }

        if (record.eventCode.has_value()
            && !record.eventCode
                    ->trimmed()
                    .isEmpty()) {
            m_hasEventCodeData = true;

            m_knownEventCodes.insert(
                record.eventCode.value()
                );
        }

        if (record.entityId.has_value()
            && !record.entityId
                    ->trimmed()
                    .isEmpty()) {
            m_hasEntityData = true;

            m_knownEntities.insert(
                record.entityId.value()
                );
        }

        for (
            auto iterator =
            record.customAttributes.constBegin();
            iterator !=
            record.customAttributes.constEnd();
            ++iterator
            ) {
            if (iterator.key().isEmpty()) {
                continue;
            }

            m_hasCustomFieldData = true;

            m_knownCustomFields.insert(
                iterator.key()
                );
        }

        if (!record.timestamp.has_value()) {
            continue;
        }

        const QDateTime timestamp =
            record.timestamp->toUTC();

        if (!timestamp.isValid()) {
            continue;
        }

        if (!m_firstTimestamp.has_value()
            || timestamp < *m_firstTimestamp) {
            m_firstTimestamp = timestamp;
        }

        if (!m_lastTimestamp.has_value()
            || timestamp > *m_lastTimestamp) {
            m_lastTimestamp = timestamp;
        }
    }

    m_availableSubsystems =
        m_knownSubsystems.values();

    m_availableEventCodes =
        m_knownEventCodes.values();

    m_availableCustomFields =
        m_knownCustomFields.values();

    std::sort(
        m_availableCustomFields.begin(),
        m_availableCustomFields.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );

    m_availableEntities =
        m_knownEntities.values();

    std::sort(
        m_availableSubsystems.begin(),
        m_availableSubsystems.end(),
        [](const QString &left,
           const QString &right) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );
}

void InvestigationSession::
    updateDerivedDataForAppendedRecords(
        const QVector<InvestigationRecord> &records
        )
{
    bool subsystemListChanged = false;
    bool customFieldListChanged = false;

    for (const InvestigationRecord &record
         : records) {
        m_hasSeverityData =
            m_hasSeverityData
            || record.severity.has_value();

        if (record.subsystem.has_value()
            && !record.subsystem->isEmpty()
            && !m_knownSubsystems.contains(
                record.subsystem.value()
                )) {
            m_hasSubsystemData = true;

            m_knownSubsystems.insert(
                record.subsystem.value()
                );

            m_availableSubsystems.append(
                record.subsystem.value()
                );

            subsystemListChanged = true;
        }

        if (record.eventCode.has_value()
            && !record.eventCode
                    ->trimmed()
                    .isEmpty()) {
            m_hasEventCodeData = true;

            if (!m_knownEventCodes.contains(
                    record.eventCode.value()
                    )) {
                m_knownEventCodes.insert(
                    record.eventCode.value()
                    );

                m_availableEventCodes.append(
                    record.eventCode.value()
                    );
            }
        }

        if (record.entityId.has_value()
            && !record.entityId
                    ->trimmed()
                    .isEmpty()) {
            m_hasEntityData = true;

            if (!m_knownEntities.contains(
                    record.entityId.value()
                    )) {
                m_knownEntities.insert(
                    record.entityId.value()
                    );

                m_availableEntities.append(
                    record.entityId.value()
                    );
            }
        }

        for (
            auto iterator =
            record.customAttributes.constBegin();
            iterator !=
            record.customAttributes.constEnd();
            ++iterator
            ) {
            if (iterator.key().isEmpty()) {
                continue;
            }

            m_hasCustomFieldData = true;

            if (!m_knownCustomFields.contains(
                    iterator.key()
                    )) {
                m_knownCustomFields.insert(
                    iterator.key()
                    );

                m_availableCustomFields.append(
                    iterator.key()
                    );

                customFieldListChanged = true;
            }
        }

        if (!record.timestamp.has_value()) {
            continue;
        }

        const QDateTime timestamp =
            record.timestamp->toUTC();

        if (!timestamp.isValid()) {
            continue;
        }

        if (!m_firstTimestamp.has_value()
            || timestamp < *m_firstTimestamp) {
            m_firstTimestamp =
                timestamp;
        }

        if (!m_lastTimestamp.has_value()
            || timestamp > *m_lastTimestamp) {
            m_lastTimestamp =
                timestamp;
        }
    }

    if (subsystemListChanged) {
        std::sort(
            m_availableSubsystems.begin(),
            m_availableSubsystems.end(),
            [](
                const QString &left,
                const QString &right
                ) {
                return left.compare(
                           right,
                           Qt::CaseInsensitive
                           ) < 0;
            }
            );
    }

    if (customFieldListChanged) {
        std::sort(
            m_availableCustomFields.begin(),
            m_availableCustomFields.end(),
            [](
                const QString &left,
                const QString &right
                ) {
                return left.compare(
                           right,
                           Qt::CaseInsensitive
                           ) < 0;
            }
            );
    }
}

InvestigationBurstTimingMode
InvestigationSession::burstTimingMode() const
{
    return m_burstTimingMode;
}

void InvestigationSession::setBurstTimingMode(
    InvestigationBurstTimingMode mode
    )
{
    m_burstTimingMode =
        mode;
}

const BurstDetectionSettings &
    InvestigationSession::
    burstDetectionSettings() const
{
    return m_burstDetectionSettings;
}

void InvestigationSession::
    setBurstDetectionSettings(
        const BurstDetectionSettings &settings
        )
{
    if (!settings.isValid()) {
        return;
    }

    m_burstDetectionSettings =
        settings;
}

InvestigationSessionSnapshot
InvestigationSession::captureSnapshot() const
{
    InvestigationSessionSnapshot snapshot;

    /*
     * Phase 15 snapshot capture preserves normalized
     * investigation evidence only.
     *
     * Do not claim CompleteSource unless the actual
     * byte-faithful source material is captured into the
     * snapshot as well.
     */
    snapshot.sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    /*
     * Preserve source continuity whenever this session
     * knows where its evidence originated.
     *
     * Active external backing takes precedence. A
     * SnapshotBacked session may instead retain a
     * dormant reconnect hint from a previous external
     * source relationship.
     */
    const InvestigationExternalSourceBinding
        *continuitySource =
        m_backing.externalSource();

    if (continuitySource == nullptr
        && m_reconnectSourceHint.has_value()) {
        continuitySource =
            &*m_reconnectSourceHint;
    }

    if (continuitySource != nullptr) {
        InvestigationSnapshotSourceContinuity
            continuity;

        continuity.sourcePath =
            continuitySource->sourcePath;

        continuity.logicalSourceKey =
            continuitySource
                    ->logicalSourceKey
                    .trimmed()
                    .isEmpty()
                ? continuitySource->sourcePath
                : continuitySource
                      ->logicalSourceKey;

        continuity.sourceFamilyConfiguration =
            continuitySource
                ->sourceFamilyConfiguration;

        /*
         * Physical rotated members are discovery
         * results. Persist only the durable discovery
         * rule/configuration.
         */
        continuity
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .clear();

        continuity.sourceGeneration =
            continuitySource->sourceGeneration;

        continuity.sourceIdentity =
            continuitySource->sourceIdentity;

        snapshot.sourceContinuity =
            std::move(continuity);
    }

    snapshot.importProfile =
        m_importProfile;

    snapshot.records =
        m_investigationController
            .allRecords();

    snapshot.diagnostics =
        m_diagnostics;

    snapshot.processedRecordCount =
        m_processedRecordCount;

    snapshot.sourceTruncated =
        m_sourceTruncated;

    return snapshot;
}

std::unique_ptr<InvestigationSession>
InvestigationSession::createSnapshotBacked(
    QString sessionId,
    const QString &snapshotPath,
    InvestigationSessionSnapshot snapshot,
    std::optional<
        InvestigationExternalSourceBinding>
        reconnectSourceHint
    )
{
    if (!reconnectSourceHint.has_value()) {
        reconnectSourceHint =
            snapshotReconnectSourceHint(
                snapshot
                );
    }

    /*
     * Recover descriptive provenance from the most
     * recent saved record that contains source
     * identity.
     *
     * This is deliberately not an external dependency.
     * The operational backing remains the .tsinv file.
     */
    InvestigationSessionSourceMetadata
        sourceMetadata =
        snapshotSourceMetadata(
            snapshot
            );

    ImportResult result;

    result.records =
        std::move(snapshot.records);

    result.diagnostics =
        std::move(snapshot.diagnostics);

    result.processedRecordCount =
        snapshot.processedRecordCount;

    result.sourceTruncated =
        snapshot.sourceTruncated;

    InvestigationSnapshotBinding
        snapshotBinding;

    snapshotBinding.snapshotPath =
        QFileInfo(
            snapshotPath
            ).absoluteFilePath();

    snapshotBinding.sourceFidelity =
        snapshot.sourceFidelity;

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::
        snapshotBacked(
            std::move(
                snapshotBinding
                )
            );

    auto session =
        std::unique_ptr<InvestigationSession>(
            new InvestigationSession(
                std::move(sessionId),
                std::move(backing),
                std::move(sourceMetadata),
                std::move(
                    snapshot.importProfile
                    ),
                std::move(result)
                )
            );

    session->m_reconnectSourceHint =
        std::move(
            reconnectSourceHint
            );

    return session;
}

void InvestigationSession::
    updateExternalSourceRuntimeState(
        quint64 sourceGeneration,
        const SourcePhysicalIdentity *sourceIdentity
        )
{
    InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (!externalSource) {
        return;
    }

    externalSource->sourceGeneration =
        sourceGeneration;

    if (sourceIdentity) {
        externalSource->sourceIdentity =
            *sourceIdentity;
    } else {
        externalSource->sourceIdentity.reset();
    }
}

bool InvestigationSession::
    updateExternalSourceLogicalKey(
        const QString &logicalSourceKey
        )
{
    if (logicalSourceKey.trimmed().isEmpty()) {
        return false;
    }

    InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (externalSource == nullptr) {
        return false;
    }

    externalSource->logicalSourceKey =
        logicalSourceKey;

    return true;
}

InvestigationSessionSourceRelocationResult
InvestigationSession::redefineSourcePath(
    const QString &candidatePath
    )
{
    InvestigationSessionSourceRelocationResult
        result;

    if (candidatePath.trimmed().isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "A source path must be selected."
                );

        return result;
    }

    InvestigationExternalSourceBinding
        *sourceBinding =
        m_backing.externalSource();

    const bool hasActiveExternalSource =
        sourceBinding != nullptr;

    if (sourceBinding == nullptr
        && m_reconnectSourceHint.has_value()) {
        sourceBinding =
            &*m_reconnectSourceHint;
    }

    if (sourceBinding == nullptr) {
        result.errorMessage =
            QStringLiteral(
                "This investigation does not have an "
                "external source relationship that can "
                "be relocated."
                );

        return result;
    }

    qint64 minimumCandidateSize = 0;

    /*
     * If live following is active or paused, reconcile
     * any pending truncation/replacement before deciding
     * which physical generation the relocation candidate
     * must represent.
     */
    if (hasActiveExternalSource
        && m_liveFollowCoordinator
        && !m_liveFollowCoordinator
                ->state()
                .isStopped()) {
        const LiveSessionSourceSynchronizationResult
            synchronization =
            m_liveFollowCoordinator
                ->synchronizeSourceForRelocation();

        if (!synchronization.succeeded) {
            result.errorMessage =
                synchronization.errorMessage;

            return result;
        }

        sourceBinding =
            m_backing.externalSource();

        if (sourceBinding == nullptr) {
            result.errorMessage =
                QStringLiteral(
                    "The active external source binding "
                    "became unavailable during relocation."
                    );

            return result;
        }

        minimumCandidateSize =
            std::max<qint64>(
                0,
                synchronization
                    .observation
                    .observedSizeBytes
                );
    }

    if (!sourceBinding
             ->sourceIdentity
             .has_value()) {
        result.errorMessage =
            QStringLiteral(
                "TraceScope does not have enough saved "
                "physical source identity information "
                "to verify this relocation."
                );

        return result;
    }

    const SourcePhysicalIdentity
        expectedIdentity =
        *sourceBinding->sourceIdentity;

    const QString currentSourcePath =
        sourceBinding->sourcePath;

    const QString absoluteCandidatePath =
        QFileInfo(
            candidatePath
            ).absoluteFilePath();

    /*
     * Once a live coordinator exists, its cursor is
     * authoritative. In particular, a source reset
     * legitimately moves that cursor back to zero for
     * the new generation.
     */
    qint64 continuationOffset =
        m_initialLiveFollowByteOffset;

    if (hasActiveExternalSource
        && m_liveFollowCoordinator) {
        continuationOffset =
            m_liveFollowCoordinator
                ->state()
                .readOffset();
    }

    if (continuationOffset >= 0) {
        minimumCandidateSize =
            std::max(
                minimumCandidateSize,
                continuationOffset
                );
    }

    const SourceRelocationVerificationResult
        verification =
        verifySourceRelocationCandidate(
            absoluteCandidatePath,
            expectedIdentity
            );

    if (!verification.verified) {
        result.errorMessage =
            verification.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The selected file could not be "
                      "verified as the same logical "
                      "source."
                      )
                : verification.errorMessage;

        return result;
    }

    if (verification
            .candidateIdentity
            .observedSizeBytes
        < minimumCandidateSize) {
        result.errorMessage =
            QStringLiteral(
                "The selected source does not contain "
                "the complete physical range currently "
                "represented by this investigation."
                );

        return result;
    }

    /*
     * The producer is independent from TraceScope and
     * may continue changing the source while candidate
     * verification runs. Recheck the old path before
     * committing the relocation.
     */
    if (hasActiveExternalSource) {
        const QFileInfo currentSourceInfo(
            currentSourcePath
            );

        if (!currentSourceInfo.exists()
            || !currentSourceInfo.isFile()) {
            result.errorMessage =
                QStringLiteral(
                    "The source changed while relocation "
                    "was being verified. Try again."
                    );

            return result;
        }

        if (sourcePhysicalIdentityChanged(
                currentSourcePath,
                expectedIdentity
                )
            || currentSourceInfo.size()
                   > verification
                         .candidateIdentity
                         .observedSizeBytes) {
            result.errorMessage =
                QStringLiteral(
                    "The source changed while relocation "
                    "was being verified. Try again."
                    );

            return result;
        }
    }

    const QString logicalSourceKey =
        sourceBinding
                ->logicalSourceKey
                .trimmed()
                .isEmpty()
            ? sourceBinding->sourcePath
            : sourceBinding->logicalSourceKey;

    if (hasActiveExternalSource
        && m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    sourceBinding->sourcePath =
        absoluteCandidatePath;

    sourceBinding->logicalSourceKey =
        logicalSourceKey;

    sourceBinding->sourceIdentity =
        verification.candidateIdentity;

    sourceBinding
        ->sourceFamilyConfiguration
        .rotatedSourcePaths
        .clear();

    /*
     * Relocation preserves the current logical source
     * generation. It does not create another one.
     */

    if (hasActiveExternalSource) {
        if (continuationOffset >= 0) {
            m_initialLiveFollowByteOffset =
                continuationOffset;
        }

        refreshSourceMetadata();
    }

    result.succeeded = true;

    return result;
}

InvestigationSessionSourceReloadResult
    InvestigationSession::
    applyPreparedSourceBackedReload(
        InvestigationExternalSourceBinding
            externalSourceBinding,
        ImportProfile importProfile,
        ImportResult importResult,
        qint64 initialLiveFollowByteOffset
        )
{
    InvestigationSessionSourceReloadResult
        result;

    if (m_backing.mode()
        != InvestigationSessionBackingMode::
        SourceBacked) {
        result.errorMessage =
            QStringLiteral(
                "Prepared source reload requires a "
                "Source-backed investigation session."
                );

        return result;
    }

    if (externalSourceBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The prepared external source does "
                "not contain a valid source path."
                );

        return result;
    }

    if (externalSourceBinding
            .logicalSourceKey
            .trimmed()
            .isEmpty()) {
        externalSourceBinding.logicalSourceKey =
            externalSourceBinding.sourcePath;
    }

    /*
     * All source I/O, identity verification,
     * generation handling, and import work has already
     * completed before this mutation boundary.
     */
    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    m_backing =
        InvestigationSessionBacking::
        sourceBacked(
            std::move(
                externalSourceBinding
                )
            );

    m_reconnectSourceHint.reset();

    m_importProfile =
        std::move(
            importProfile
            );

    m_initialLiveFollowByteOffset =
        std::max<qint64>(
            0,
            initialLiveFollowByteOffset
            );

    refreshSourceMetadata();

    /*
     * State survives automatically for stable record
     * IDs and disappears for records no longer present.
     */
    installImportResult(
        std::move(
            importResult
            )
        );

    result.succeeded = true;

    return result;
}

bool InvestigationSession::
    applySnapshotReload(
        InvestigationSessionSnapshot snapshot
        )
{
    if (m_backing.mode()
        != InvestigationSessionBackingMode::
        SnapshotBacked) {
        return false;
    }

    /*
     * Everything required for the replacement has
     * already been prepared by the caller. From here
     * on, no I/O or parsing can fail.
     */
    InvestigationSessionSourceMetadata
        sourceMetadata =
        snapshotSourceMetadata(
            snapshot
            );

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

    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    InvestigationSnapshotBinding
        *snapshotBinding =
        m_backing.snapshot();

    if (snapshotBinding == nullptr) {
        return false;
    }

    snapshotBinding->sourceFidelity =
        snapshot.sourceFidelity;

    m_sourceMetadata =
        std::move(
            sourceMetadata
            );

    m_importProfile =
        std::move(
            profile
            );

    m_initialLiveFollowByteOffset =
        0;

    installImportResult(
        std::move(result)
        );

    return true;
}

InvestigationSessionHybridReloadResult
InvestigationSession::applyHybridReload(
    InvestigationSessionSnapshot snapshot,
    HybridInvestigationReconstructionResult
        reconstruction
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

    if (!reconstruction.succeeded) {
        result.errorMessage =
            reconstruction.errorMessage.isEmpty()
                ? QStringLiteral(
                      "Hybrid investigation "
                      "reconstruction failed."
                      )
                : reconstruction.errorMessage;

        return result;
    }

    /*
     * Preparation is complete. This is the mutation
     * boundary for the currently open session.
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

    if (mutableSnapshotBinding != nullptr) {
        mutableSnapshotBinding
            ->sourceFidelity =
            snapshot.sourceFidelity;
    }

    m_importProfile =
        std::move(
            snapshot.importProfile
            );

    m_initialLiveFollowByteOffset =
        0;

    refreshSourceMetadata();

    installImportResult(
        std::move(
            reconstruction.importResult
            )
        );

    result.succeeded = true;

    return result;
}

InvestigationSessionHybridReloadResult
InvestigationSession::applyHybridReconnect(
    InvestigationSessionSnapshot snapshot,
    HybridInvestigationReconstructionResult
        reconstruction
    )
{
    InvestigationSessionHybridReloadResult
        result;

    /*
     * Reconnect is specifically the
     * SnapshotBacked -> Hybrid transition.
     */
    if (m_backing.mode()
        != InvestigationSessionBackingMode::
        SnapshotBacked) {
        result.errorMessage =
            QStringLiteral(
                "Reconnect requires a Snapshot-backed "
                "investigation session."
                );

        return result;
    }

    InvestigationSnapshotBinding
        *snapshotBinding =
        m_backing.snapshot();

    if (snapshotBinding == nullptr) {
        result.errorMessage =
            QStringLiteral(
                "The Snapshot-backed investigation "
                "does not contain a durable snapshot "
                "binding."
                );

        return result;
    }

    if (!reconstruction.succeeded) {
        result.errorMessage =
            reconstruction.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The external source could not "
                      "be reconstructed against the "
                      "saved investigation."
                      )
                : reconstruction.errorMessage;

        return result;
    }

    /*
     * Reconnect is different from ordinary Hybrid
     * reload.
     *
     * Hybrid reload may legitimately succeed with a
     * temporarily missing source because the snapshot
     * remains its evidence floor.
     *
     * Reconnect, however, is explicitly establishing
     * a new external connection. If the selected
     * source disappeared during preparation, do not
     * create a Hybrid session that is immediately
     * disconnected.
     */
    if (!reconstruction.externalSourceAvailable) {
        result.errorMessage =
            QStringLiteral(
                "The selected external source is no "
                "longer available."
                );

        return result;
    }

    if (reconstruction
            .externalSourceBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The reconstructed external source "
                "does not contain a valid path."
                );

        return result;
    }

    /*
     * Everything that can fail has completed.
     *
     * From this point onward we commit the prepared
     * candidate atomically to the live session.
     */
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

    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    snapshotBinding->sourceFidelity =
        snapshot.sourceFidelity;

    /*
     * InvestigationSessionBacking owns the actual
     * state transition:
     *
     * SnapshotBacked -> Hybrid
     */
    m_backing.connectExternalSource(
        std::move(
            reconstruction
                .externalSourceBinding
            )
        );

    m_reconnectSourceHint.reset();

    m_importProfile =
        std::move(
            snapshot.importProfile
            );

    /*
     * We do not persist live parser/framer state.
     * A newly reconnected Hybrid source therefore
     * starts future live following conservatively
     * from byte zero and relies on stable identity
     * deduplication.
     */
    m_initialLiveFollowByteOffset =
        0;

    refreshSourceMetadata();

    installImportResult(
        std::move(
            reconstruction.importResult
            )
        );

    result.succeeded = true;

    return result;
}

InvestigationSessionSourceAuthoritativeResult
    InvestigationSession::
    applySourceAuthoritativeTransition(
        InvestigationExternalSourceBinding
            externalSourceBinding,
        ImportProfile importProfile,
        ImportResult importResult,
        qint64 initialLiveFollowByteOffset
        )
{
    InvestigationSessionSourceAuthoritativeResult
        result;

    /*
     * "Use Source as Authoritative" is specifically
     * the Hybrid -> SourceBacked lifecycle transition.
     */
    if (m_backing.mode()
        != InvestigationSessionBackingMode::
        Hybrid) {
        result.errorMessage =
            QStringLiteral(
                "Using the external source as "
                "authoritative requires a Hybrid "
                "investigation session."
                );

        return result;
    }

    if (externalSourceBinding
            .sourcePath
            .trimmed()
            .isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The prepared external source does "
                "not contain a valid source path."
                );

        return result;
    }

    if (externalSourceBinding
            .logicalSourceKey
            .trimmed()
            .isEmpty()) {
        externalSourceBinding.logicalSourceKey =
            externalSourceBinding.sourcePath;
    }

    /*
     * Everything that can fail has already completed.
     *
     * From this point onward we commit the prepared
     * SourceBacked candidate to the existing session.
     */
    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    /*
     * Replace the entire Hybrid backing rather than
     * merely detaching the old snapshot.
     *
     * The prepared external binding may contain a
     * newer physical identity, generation, discovered
     * source-family members, or relocated source path.
     */
    m_backing =
        InvestigationSessionBacking::
        sourceBacked(
            std::move(
                externalSourceBinding
                )
            );

    m_reconnectSourceHint.reset();

    m_importProfile =
        std::move(
            importProfile
            );

    m_initialLiveFollowByteOffset =
        std::max<qint64>(
            0,
            initialLiveFollowByteOffset
            );

    refreshSourceMetadata();

    /*
     * Replacing the normalized result deliberately
     * removes snapshot-only evidence.
     *
     * installImportResult() retains investigation
     * state for record IDs that survive the source
     * reconstruction and removes state attached only
     * to records that disappear.
     */
    installImportResult(
        std::move(
            importResult
            )
        );

    result.succeeded = true;

    return result;
}

bool InvestigationSession::
    applySnapshotOnlyTransition(
        const QString &snapshotPath,
        InvestigationSnapshotSourceFidelity
            sourceFidelity
        )
{
    if (snapshotPath.trimmed().isEmpty()) {
        return false;
    }

    const InvestigationSessionBackingMode mode =
        m_backing.mode();

    if (mode
            != InvestigationSessionBackingMode::
            SourceBacked
        && mode
               != InvestigationSessionBackingMode::
               Hybrid) {
        return false;
    }

    const InvestigationExternalSourceBinding
        *externalSource =
        m_backing.externalSource();

    if (externalSource == nullptr) {
        return false;
    }

    /*
     * Preserve the complete last-known external
     * binding before removing it from operational
     * backing.
     *
     * SnapshotBacked does not depend on this source,
     * but retaining the binding lets an explicit
     * future Reconnect Source restore continuity.
     */
    InvestigationExternalSourceBinding
        reconnectHint =
        *externalSource;

    InvestigationSnapshotBinding
        snapshotBinding;

    snapshotBinding.snapshotPath =
        QFileInfo(
            snapshotPath
            ).absoluteFilePath();

    snapshotBinding.sourceFidelity =
        sourceFidelity;

    /*
     * Everything required to make the transition has
     * already been persisted successfully by the
     * caller.
     *
     * From here onward there is no I/O or parsing that
     * can fail.
     */
    if (m_liveFollowCoordinator) {
        m_liveFollowCoordinator->stop();
        m_liveFollowCoordinator.reset();
    }

    m_reconnectSourceHint =
        std::move(
            reconnectHint
            );

    /*
     * Replace either:
     *
     * SourceBacked -> SnapshotBacked
     *
     * or:
     *
     * Hybrid -> SnapshotBacked
     *
     * in one value assignment. In the Hybrid case the
     * previous snapshot binding is intentionally
     * superseded by the newly captured snapshot.
     */
    m_backing =
        InvestigationSessionBacking::
        snapshotBacked(
            std::move(
                snapshotBinding
                )
            );

    /*
     * Live parser/framer state is no longer meaningful
     * while the investigation is SnapshotBacked.
     */
    m_initialLiveFollowByteOffset =
        0;

    /*
     * Deliberately leave the normalized records,
     * diagnostics, investigation state, selection,
     * filters, and presentation state untouched.
     *
     * The fresh snapshot persisted by the caller was
     * captured from this exact in-memory state.
     */
    return true;
}