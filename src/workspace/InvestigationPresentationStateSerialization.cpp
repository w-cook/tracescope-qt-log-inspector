#include "InvestigationPresentationStateSerialization.h"

#include <cmath>
#include <limits>

#include <QJsonArray>
#include <QJsonValue>

namespace
{

constexpr double MaxExactJsonInteger =
    9007199254740991.0;

PresentationStateDeserializationResult
failure(
    const QString &message
    )
{
    PresentationStateDeserializationResult result;

    result.errorCode =
        QStringLiteral(
            "INVALID_PRESENTATION_STATE"
            );

    result.errorMessage =
        message;

    return result;
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
        || value
               < std::numeric_limits<int>::min()
        || value
               > std::numeric_limits<int>::max()) {
        return false;
    }

    result =
        static_cast<int>(value);

    return true;
}

QJsonArray intVectorToJson(
    const QVector<int> &values
    )
{
    QJsonArray array;

    for (const int value : values) {
        array.append(value);
    }

    return array;
}

std::optional<QVector<int>>
intVectorFromJson(
    const QJsonValue &value
    )
{
    if (!value.isArray()) {
        return std::nullopt;
    }

    QVector<int> values;

    for (const QJsonValue &item
         : value.toArray()) {
        if (!item.isDouble()) {
            return std::nullopt;
        }

        const double number =
            item.toDouble();

        if (!std::isfinite(number)
            || std::floor(number) != number
            || number < 0
            || number
                   > std::numeric_limits<int>::max()) {
            return std::nullopt;
        }

        values.append(
            static_cast<int>(number)
            );
    }

    return values;
}

QJsonObject scrollToJson(
    const InvestigationScrollState &state
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("horizontal"),
        state.horizontalValue
        );

    object.insert(
        QStringLiteral("vertical"),
        state.verticalValue
        );

    return object;
}

bool scrollFromJson(
    const QJsonValue &value,
    InvestigationScrollState &state
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return
        readInteger(
            object,
            QStringLiteral("horizontal"),
            state.horizontalValue
            )
        &&
        readInteger(
            object,
            QStringLiteral("vertical"),
            state.verticalValue
            )
        &&
        state.horizontalValue >= 0
        &&
        state.verticalValue >= 0;
}

QJsonObject tableToJson(
    const InvestigationTablePresentationState
        &state
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("currentRow"),
        state.currentRow
        );

    object.insert(
        QStringLiteral("currentColumn"),
        state.currentColumn
        );

    object.insert(
        QStringLiteral("scroll"),
        scrollToJson(state.scroll)
        );

    return object;
}

bool tableFromJson(
    const QJsonValue &value,
    InvestigationTablePresentationState &state
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    return
        readInteger(
            object,
            QStringLiteral("currentRow"),
            state.currentRow
            )
        &&
        readInteger(
            object,
            QStringLiteral("currentColumn"),
            state.currentColumn
            )
        &&
        state.currentRow >= -1
        &&
        state.currentColumn >= -1
        &&
        scrollFromJson(
            object.value(
                QStringLiteral("scroll")
                ),
            state.scroll
            );
}

QString sortOrderToJson(
    Qt::SortOrder order
    )
{
    return order == Qt::DescendingOrder
               ? QStringLiteral("descending")
               : QStringLiteral("ascending");
}

std::optional<Qt::SortOrder>
sortOrderFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("ascending")) {
        return Qt::AscendingOrder;
    }

    if (value == QStringLiteral("descending")) {
        return Qt::DescendingOrder;
    }

    return std::nullopt;
}

QString timelineBreakdownToJson(
    InvestigationTimelineBreakdown breakdown
    )
{
    return breakdown
                   == InvestigationTimelineBreakdown::
                   Subsystem
               ? QStringLiteral("subsystem")
               : QStringLiteral("severity");
}

std::optional<InvestigationTimelineBreakdown>
timelineBreakdownFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("severity")) {
        return InvestigationTimelineBreakdown::
            Severity;
    }

    if (value == QStringLiteral("subsystem")) {
        return InvestigationTimelineBreakdown::
            Subsystem;
    }

    return std::nullopt;
}

