#include "InvestigationComparisonPersistenceSerialization.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QJsonArray>
#include <QJsonValue>

#include "../domain/RecordSeverity.h"

namespace
{
constexpr double MaxExactJsonInteger =
    9007199254740991.0;

ComparisonPersistenceDeserializationResult
failure(
    const QString &message
    )
{
    ComparisonPersistenceDeserializationResult
        result;

    result.errorCode =
        QStringLiteral(
            "INVALID_COMPARISON"
            );

    result.errorMessage = message;

    return result;
}

QJsonValue integerToJson(
    qint64 value
    )
{
    return QJsonValue(
        static_cast<double>(value)
        );
}

bool readInteger(
    const QJsonObject &object,
    const QString &key,
    qint64 &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || std::abs(number)
               > MaxExactJsonInteger) {
        return false;
    }

    result =
        static_cast<qint64>(number);

    return true;
}

bool readInteger(
    const QJsonObject &object,
    const QString &key,
    int &result
    )
{
    qint64 value = 0;

    if (!readInteger(
            object,
            key,
            value
            )
        ||
        value <
            std::numeric_limits<int>::min()
        ||
        value >
            std::numeric_limits<int>::max()) {
        return false;
    }

    result =
        static_cast<int>(value);

    return true;
}

bool readDouble(
    const QJsonObject &object,
    const QString &key,
    double &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isDouble()) {
        return false;
    }

    result = value.toDouble();

    return std::isfinite(result);
}

bool readString(
    const QJsonObject &object,
    const QString &key,
    QString &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isString()) {
        return false;
    }

    result = value.toString();

    return true;
}

QJsonValue dateTimeToJson(
    const QDateTime &value
    )
{
    if (!value.isValid()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    return value.toUTC().toString(
        Qt::ISODateWithMs
        );
}

bool readDateTime(
    const QJsonValue &value,
    QDateTime &result
    )
{
    if (value.isNull()) {
        result = QDateTime();
        return true;
    }

    if (!value.isString()) {
        return false;
    }

    const QDateTime parsed =
        QDateTime::fromString(
            value.toString(),
            Qt::ISODateWithMs
            );

    if (!parsed.isValid()) {
        return false;
    }

    result = parsed;

    return true;
}

QJsonValue optionalDoubleToJson(
    const std::optional<double> &value
    )
{
    if (!value.has_value()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    return *value;
}

bool readOptionalDouble(
    const QJsonValue &value,
    std::optional<double> &result
    )
{
    if (value.isNull()) {
        result.reset();
        return true;
    }

    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)) {
        return false;
    }

    result = number;

    return true;
}

QJsonObject burstSettingsToJson(
    const BurstDetectionSettings &settings
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("windowMilliseconds"),
        integerToJson(
            settings.windowMilliseconds
            )
        );

    object.insert(
        QStringLiteral(
            "elevatedEventThreshold"
            ),
        settings.elevatedEventThreshold
        );

    object.insert(
        QStringLiteral(
            "errorCriticalThreshold"
            ),
        settings.errorCriticalThreshold
        );

    object.insert(
        QStringLiteral(
            "mergeGapMilliseconds"
            ),
        integerToJson(
            settings.mergeGapMilliseconds
            )
        );

    return object;
}

bool burstSettingsFromJson(
    const QJsonValue &value,
    BurstDetectionSettings &settings
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return readInteger(
               object,
               QStringLiteral(
                   "windowMilliseconds"
                   ),
               settings.windowMilliseconds
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "elevatedEventThreshold"
                   ),
               settings.elevatedEventThreshold
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "errorCriticalThreshold"
                   ),
               settings.errorCriticalThreshold
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "mergeGapMilliseconds"
                   ),
               settings.mergeGapMilliseconds
               )
           &&
           settings.isValid();
}

QJsonObject valueDifferenceToJson(
    const InvestigationValueDifference
        &difference
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("value"),
        difference.value
        );

    object.insert(
        QStringLiteral("baselineCount"),
        integerToJson(
            difference.baselineCount
            )
        );

    object.insert(
        QStringLiteral("comparisonCount"),
        integerToJson(
            difference.comparisonCount
            )
        );

    return object;
}

