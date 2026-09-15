#include "InvestigationSession.h"

#include <QFileInfo>
#include <QUuid>
#include <QSet>

#include <algorithm>
#include <utility>

#include "../live/LiveSessionFollowCoordinator.h"

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
            InvestigationExternalSourceBinding {
                QFileInfo(
                    filePath
                    ).absoluteFilePath(),
                std::move(
                    sourceFamilyConfiguration
                    )
            }
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
     * Phase 15 captures the complete normalized
     * investigation but does not archive byte-faithful
     * original source material.
     *
     * If this session was itself loaded from a
     * higher-fidelity future snapshot, preserve that
     * capability marker.
     */
    const InvestigationSnapshotBinding
        *snapshotBinding =
        m_backing.snapshot();

    snapshot.sourceFidelity =
        snapshotBinding
            ? snapshotBinding->sourceFidelity
            : InvestigationSnapshotSourceFidelity::
            NormalizedOnly;

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
    InvestigationSessionSnapshot snapshot
    )
{
    InvestigationSessionSourceMetadata
        sourceMetadata;

    /*
     * Recover descriptive provenance from the most
     * recent saved record that contains source
     * identity.
     *
     * This is deliberately not an external dependency.
     * The operational backing remains the .tsinv file.
     */
    for (
        auto iterator =
        snapshot.records.crbegin();
        iterator != snapshot.records.crend();
        ++iterator
        ) {
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

    return std::unique_ptr<
        InvestigationSession>(
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