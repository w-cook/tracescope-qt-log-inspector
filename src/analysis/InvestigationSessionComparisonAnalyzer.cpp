#include "InvestigationSessionComparisonAnalyzer.h"

#include <QJsonValue>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

#include "InvestigationBurstAnalyzer.h"

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

constexpr int MaximumCategoricalCustomFieldValues = 20;

enum class CustomValueReadResult
{
    Missing,
    Scalar,
    Unsupported
};

CustomValueReadResult readCustomScalarValue(
    const QVariant &value,
    QString &text
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return CustomValueReadResult::Missing;
    }

    const QJsonValue jsonValue =
        QJsonValue::fromVariant(value);

    /*
     * Structured values are preserved by the
     * investigation model, but automatically
     * comparing arbitrary objects or arrays would
     * produce noisy and poorly defined results.
     */
    if (jsonValue.isObject()
        || jsonValue.isArray()) {
        return CustomValueReadResult::Unsupported;
    }

    text =
        value.toString().trimmed();

    if (text.isEmpty()) {
        return CustomValueReadResult::Missing;
    }

    return CustomValueReadResult::Scalar;
}

QVector<QString> customScalarValuesFor(
    const QVector<InvestigationRecord> &records,
    const QString &fieldName,
    bool &supported
    )
{
    supported = true;

    QVector<QString> values;

    for (const InvestigationRecord &record
         : records) {
        const auto iterator =
            record.customAttributes.constFind(
                fieldName
                );

        if (iterator
            == record.customAttributes.cend()) {
            continue;
        }

        QString text;

        const CustomValueReadResult readResult =
            readCustomScalarValue(
                iterator.value(),
                text
                );

        if (readResult
            == CustomValueReadResult::Unsupported) {
            supported = false;
            return {};
        }

        if (readResult
            == CustomValueReadResult::Missing) {
            continue;
        }

        values.append(text);
    }

    return values;
}

bool parseFiniteNumber(
    const QString &text,
    double &number
    )
{
    bool converted = false;

    const double parsed =
        text.toDouble(
            &converted
            );

    if (!converted
        || !std::isfinite(parsed)) {
        return false;
    }

    number = parsed;

    return true;
}

bool numericValuesFor(
    const QVector<QString> &values,
    QVector<double> &numbers
    )
{
    numbers.clear();
    numbers.reserve(values.size());

    for (const QString &value
         : values) {
        double number = 0.0;

        if (!parseFiniteNumber(
                value,
                number
                )) {
            numbers.clear();
            return false;
        }

        numbers.append(number);
    }

    return !numbers.isEmpty();
}

InvestigationNumericFieldSummary
numericSummaryFor(
    QVector<double> values
    )
{
    InvestigationNumericFieldSummary summary;

    if (values.isEmpty()) {
        return summary;
    }

    std::sort(
        values.begin(),
        values.end()
        );

    summary.populatedRecordCount =
        values.size();

    summary.minimum =
        values.first();

    summary.maximum =
        values.last();

    const qsizetype middle =
        values.size() / 2;

    if (values.size() % 2 != 0) {
        summary.median =
            values.at(middle);
    } else {
        summary.median =
            (
                values.at(middle - 1)
                + values.at(middle)
                )
            / 2.0;
    }

    return summary;
}

bool numericSummariesDiffer(
    const InvestigationNumericFieldSummary &baseline,
    const InvestigationNumericFieldSummary &comparison
    )
{
    /*
     * Populated-record count alone is not treated
     * as an investigation signal. The numeric
     * distribution itself must differ.
     */
    return baseline.minimum
               != comparison.minimum
           || baseline.median
                  != comparison.median
           || baseline.maximum
                  != comparison.maximum;
}

QMap<QString, qint64> categoricalCountsFor(
    const QVector<QString> &values
    )
{
    QMap<QString, qint64> counts;

    for (const QString &value
         : values) {
        ++counts[value];
    }

    return counts;
}

QSet<QString> customFieldNamesFor(
    const QVector<InvestigationRecord> &records
    )
{
    QSet<QString> names;

    for (const InvestigationRecord &record
         : records) {
        for (auto iterator =
             record.customAttributes.cbegin();
             iterator
             != record.customAttributes.cend();
             ++iterator) {
            if (!iterator.key()
                     .trimmed()
                     .isEmpty()) {
                names.insert(
                    iterator.key()
                    );
            }
        }
    }

    return names;
}