bool valueDifferenceFromJson(
    const QJsonValue &value,
    InvestigationValueDifference
        &difference
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return readString(
               object,
               QStringLiteral("value"),
               difference.value
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "baselineCount"
                   ),
               difference.baselineCount
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "comparisonCount"
                   ),
               difference.comparisonCount
               );
}

QJsonArray valueDifferencesToJson(
    const QVector<InvestigationValueDifference>
        &differences
    )
{
    QJsonArray array;

    for (const auto &difference
         : differences) {
        array.append(
            valueDifferenceToJson(
                difference
                )
            );
    }

    return array;
}

bool valueDifferencesFromJson(
    const QJsonValue &value,
    QVector<InvestigationValueDifference>
        &differences
    )
{
    if (!value.isArray()) {
        return false;
    }

    for (const QJsonValue &item
         : value.toArray()) {
        InvestigationValueDifference
            difference;

        if (!valueDifferenceFromJson(
                item,
                difference
                )) {
            return false;
        }

        differences.append(
            std::move(difference)
            );
    }

    return true;
}

QJsonObject dimensionToJson(
    const InvestigationDimensionComparison
        &dimension
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "baselinePopulatedRecordCount"
            ),
        integerToJson(
            dimension
                .baselinePopulatedRecordCount
            )
        );

    object.insert(
        QStringLiteral(
            "comparisonPopulatedRecordCount"
            ),
        integerToJson(
            dimension
                .comparisonPopulatedRecordCount
            )
        );

    object.insert(
        QStringLiteral("differences"),
        valueDifferencesToJson(
            dimension.differences
            )
        );

    return object;
}

bool dimensionFromJson(
    const QJsonValue &value,
    InvestigationDimensionComparison
        &dimension
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return readInteger(
               object,
               QStringLiteral(
                   "baselinePopulatedRecordCount"
                   ),
               dimension
                   .baselinePopulatedRecordCount
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "comparisonPopulatedRecordCount"
                   ),
               dimension
                   .comparisonPopulatedRecordCount
               )
           &&
           valueDifferencesFromJson(
               object.value(
                   QStringLiteral(
                       "differences"
                       )
                   ),
               dimension.differences
               );
}

QJsonObject timingToJson(
    const InvestigationTimingSummary &timing
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "timestampedRecordCount"
            ),
        integerToJson(
            timing.timestampedRecordCount
            )
        );

    object.insert(
        QStringLiteral("firstTimestamp"),
        timing.firstTimestamp.has_value()
            ? dateTimeToJson(
                  *timing.firstTimestamp
                  )
            : QJsonValue(
                  QJsonValue::Null
                  )
        );

    object.insert(
        QStringLiteral("lastTimestamp"),
        timing.lastTimestamp.has_value()
            ? dateTimeToJson(
                  *timing.lastTimestamp
                  )
            : QJsonValue(
                  QJsonValue::Null
                  )
        );

    object.insert(
        QStringLiteral(
            "durationMilliseconds"
            ),
        integerToJson(
            timing.durationMilliseconds
            )
        );

    object.insert(
        QStringLiteral("recordsPerMinute"),
        optionalDoubleToJson(
            timing.recordsPerMinute
            )
        );

    return object;
}

bool timingFromJson(
    const QJsonValue &value,
    InvestigationTimingSummary &timing
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    QDateTime firstTimestamp;
    QDateTime lastTimestamp;

    if (!readInteger(
            object,
            QStringLiteral(
                "timestampedRecordCount"
                ),
            timing.timestampedRecordCount
            )
        ||
        !readInteger(
            object,
            QStringLiteral(
                "durationMilliseconds"
                ),
            timing.durationMilliseconds
            )
        ||
        !readDateTime(
            object.value(
                QStringLiteral(
                    "firstTimestamp"
                    )
                ),
            firstTimestamp
            )
        ||
        !readDateTime(
            object.value(
                QStringLiteral(
                    "lastTimestamp"
                    )
                ),
            lastTimestamp
            )
        ||
        !readOptionalDouble(
            object.value(
                QStringLiteral(
                    "recordsPerMinute"
                    )
                ),
            timing.recordsPerMinute
            )) {
        return false;
    }

    if (firstTimestamp.isValid()) {
        timing.firstTimestamp =
            firstTimestamp;
    }

    if (lastTimestamp.isValid()) {
        timing.lastTimestamp =
            lastTimestamp;
    }

    return true;
}

