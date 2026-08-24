#include "InvestigationSessionComparisonAnalyzer.h"

#include <QMap>

#include <array>
#include <optional>

namespace
{
constexpr std::array<RecordSeverity, 6>
    OrderedSeverities = {
        RecordSeverity::Trace,
        RecordSeverity::Debug,
        RecordSeverity::Info,
        RecordSeverity::Warning,
        RecordSeverity::Error,
        RecordSeverity::Critical
};

bool hasValue(
    const std::optional<QString> &value
    )
{
    return value.has_value()
    && !value->trimmed().isEmpty();
}

bool isElevated(
    const std::optional<RecordSeverity> &severity
    )
{
    if (!severity.has_value()) {
        return false;
    }

    return severity.value()
               == RecordSeverity::Warning
           || severity.value()
                  == RecordSeverity::Error
           || severity.value()
                  == RecordSeverity::Critical;
}

InvestigationTimingSummary timingSummaryFor(
    const QVector<InvestigationRecord> &records
    )
{
    InvestigationTimingSummary summary;

    for (const InvestigationRecord &record
         : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        ++summary.timestampedRecordCount;

        if (!summary.firstTimestamp.has_value()
            || record.timestamp.value()
                   < summary.firstTimestamp.value()) {
            summary.firstTimestamp =
                record.timestamp.value();
        }

        if (!summary.lastTimestamp.has_value()
            || record.timestamp.value()
                   > summary.lastTimestamp.value()) {
            summary.lastTimestamp =
                record.timestamp.value();
        }
    }

    if (!summary.firstTimestamp.has_value()
        || !summary.lastTimestamp.has_value()) {
        return summary;
    }

    summary.durationMilliseconds =
        summary.firstTimestamp->msecsTo(
            summary.lastTimestamp.value()
            );

    /*
     * A rate requires a measurable positive
     * timestamp span. One timestamp, or multiple
     * records sharing exactly one timestamp,
     * provides no meaningful records-per-minute
     * denominator.
     */
    if (summary.durationMilliseconds > 0) {
        summary.recordsPerMinute =
            static_cast<double>(
                summary.timestampedRecordCount
                )
            * 60'000.0
            / static_cast<double>(
                summary.durationMilliseconds
                );
    }

    return summary;
}

InvestigationSeverityComparison severityComparisonFor(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords
    )
{
    InvestigationSeverityComparison result;

    QMap<RecordSeverity, qint64>
        baselineCounts;

    QMap<RecordSeverity, qint64>
        comparisonCounts;

    for (const InvestigationRecord &record
         : baselineRecords) {
        if (!record.severity.has_value()) {
            continue;
        }

        ++result.baselinePopulatedRecordCount;

        ++baselineCounts[
            record.severity.value()
        ];
    }

    for (const InvestigationRecord &record
         : comparisonRecords) {
        if (!record.severity.has_value()) {
            continue;
        }

        ++result.comparisonPopulatedRecordCount;

        ++comparisonCounts[
            record.severity.value()
        ];
    }

    /*
     * Do not manufacture zero-valued comparisons
     * when either session lacks severity data.
     */
    if (!result.comparable()) {
        return result;
    }

    for (const RecordSeverity severity
         : OrderedSeverities) {
        const qint64 baselineCount =
            baselineCounts.value(
                severity,
                0
                );

        const qint64 comparisonCount =
            comparisonCounts.value(
                severity,
                0
                );

        if (baselineCount == comparisonCount) {
            continue;
        }

        InvestigationSeverityDifference
            difference;

        difference.severity = severity;
        difference.baselineCount =
            baselineCount;
        difference.comparisonCount =
            comparisonCount;

        result.differences.append(
            difference
            );
    }

    return result;
}

template<typename ValueSelector>
qint64 populatedValueCount(
    const QVector<InvestigationRecord> &records,
    ValueSelector selector,
    bool requireSeverity
    )
{
    qint64 count = 0;

    for (const InvestigationRecord &record
         : records) {
        if (!hasValue(selector(record))) {
            continue;
        }

        /*
         * Elevated subsystem/entity comparison
         * requires severity to be populated so the
         * record can be classified as elevated or
         * non-elevated.
         *
         * Importantly, the severity does not need
         * to BE elevated. A healthy baseline with
         * zero elevated records is still fully
         * comparable.
         */
        if (requireSeverity
            && !record.severity.has_value()) {
            continue;
        }

        ++count;
    }

    return count;
}

template<typename ValueSelector>
QMap<QString, qint64> valueCounts(
    const QVector<InvestigationRecord> &records,
    ValueSelector selector,
    bool elevatedOnly
    )
{
    QMap<QString, qint64> counts;

    for (const InvestigationRecord &record
         : records) {
        if (elevatedOnly
            && !isElevated(record.severity)) {
            continue;
        }

        const std::optional<QString> &value =
            selector(record);

        if (!hasValue(value)) {
            continue;
        }

        ++counts[value.value()];
    }

    return counts;
}

template<typename ValueSelector>
InvestigationDimensionComparison
dimensionComparisonFor(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords,
    ValueSelector selector,
    bool elevatedOnly
    )
{
    InvestigationDimensionComparison result;

    result.baselinePopulatedRecordCount =
        populatedValueCount(
            baselineRecords,
            selector,
            elevatedOnly
            );

    result.comparisonPopulatedRecordCount =
        populatedValueCount(
            comparisonRecords,
            selector,
            elevatedOnly
            );

    /*
     * One session having no usable data for this
     * dimension means the dimension is unavailable,
     * not that every value has a zero count there.
     */
    if (!result.comparable()) {
        return result;
    }

    const QMap<QString, qint64>
        baselineCounts =
        valueCounts(
            baselineRecords,
            selector,
            elevatedOnly
            );

    const QMap<QString, qint64>
        comparisonCounts =
        valueCounts(
            comparisonRecords,
            selector,
            elevatedOnly
            );

    /*
     * QMap gives us deterministic case-sensitive
     * value ordering without introducing UI-specific
     * ranking into the analysis layer.
     */
    QMap<QString, bool> allValues;

    for (auto iterator =
         baselineCounts.cbegin();
         iterator != baselineCounts.cend();
         ++iterator) {
        allValues.insert(
            iterator.key(),
            true
            );
    }

    for (auto iterator =
         comparisonCounts.cbegin();
         iterator != comparisonCounts.cend();
         ++iterator) {
        allValues.insert(
            iterator.key(),
            true
            );
    }

    for (auto iterator =
         allValues.cbegin();
         iterator != allValues.cend();
         ++iterator) {
        const qint64 baselineCount =
            baselineCounts.value(
                iterator.key(),
                0
                );

        const qint64 comparisonCount =
            comparisonCounts.value(
                iterator.key(),
                0
                );

        /*
         * Unchanged values provide no comparison
         * signal and are deliberately omitted.
         */
        if (baselineCount == comparisonCount) {
            continue;
        }

        InvestigationValueDifference
            difference;

        difference.value =
            iterator.key();

        difference.baselineCount =
            baselineCount;

        difference.comparisonCount =
            comparisonCount;

        result.differences.append(
            difference
            );
    }

    return result;
}
}