InvestigationCustomFieldComparison
customFieldComparisonFor(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords
    )
{
    InvestigationCustomFieldComparison result;

    const QSet<QString> baselineFieldNames =
        customFieldNamesFor(
            baselineRecords
            );

    const QSet<QString> comparisonFieldNames =
        customFieldNamesFor(
            comparisonRecords
            );

    QStringList sharedFieldNames;

    for (const QString &fieldName
         : baselineFieldNames) {
        if (comparisonFieldNames.contains(
                fieldName
                )) {
            sharedFieldNames.append(
                fieldName
                );
        }
    }

    /*
     * Match custom fields only by exact key.
     * Guessing that differently named fields have
     * the same meaning would make comparison less
     * reproducible.
     */
    std::sort(
        sharedFieldNames.begin(),
        sharedFieldNames.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            const int caseInsensitive =
                QString::compare(
                    left,
                    right,
                    Qt::CaseInsensitive
                    );

            if (caseInsensitive != 0) {
                return caseInsensitive < 0;
            }

            return QString::compare(
                       left,
                       right,
                       Qt::CaseSensitive
                       )
                   < 0;
        }
        );

    for (const QString &fieldName
         : std::as_const(sharedFieldNames)) {
        bool baselineSupported = true;
        bool comparisonSupported = true;

        const QVector<QString> baselineValues =
            customScalarValuesFor(
                baselineRecords,
                fieldName,
                baselineSupported
                );

        const QVector<QString> comparisonValues =
            customScalarValuesFor(
                comparisonRecords,
                fieldName,
                comparisonSupported
                );

        /*
         * A custom field must contain at least one
         * usable scalar value in both sessions.
         */
        if (!baselineSupported
            || !comparisonSupported
            || baselineValues.isEmpty()
            || comparisonValues.isEmpty()) {
            continue;
        }

        QVector<double> baselineNumbers;
        QVector<double> comparisonNumbers;

        const bool baselineNumeric =
            numericValuesFor(
                baselineValues,
                baselineNumbers
                );

        const bool comparisonNumeric =
            numericValuesFor(
                comparisonValues,
                comparisonNumbers
                );

        /*
         * Only classify a field as numeric when
         * every populated value on BOTH sides is a
         * finite number.
         */
        if (baselineNumeric
            && comparisonNumeric) {
            InvestigationNumericCustomFieldComparison
                comparison;

            comparison.fieldName =
                fieldName;

            comparison.baseline =
                numericSummaryFor(
                    baselineNumbers
                    );

            comparison.comparison =
                numericSummaryFor(
                    comparisonNumbers
                    );

            if (numericSummariesDiffer(
                    comparison.baseline,
                    comparison.comparison
                    )) {
                result.numericFields.append(
                    comparison
                    );
            }

            continue;
        }

        const QMap<QString, qint64>
            baselineCounts =
            categoricalCountsFor(
                baselineValues
                );

        const QMap<QString, qint64>
            comparisonCounts =
            categoricalCountsFor(
                comparisonValues
                );

        QSet<QString> distinctValues;

        for (auto iterator =
             baselineCounts.cbegin();
             iterator != baselineCounts.cend();
             ++iterator) {
            distinctValues.insert(
                iterator.key()
                );
        }

        for (auto iterator =
             comparisonCounts.cbegin();
             iterator != comparisonCounts.cend();
             ++iterator) {
            distinctValues.insert(
                iterator.key()
                );
        }

        /*
         * High-cardinality custom fields are more
         * likely to be identifiers or otherwise
         * produce comparison noise than useful
         * investigation context.
         */
        if (distinctValues.size()
            > MaximumCategoricalCustomFieldValues) {
            continue;
        }

        QStringList sortedValues =
            distinctValues.values();

        std::sort(
            sortedValues.begin(),
            sortedValues.end(),
            [](
                const QString &left,
                const QString &right
                ) {
                return QString::compare(
                           left,
                           right,
                           Qt::CaseSensitive
                           )
                       < 0;
            }
            );

        InvestigationCategoricalCustomFieldComparison
            comparison;

        comparison.fieldName =
            fieldName;

        for (const QString &value
             : std::as_const(sortedValues)) {
            const qint64 baselineCount =
                baselineCounts.value(
                    value,
                    0
                    );

            const qint64 comparisonCount =
                comparisonCounts.value(
                    value,
                    0
                    );

            /*
             * Frequency-only changes are deliberately
             * ignored for categorical custom fields.
             * Surface only values that genuinely
             * appeared or disappeared.
             */
            if ((baselineCount > 0
                 && comparisonCount > 0)
                || (baselineCount == 0
                    && comparisonCount == 0)) {
                continue;
            }

            InvestigationValueDifference
                difference;

            difference.value = value;
            difference.baselineCount =
                baselineCount;
            difference.comparisonCount =
                comparisonCount;

            comparison.changedValues.append(
                difference
                );
        }

        if (!comparison.changedValues.isEmpty()) {
            result.categoricalFields.append(
                comparison
                );
        }
    }

    return result;
}

