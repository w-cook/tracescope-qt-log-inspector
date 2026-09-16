#pragma once

#include <memory>
#include <optional>

#include <QDateTime>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "InvestigationSessionBacking.h"
#include "InvestigationStateStore.h"

#include "../analysis/BurstDetectionSettings.h"
#include "../controllers/InvestigationController.h"
#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"
#include "../persistence/InvestigationSessionSnapshot.h"
#include "../sources/SourceFamilyConfiguration.h"

struct InvestigationSessionSourceMetadata
{
    QString sourcePath;
    QString sourceName;

    qint64 sourceSizeBytes = 0;

    QDateTime sourceLastModified;
    QDateTime importedAtUtc;
};

enum class InvestigationReviewTab
{
    IssueSummary,
    Findings,
    Analytics
};

enum class InvestigationAnalyticsTab
{
    Overview,
    Bursts
};

enum class InvestigationTimelineBreakdown
{
    Severity,
    Subsystem
};

enum class InvestigationBurstTimingMode
{
    Auto,
    Manual
};

struct HybridInvestigationReconstructionResult;

class ILogImporter;
class LiveSessionFollowCoordinator;

struct InvestigationSessionHybridReloadResult
{
    bool succeeded = false;

    QString errorMessage;

    bool externalSourceAvailable = false;

    bool sourceGenerationChanged = false;

    qint64 duplicateReplayRecordCount = 0;
    qint64 appendedReplayRecordCount = 0;
    qint64 replaySkippedRecordCount = 0;
};

class InvestigationSession
{
public:
    InvestigationSession(
        const QString &filePath,
        ImportProfile profile,
        ImportResult result,
        SourceFamilyConfiguration
            sourceFamilyConfiguration = {}
        );

    InvestigationSession(
        QString sessionId,
        const QString &filePath,
        ImportProfile profile,
        ImportResult result,
        SourceFamilyConfiguration
            sourceFamilyConfiguration = {}
        );

    ~InvestigationSession();

    const QString &id() const;

    const QString &selectedRecordId() const;

    void setSelectedRecordId(
        const QString &recordId
        );

    const InvestigationSessionSourceMetadata &
    sourceMetadata() const;

    const InvestigationSessionBacking &
    backing() const;

    const InvestigationExternalSourceBinding *
    reconnectSourceHint() const;

    bool updateSnapshotBackingPath(
        const QString &snapshotPath
        );

    QString externalSourcePath() const;
    qint64 initialLiveFollowByteOffset() const;

    void setInitialLiveFollowByteOffset(
        qint64 byteOffset
        );

    const SourceFamilyConfiguration &
    sourceFamilyConfiguration() const;

    void setSourceFamilyConfiguration(
        SourceFamilyConfiguration configuration
        );

    const ImportProfile &importProfile() const;

    const QVector<ImportDiagnostic> &
    diagnostics() const;

    qint64 processedRecordCount() const;
    qint64 importedRecordCount() const;
    qint64 skippedRecordCount() const;

    bool sourceTruncated() const;

    InvestigationController *
    investigationController();

    const InvestigationController *
    investigationController() const;

    InvestigationStateStore *
    investigationStateStore();

    const InvestigationStateStore *
    investigationStateStore() const;

    void reload(
        ImportResult result
        );

    void appendLiveImportResult(
        ImportResult result
        );

    bool supportsLiveFollowing() const;

    LiveSessionFollowCoordinator *
    ensureLiveFollowCoordinator();

    LiveSessionFollowCoordinator *
    liveFollowCoordinator();

    const LiveSessionFollowCoordinator *
    liveFollowCoordinator() const;

    bool hasSeverityData() const;
    bool hasSubsystemData() const;

    const QStringList &
    availableSubsystems() const;

    bool hasEventCodeData() const;
    bool hasEntityData() const;

    const QStringList &
    availableEventCodes() const;

    bool hasCustomFieldData() const;

    const QStringList &
    availableCustomFields() const;

    const QStringList &
    availableEntities() const;

    const std::optional<QDateTime> &
    firstTimestamp() const;

    const std::optional<QDateTime> &
    lastTimestamp() const;

    const QVector<int> &
    columnWidths() const;

    InvestigationReviewTab reviewTab() const;

    InvestigationAnalyticsTab analyticsTab() const;

    void setReviewTab(
        InvestigationReviewTab tab
        );

    void setAnalyticsTab(
        InvestigationAnalyticsTab tab
        );

    void setColumnWidths(
        QVector<int> widths
        );

    InvestigationTimelineBreakdown
    timelineBreakdown() const;

