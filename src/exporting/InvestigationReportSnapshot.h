#pragma once

#include <optional>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QtGlobal>

#include "../analysis/BurstDetectionSettings.h"
#include "../analysis/EventCountBucket.h"
#include "../analysis/InvestigationBurst.h"
#include "../analysis/InvestigationCadence.h"
#include "../analysis/InvestigationSessionComparison.h"
#include "../analysis/InvestigationValueFrequency.h"
#include "../analysis/InvestigationValueTrendBucket.h"
#include "../analysis/TelemetryIssueGroup.h"

#include "../domain/InvestigationRecord.h"
#include "../domain/InvestigationRecordState.h"

#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"

/*
 * ---------------------------------------------------------
 * Source/session context
 * ---------------------------------------------------------
 */

struct InvestigationReportSourceSnapshot
{
    QString sessionId;
    QString documentTitle;

    /*
     * Deliberately retain only shareable source
     * identity rather than the complete local
     * workstation path.
     */
    QString sourceName;

    qint64 sourceSizeBytes = 0;

    QDateTime sourceLastModified;
    QDateTime importedAtUtc;

    QString importProfileName;
    QString importerId;

    qint64 processedRecordCount = 0;
    qint64 importedRecordCount = 0;
    qint64 skippedRecordCount = 0;

    bool sourceTruncated = false;

    /*
     * Import diagnostics are part of investigation
     * provenance/data quality and remain available
     * regardless of whether the optional technical
     * appendix was requested.
     */
    QVector<ImportDiagnostic> diagnostics;

    /*
     * The complete mapping/configuration is retained
     * only when the report configuration requests
     * the technical appendix.
     */
    std::optional<ImportProfile>
        technicalImportProfile;
};

/*
 * ---------------------------------------------------------
 * Captured investigation scope
 * ---------------------------------------------------------
 */

struct InvestigationReportFilterSnapshot
{
    QStringList severities;
    QStringList subsystems;

    QString searchText;

    QStringList eventCodes;
    QStringList entityIds;

    std::optional<QDateTime> startTime;
    std::optional<QDateTime> endTime;

    QMap<QString, QStringList>
        customFieldFilters;

    QStringList findingStatuses;

    bool bookmarkedOnly = false;
};

struct InvestigationReportRecordContext
{
    qint64 totalRecordCount = 0;

    qint64 visibleRecordCount = 0;
    qint64 visibleTimestampedRecordCount = 0;

    std::optional<QDateTime>
        visibleFirstTimestamp;

    std::optional<QDateTime>
        visibleLastTimestamp;

    /*
     * Analytics deliberately ignores annotation-only
     * finding/bookmark filters, so its input population
     * may differ from the currently visible table.
     */
    qint64 analysisRecordCount = 0;
    qint64 analysisTimestampedRecordCount = 0;

    std::optional<QDateTime>
        analysisFirstTimestamp;

    std::optional<QDateTime>
        analysisLastTimestamp;
};

struct InvestigationReportCapabilities
{
    bool hasTimestampData = false;
    bool hasSeverityData = false;
    bool hasSubsystemData = false;
    bool hasEventCodeData = false;
    bool hasEntityData = false;
    bool hasCustomFieldData = false;
};

/*
 * ---------------------------------------------------------
 * Summary and deterministic analysis
 * ---------------------------------------------------------
 */

struct InvestigationReportSeveritySummary
{
    qint64 traceCount = 0;
    qint64 debugCount = 0;
    qint64 infoCount = 0;
    qint64 warningCount = 0;
    qint64 errorCount = 0;
    qint64 criticalCount = 0;

    /*
     * Records for which no canonical severity was
     * available remain part of the captured population.
     */
    qint64 unspecifiedCount = 0;

    qint64 totalCount() const
    {
        return traceCount
               + debugCount
               + infoCount
               + warningCount
               + errorCount
               + criticalCount
               + unspecifiedCount;
    }
};

struct InvestigationReportTimelineSnapshot
{
    bool available = false;

    qint64 intervalMilliseconds = 0;

    QVector<EventCountBucket> buckets;
};

struct InvestigationReportSubsystemTrendSnapshot
{
    bool available = false;

    qint64 intervalMilliseconds = 0;

    QVector<InvestigationValueTrendBucket>
        buckets;
};

enum class InvestigationReportBurstTimingMode
{
    Auto,
    Manual
};

struct InvestigationReportBurstSnapshot
{
    /*
     * Distinguish unsupported analysis from the
     * perfectly valid result of zero detected bursts.
     */
    bool available = false;

    InvestigationReportBurstTimingMode timingMode =
        InvestigationReportBurstTimingMode::Auto;

    BurstDetectionSettings configuredSettings;

    /*
     * In Auto mode the effective window/merge values
     * may differ from the configured values because
     * cadence analysis supplies recommendations.
     */
    BurstDetectionSettings effectiveSettings;

    InvestigationCadence cadence;

    QVector<InvestigationBurst> bursts;
};

/*
 * ---------------------------------------------------------
 * Investigator state and supporting evidence
 * ---------------------------------------------------------
 */