InvestigationSessionComparison
InvestigationSessionComparisonAnalyzer::compare(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords
    ) const
{
    InvestigationSessionComparison result;

    result.totalRecords.baselineCount =
        baselineRecords.size();

    result.totalRecords.comparisonCount =
        comparisonRecords.size();

    result.baselineTiming =
        timingSummaryFor(
            baselineRecords
            );

    result.comparisonTiming =
        timingSummaryFor(
            comparisonRecords
            );

    result.severity =
        severityComparisonFor(
            baselineRecords,
            comparisonRecords
            );

    result.eventCodes =
        dimensionComparisonFor(
            baselineRecords,
            comparisonRecords,
            [](
                const InvestigationRecord &record
                ) -> const std::optional<QString> & {
                return record.eventCode;
            },
            false
            );

    result.elevatedSubsystems =
        dimensionComparisonFor(
            baselineRecords,
            comparisonRecords,
            [](
                const InvestigationRecord &record
                ) -> const std::optional<QString> & {
                return record.subsystem;
            },
            true
            );

    result.elevatedEntities =
        dimensionComparisonFor(
            baselineRecords,
            comparisonRecords,
            [](
                const InvestigationRecord &record
                ) -> const std::optional<QString> & {
                return record.entityId;
            },
            true
            );

    return result;
}