QJsonObject severityToJson(
    const InvestigationSeverityComparison
        &severity
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "baselinePopulatedRecordCount"
            ),
        integerToJson(
            severity
                .baselinePopulatedRecordCount
            )
        );

    object.insert(
        QStringLiteral(
            "comparisonPopulatedRecordCount"
            ),
        integerToJson(
            severity
                .comparisonPopulatedRecordCount
            )
        );

    QJsonArray differences;

    for (const auto &difference
         : severity.differences) {
        QJsonObject item;

        item.insert(
            QStringLiteral("severity"),
            recordSeverityToString(
                difference.severity
                )
            );

        item.insert(
            QStringLiteral("baselineCount"),
            integerToJson(
                difference.baselineCount
                )
            );

        item.insert(
            QStringLiteral(
                "comparisonCount"
                ),
            integerToJson(
                difference.comparisonCount
                )
            );

        differences.append(item);
    }

    object.insert(
        QStringLiteral("differences"),
        differences
        );

    return object;
}

bool severityFromJson(
    const QJsonValue &value,
    InvestigationSeverityComparison
        &severity
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    if (!readInteger(
            object,
            QStringLiteral(
                "baselinePopulatedRecordCount"
                ),
            severity
                .baselinePopulatedRecordCount
            )
        ||
        !readInteger(
            object,
            QStringLiteral(
                "comparisonPopulatedRecordCount"
                ),
            severity
                .comparisonPopulatedRecordCount
            )) {
        return false;
    }

    const QJsonValue differencesValue =
        object.value(
            QStringLiteral("differences")
            );

    if (!differencesValue.isArray()) {
        return false;
    }

    for (const QJsonValue &itemValue
         : differencesValue.toArray()) {
        if (!itemValue.isObject()) {
            return false;
        }

        const QJsonObject item =
            itemValue.toObject();

        QString severityText;

        InvestigationSeverityDifference
            difference;

        if (!readString(
                item,
                QStringLiteral("severity"),
                severityText
                )
            ||
            !readInteger(
                item,
                QStringLiteral(
                    "baselineCount"
                    ),
                difference.baselineCount
                )
            ||
            !readInteger(
                item,
                QStringLiteral(
                    "comparisonCount"
                    ),
                difference.comparisonCount
                )) {
            return false;
        }

        const auto parsedSeverity =
            parseRecordSeverity(
                severityText
                );

        if (!parsedSeverity.has_value()) {
            return false;
        }

        difference.severity =
            *parsedSeverity;

        severity.differences.append(
            difference
            );
    }

    return true;
}

QJsonObject numericSummaryToJson(
    const InvestigationNumericFieldSummary
        &summary
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "populatedRecordCount"
            ),
        integerToJson(
            summary.populatedRecordCount
            )
        );

    object.insert(
        QStringLiteral("minimum"),
        summary.minimum
        );

    object.insert(
        QStringLiteral("median"),
        summary.median
        );

    object.insert(
        QStringLiteral("maximum"),
        summary.maximum
        );

    return object;
}

bool numericSummaryFromJson(
    const QJsonValue &value,
    InvestigationNumericFieldSummary
        &summary
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return readInteger(
               object,
               QStringLiteral(
                   "populatedRecordCount"
                   ),
               summary.populatedRecordCount
               )
           &&
           readDouble(
               object,
               QStringLiteral("minimum"),
               summary.minimum
               )
           &&
           readDouble(
               object,
               QStringLiteral("median"),
               summary.median
               )
           &&
           readDouble(
               object,
               QStringLiteral("maximum"),
               summary.maximum
               );
}

QJsonObject customFieldsToJson(
    const InvestigationCustomFieldComparison
        &customFields
    )
{
    QJsonObject object;

    QJsonArray categorical;

    for (const auto &field
         : customFields.categoricalFields) {
        QJsonObject item;

        item.insert(
            QStringLiteral("fieldName"),
            field.fieldName
            );

        item.insert(
            QStringLiteral("changedValues"),
            valueDifferencesToJson(
                field.changedValues
                )
            );

        categorical.append(item);
    }

    object.insert(
        QStringLiteral("categoricalFields"),
        categorical
        );

    QJsonArray numeric;

    for (const auto &field
         : customFields.numericFields) {
        QJsonObject item;

        item.insert(
            QStringLiteral("fieldName"),
            field.fieldName
            );

        item.insert(
            QStringLiteral("baseline"),
            numericSummaryToJson(
                field.baseline
                )
            );

        item.insert(
            QStringLiteral("comparison"),
            numericSummaryToJson(
                field.comparison
                )
            );

        numeric.append(item);
    }

    object.insert(
        QStringLiteral("numericFields"),
        numeric
        );

    return object;
}