QString reviewTabToJson(
    InvestigationReviewTab tab
    )
{
    switch (tab) {
    case InvestigationReviewTab::IssueSummary:
        return QStringLiteral("issueSummary");

    case InvestigationReviewTab::Findings:
        return QStringLiteral("findings");

    case InvestigationReviewTab::Analytics:
        return QStringLiteral("analytics");
    }

    return QStringLiteral("issueSummary");
}

std::optional<InvestigationReviewTab>
reviewTabFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("issueSummary")) {
        return InvestigationReviewTab::IssueSummary;
    }

    if (value == QStringLiteral("findings")) {
        return InvestigationReviewTab::Findings;
    }

    if (value == QStringLiteral("analytics")) {
        return InvestigationReviewTab::Analytics;
    }

    return std::nullopt;
}

QString analyticsTabToJson(
    InvestigationAnalyticsTab tab
    )
{
    return tab
                   == InvestigationAnalyticsTab::
                   Bursts
               ? QStringLiteral("bursts")
               : QStringLiteral("overview");
}

std::optional<InvestigationAnalyticsTab>
analyticsTabFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("overview")) {
        return InvestigationAnalyticsTab::Overview;
    }

    if (value == QStringLiteral("bursts")) {
        return InvestigationAnalyticsTab::Bursts;
    }

    return std::nullopt;
}

QString burstTimingModeToJson(
    InvestigationBurstTimingMode mode
    )
{
    return mode
                   == InvestigationBurstTimingMode::
                   Manual
               ? QStringLiteral("manual")
               : QStringLiteral("auto");
}

std::optional<InvestigationBurstTimingMode>
burstTimingModeFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("auto")) {
        return InvestigationBurstTimingMode::Auto;
    }

    if (value == QStringLiteral("manual")) {
        return InvestigationBurstTimingMode::Manual;
    }

    return std::nullopt;
}

