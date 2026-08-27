#pragma once

#include <optional>

#include <QStringList>
#include <QDateTime>
#include <QString>
#include <QVector>

#include "InvestigationStateStore.h"

#include "../analysis/BurstDetectionSettings.h"
#include "../controllers/InvestigationController.h"
#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"

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

class InvestigationSession
{
public:
    InvestigationSession(
        const QString &filePath,
        ImportProfile profile,
        ImportResult result
        );

    const QString &id() const;

    const QString &selectedRecordId() const;

    void setSelectedRecordId(
        const QString &recordId
        );

    const InvestigationSessionSourceMetadata &
    sourceMetadata() const;

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

private:
    QString m_id;

    QString m_selectedRecordId;

    InvestigationSessionSourceMetadata
        m_sourceMetadata;

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

    void refreshSourceMetadata();

    void installImportResult(
        ImportResult result
        );

    void rebuildDerivedData(
        const QVector<InvestigationRecord> &records
        );
};