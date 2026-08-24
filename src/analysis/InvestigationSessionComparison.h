#pragma once

#include <optional>

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QtGlobal>
#include <QStringList>

#include "../domain/RecordSeverity.h"

struct InvestigationCountDifference
{
    qint64 baselineCount = 0;
    qint64 comparisonCount = 0;

    qint64 delta() const
    {
        return comparisonCount
               - baselineCount;
    }
};

struct InvestigationTimingSummary
{
    qint64 timestampedRecordCount = 0;

    std::optional<QDateTime> firstTimestamp;
    std::optional<QDateTime> lastTimestamp;

    qint64 durationMilliseconds = 0;

    std::optional<double> recordsPerMinute;

    bool available() const
    {
        return firstTimestamp.has_value()
        && lastTimestamp.has_value();
    }

    bool rateAvailable() const
    {
        return recordsPerMinute.has_value();
    }
};

struct InvestigationValueDifference
{
    QString value;

    qint64 baselineCount = 0;
    qint64 comparisonCount = 0;

    qint64 delta() const
    {
        return comparisonCount
               - baselineCount;
    }

    bool appearsOnlyInBaseline() const
    {
        return baselineCount > 0
               && comparisonCount == 0;
    }

    bool appearsOnlyInComparison() const
    {
        return baselineCount == 0
               && comparisonCount > 0;
    }
};

struct InvestigationDimensionComparison
{
    qint64 baselinePopulatedRecordCount = 0;
    qint64 comparisonPopulatedRecordCount = 0;

    QVector<InvestigationValueDifference>
        differences;

    bool comparable() const
    {
        return baselinePopulatedRecordCount > 0
               && comparisonPopulatedRecordCount > 0;
    }
};

struct InvestigationSeverityDifference
{
    RecordSeverity severity =
        RecordSeverity::Info;

    qint64 baselineCount = 0;
    qint64 comparisonCount = 0;

    qint64 delta() const
    {
        return comparisonCount
               - baselineCount;
    }
};

struct InvestigationSeverityComparison
{
    qint64 baselinePopulatedRecordCount = 0;
    qint64 comparisonPopulatedRecordCount = 0;

    QVector<InvestigationSeverityDifference>
        differences;

    bool comparable() const
    {
        return baselinePopulatedRecordCount > 0
               && comparisonPopulatedRecordCount > 0;
    }
};

struct InvestigationNumericFieldSummary
{
    qint64 populatedRecordCount = 0;

    double minimum = 0.0;
    double median = 0.0;
    double maximum = 0.0;
};

struct InvestigationNumericCustomFieldComparison
{
    QString fieldName;

    InvestigationNumericFieldSummary baseline;
    InvestigationNumericFieldSummary comparison;
};

struct InvestigationCategoricalCustomFieldComparison
{
    QString fieldName;

    /*
     * Only values that actually appear or disappear
     * are retained. Shared values whose occurrence
     * counts merely changed are intentionally omitted
     * to keep custom-field comparison focused on
     * meaningful context/configuration differences.
     */
    QVector<InvestigationValueDifference>
        changedValues;
};

struct InvestigationCustomFieldComparison
{
    QVector<InvestigationCategoricalCustomFieldComparison>
        categoricalFields;

    QVector<InvestigationNumericCustomFieldComparison>
        numericFields;
};

struct InvestigationSessionComparison
{
    InvestigationCountDifference totalRecords;

    InvestigationTimingSummary baselineTiming;
    InvestigationTimingSummary comparisonTiming;

    InvestigationSeverityComparison severity;

    /*
     * All event-code count changes, including
     * values that appear only in one session.
     */
    InvestigationDimensionComparison eventCodes;

    /*
     * Warning/Error/Critical records grouped by
     * subsystem. General subsystem traffic is
     * intentionally not the primary comparison.
     */
    InvestigationDimensionComparison
        elevatedSubsystems;

    /*
     * Warning/Error/Critical records grouped by
     * entity. This helps identify whether degraded
     * behavior is concentrated on particular
     * devices, requests, components, or other
     * source-defined entities.
     */
    InvestigationDimensionComparison
        elevatedEntities;

    /*
     * Shared custom fields are compared conservatively:
     *
     * - categorical fields surface appearing/disappearing
     *   values rather than raw frequency changes
     * - numeric fields surface compact distribution
     *   summaries
     * - unchanged or unsuitable fields are omitted
     */
    InvestigationCustomFieldComparison customFields;
};