struct InvestigationReportEvidenceRecord
{
    /*
     * Immutable shareable copy of the investigation
     * record. Canonical fields, custom fields, source
     * filename/record number, and raw source content are
     * preserved. The originating workstation's local
     * source path is intentionally removed at capture.
     */
    InvestigationRecord record;

    /*
     * Empty for evidence that was included only because
     * it contributed to deterministic burst analysis.
     *
     * Non-empty state preserves bookmarks, notes, and
     * finding classifications.
     */
    InvestigationRecordState state;

    /*
     * A record may simultaneously contain investigator
     * state and contribute to a detected burst. It should
     * still appear only once in the evidence collection.
     */
    bool burstEvidence = false;
};

/*
 * ---------------------------------------------------------
 * Complete captured state for one source investigation
 * ---------------------------------------------------------
 */

struct InvestigationReportSessionSnapshot
{
    InvestigationReportSourceSnapshot source;

    InvestigationReportFilterSnapshot filters;

    InvestigationReportRecordContext
        recordContext;

    InvestigationReportCapabilities
        capabilities;

    /*
     * Visible summary reflects what the investigator
     * currently sees after all active filters.
     *
     * Analysis summary reflects the data-oriented
     * analytics population, which intentionally ignores
     * annotation-only bookmark/finding filters.
     */
    InvestigationReportSeveritySummary
        visibleSeveritySummary;

    InvestigationReportSeveritySummary
        analysisSeveritySummary;

    /*
     * The timeline represents the captured investigation
     * view rather than a transient widget viewport.
     */
    InvestigationReportTimelineSnapshot timeline;

    /*
     * Warning/Error/Critical grouping used by the
     * investigation issue summary.
     */
    QVector<TelemetryIssueGroup>
        elevatedIssueGroups;

    QVector<InvestigationValueFrequency>
        eventCodeFrequencies;

    QVector<InvestigationValueFrequency>
        entityFrequencies;

    QVector<InvestigationValueFrequency>
        subsystemFrequencies;

    InvestigationReportSubsystemTrendSnapshot
        subsystemTrends;

    InvestigationReportBurstSnapshot
        burstAnalysis;

    /*
     * Includes every stateful investigator record.
     *
     * When supporting evidence was requested, this may
     * additionally include burst-contributing records.
     */
    QVector<InvestigationReportEvidenceRecord>
        evidenceRecords;
};

/*
 * ---------------------------------------------------------
 * Captured comparison documents
 * ---------------------------------------------------------
 */

struct InvestigationReportComparisonSnapshot
{
    QString comparisonId;
    QString documentTitle;

    QString baselineSessionId;
    QString comparisonSessionId;

    /*
     * Preserve names even when the corresponding source
     * sessions are not themselves included as full report
     * sections.
     */
    QString baselineSourceName;
    QString comparisonSourceName;

    std::optional<BurstDetectionSettings>
        requestedBurstSettings;

    /*
     * Reuse the already-immutable Phase 12 comparison
     * result. Report generation must not recalculate it.
     */
    InvestigationSessionComparison analysis;
};

/*
 * ---------------------------------------------------------
 * Cross-source investigation context
 * ---------------------------------------------------------
 */

struct InvestigationReportSourceTimeCoverage
{
    QString sessionId;
    QString sourceName;

    qint64 timestampedRecordCount = 0;

    std::optional<QDateTime> firstTimestamp;
    std::optional<QDateTime> lastTimestamp;

    bool available() const
    {
        return firstTimestamp.has_value()
        && lastTimestamp.has_value();
    }
};

struct InvestigationReportChronologyEntry
{
    QString sessionId;
    QString sourceName;

    QString recordId;

    /*
     * Cross-source chronology contains only records with
     * canonical timestamps. Full record detail remains in
     * the owning session's evidence collection.
     */
    QDateTime timestamp;

    std::optional<RecordSeverity> severity;

    std::optional<QString> subsystem;
    std::optional<QString> eventCode;
    std::optional<QString> entityId;
    std::optional<QString> message;

    InvestigationRecordState state;

    bool burstEvidence = false;
};

/*
 * ---------------------------------------------------------
 * Root immutable report
 * ---------------------------------------------------------
 */

struct InvestigationReportSnapshot
{
    QString title;
    QString context;

    QDateTime generatedAtUtc;

    bool supportingEvidenceIncluded = true;
    bool technicalAppendixIncluded = true;

    /*
     * Order is deterministic and should follow the
     * resolved report/document selection order.
     */
    QVector<InvestigationReportSessionSnapshot>
        sessions;

    QVector<InvestigationReportComparisonSnapshot>
        comparisons;

    /*
     * Cross-source structures deliberately avoid
     * pretending heterogeneous sessions are analytically
     * comparable. They describe only shared chronology
     * and evidence relationships that remain meaningful
     * across source formats.
     */
    QVector<InvestigationReportSourceTimeCoverage>
        sourceTimeCoverage;

    /*
     * Already sorted by canonical timestamp at the
     * snapshot boundary. The HTML renderer should not
     * perform investigation analysis to construct it.
     *
     * The eventual report must explain that TraceScope
     * does not infer clock synchronization/skew between
     * independent sources.
     */
    QVector<InvestigationReportChronologyEntry>
        crossSourceChronology;
};