bool customFieldsFromJson(
    const QJsonValue &value,
    InvestigationCustomFieldComparison
        &customFields
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    const QJsonValue categoricalValue =
        object.value(
            QStringLiteral(
                "categoricalFields"
                )
            );

    const QJsonValue numericValue =
        object.value(
            QStringLiteral(
                "numericFields"
                )
            );

    if (!categoricalValue.isArray()
        || !numericValue.isArray()) {
        return false;
    }

    for (const QJsonValue &itemValue
         : categoricalValue.toArray()) {
        if (!itemValue.isObject()) {
            return false;
        }

        const QJsonObject item =
            itemValue.toObject();

        InvestigationCategoricalCustomFieldComparison
            field;

        if (!readString(
                item,
                QStringLiteral("fieldName"),
                field.fieldName
                )
            ||
            !valueDifferencesFromJson(
                item.value(
                    QStringLiteral(
                        "changedValues"
                        )
                    ),
                field.changedValues
                )) {
            return false;
        }

        customFields.categoricalFields.append(
            std::move(field)
            );
    }

    for (const QJsonValue &itemValue
         : numericValue.toArray()) {
        if (!itemValue.isObject()) {
            return false;
        }

        const QJsonObject item =
            itemValue.toObject();

        InvestigationNumericCustomFieldComparison
            field;

        if (!readString(
                item,
                QStringLiteral("fieldName"),
                field.fieldName
                )
            ||
            !numericSummaryFromJson(
                item.value(
                    QStringLiteral("baseline")
                    ),
                field.baseline
                )
            ||
            !numericSummaryFromJson(
                item.value(
                    QStringLiteral("comparison")
                    ),
                field.comparison
                )) {
            return false;
        }

        customFields.numericFields.append(
            std::move(field)
            );
    }

    return true;
}

QJsonValue frequencyToJson(
    const std::optional<
        InvestigationValueFrequency>
        &frequency
    )
{
    if (!frequency.has_value()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    QJsonObject object;

    object.insert(
        QStringLiteral("value"),
        frequency->value
        );

    object.insert(
        QStringLiteral("count"),
        frequency->count
        );

    return object;
}

bool frequencyFromJson(
    const QJsonValue &value,
    std::optional<
        InvestigationValueFrequency>
        &frequency
    )
{
    if (value.isNull()) {
        frequency.reset();
        return true;
    }

    if (!value.isObject()) {
        return false;
    }

    InvestigationValueFrequency restored;

    const QJsonObject object =
        value.toObject();

    if (!readString(
            object,
            QStringLiteral("value"),
            restored.value
            )
        ||
        !readInteger(
            object,
            QStringLiteral("count"),
            restored.count
            )) {
        return false;
    }

    frequency = restored;

    return true;
}

QJsonObject burstSummaryToJson(
    const InvestigationBurstSessionSummary
        &summary
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("available"),
        summary.available
        );

    object.insert(
        QStringLiteral("burstCount"),
        summary.burstCount
        );

    object.insert(
        QStringLiteral(
            "elevatedRecordCountInBursts"
            ),
        summary.elevatedRecordCountInBursts
        );

    object.insert(
        QStringLiteral(
            "peakBurstElevatedCount"
            ),
        summary.peakBurstElevatedCount
        );

    object.insert(
        QStringLiteral(
            "longestBurstDurationMilliseconds"
            ),
        integerToJson(
            summary
                .longestBurstDurationMilliseconds
            )
        );

    object.insert(
        QStringLiteral(
            "dominantSubsystem"
            ),
        frequencyToJson(
            summary.dominantSubsystem
            )
        );

    object.insert(
        QStringLiteral(
            "dominantEventCode"
            ),
        frequencyToJson(
            summary.dominantEventCode
            )
        );

    object.insert(
        QStringLiteral("dominantEntity"),
        frequencyToJson(
            summary.dominantEntity
            )
        );

    return object;
}