    void setTimelineBreakdown(
        InvestigationTimelineBreakdown breakdown
        );

    int subsystemTrendLimit() const;

    void setSubsystemTrendLimit(
        int limit
        );

    InvestigationBurstTimingMode
    burstTimingMode() const;

    void setBurstTimingMode(
        InvestigationBurstTimingMode mode
        );

    const BurstDetectionSettings &
    burstDetectionSettings() const;

    void setBurstDetectionSettings(
        const BurstDetectionSettings &settings
        );

    InvestigationSessionSnapshot
    captureSnapshot() const;

    static std::unique_ptr<InvestigationSession>
    createSnapshotBacked(
        QString sessionId,
        const QString &snapshotPath,
        InvestigationSessionSnapshot snapshot,
        std::optional<
            InvestigationExternalSourceBinding>
            reconnectSourceHint = std::nullopt
        );

    static std::unique_ptr<InvestigationSession>
    createHybrid(
        QString sessionId,
        const QString &snapshotPath,
        InvestigationSessionSnapshot snapshot,
        InvestigationExternalSourceBinding
            externalSourceBinding
        );

    InvestigationSessionHybridReloadResult
    reloadHybrid(
        const ILogImporter &importer
        );

    void updateExternalSourceRuntimeState(
        quint64 sourceGeneration,
        const SourcePhysicalIdentity *sourceIdentity
        );

    bool applySnapshotReload(
        InvestigationSessionSnapshot snapshot
        );

    InvestigationSessionHybridReloadResult
    applyHybridReload(
        InvestigationSessionSnapshot snapshot,
        HybridInvestigationReconstructionResult
            reconstruction
        );

    InvestigationSessionHybridReloadResult
    applyHybridReconnect(
        InvestigationSessionSnapshot snapshot,
        HybridInvestigationReconstructionResult
            reconstruction
        );

    bool applySnapshotOnlyTransition(
        const QString &snapshotPath,
        InvestigationSnapshotSourceFidelity
            sourceFidelity
        );

private:
    QString m_id;

    QString m_selectedRecordId;

    InvestigationSessionSourceMetadata
        m_sourceMetadata;

    qint64 m_initialLiveFollowByteOffset = -1;

    InvestigationSessionBacking
        m_backing;

    std::optional<
        InvestigationExternalSourceBinding>
        m_reconnectSourceHint;

    ImportProfile m_importProfile;

    QVector<ImportDiagnostic>
        m_diagnostics;

    qint64 m_processedRecordCount = 0;

    bool m_sourceTruncated = false;

    InvestigationController
        m_investigationController;

    InvestigationStateStore
        m_investigationStateStore;

    bool m_hasSeverityData = false;
    bool m_hasSubsystemData = false;

    QStringList m_availableSubsystems;

    bool m_hasCustomFieldData = false;

    QStringList m_availableCustomFields;

    bool m_hasEventCodeData = false;
    bool m_hasEntityData = false;

    QStringList m_availableEventCodes;
    QStringList m_availableEntities;

    QSet<QString> m_recordIds;
    QSet<QString> m_knownSubsystems;
    QSet<QString> m_knownEventCodes;
    QSet<QString> m_knownEntities;
    QSet<QString> m_knownCustomFields;

    std::optional<QDateTime>
        m_firstTimestamp;

    std::optional<QDateTime>
        m_lastTimestamp;

    QVector<int> m_columnWidths;

    InvestigationReviewTab m_reviewTab =
        InvestigationReviewTab::IssueSummary;

    InvestigationAnalyticsTab m_analyticsTab =
        InvestigationAnalyticsTab::Overview;

    InvestigationTimelineBreakdown
        m_timelineBreakdown =
        InvestigationTimelineBreakdown::Severity;

    int m_subsystemTrendLimit = 5;

    InvestigationBurstTimingMode
        m_burstTimingMode =
        InvestigationBurstTimingMode::Auto;

    BurstDetectionSettings
        m_burstDetectionSettings;

    std::unique_ptr<LiveSessionFollowCoordinator>
        m_liveFollowCoordinator;

    InvestigationSession(
        QString sessionId,
        InvestigationSessionBacking backing,
        InvestigationSessionSourceMetadata
            sourceMetadata,
        ImportProfile profile,
        ImportResult result
        );

    void refreshSourceMetadata();

    void installImportResult(
        ImportResult result
        );

    void rebuildDerivedData(
        const QVector<InvestigationRecord> &records
        );

    void updateDerivedDataForAppendedRecords(
        const QVector<InvestigationRecord> &records
        );
};