bool burstAnalysisAvailableFor(
    const QVector<InvestigationRecord> &records
    )
{
    for (const InvestigationRecord &record
         : records) {
        if (record.timestamp.has_value()
            && record.timestamp->isValid()
            && record.severity.has_value()) {
            return true;
        }
    }

    return false;
}

template<typename CountsSelector>
std::optional<InvestigationValueFrequency>
dominantBurstValue(
    const QVector<InvestigationBurst> &bursts,
    CountsSelector selector
    )
{
    QMap<QString, int> combinedCounts;

    for (const InvestigationBurst &burst
         : bursts) {
        const QMap<QString, int> &counts =
            selector(burst);

        for (auto iterator =
             counts.cbegin();
             iterator != counts.cend();
             ++iterator) {
            combinedCounts[iterator.key()] +=
                iterator.value();
        }
    }

    if (combinedCounts.isEmpty()) {
        return std::nullopt;
    }

    QString dominantValue;
    int dominantCount = -1;

    /*
     * QMap iteration is case-sensitive key order.
     * Updating only for a strictly larger count
     * makes that ordering the deterministic
     * tie-breaker.
     */
    for (auto iterator =
         combinedCounts.cbegin();
         iterator != combinedCounts.cend();
         ++iterator) {
        if (iterator.value()
            <= dominantCount) {
            continue;
        }

        dominantValue =
            iterator.key();

        dominantCount =
            iterator.value();
    }

    InvestigationValueFrequency result;

    result.value =
        dominantValue;

    result.count =
        dominantCount;

    return result;
}

InvestigationBurstSessionSummary
burstSummaryFor(
    const QVector<InvestigationRecord> &records,
    const BurstDetectionSettings &settings
    )
{
    InvestigationBurstSessionSummary summary;

    summary.available =
        burstAnalysisAvailableFor(
            records
            );

    /*
     * A session with usable timestamp/severity
     * information remains available even if no
     * burst is detected.
     */
    if (!summary.available) {
        return summary;
    }

    InvestigationBurstAnalyzer analyzer;

    const QVector<InvestigationBurst> bursts =
        analyzer.detectBursts(
            records,
            settings
            );

    summary.burstCount =
        bursts.size();

    for (const InvestigationBurst &burst
         : bursts) {
        const int elevatedCount =
            burst.totalElevatedCount();

        summary.elevatedRecordCountInBursts +=
            elevatedCount;

        summary.peakBurstElevatedCount =
            std::max(
                summary.peakBurstElevatedCount,
                elevatedCount
                );

        summary.longestBurstDurationMilliseconds =
            std::max(
                summary.longestBurstDurationMilliseconds,
                burst.durationMilliseconds()
                );
    }

    summary.dominantSubsystem =
        dominantBurstValue(
            bursts,
            [](
                const InvestigationBurst &burst
                ) -> const QMap<QString, int> & {
                return burst.subsystemCounts;
            }
            );

    summary.dominantEventCode =
        dominantBurstValue(
            bursts,
            [](
                const InvestigationBurst &burst
                ) -> const QMap<QString, int> & {
                return burst.eventCodeCounts;
            }
            );

    summary.dominantEntity =
        dominantBurstValue(
            bursts,
            [](
                const InvestigationBurst &burst
                ) -> const QMap<QString, int> & {
                return burst.entityCounts;
            }
            );

    return summary;
}

InvestigationBurstComparison
burstComparisonFor(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords,
    const BurstDetectionSettings &settings
    )
{
    InvestigationBurstComparison result;

    result.settings =
        settings;

    result.baseline =
        burstSummaryFor(
            baselineRecords,
            settings
            );

    result.comparison =
        burstSummaryFor(
            comparisonRecords,
            settings
            );

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

    result.customFields =
        customFieldComparisonFor(
            baselineRecords,
            comparisonRecords
            );

    return result;
}

InvestigationSessionComparison
InvestigationSessionComparisonAnalyzer::compare(
    const QVector<InvestigationRecord> &baselineRecords,
    const QVector<InvestigationRecord> &comparisonRecords,
    const BurstDetectionSettings &burstSettings
    ) const
{
    InvestigationSessionComparison result =
        compare(
            baselineRecords,
            comparisonRecords
            );

    /*
     * Invalid shared settings mean burst comparison
     * was not performed. The canonical comparison
     * remains completely usable.
     */
    if (!burstSettings.isValid()) {
        return result;
    }

    result.bursts =
        burstComparisonFor(
            baselineRecords,
            comparisonRecords,
            burstSettings
            );

    return result;
}