bool burstSummaryFromJson(
    const QJsonValue &value,
    InvestigationBurstSessionSummary
        &summary
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    const QJsonValue availableValue =
        object.value(
            QStringLiteral("available")
            );

    if (!availableValue.isBool()) {
        return false;
    }

    summary.available =
        availableValue.toBool();

    return readInteger(
               object,
               QStringLiteral("burstCount"),
               summary.burstCount
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "elevatedRecordCountInBursts"
                   ),
               summary
                   .elevatedRecordCountInBursts
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "peakBurstElevatedCount"
                   ),
               summary
                   .peakBurstElevatedCount
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "longestBurstDurationMilliseconds"
                   ),
               summary
                   .longestBurstDurationMilliseconds
               )
           &&
           frequencyFromJson(
               object.value(
                   QStringLiteral(
                       "dominantSubsystem"
                       )
                   ),
               summary.dominantSubsystem
               )
           &&
           frequencyFromJson(
               object.value(
                   QStringLiteral(
                       "dominantEventCode"
                       )
                   ),
               summary.dominantEventCode
               )
           &&
           frequencyFromJson(
               object.value(
                   QStringLiteral(
                       "dominantEntity"
                       )
                   ),
               summary.dominantEntity
               );
}

QJsonObject sourceToJson(
    const PersistedInvestigationComparisonSource
        &source
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("sessionId"),
        source.sessionId
        );

    object.insert(
        QStringLiteral("sourcePath"),
        source.sourcePath
        );

    object.insert(
        QStringLiteral("sourceName"),
        source.sourceName
        );

    object.insert(
        QStringLiteral("sourceSizeBytes"),
        integerToJson(
            source.sourceSizeBytes
            )
        );

    object.insert(
        QStringLiteral(
            "sourceLastModified"
            ),
        dateTimeToJson(
            source.sourceLastModified
            )
        );

    object.insert(
        QStringLiteral("importedAtUtc"),
        dateTimeToJson(
            source.importedAtUtc
            )
        );

    return object;
}

bool sourceFromJson(
    const QJsonValue &value,
    PersistedInvestigationComparisonSource
        &source
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return readString(
               object,
               QStringLiteral("sessionId"),
               source.sessionId
               )
           &&
           readString(
               object,
               QStringLiteral("sourcePath"),
               source.sourcePath
               )
           &&
           readString(
               object,
               QStringLiteral("sourceName"),
               source.sourceName
               )
           &&
           readInteger(
               object,
               QStringLiteral(
                   "sourceSizeBytes"
                   ),
               source.sourceSizeBytes
               )
           &&
           readDateTime(
               object.value(
                   QStringLiteral(
                       "sourceLastModified"
                       )
                   ),
               source.sourceLastModified
               )
           &&
           readDateTime(
               object.value(
                   QStringLiteral(
                       "importedAtUtc"
                       )
                   ),
               source.importedAtUtc
               );
}

QJsonObject comparisonPresentationStateToJson(
    const InvestigationComparisonPresentationState
        &state
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "horizontalScrollValue"
            ),
        state.scroll.horizontalValue
        );

    object.insert(
        QStringLiteral(
            "verticalScrollValue"
            ),
        state.scroll.verticalValue
        );

    return object;
}

bool comparisonPresentationStateFromJson(
    const QJsonValue &value,
    InvestigationComparisonPresentationState
        &state
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    int horizontalValue = 0;
    int verticalValue = 0;

    if (!readInteger(
            object,
            QStringLiteral(
                "horizontalScrollValue"
                ),
            horizontalValue
            )
        || !readInteger(
            object,
            QStringLiteral(
                "verticalScrollValue"
                ),
            verticalValue
            )
        || horizontalValue < 0
        || verticalValue < 0) {
        return false;
    }

    state.scroll.horizontalValue =
        horizontalValue;

    state.scroll.verticalValue =
        verticalValue;

    return true;
}
}