QJsonValue optionalDateTimeToJson(
    const std::optional<QDateTime> &value
    )
{
    if (!value.has_value()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    return value
        ->toUTC()
        .toString(
            Qt::ISODateWithMs
            );
}

bool optionalDateTimeFromJson(
    const QJsonValue &value,
    std::optional<QDateTime> &result
    )
{
    if (value.isNull()) {
        result.reset();
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

    result =
        parsed.toUTC();

    return true;
}

QJsonObject burstSettingsToJson(
    const BurstDetectionSettings &settings
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("windowMilliseconds"),
        static_cast<double>(
            settings.windowMilliseconds
            )
        );

    object.insert(
        QStringLiteral("elevatedEventThreshold"),
        settings.elevatedEventThreshold
        );

    object.insert(
        QStringLiteral("errorCriticalThreshold"),
        settings.errorCriticalThreshold
        );

    object.insert(
        QStringLiteral("mergeGapMilliseconds"),
        static_cast<double>(
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

    return
        readInteger(
            object,
            QStringLiteral("windowMilliseconds"),
            settings.windowMilliseconds
            )
        &&
        readInteger(
            object,
            QStringLiteral("elevatedEventThreshold"),
            settings.elevatedEventThreshold
            )
        &&
        readInteger(
            object,
            QStringLiteral("errorCriticalThreshold"),
            settings.errorCriticalThreshold
            )
        &&
        readInteger(
            object,
            QStringLiteral("mergeGapMilliseconds"),
            settings.mergeGapMilliseconds
            )
        &&
        settings.isValid();
}

QJsonObject analyticsToJson(
    const InvestigationAnalyticsPresentationState
        &state
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("selectedTab"),
        analyticsTabToJson(
            state.selectedTab
            )
        );

    object.insert(
        QStringLiteral("overviewSplitterSizes"),
        intVectorToJson(
            state.overviewSplitterSizes
            )
        );

    object.insert(
        QStringLiteral("eventCodeTable"),
        tableToJson(
            state.eventCodeTable
            )
        );

    object.insert(
        QStringLiteral("entityTable"),
        tableToJson(
            state.entityTable
            )
        );

    object.insert(
        QStringLiteral("burstSplitterSizes"),
        intVectorToJson(
            state.burstSplitterSizes
            )
        );

    object.insert(
        QStringLiteral("selectedBurstStartTimestamp"),
        optionalDateTimeToJson(
            state.selectedBurstStartTimestamp
            )
        );

    object.insert(
        QStringLiteral("selectedBurstEndTimestamp"),
        optionalDateTimeToJson(
            state.selectedBurstEndTimestamp
            )
        );

    object.insert(
        QStringLiteral("burstTable"),
        tableToJson(
            state.burstTable
            )
        );

    object.insert(
        QStringLiteral("burstDetailScroll"),
        scrollToJson(
            state.burstDetailScroll
            )
        );

    return object;
}

bool analyticsFromJson(
    const QJsonValue &value,
    InvestigationAnalyticsPresentationState
        &state
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    const QJsonValue selectedTabValue =
        object.value(
            QStringLiteral("selectedTab")
            );

    if (!selectedTabValue.isString()) {
        return false;
    }

    const auto selectedTab =
        analyticsTabFromJson(
            selectedTabValue.toString()
            );

    const auto overviewSizes =
        intVectorFromJson(
            object.value(
                QStringLiteral(
                    "overviewSplitterSizes"
                    )
                )
            );

    const auto burstSizes =
        intVectorFromJson(
            object.value(
                QStringLiteral(
                    "burstSplitterSizes"
                    )
                )
            );

    if (!selectedTab.has_value()
        || !overviewSizes.has_value()
        || !burstSizes.has_value()) {
        return false;
    }

    state.selectedTab =
        *selectedTab;

    state.overviewSplitterSizes =
        *overviewSizes;

    state.burstSplitterSizes =
        *burstSizes;

    if (state.selectedBurstStartTimestamp.has_value()
        != state.selectedBurstEndTimestamp.has_value()) {
        return false;
    }

    return
        tableFromJson(
            object.value(
                QStringLiteral("eventCodeTable")
                ),
            state.eventCodeTable
            )
        &&
        tableFromJson(
            object.value(
                QStringLiteral("entityTable")
                ),
            state.entityTable
            )
        &&
        optionalDateTimeFromJson(
            object.value(
                QStringLiteral(
                    "selectedBurstStartTimestamp"
                    )
                ),
            state.selectedBurstStartTimestamp
            )
        &&
        optionalDateTimeFromJson(
            object.value(
                QStringLiteral(
                    "selectedBurstEndTimestamp"
                    )
                ),
            state.selectedBurstEndTimestamp
            )
        &&
        tableFromJson(
            object.value(
                QStringLiteral("burstTable")
                ),
            state.burstTable
            )
        &&
        scrollFromJson(
            object.value(
                QStringLiteral("burstDetailScroll")
                ),
            state.burstDetailScroll
            );
}
}

QJsonObject
    InvestigationPresentationStateSerializer::
    serialize(
        const InvestigationSessionPresentationState
            &state
        ) const
{
    QJsonObject object;

    QJsonObject eventTable;

    eventTable.insert(
        QStringLiteral("selectedRecordId"),
        state.eventTable.selectedRecordId
        );

    eventTable.insert(
        QStringLiteral("columnWidths"),
        intVectorToJson(
            state.eventTable.columnWidths
            )
        );

    eventTable.insert(
        QStringLiteral("sortColumn"),
        state.eventTable.sortColumn
        );

    eventTable.insert(
        QStringLiteral("sortOrder"),
        sortOrderToJson(
            state.eventTable.sortOrder
            )
        );

    eventTable.insert(
        QStringLiteral("scroll"),
        scrollToJson(
            state.eventTable.scroll
            )
        );

    object.insert(
        QStringLiteral("eventTable"),
        eventTable
        );

    object.insert(
        QStringLiteral("eventDetailScroll"),
        scrollToJson(
            state.eventDetailScroll
            )
        );

    QJsonObject timeline;

    timeline.insert(
        QStringLiteral("intervalMilliseconds"),
        static_cast<double>(
            state.timeline.intervalMilliseconds
            )
        );

    timeline.insert(
        QStringLiteral("breakdown"),
        timelineBreakdownToJson(
            state.timeline.breakdown
            )
        );

    timeline.insert(
        QStringLiteral("subsystemTrendLimit"),
        state.timeline.subsystemTrendLimit
        );

    timeline.insert(
        QStringLiteral("horizontalScrollValue"),
        state.timeline.horizontalScrollValue
        );

    object.insert(
        QStringLiteral("timeline"),
        timeline
        );

    QJsonObject review;

    review.insert(
        QStringLiteral("selectedTab"),
        reviewTabToJson(
            state.review.selectedTab
            )
        );

    review.insert(
        QStringLiteral("issueSummaryTable"),
        tableToJson(
            state.review.issueSummaryTable
            )
        );

    review.insert(
        QStringLiteral("findingsTable"),
        tableToJson(
            state.review.findingsTable
            )
        );

    review.insert(
        QStringLiteral("analytics"),
        analyticsToJson(
            state.review.analytics
            )
        );

    object.insert(
        QStringLiteral("review"),
        review
        );

    object.insert(
        QStringLiteral("mainSplitterSizes"),
        intVectorToJson(
            state.mainSplitterSizes
            )
        );

    object.insert(
        QStringLiteral("bottomSplitterSizes"),
        intVectorToJson(
            state.bottomSplitterSizes
            )
        );

    object.insert(
        QStringLiteral("burstTimingMode"),
        burstTimingModeToJson(
            state.burstTimingMode
            )
        );

    object.insert(
        QStringLiteral("burstDetectionSettings"),
        burstSettingsToJson(
            state.burstDetectionSettings
            )
        );

    return object;
}

PresentationStateDeserializationResult
    InvestigationPresentationStateSerializer::
    deserialize(
        const QJsonObject &object
        ) const
{
    InvestigationSessionPresentationState state;

    /*
     * Event table
     */
    const QJsonValue eventTableValue =
        object.value(
            QStringLiteral("eventTable")
            );

    if (!eventTableValue.isObject()) {
        return failure(
            QStringLiteral(
                "eventTable must be an object."
                )
            );
    }

    const QJsonObject eventTable =
        eventTableValue.toObject();

    const QJsonValue recordIdValue =
        eventTable.value(
            QStringLiteral("selectedRecordId")
            );

    const auto columnWidths =
        intVectorFromJson(
            eventTable.value(
                QStringLiteral("columnWidths")
                )
            );

    const QJsonValue sortOrderValue =
        eventTable.value(
            QStringLiteral("sortOrder")
            );

    if (!recordIdValue.isString()
        || !columnWidths.has_value()
        || !sortOrderValue.isString()
        || !readInteger(
            eventTable,
            QStringLiteral("sortColumn"),
            state.eventTable.sortColumn
            )
        || state.eventTable.sortColumn < -1
        || !scrollFromJson(
            eventTable.value(
                QStringLiteral("scroll")
                ),
            state.eventTable.scroll
            )) {
        return failure(
            QStringLiteral(
                "eventTable contains invalid values."
                )
            );
    }

    const auto sortOrder =
        sortOrderFromJson(
            sortOrderValue.toString()
            );

    if (!sortOrder.has_value()) {
        return failure(
            QStringLiteral(
                "eventTable sortOrder is invalid."
                )
            );
    }

    state.eventTable.selectedRecordId =
        recordIdValue.toString();

    state.eventTable.columnWidths =
        *columnWidths;

    state.eventTable.sortOrder =
        *sortOrder;

    if (!scrollFromJson(
            object.value(
                QStringLiteral(
                    "eventDetailScroll"
                    )
                ),
            state.eventDetailScroll
            )) {
        return failure(
            QStringLiteral(
                "eventDetailScroll is invalid."
                )
            );
    }

    /*
     * Timeline
     */
    const QJsonValue timelineValue =
        object.value(
            QStringLiteral("timeline")
            );

    if (!timelineValue.isObject()) {
        return failure(
            QStringLiteral(
                "timeline must be an object."
                )
            );
    }

    const QJsonObject timeline =
        timelineValue.toObject();

    const QJsonValue breakdownValue =
        timeline.value(
            QStringLiteral("breakdown")
            );

    if (!breakdownValue.isString()
        || !readInteger(
            timeline,
            QStringLiteral(
                "intervalMilliseconds"
                ),
            state.timeline.intervalMilliseconds
            )
        || state.timeline.intervalMilliseconds < 0
        || !readInteger(
            timeline,
            QStringLiteral(
                "subsystemTrendLimit"
                ),
            state.timeline.subsystemTrendLimit
            )
        || state.timeline.subsystemTrendLimit <= 0
        || !readInteger(
            timeline,
            QStringLiteral(
                "horizontalScrollValue"
                ),
            state.timeline.horizontalScrollValue
            )
        || state.timeline.horizontalScrollValue < 0) {
        return failure(
            QStringLiteral(
                "timeline contains invalid values."
                )
            );
    }

    const auto breakdown =
        timelineBreakdownFromJson(
            breakdownValue.toString()
            );

    if (!breakdown.has_value()) {
        return failure(
            QStringLiteral(
                "timeline breakdown is invalid."
                )
            );
    }

    state.timeline.breakdown =
        *breakdown;

    /*
     * Review
     */
    const QJsonValue reviewValue =
        object.value(
            QStringLiteral("review")
            );

    if (!reviewValue.isObject()) {
        return failure(
            QStringLiteral(
                "review must be an object."
                )
            );
    }

    const QJsonObject review =
        reviewValue.toObject();

    const QJsonValue reviewTabValue =
        review.value(
            QStringLiteral("selectedTab")
            );

    if (!reviewTabValue.isString()) {
        return failure(
            QStringLiteral(
                "review selectedTab is invalid."
                )
            );
    }

    const auto reviewTab =
        reviewTabFromJson(
            reviewTabValue.toString()
            );

    if (!reviewTab.has_value()
        || !tableFromJson(
            review.value(
                QStringLiteral(
                    "issueSummaryTable"
                    )
                ),
            state.review.issueSummaryTable
            )
        || !tableFromJson(
            review.value(
                QStringLiteral(
                    "findingsTable"
                    )
                ),
            state.review.findingsTable
            )
        || !analyticsFromJson(
            review.value(
                QStringLiteral("analytics")
                ),
            state.review.analytics
            )) {
        return failure(
            QStringLiteral(
                "review presentation state is invalid."
                )
            );
    }

    state.review.selectedTab =
        *reviewTab;

    const auto mainSizes =
        intVectorFromJson(
            object.value(
                QStringLiteral(
                    "mainSplitterSizes"
                    )
                )
            );

    const auto bottomSizes =
        intVectorFromJson(
            object.value(
                QStringLiteral(
                    "bottomSplitterSizes"
                    )
                )
            );

    const QJsonValue timingModeValue =
        object.value(
            QStringLiteral("burstTimingMode")
            );

    if (!mainSizes.has_value()
        || !bottomSizes.has_value()
        || !timingModeValue.isString()) {
        return failure(
            QStringLiteral(
                "session splitter or burst timing state is invalid."
                )
            );
    }

    const auto timingMode =
        burstTimingModeFromJson(
            timingModeValue.toString()
            );

    if (!timingMode.has_value()
        || !burstSettingsFromJson(
            object.value(
                QStringLiteral(
                    "burstDetectionSettings"
                    )
                ),
            state.burstDetectionSettings
            )) {
        return failure(
            QStringLiteral(
                "burst presentation settings are invalid."
                )
            );
    }

    state.mainSplitterSizes =
        *mainSizes;

    state.bottomSplitterSizes =
        *bottomSizes;

    state.burstTimingMode =
        *timingMode;

    PresentationStateDeserializationResult result;

    result.state =
        std::move(state);

    return result;
}