QJsonObject
    InvestigationComparisonPersistenceSerializer::
    serialize(
        const PersistedInvestigationComparison
            &comparison
        ) const
{
    QJsonObject object;

    object.insert(
        QStringLiteral("comparisonId"),
        comparison.comparisonId
        );

    object.insert(
        QStringLiteral("baselineSource"),
        sourceToJson(
            comparison.baselineSource
            )
        );

    object.insert(
        QStringLiteral("comparisonSource"),
        sourceToJson(
            comparison.comparisonSource
            )
        );

    object.insert(
        QStringLiteral(
            "requestedBurstSettings"
            ),
        comparison.requestedBurstSettings
                .has_value()
            ? QJsonValue(
                  burstSettingsToJson(
                      *comparison
                           .requestedBurstSettings
                      )
                  )
            : QJsonValue(
                  QJsonValue::Null
                  )
        );

    QJsonObject analysis;

    QJsonObject totalRecords;

    totalRecords.insert(
        QStringLiteral("baselineCount"),
        integerToJson(
            comparison.analysis
                .totalRecords
                .baselineCount
            )
        );

    totalRecords.insert(
        QStringLiteral("comparisonCount"),
        integerToJson(
            comparison.analysis
                .totalRecords
                .comparisonCount
            )
        );

    analysis.insert(
        QStringLiteral("totalRecords"),
        totalRecords
        );

    analysis.insert(
        QStringLiteral("baselineTiming"),
        timingToJson(
            comparison.analysis.baselineTiming
            )
        );

    analysis.insert(
        QStringLiteral("comparisonTiming"),
        timingToJson(
            comparison.analysis
                .comparisonTiming
            )
        );

    analysis.insert(
        QStringLiteral("severity"),
        severityToJson(
            comparison.analysis.severity
            )
        );

    analysis.insert(
        QStringLiteral("eventCodes"),
        dimensionToJson(
            comparison.analysis.eventCodes
            )
        );

    analysis.insert(
        QStringLiteral(
            "elevatedSubsystems"
            ),
        dimensionToJson(
            comparison.analysis
                .elevatedSubsystems
            )
        );

    analysis.insert(
        QStringLiteral(
            "elevatedEntities"
            ),
        dimensionToJson(
            comparison.analysis
                .elevatedEntities
            )
        );

    analysis.insert(
        QStringLiteral("customFields"),
        customFieldsToJson(
            comparison.analysis.customFields
            )
        );

    if (comparison.analysis.bursts.has_value()) {
        QJsonObject bursts;

        bursts.insert(
            QStringLiteral("settings"),
            burstSettingsToJson(
                comparison.analysis
                    .bursts
                    ->settings
                )
            );

        bursts.insert(
            QStringLiteral("baseline"),
            burstSummaryToJson(
                comparison.analysis
                    .bursts
                    ->baseline
                )
            );

        bursts.insert(
            QStringLiteral("comparison"),
            burstSummaryToJson(
                comparison.analysis
                    .bursts
                    ->comparison
                )
            );

        analysis.insert(
            QStringLiteral("bursts"),
            bursts
            );
    } else {
        analysis.insert(
            QStringLiteral("bursts"),
            QJsonValue(
                QJsonValue::Null
                )
            );
    }

    object.insert(
        QStringLiteral("analysis"),
        analysis
        );

    object.insert(
        QStringLiteral("presentationState"),
        comparisonPresentationStateToJson(
            comparison.presentationState
            )
        );

    return object;
}

ComparisonPersistenceDeserializationResult
    InvestigationComparisonPersistenceSerializer::
    deserialize(
        const QJsonObject &object
        ) const
{
    PersistedInvestigationComparison
        comparison;

    if (!readString(
            object,
            QStringLiteral("comparisonId"),
            comparison.comparisonId
            )
        ||
        comparison.comparisonId.isEmpty()) {
        return failure(
            QStringLiteral(
                "Comparison ID is missing or invalid."
                )
            );
    }

    if (!sourceFromJson(
            object.value(
                QStringLiteral(
                    "baselineSource"
                    )
                ),
            comparison.baselineSource
            )
        ||
        !sourceFromJson(
            object.value(
                QStringLiteral(
                    "comparisonSource"
                    )
                ),
            comparison.comparisonSource
            )) {
        return failure(
            QStringLiteral(
                "Comparison source metadata is invalid."
                )
            );
    }

    const QJsonValue requestedSettings =
        object.value(
            QStringLiteral(
                "requestedBurstSettings"
                )
            );

    if (requestedSettings.isNull()) {
        comparison.requestedBurstSettings
            .reset();
    } else {
        BurstDetectionSettings settings;

        if (!burstSettingsFromJson(
                requestedSettings,
                settings
                )) {
            return failure(
                QStringLiteral(
                    "Requested burst settings are invalid."
                    )
                );
        }

        comparison.requestedBurstSettings =
            settings;
    }

    const QJsonValue analysisValue =
        object.value(
            QStringLiteral("analysis")
            );

    if (!analysisValue.isObject()) {
        return failure(
            QStringLiteral(
                "Comparison analysis is missing or invalid."
                )
            );
    }

    const QJsonObject analysis =
        analysisValue.toObject();

    const QJsonValue totalValue =
        analysis.value(
            QStringLiteral("totalRecords")
            );

    if (!totalValue.isObject()) {
        return failure(
            QStringLiteral(
                "Comparison record totals are invalid."
                )
            );
    }

    const QJsonObject total =
        totalValue.toObject();

    if (!readInteger(
            total,
            QStringLiteral("baselineCount"),
            comparison.analysis
                .totalRecords
                .baselineCount
            )
        ||
        !readInteger(
            total,
            QStringLiteral(
                "comparisonCount"
                ),
            comparison.analysis
                .totalRecords
                .comparisonCount
            )
        ||
        !timingFromJson(
            analysis.value(
                QStringLiteral(
                    "baselineTiming"
                    )
                ),
            comparison.analysis
                .baselineTiming
            )
        ||
        !timingFromJson(
            analysis.value(
                QStringLiteral(
                    "comparisonTiming"
                    )
                ),
            comparison.analysis
                .comparisonTiming
            )
        ||
        !severityFromJson(
            analysis.value(
                QStringLiteral("severity")
                ),
            comparison.analysis.severity
            )
        ||
        !dimensionFromJson(
            analysis.value(
                QStringLiteral("eventCodes")
                ),
            comparison.analysis.eventCodes
            )
        ||
        !dimensionFromJson(
            analysis.value(
                QStringLiteral(
                    "elevatedSubsystems"
                    )
                ),
            comparison.analysis
                .elevatedSubsystems
            )
        ||
        !dimensionFromJson(
            analysis.value(
                QStringLiteral(
                    "elevatedEntities"
                    )
                ),
            comparison.analysis
                .elevatedEntities
            )
        ||
        !customFieldsFromJson(
            analysis.value(
                QStringLiteral(
                    "customFields"
                    )
                ),
            comparison.analysis.customFields
            )) {
        return failure(
            QStringLiteral(
                "Comparison analysis contains invalid data."
                )
            );
    }

    const QJsonValue burstsValue =
        analysis.value(
            QStringLiteral("bursts")
            );

    if (burstsValue.isNull()) {
        comparison.analysis.bursts.reset();
    } else {
        if (!burstsValue.isObject()) {
            return failure(
                QStringLiteral(
                    "Burst comparison is invalid."
                    )
                );
        }

        const QJsonObject bursts =
            burstsValue.toObject();

        InvestigationBurstComparison
            burstComparison;

        if (!burstSettingsFromJson(
                bursts.value(
                    QStringLiteral("settings")
                    ),
                burstComparison.settings
                )
            ||
            !burstSummaryFromJson(
                bursts.value(
                    QStringLiteral("baseline")
                    ),
                burstComparison.baseline
                )
            ||
            !burstSummaryFromJson(
                bursts.value(
                    QStringLiteral(
                        "comparison"
                        )
                    ),
                burstComparison.comparison
                )) {
            return failure(
                QStringLiteral(
                    "Burst comparison contains invalid data."
                    )
                );
        }

        comparison.analysis.bursts =
            std::move(burstComparison);
    }

    const QJsonValue presentationValue =
        object.value(
            QStringLiteral(
                "presentationState"
                )
            );

    if (!presentationValue.isUndefined()
        && !comparisonPresentationStateFromJson(
            presentationValue,
            comparison.presentationState
            )) {
        return failure(
            QStringLiteral(
                "Comparison presentation state is invalid."
                )
            );
    }

    ComparisonPersistenceDeserializationResult
        result;

    result.comparison =
        std::move(comparison);

    return result;
}