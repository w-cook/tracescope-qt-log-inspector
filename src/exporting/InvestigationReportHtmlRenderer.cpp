#include "InvestigationReportHtmlRenderer.h"

#include <algorithm>
#include <cmath>

#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

namespace
{

QString escaped(
    const QString &value
    )
{
    return value.toHtmlEscaped();
}

QString sourceDisplayName(
    const InvestigationReportSourceSnapshot &source
    )
{
    const QString sourceName =
        source.sourceName.trimmed();

    return sourceName.isEmpty()
               ? source.sessionId
               : sourceName;
}

QString timestampText(
    const QDateTime &timestamp
    )
{
    if (!timestamp.isValid()) {
        return QStringLiteral("Unavailable");
    }

    return timestamp
        .toUTC()
        .toString(
            Qt::ISODateWithMs
            );
}

QString timestampText(
    const std::optional<QDateTime> &timestamp
    )
{
    return timestamp.has_value()
    ? timestampText(*timestamp)
    : QStringLiteral("Unavailable");
}

QString durationText(
    qint64 milliseconds
    )
{
    if (milliseconds < 1000) {
        return QStringLiteral("%1 ms")
        .arg(milliseconds);
    }

    if (milliseconds < 60 * 1000) {
        return QStringLiteral("%1 s")
        .arg(
            static_cast<double>(
                milliseconds
                )
                / 1000.0,
            0,
            'f',
            1
            );
    }

    if (milliseconds
        < 60 * 60 * 1000) {
        return QStringLiteral("%1 min")
        .arg(
            static_cast<double>(
                milliseconds
                )
                / static_cast<double>(
                    60 * 1000
                    ),
            0,
            'f',
            1
            );
    }

    if (milliseconds
        < 24LL * 60 * 60 * 1000) {
        return QStringLiteral("%1 h")
        .arg(
            static_cast<double>(
                milliseconds
                )
                / static_cast<double>(
                    60 * 60 * 1000
                    ),
            0,
            'f',
            1
            );
    }

    return QStringLiteral("%1 d")
        .arg(
            static_cast<double>(
                milliseconds
                )
                / static_cast<double>(
                    24LL * 60 * 60 * 1000
                    ),
            0,
            'f',
            1
            );
}

QString severityText(
    RecordSeverity severity
    )
{
    switch (severity) {
    case RecordSeverity::Trace:
        return QStringLiteral("Trace");

    case RecordSeverity::Debug:
        return QStringLiteral("Debug");

    case RecordSeverity::Info:
        return QStringLiteral("Info");

    case RecordSeverity::Warning:
        return QStringLiteral("Warning");

    case RecordSeverity::Error:
        return QStringLiteral("Error");

    case RecordSeverity::Critical:
        return QStringLiteral("Critical");
    }

    return QStringLiteral("Unknown");
}

QString severityText(
    const std::optional<RecordSeverity> &severity
    )
{
    return severity.has_value()
    ? severityText(*severity)
    : QStringLiteral("Unspecified");
}

QString findingStatusText(
    FindingStatus status
    )
{
    switch (status) {
    case FindingStatus::None:
        return QStringLiteral("None");

    case FindingStatus::Open:
        return QStringLiteral("Open");

    case FindingStatus::Resolved:
        return QStringLiteral("Resolved");

    case FindingStatus::Dismissed:
        return QStringLiteral("Dismissed");
    }

    return QStringLiteral("None");
}

QString investigationStateText(
    const InvestigationRecordState &state,
    bool burstEvidence
    )
{
    QStringList parts;

    if (state.bookmarked) {
        parts.append(
            QStringLiteral("Bookmarked")
            );
    }

    if (state.findingStatus
        != FindingStatus::None) {
        parts.append(
            QStringLiteral("Finding: %1")
                .arg(
                    findingStatusText(
                        state.findingStatus
                        )
                    )
            );
    }

    if (!state.note.trimmed().isEmpty()) {
        parts.append(
            QStringLiteral("Note")
            );
    }

    if (burstEvidence) {
        parts.append(
            QStringLiteral("Burst evidence")
            );
    }

    return parts.isEmpty()
               ? QStringLiteral("Evidence")
               : parts.join(
                     QStringLiteral(", ")
                     );
}

QString joinedValues(
    const QStringList &values
    )
{
    if (values.isEmpty()) {
        return QStringLiteral("None");
    }

    QStringList escapedValues;

    escapedValues.reserve(
        values.size()
        );

    for (const QString &value : values) {
        escapedValues.append(
            escaped(value)
            );
    }

    return escapedValues.join(
        QStringLiteral(", ")
        );
}

QString sessionAnchor(
    int index
    )
{
    return QStringLiteral("session-%1")
    .arg(index + 1);
}

QString comparisonAnchor(
    int index
    )
{
    return QStringLiteral("comparison-%1")
    .arg(index + 1);
}

void appendMetric(
    QTextStream &out,
    const QString &label,
    const QString &value
    )
{
    out
        << "<div class=\"metric\">"
        << "<div class=\"metric-label\">"
        << escaped(label)
        << "</div>"
        << "<div class=\"metric-value\">"
        << escaped(value)
        << "</div>"
        << "</div>";
}

void appendSeverityTable(
    QTextStream &out,
    const InvestigationReportSeveritySummary &visible,
    const InvestigationReportSeveritySummary &analysis
    )
{
    out
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Severity</th>"
        << "<th>Visible</th>"
        << "<th>Analysis</th>"
        << "</tr></thead>"
        << "<tbody>";

    const auto appendRow =
        [&out](
            const QString &label,
            qint64 visibleCount,
            qint64 analysisCount
            ) {
            out
                << "<tr><td>"
                << escaped(label)
                << "</td><td class=\"number\">"
                << visibleCount
                << "</td><td class=\"number\">"
                << analysisCount
                << "</td></tr>";
        };

    appendRow(
        QStringLiteral("Trace"),
        visible.traceCount,
        analysis.traceCount
        );

    appendRow(
        QStringLiteral("Debug"),
        visible.debugCount,
        analysis.debugCount
        );

    appendRow(
        QStringLiteral("Info"),
        visible.infoCount,
        analysis.infoCount
        );

    appendRow(
        QStringLiteral("Warning"),
        visible.warningCount,
        analysis.warningCount
        );

    appendRow(
        QStringLiteral("Error"),
        visible.errorCount,
        analysis.errorCount
        );

    appendRow(
        QStringLiteral("Critical"),
        visible.criticalCount,
        analysis.criticalCount
        );

    appendRow(
        QStringLiteral("Unspecified"),
        visible.unspecifiedCount,
        analysis.unspecifiedCount
        );

    appendRow(
        QStringLiteral("Total"),
        visible.totalCount(),
        analysis.totalCount()
        );

    out
        << "</tbody></table>"
        << "</div>";
}

bool hasActiveFilters(
    const InvestigationReportFilterSnapshot &filters
    )
{
    return !filters.severities.isEmpty()
    || !filters.subsystems.isEmpty()
        || !filters.searchText.isEmpty()
        || !filters.eventCodes.isEmpty()
        || !filters.entityIds.isEmpty()
        || filters.startTime.has_value()
        || filters.endTime.has_value()
        || !filters.customFieldFilters.isEmpty()
        || !filters.findingStatuses.isEmpty()
        || filters.bookmarkedOnly;
}

void appendFilterScope(
    QTextStream &out,
    const InvestigationReportFilterSnapshot &filters
    )
{
    out
        << "<details>"
        << "<summary>Captured investigation filters</summary>"
        << "<div class=\"details-body\">";

    if (!hasActiveFilters(filters)) {
        out
            << "<p class=\"muted\">"
            << "No investigation filters were active "
               "when this report snapshot was captured."
            << "</p></div></details>";

        return;
    }

    out
        << "<dl class=\"definition-grid\">";

    const auto appendDefinition =
        [&out](
            const QString &label,
            const QString &value
            ) {
            out
                << "<dt>"
                << escaped(label)
                << "</dt><dd>"
                << value
                << "</dd>";
        };

    if (!filters.severities.isEmpty()) {
        appendDefinition(
            QStringLiteral("Severities"),
            joinedValues(
                filters.severities
                )
            );
    }

    if (!filters.subsystems.isEmpty()) {
        appendDefinition(
            QStringLiteral("Subsystems"),
            joinedValues(
                filters.subsystems
                )
            );
    }

    if (!filters.searchText.isEmpty()) {
        appendDefinition(
            QStringLiteral("Search"),
            escaped(
                filters.searchText
                )
            );
    }

    if (!filters.eventCodes.isEmpty()) {
        appendDefinition(
            QStringLiteral("Event codes"),
            joinedValues(
                filters.eventCodes
                )
            );
    }

    if (!filters.entityIds.isEmpty()) {
        appendDefinition(
            QStringLiteral("Entities"),
            joinedValues(
                filters.entityIds
                )
            );
    }

    if (filters.startTime.has_value()) {
        appendDefinition(
            QStringLiteral("Start time"),
            escaped(
                timestampText(
                    filters.startTime
                    )
                )
            );
    }

    if (filters.endTime.has_value()) {
        appendDefinition(
            QStringLiteral("End time"),
            escaped(
                timestampText(
                    filters.endTime
                    )
                )
            );
    }

    if (!filters.findingStatuses.isEmpty()) {
        appendDefinition(
            QStringLiteral("Finding statuses"),
            joinedValues(
                filters.findingStatuses
                )
            );
    }

    if (filters.bookmarkedOnly) {
        appendDefinition(
            QStringLiteral("Bookmarks"),
            QStringLiteral(
                "Bookmarked records only"
                )
            );
    }

    for (
        auto iterator =
        filters.customFieldFilters.cbegin();
        iterator !=
        filters.customFieldFilters.cend();
        ++iterator
        ) {
        appendDefinition(
            QStringLiteral("Custom: %1")
                .arg(iterator.key()),
            joinedValues(
                iterator.value()
                )
            );
    }

    out
        << "</dl>"
        << "</div>"
        << "</details>";
}

void appendCapabilities(
    QTextStream &out,
    const InvestigationReportCapabilities &capabilities
    )
{
    const auto text =
        [](bool available) {
            return available
                       ? QStringLiteral("Available")
                       : QStringLiteral("Not populated");
        };

    out
        << "<details>"
        << "<summary>Source capabilities</summary>"
        << "<div class=\"details-body\">"
        << "<dl class=\"definition-grid\">";

    const auto append =
        [&out, &text](
            const QString &label,
            bool available
            ) {
            out
                << "<dt>"
                << escaped(label)
                << "</dt><dd>"
                << text(available)
                << "</dd>";
        };

    append(
        QStringLiteral("Timestamps"),
        capabilities.hasTimestampData
        );

    append(
        QStringLiteral("Severity"),
        capabilities.hasSeverityData
        );

    append(
        QStringLiteral("Subsystem"),
        capabilities.hasSubsystemData
        );

    append(
        QStringLiteral("Event code"),
        capabilities.hasEventCodeData
        );

    append(
        QStringLiteral("Entity"),
        capabilities.hasEntityData
        );

    append(
        QStringLiteral("Custom fields"),
        capabilities.hasCustomFieldData
        );

    out
        << "</dl>"
        << "</div>"
        << "</details>";
}

void appendTimelineSvg(
    QTextStream &out,
    const InvestigationReportTimelineSnapshot &timeline
    )
{
    if (!timeline.available) {
        out
            << "<p class=\"muted\">"
            << "Timeline analysis is unavailable for this source."
            << "</p>";

        return;
    }

    if (timeline.buckets.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No timestamped records are available for the "
               "captured analysis population."
            << "</p>";

        return;
    }

    int maximumCount = 0;

    for (const EventCountBucket &bucket
         : timeline.buckets) {
        maximumCount =
            std::max(
                maximumCount,
                bucket.totalCount()
                );
    }

    if (maximumCount <= 0) {
        out
            << "<p class=\"muted\">"
            << "The captured timeline contains no events."
            << "</p>";

        return;
    }

    constexpr double chartTop = 18.0;
    constexpr double chartBottom = 178.0;
    constexpr double chartHeight =
        chartBottom - chartTop;

    constexpr double leftMargin = 48.0;
    constexpr double rightMargin = 18.0;
    constexpr double barWidth = 12.0;
    constexpr double barStep = 18.0;

    const double plotWidth =
        std::max(
            560.0,
            static_cast<double>(
                timeline.buckets.size()
                )
                * barStep
            );

    const double svgWidth =
        leftMargin
        + plotWidth
        + rightMargin;

    constexpr double svgHeight = 220.0;

    out
        << "<div class=\"timeline-chart\">"
        << "<div class=\"timeline-meta\">"
        << "Analysis population · "
        << escaped(
               durationText(
                   timeline.intervalMilliseconds
                   )
               )
        << " buckets"
        << "</div>"
        << "<div class=\"timeline-scroll\">"
        << "<svg class=\"timeline-svg\" "
        << "viewBox=\"0 0 "
        << QString::number(svgWidth, 'f', 0)
        << " "
        << QString::number(svgHeight, 'f', 0)
        << "\" "
        << "role=\"img\" "
        << "aria-label=\"Event activity timeline\">";

    /*
     * Baseline and maximum-count reference.
     */
    out
        << "<line class=\"timeline-axis\" "
        << "x1=\""
        << QString::number(leftMargin, 'f', 0)
        << "\" y1=\""
        << QString::number(chartBottom, 'f', 0)
        << "\" x2=\""
        << QString::number(
               leftMargin + plotWidth,
               'f',
               0
               )
        << "\" y2=\""
        << QString::number(chartBottom, 'f', 0)
        << "\"/>";

    out
        << "<text class=\"timeline-axis-label\" "
        << "x=\"6\" y=\""
        << QString::number(
               chartTop + 5.0,
               'f',
               0
               )
        << "\">"
        << maximumCount
        << "</text>";

    const int labelStep =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    static_cast<double>(
                        timeline.buckets.size()
                        )
                    / 8.0
                    )
                )
            );

    const auto appendSegment =
        [&out, maximumCount](
            double x,
            double &currentY,
            int count,
            const char *cssClass
            ) {
            if (count <= 0) {
                return;
            }

            const double height =
                static_cast<double>(count)
                / static_cast<double>(
                    maximumCount
                    )
                * chartHeight;

            currentY -=
                height;

            out
                << "<rect class=\""
                << cssClass
                << "\" x=\""
                << QString::number(x, 'f', 1)
                << "\" y=\""
                << QString::number(
                       currentY,
                       'f',
                       1
                       )
                << "\" width=\""
                << QString::number(
                       barWidth,
                       'f',
                       1
                       )
                << "\" height=\""
                << QString::number(
                       height,
                       'f',
                       1
                       )
                << "\"/>";
        };

    for (int index = 0;
         index < timeline.buckets.size();
         ++index) {
        const EventCountBucket &bucket =
            timeline.buckets.at(index);

        const double x =
            leftMargin
            + static_cast<double>(index)
                  * barStep;

        double currentY =
            chartBottom;

        out
            << "<g>"
            << "<title>"
            << escaped(bucket.label)
            << " — "
            << bucket.totalCount()
            << " records"
            << "</title>";

        appendSegment(
            x,
            currentY,
            bucket.traceCount,
            "timeline-trace"
            );

        appendSegment(
            x,
            currentY,
            bucket.debugCount,
            "timeline-debug"
            );

        appendSegment(
            x,
            currentY,
            bucket.infoCount,
            "timeline-info"
            );

        appendSegment(
            x,
            currentY,
            bucket.unspecifiedCount,
            "timeline-unspecified"
            );

        appendSegment(
            x,
            currentY,
            bucket.warningCount,
            "timeline-warning"
            );

        appendSegment(
            x,
            currentY,
            bucket.errorCount,
            "timeline-error"
            );

        appendSegment(
            x,
            currentY,
            bucket.criticalCount,
            "timeline-critical"
            );

        out
            << "</g>";

        const bool showLabel =
            index % labelStep == 0
            || index
                   == timeline.buckets.size() - 1;

        if (showLabel) {
            out
                << "<text class=\"timeline-axis-label\" "
                << "x=\""
                << QString::number(
                       x + barWidth / 2.0,
                       'f',
                       1
                       )
                << "\" y=\"202\" "
                << "text-anchor=\"middle\">"
                << escaped(bucket.label)
                << "</text>";
        }
    }

    out
        << "</svg>"
        << "</div>"
        << "<div class=\"timeline-legend\">"
        << "<span><i class=\"legend-info\"></i>Info / lower</span>"
        << "<span><i class=\"legend-warning\"></i>Warning</span>"
        << "<span><i class=\"legend-error\"></i>Error</span>"
        << "<span><i class=\"legend-critical\"></i>Critical</span>"
        << "<span><i class=\"legend-unspecified\"></i>Unspecified</span>"
        << "</div>"
        << "</div>";
}

void appendIssueGroups(
    QTextStream &out,
    const QVector<TelemetryIssueGroup> &groups
    )
{
    if (groups.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No grouped warning/error activity was "
               "identified in the captured analysis population."
            << "</p>";

        return;
    }

    out
        << "<details open>"
        << "<summary>Subsystem Issue Groups ("
        << groups.size()
        << ")</summary>"
        << "<div class=\"details-body table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Subsystem</th>"
        << "<th>Warnings</th>"
        << "<th>Errors / Critical</th>"
        << "<th>Total elevated</th>"
        << "</tr></thead>"
        << "<tbody>";

    for (const TelemetryIssueGroup &group
         : groups) {
        out
            << "<tr><td>"
            << escaped(
                   group.subsystem.isEmpty()
                       ? QStringLiteral(
                             "Unspecified"
                             )
                       : group.subsystem
                   )
            << "</td>"
            << "<td class=\"number\">"
            << group.warningCount
            << "</td>"
            << "<td class=\"number\">"
            << group.errorCount
            << "</td>"
            << "<td class=\"number\">"
            << group.totalCount()
            << "</td></tr>";
    }

    out
        << "</tbody></table>"
        << "</div>"
        << "</details>";
}

void appendFrequencyTable(
    QTextStream &out,
    const QString &title,
    const QVector<InvestigationValueFrequency>
        &frequencies
    )
{
    out
        << "<details>"
        << "<summary>"
        << escaped(title)
        << " ("
        << frequencies.size()
        << ")</summary>"
        << "<div class=\"details-body\">";

    if (frequencies.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No populated values were available."
            << "</p>"
            << "</div></details>";

        return;
    }

    out
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Value</th>"
        << "<th>Count</th>"
        << "</tr></thead>"
        << "<tbody>";

    for (
        const InvestigationValueFrequency
            &frequency
        : frequencies
        ) {
        out
            << "<tr><td>"
            << escaped(
                   frequency.value.isEmpty()
                       ? QStringLiteral(
                             "Unspecified"
                             )
                       : frequency.value
                   )
            << "</td>"
            << "<td class=\"number\">"
            << frequency.count
            << "</td></tr>";
    }

    out
        << "</tbody></table>"
        << "</div>"
        << "</div>"
        << "</details>";
}

void appendComparison(
    QTextStream &out,
    const InvestigationReportComparisonSnapshot &comparison,
    int index
    )
{
    const QString anchor =
        comparisonAnchor(index);

    out
        << "<article class=\"report-section comparison-section\" id=\""
        << anchor
        << "\">"
        << "<div class=\"section-kicker\">Immutable Session Comparison</div>"
        << "<h2>"
        << escaped(
               comparison.documentTitle
               )
        << "</h2>";

    out
        << "<div class=\"comparison-route\">"
        << "<div><span class=\"route-label\">Baseline</span>"
        << "<strong>"
        << escaped(
               comparison.baselineSourceName
               )
        << "</strong></div>"
        << "<div class=\"route-arrow\">→</div>"
        << "<div><span class=\"route-label\">Comparison</span>"
        << "<strong>"
        << escaped(
               comparison.comparisonSourceName
               )
        << "</strong></div>"
        << "</div>";

    out
        << "<div class=\"metrics\">";

    appendMetric(
        out,
        QStringLiteral("Baseline records"),
        QString::number(
            comparison.analysis
                .totalRecords
                .baselineCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Comparison records"),
        QString::number(
            comparison.analysis
                .totalRecords
                .comparisonCount
            )
        );

    const qint64 delta =
        comparison.analysis
            .totalRecords
            .delta();

    appendMetric(
        out,
        QStringLiteral("Record delta"),
        delta > 0
            ? QStringLiteral("+%1")
                  .arg(delta)
            : QString::number(delta)
        );

    out
        << "</div>";

    out
        << "<h3>Captured Timing</h3>"
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th></th>"
        << "<th>Baseline</th>"
        << "<th>Comparison</th>"
        << "</tr></thead>"
        << "<tbody>"
        << "<tr><th>Timestamped records</th>"
        << "<td class=\"number\">"
        << comparison.analysis
               .baselineTiming
               .timestampedRecordCount
        << "</td>"
        << "<td class=\"number\">"
        << comparison.analysis
               .comparisonTiming
               .timestampedRecordCount
        << "</td></tr>"
        << "<tr><th>First timestamp</th><td>"
        << escaped(
               timestampText(
                   comparison.analysis
                       .baselineTiming
                       .firstTimestamp
                   )
               )
        << "</td><td>"
        << escaped(
               timestampText(
                   comparison.analysis
                       .comparisonTiming
                       .firstTimestamp
                   )
               )
        << "</td></tr>"
        << "<tr><th>Last timestamp</th><td>"
        << escaped(
               timestampText(
                   comparison.analysis
                       .baselineTiming
                       .lastTimestamp
                   )
               )
        << "</td><td>"
        << escaped(
               timestampText(
                   comparison.analysis
                       .comparisonTiming
                       .lastTimestamp
                   )
               )
        << "</td></tr>"
        << "<tr><th>Duration</th><td>"
        << escaped(
               durationText(
                   comparison.analysis
                       .baselineTiming
                       .durationMilliseconds
                   )
               )
        << "</td><td>"
        << escaped(
               durationText(
                   comparison.analysis
                       .comparisonTiming
                       .durationMilliseconds
                   )
               )
        << "</td></tr>"
        << "</tbody></table>"
        << "</div>";

    out
        << "<p class=\"note\">"
        << "This comparison is the immutable analysis "
           "captured by the TraceScope comparison document. "
           "It was not recalculated from current session state "
           "during report generation."
        << "</p>";

    out
        << "</article>";
}

QStringList sortedCustomAttributeKeys(
    const InvestigationRecord &record
    )
{
    QStringList keys =
        record.customAttributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            const int insensitive =
                left.compare(
                    right,
                    Qt::CaseInsensitive
                    );

            if (insensitive != 0) {
                return insensitive < 0;
            }

            return left.compare(
                       right,
                       Qt::CaseSensitive
                       ) < 0;
        }
        );

    return keys;
}

QString customAttributeText(
    const QVariant &value
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return QStringLiteral("null");
    }

    const QJsonValue jsonValue =
        QJsonValue::fromVariant(
            value
            );

    if (jsonValue.isObject()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toObject()
                )
                .toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    if (jsonValue.isArray()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toArray()
                )
                .toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    if (jsonValue.isBool()) {
        return jsonValue.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}

QString burstTimingModeText(
    InvestigationReportBurstTimingMode mode
    )
{
    switch (mode) {
    case InvestigationReportBurstTimingMode::Auto:
        return QStringLiteral("Auto");

    case InvestigationReportBurstTimingMode::Manual:
        return QStringLiteral("Manual");
    }

    return QStringLiteral("Unknown");
}

void appendCadence(
    QTextStream &out,
    const InvestigationCadence &cadence
    )
{
    out
        << "<div class=\"metrics\">";

    appendMetric(
        out,
        QStringLiteral("Timestamped records"),
        QString::number(
            cadence.timestampCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Positive gaps"),
        QString::number(
            cadence.positiveGapCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Zero gaps"),
        QString::number(
            cadence.zeroGapCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Median gap"),
        durationText(
            static_cast<qint64>(
                cadence.medianPositiveGapMilliseconds
                )
            )
        );

    out
        << "</div>"
        << "<details>"
        << "<summary>Cadence statistics</summary>"
        << "<div class=\"details-body\">"
        << "<dl class=\"definition-grid\">";

    const auto append =
        [&out](
            const QString &label,
            const QString &value
            ) {
            out
                << "<dt>"
                << escaped(label)
                << "</dt><dd>"
                << escaped(value)
                << "</dd>";
        };

    append(
        QStringLiteral("Minimum positive gap"),
        durationText(
            cadence.minimumPositiveGapMilliseconds
            )
        );

    append(
        QStringLiteral("Median positive gap"),
        durationText(
            static_cast<qint64>(
                cadence.medianPositiveGapMilliseconds
                )
            )
        );

    append(
        QStringLiteral("Mean positive gap"),
        durationText(
            static_cast<qint64>(
                cadence.meanPositiveGapMilliseconds
                )
            )
        );

    append(
        QStringLiteral("90th percentile gap"),
        durationText(
            cadence.p90PositiveGapMilliseconds
            )
        );

    append(
        QStringLiteral("Maximum positive gap"),
        durationText(
            cadence.maximumPositiveGapMilliseconds
            )
        );

    append(
        QStringLiteral("Recommended burst window"),
        durationText(
            cadence.recommendedBurstWindowMilliseconds
            )
        );

    append(
        QStringLiteral("Recommended merge gap"),
        durationText(
            cadence.recommendedMergeGapMilliseconds
            )
        );

    append(
        QStringLiteral("Recommendation basis"),
        cadence.usesFallbackRecommendation
            ? QStringLiteral(
                  "Fallback defaults"
                  )
            : QStringLiteral(
                  "Observed timestamp cadence"
                  )
        );

    out
        << "</dl>"
        << "</div>"
        << "</details>";
}

void appendBurstSettings(
    QTextStream &out,
    const BurstDetectionSettings &settings
    )
{
    out
        << "<dl class=\"definition-grid\">"
        << "<dt>Window</dt><dd>"
        << escaped(
               durationText(
                   settings.windowMilliseconds
                   )
               )
        << "</dd>"
        << "<dt>Elevated-event threshold</dt><dd>"
        << settings.elevatedEventThreshold
        << "</dd>"
        << "<dt>Error/Critical threshold</dt><dd>"
        << settings.errorCriticalThreshold
        << "</dd>"
        << "<dt>Merge gap</dt><dd>"
        << escaped(
               durationText(
                   settings.mergeGapMilliseconds
                   )
               )
        << "</dd>"
        << "</dl>";
}

QString burstTriggerText(
    const InvestigationBurst &burst
    )
{
    QStringList triggers;

    if (burst.triggeredByElevatedThreshold) {
        triggers.append(
            QStringLiteral(
                "Elevated-event threshold"
                )
            );
    }

    if (
        burst
            .triggeredByErrorCriticalThreshold
        ) {
        triggers.append(
            QStringLiteral(
                "Error/Critical threshold"
                )
            );
    }

    return triggers.isEmpty()
               ? QStringLiteral("Detected burst")
               : triggers.join(
                     QStringLiteral(", ")
                     );
}

void appendBurstAnalysis(
    QTextStream &out,
    const InvestigationReportBurstSnapshot &analysis
    )
{
    if (!analysis.available) {
        out
            << "<p class=\"muted\">"
            << "Burst analysis is unavailable for this source."
            << "</p>";

        return;
    }

    out
        << "<div class=\"metrics\">";

    appendMetric(
        out,
        QStringLiteral("Detected bursts"),
        QString::number(
            analysis.bursts.size()
            )
        );

    appendMetric(
        out,
        QStringLiteral("Timing mode"),
        burstTimingModeText(
            analysis.timingMode
            )
        );

    appendMetric(
        out,
        QStringLiteral("Effective window"),
        durationText(
            analysis
                .effectiveSettings
                .windowMilliseconds
            )
        );

    appendMetric(
        out,
        QStringLiteral("Effective merge gap"),
        durationText(
            analysis
                .effectiveSettings
                .mergeGapMilliseconds
            )
        );

    out
        << "</div>";

    out
        << "<details>"
        << "<summary>Burst detection settings</summary>"
        << "<div class=\"details-body\">"
        << "<h4>Configured</h4>";

    appendBurstSettings(
        out,
        analysis.configuredSettings
        );

    out
        << "<h4>Effective</h4>";

    appendBurstSettings(
        out,
        analysis.effectiveSettings
        );

    out
        << "</div>"
        << "</details>";

    if (analysis.bursts.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "Burst analysis was available, but no bursts "
               "met the captured detection thresholds."
            << "</p>";

        return;
    }

    out
        << "<details open>"
        << "<summary>Detected Bursts ("
        << analysis.bursts.size()
        << ")</summary>"
        << "<div class=\"details-body table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Start</th>"
        << "<th>End</th>"
        << "<th>Duration</th>"
        << "<th>Warning</th>"
        << "<th>Error</th>"
        << "<th>Critical</th>"
        << "<th>Total elevated</th>"
        << "<th>Trigger</th>"
        << "</tr></thead>"
        << "<tbody>";

    for (const InvestigationBurst &burst
         : analysis.bursts) {
        out
            << "<tr>"
            << "<td>"
            << escaped(
                   timestampText(
                       burst.startTimestamp
                       )
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   timestampText(
                       burst.endTimestamp
                       )
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   durationText(
                       burst.durationMilliseconds()
                       )
                   )
            << "</td>"
            << "<td class=\"number\">"
            << burst.warningCount
            << "</td>"
            << "<td class=\"number\">"
            << burst.errorCount
            << "</td>"
            << "<td class=\"number\">"
            << burst.criticalCount
            << "</td>"
            << "<td class=\"number\">"
            << burst.totalElevatedCount()
            << "</td>"
            << "<td>"
            << escaped(
                   burstTriggerText(
                       burst
                       )
                   )
            << "</td>"
            << "</tr>";
    }

    out
        << "</tbody></table>"
        << "</div>"
        << "</details>";
}

bool hasInvestigatorState(
    const InvestigationReportEvidenceRecord &evidence
    )
{
    return !evidence.state.isEmpty();
}

void appendInvestigatorAnnotations(
    QTextStream &out,
    const QVector<InvestigationReportEvidenceRecord>
        &evidenceRecords
    )
{
    QVector<
        const InvestigationReportEvidenceRecord *
        > stateful;

    for (
        const InvestigationReportEvidenceRecord
            &evidence
        : evidenceRecords
        ) {
        if (hasInvestigatorState(
                evidence
                )) {
            stateful.append(
                &evidence
                );
        }
    }

    if (stateful.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No bookmarks, analyst notes, or finding "
               "classifications were captured for this source."
            << "</p>";

        return;
    }

    out
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Record</th>"
        << "<th>Timestamp</th>"
        << "<th>Severity</th>"
        << "<th>Message</th>"
        << "<th>Bookmark</th>"
        << "<th>Finding</th>"
        << "<th>Note</th>"
        << "</tr></thead>"
        << "<tbody>";

    for (
        const InvestigationReportEvidenceRecord
            *evidence
        : stateful
        ) {
        const InvestigationRecord &record =
            evidence->record;

        out
            << "<tr>"
            << "<td>"
            << escaped(
                   record.recordId
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   record.timestamp.has_value()
                       ? timestampText(
                             *record.timestamp
                             )
                       : QStringLiteral(
                             "Unavailable"
                             )
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   severityText(
                       record.severity
                       )
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   record.message.has_value()
                       ? *record.message
                       : QString()
                   )
            << "</td>"
            << "<td>"
            << (
                   evidence->state.bookmarked
                       ? "Yes"
                       : "No"
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   findingStatusText(
                       evidence
                           ->state
                           .findingStatus
                       )
                   )
            << "</td>"
            << "<td class=\"preserve-whitespace\">"
            << escaped(
                   evidence
                       ->state
                       .note
                   )
            << "</td>"
            << "</tr>";
    }

    out
        << "</tbody></table>"
        << "</div>";
}

void appendCustomAttributes(
    QTextStream &out,
    const InvestigationRecord &record
    )
{
    const QStringList keys =
        sortedCustomAttributeKeys(
            record
            );

    if (keys.isEmpty()) {
        return;
    }

    out
        << "<h5>Custom Attributes</h5>"
        << "<dl class=\"definition-grid\">";

    for (const QString &key : keys) {
        out
            << "<dt>"
            << escaped(key)
            << "</dt><dd class=\"preserve-whitespace\">"
            << escaped(
                   customAttributeText(
                       record
                           .customAttributes
                           .value(key)
                       )
                   )
            << "</dd>";
    }

    out
        << "</dl>";
}

void appendEvidenceRecord(
    QTextStream &out,
    const InvestigationReportEvidenceRecord
        &evidence,
    int index
    )
{
    const InvestigationRecord &record =
        evidence.record;

    QString summary =
        record.message.has_value()
                && !record.message
                        ->trimmed()
                        .isEmpty()
            ? *record.message
            : record.recordId;

    if (summary.trimmed().isEmpty()) {
        summary =
            QStringLiteral("Evidence record %1")
                .arg(index + 1);
    }

    out
        << "<details class=\"evidence-record\">"
        << "<summary>"
        << escaped(
               severityText(
                   record.severity
                   )
               )
        << " · "
        << escaped(summary)
        << "</summary>"
        << "<div class=\"details-body\">";

    out
        << "<div class=\"evidence-tags\">";

    if (evidence.state.bookmarked) {
        out
            << "<span class=\"evidence-tag\">Bookmarked</span>";
    }

    if (
        evidence.state.findingStatus
        != FindingStatus::None
        ) {
        out
            << "<span class=\"evidence-tag\">"
            << escaped(
                   QStringLiteral(
                       "Finding: %1"
                       )
                       .arg(
                           findingStatusText(
                               evidence
                                   .state
                                   .findingStatus
                               )
                           )
                   )
            << "</span>";
    }

    if (
        !evidence
             .state
             .note
             .trimmed()
             .isEmpty()
        ) {
        out
            << "<span class=\"evidence-tag\">Note</span>";
    }

    if (evidence.burstEvidence) {
        out
            << "<span class=\"evidence-tag\">Burst evidence</span>";
    }

    out
        << "</div>";

    out
        << "<dl class=\"definition-grid\">"
        << "<dt>Record ID</dt><dd>"
        << escaped(record.recordId)
        << "</dd>"
        << "<dt>Timestamp</dt><dd>"
        << escaped(
               record.timestamp.has_value()
                   ? timestampText(
                         *record.timestamp
                         )
                   : QStringLiteral(
                         "Unavailable"
                         )
               )
        << "</dd>"
        << "<dt>Severity</dt><dd>"
        << escaped(
               severityText(
                   record.severity
                   )
               )
        << "</dd>";

    if (record.subsystem.has_value()) {
        out
            << "<dt>Subsystem</dt><dd>"
            << escaped(
                   *record.subsystem
                   )
            << "</dd>";
    }

    if (record.eventCode.has_value()) {
        out
            << "<dt>Event code</dt><dd>"
            << escaped(
                   *record.eventCode
                   )
            << "</dd>";
    }

    if (record.entityId.has_value()) {
        out
            << "<dt>Entity ID</dt><dd>"
            << escaped(
                   *record.entityId
                   )
            << "</dd>";
    }

    if (record.message.has_value()) {
        out
            << "<dt>Message</dt>"
            << "<dd class=\"preserve-whitespace\">"
            << escaped(
                   *record.message
                   )
            << "</dd>";
    }

    /*
     * Deliberately expose source filename and logical
     * record number only. Never emit sourcePath here.
     */
    if (!record.source.sourceName.isEmpty()) {
        out
            << "<dt>Source</dt><dd>"
            << escaped(
                   record.source.sourceName
                   )
            << "</dd>";
    }

    if (record.source.recordNumber > 0) {
        out
            << "<dt>Source record</dt><dd>"
            << record.source.recordNumber
            << "</dd>";
    }

    out
        << "</dl>";

    if (
        !evidence
             .state
             .note
             .trimmed()
             .isEmpty()
        ) {
        out
            << "<h5>Analyst Note</h5>"
            << "<div class=\"annotation-note\">"
            << escaped(
                   evidence.state.note
                   )
            << "</div>";
    }

    appendCustomAttributes(
        out,
        record
        );

    if (!record.rawSource.isEmpty()) {
        out
            << "<h5>Raw Source</h5>"
            << "<pre class=\"raw-source\">"
            << escaped(
                   record.rawSource
                   )
            << "</pre>";
    }

    out
        << "</div>"
        << "</details>";
}

void appendEvidence(
    QTextStream &out,
    const InvestigationReportSessionSnapshot &session,
    bool supportingEvidenceIncluded
    )
{
    if (session.evidenceRecords.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No investigation evidence records were captured."
            << "</p>";

        return;
    }

    out
        << "<p class=\"muted\">";

    if (supportingEvidenceIncluded) {
        out
            << "Includes investigator-state records and "
               "additional captured burst evidence where applicable.";
    } else {
        out
            << "Additional supporting evidence was disabled for "
               "this export. Investigator-state records remain "
               "included as core investigation content.";
    }

    out
        << "</p>";

    for (int index = 0;
         index < session.evidenceRecords.size();
         ++index) {
        appendEvidenceRecord(
            out,
            session.evidenceRecords.at(index),
            index
            );
    }
}

QString importDiagnosticSeverityText(
    ImportDiagnosticSeverity severity
    )
{
    switch (severity) {
    case ImportDiagnosticSeverity::Information:
        return QStringLiteral("Information");

    case ImportDiagnosticSeverity::Warning:
        return QStringLiteral("Warning");

    case ImportDiagnosticSeverity::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Unknown");
}

QString timestampRuleTypeText(
    TimestampRuleType type
    )
{
    switch (type) {
    case TimestampRuleType::Iso8601:
        return QStringLiteral("ISO 8601");

    case TimestampRuleType::QtFormat:
        return QStringLiteral("Qt format");
    }

    return QStringLiteral("Unknown");
}

QString yesNoText(
    bool value
    )
{
    return value
               ? QStringLiteral("Yes")
               : QStringLiteral("No");
}

QString byteSizeText(
    qint64 bytes
    )
{
    if (bytes < 0) {
        return QStringLiteral("Unavailable");
    }

    if (bytes < 1024) {
        return QStringLiteral("%1 B")
        .arg(bytes);
    }

    constexpr double kibibyte =
        1024.0;

    constexpr double mebibyte =
        1024.0 * 1024.0;

    constexpr double gibibyte =
        1024.0 * 1024.0 * 1024.0;

    if (
        static_cast<double>(bytes)
        < mebibyte
        ) {
        return QStringLiteral("%1 KiB")
        .arg(
            static_cast<double>(bytes)
                / kibibyte,
            0,
            'f',
            1
            );
    }

    if (
        static_cast<double>(bytes)
        < gibibyte
        ) {
        return QStringLiteral("%1 MiB")
        .arg(
            static_cast<double>(bytes)
                / mebibyte,
            0,
            'f',
            1
            );
    }

    return QStringLiteral("%1 GiB")
        .arg(
            static_cast<double>(bytes)
                / gibibyte,
            0,
            'f',
            2
            );
}

void appendImportDiagnostics(
    QTextStream &out,
    const QVector<ImportDiagnostic> &diagnostics
    )
{
    out
        << "<details"
        << (
               diagnostics.isEmpty()
                   ? ""
                   : " open"
               )
        << ">"
        << "<summary>Import Diagnostics ("
        << diagnostics.size()
        << ")</summary>"
        << "<div class=\"details-body\">";

    if (diagnostics.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No import diagnostics were captured."
            << "</p>"
            << "</div></details>";

        return;
    }

    out
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Severity</th>"
        << "<th>Code</th>"
        << "<th>Message</th>"
        << "<th>Source context</th>"
        << "</tr></thead>"
        << "<tbody>";

    for (const ImportDiagnostic &diagnostic
         : diagnostics) {
        QString sourceContext;

        if (diagnostic.source.has_value()) {
            const RecordSourceMetadata &source =
                *diagnostic.source;

            QStringList parts;

            /*
             * Never include sourcePath even if an
             * unsanitized diagnostic reaches the
             * renderer.
             */
            if (!source.sourceName.isEmpty()) {
                parts.append(
                    source.sourceName
                    );
            }

            if (source.recordNumber > 0) {
                parts.append(
                    QStringLiteral("record %1")
                        .arg(
                            source.recordNumber
                            )
                    );
            }

            sourceContext =
                parts.join(
                    QStringLiteral(" · ")
                    );
        }

        out
            << "<tr>"
            << "<td>"
            << escaped(
                   importDiagnosticSeverityText(
                       diagnostic.severity
                       )
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   diagnostic.code
                   )
            << "</td>"
            << "<td class=\"preserve-whitespace\">"
            << escaped(
                   diagnostic.message
                   )
            << "</td>"
            << "<td>"
            << escaped(
                   sourceContext
                   )
            << "</td>"
            << "</tr>";
    }

    out
        << "</tbody></table>"
        << "</div>"
        << "</div>"
        << "</details>";
}

void appendCanonicalMapping(
    QTextStream &out,
    const QString &label,
    const QString &path
    )
{
    out
        << "<tr><td>"
        << escaped(label)
        << "</td><td>"
        << escaped(
               path.trimmed().isEmpty()
                   ? QStringLiteral(
                         "Not mapped"
                         )
                   : path
               )
        << "</td></tr>";
}

void appendTechnicalImportProfile(
    QTextStream &out,
    const ImportProfile &profile
    )
{
    out
        << "<details>"
        << "<summary>Complete Import Profile</summary>"
        << "<div class=\"details-body\">";

    out
        << "<dl class=\"definition-grid\">"
        << "<dt>Schema version</dt><dd>"
        << profile.schemaVersion
        << "</dd>"
        << "<dt>Profile name</dt><dd>"
        << escaped(profile.name)
        << "</dd>"
        << "<dt>Importer</dt><dd>"
        << escaped(profile.importerId)
        << "</dd>"
        << "<dt>Preserve unmapped fields</dt><dd>"
        << escaped(
               yesNoText(
                   profile.preserveUnmappedFields
                   )
               )
        << "</dd>"
        << "</dl>";

    if (!profile.recordPath.trimmed().isEmpty()) {
        out
            << "<h4>Record Selection</h4>"
            << "<dl class=\"definition-grid\">"
            << "<dt>Record path</dt><dd>"
            << escaped(
                   profile.recordPath
                   )
            << "</dd>"
            << "</dl>";
    }

    if (!profile.regexPattern.trimmed().isEmpty()) {
        out
            << "<h4>Regex Configuration</h4>"
            << "<pre class=\"technical-code\">"
            << escaped(
                   profile.regexPattern
                   )
            << "</pre>";
    }

    out
        << "<h4>Canonical Field Mappings</h4>"
        << "<div class=\"table-wrap\">"
        << "<table>"
        << "<thead><tr>"
        << "<th>Canonical field</th>"
        << "<th>Source path</th>"
        << "</tr></thead>"
        << "<tbody>";

    appendCanonicalMapping(
        out,
        QStringLiteral("Timestamp"),
        profile
            .canonicalFields
            .timestampPath
        );

    appendCanonicalMapping(
        out,
        QStringLiteral("Severity"),
        profile
            .canonicalFields
            .severityPath
        );

    appendCanonicalMapping(
        out,
        QStringLiteral("Subsystem"),
        profile
            .canonicalFields
            .subsystemPath
        );

    appendCanonicalMapping(
        out,
        QStringLiteral("Event code"),
        profile
            .canonicalFields
            .eventCodePath
        );

    appendCanonicalMapping(
        out,
        QStringLiteral("Entity ID"),
        profile
            .canonicalFields
            .entityIdPath
        );

    appendCanonicalMapping(
        out,
        QStringLiteral("Message"),
        profile
            .canonicalFields
            .messagePath
        );

    out
        << "</tbody></table>"
        << "</div>";

    out
        << "<details>"
        << "<summary>Custom Field Mappings ("
        << profile.customFields.size()
        << ")</summary>"
        << "<div class=\"details-body\">";

    if (profile.customFields.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No explicit custom field mappings."
            << "</p>";
    } else {
        out
            << "<div class=\"table-wrap\">"
            << "<table>"
            << "<thead><tr>"
            << "<th>Name</th>"
            << "<th>Source path</th>"
            << "</tr></thead>"
            << "<tbody>";

        for (
            const CustomFieldMapping &mapping
            : profile.customFields
            ) {
            out
                << "<tr><td>"
                << escaped(mapping.name)
                << "</td><td>"
                << escaped(mapping.sourcePath)
                << "</td></tr>";
        }

        out
            << "</tbody></table>"
            << "</div>";
    }

    out
        << "</div></details>";

    out
        << "<details>"
        << "<summary>Severity Aliases ("
        << profile.severityAliases.size()
        << ")</summary>"
        << "<div class=\"details-body\">";

    if (profile.severityAliases.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No severity aliases were configured."
            << "</p>";
    } else {
        out
            << "<div class=\"table-wrap\">"
            << "<table>"
            << "<thead><tr>"
            << "<th>Source value</th>"
            << "<th>Canonical severity</th>"
            << "</tr></thead>"
            << "<tbody>";

        for (
            auto iterator =
            profile.severityAliases.cbegin();
            iterator
            != profile.severityAliases.cend();
            ++iterator
            ) {
            out
                << "<tr><td>"
                << escaped(
                       iterator.key()
                       )
                << "</td><td>"
                << escaped(
                       severityText(
                           iterator.value()
                           )
                       )
                << "</td></tr>";
        }

        out
            << "</tbody></table>"
            << "</div>";
    }

    out
        << "</div></details>";

    out
        << "<details>"
        << "<summary>Timestamp Rules ("
        << profile.timestampRules.size()
        << ")</summary>"
        << "<div class=\"details-body\">";

    if (profile.timestampRules.isEmpty()) {
        out
            << "<p class=\"muted\">"
            << "No timestamp parsing rules were configured."
            << "</p>";
    } else {
        out
            << "<div class=\"table-wrap\">"
            << "<table>"
            << "<thead><tr>"
            << "<th>Priority</th>"
            << "<th>Type</th>"
            << "<th>Format</th>"
            << "</tr></thead>"
            << "<tbody>";

        for (int index = 0;
             index < profile.timestampRules.size();
             ++index) {
            const TimestampRule &rule =
                profile.timestampRules.at(
                    index
                    );

            out
                << "<tr><td class=\"number\">"
                << index + 1
                << "</td><td>"
                << escaped(
                       timestampRuleTypeText(
                           rule.type
                           )
                       )
                << "</td><td>"
                << escaped(
                       rule.format.trimmed().isEmpty()
                           ? QStringLiteral(
                                 "Default"
                                 )
                           : rule.format
                       )
                << "</td></tr>";
        }

        out
            << "</tbody></table>"
            << "</div>";
    }

    out
        << "</div></details>";

    out
        << "</div>"
        << "</details>";
}

void appendImportContext(
    QTextStream &out,
    const InvestigationReportSourceSnapshot &source,
    bool technicalAppendixIncluded
    )
{
    out
        << "<div class=\"table-wrap\">"
        << "<table><tbody>"
        << "<tr><th>Source name</th><td>"
        << escaped(
               sourceDisplayName(source)
               )
        << "</td></tr>"
        << "<tr><th>Source size</th><td>"
        << escaped(
               byteSizeText(
                   source.sourceSizeBytes
                   )
               )
        << "</td></tr>"
        << "<tr><th>Source last modified</th><td>"
        << escaped(
               timestampText(
                   source.sourceLastModified
                   )
               )
        << "</td></tr>"
        << "<tr><th>Imported at</th><td>"
        << escaped(
               timestampText(
                   source.importedAtUtc
                   )
               )
        << "</td></tr>"
        << "<tr><th>Importer</th><td>"
        << escaped(source.importerId)
        << "</td></tr>"
        << "<tr><th>Import profile</th><td>"
        << escaped(source.importProfileName)
        << "</td></tr>"
        << "<tr><th>Processed records</th>"
        << "<td class=\"number\">"
        << source.processedRecordCount
        << "</td></tr>"
        << "<tr><th>Imported records</th>"
        << "<td class=\"number\">"
        << source.importedRecordCount
        << "</td></tr>"
        << "<tr><th>Skipped records</th>"
        << "<td class=\"number\">"
        << source.skippedRecordCount
        << "</td></tr>"
        << "<tr><th>Source truncated</th><td>"
        << escaped(
               yesNoText(
                   source.sourceTruncated
                   )
               )
        << "</td></tr>"
        << "</tbody></table>"
        << "</div>";

    appendImportDiagnostics(
        out,
        source.diagnostics
        );

    if (!technicalAppendixIncluded) {
        out
            << "<p class=\"muted\">"
            << "The complete technical import/profile "
               "appendix was not included in this export."
            << "</p>";

        return;
    }

    if (
        source
            .technicalImportProfile
            .has_value()
        ) {
        appendTechnicalImportProfile(
            out,
            *source.technicalImportProfile
            );
    } else {
        out
            << "<p class=\"muted\">"
            << "No complete technical import profile "
               "was captured for this source."
            << "</p>";
    }
}

void appendSession(
    QTextStream &out,
    const InvestigationReportSessionSnapshot &session,
    int index,
    bool supportingEvidenceIncluded,
    bool technicalAppendixIncluded
    )
{
    const QString anchor =
        sessionAnchor(index);

    out
        << "<article class=\"report-section source-section\" id=\""
        << anchor
        << "\">"
        << "<div class=\"section-kicker\">Source Investigation</div>"
        << "<h2>"
        << escaped(
               session.source.documentTitle
               )
        << "</h2>"
        << "<p class=\"muted\">"
        << escaped(
               sourceDisplayName(
                   session.source
                   )
               )
        << " · "
        << escaped(
               session.source.importerId
               )
        << "</p>";

    /*
     * -------------------------------------------------
     * Overview metrics
     * -------------------------------------------------
     */

    out
        << "<div class=\"metrics\">";

    appendMetric(
        out,
        QStringLiteral("Source records"),
        QString::number(
            session.recordContext
                .totalRecordCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Visible records"),
        QString::number(
            session.recordContext
                .visibleRecordCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Analysis records"),
        QString::number(
            session.recordContext
                .analysisRecordCount
            )
        );

    appendMetric(
        out,
        QStringLiteral("Evidence records"),
        QString::number(
            session.evidenceRecords.size()
            )
        );

    out
        << "</div>";

    /*
     * -------------------------------------------------
     * Investigation scope
     * -------------------------------------------------
     */

    out
        << "<h3>Investigation Scope</h3>"
        << "<div class=\"table-wrap\">"
        << "<table><tbody>"

        << "<tr><th>Total imported population</th>"
        << "<td class=\"number\">"
        << session.recordContext.totalRecordCount
        << "</td></tr>"

        << "<tr><th>Visible population</th>"
        << "<td class=\"number\">"
        << session.recordContext.visibleRecordCount
        << "</td></tr>"

        << "<tr><th>Visible timestamped records</th>"
        << "<td class=\"number\">"
        << session.recordContext
               .visibleTimestampedRecordCount
        << "</td></tr>"

        << "<tr><th>Visible first timestamp</th><td>"
        << escaped(
               timestampText(
                   session.recordContext
                       .visibleFirstTimestamp
                   )
               )
        << "</td></tr>"

        << "<tr><th>Visible last timestamp</th><td>"
        << escaped(
               timestampText(
                   session.recordContext
                       .visibleLastTimestamp
                   )
               )
        << "</td></tr>"

        << "<tr><th>Analysis population</th>"
        << "<td class=\"number\">"
        << session.recordContext.analysisRecordCount
        << "</td></tr>"

        << "<tr><th>Analysis timestamped records</th>"
        << "<td class=\"number\">"
        << session.recordContext
               .analysisTimestampedRecordCount
        << "</td></tr>"

        << "<tr><th>Analysis first timestamp</th><td>"
        << escaped(
               timestampText(
                   session.recordContext
                       .analysisFirstTimestamp
                   )
               )
        << "</td></tr>"

        << "<tr><th>Analysis last timestamp</th><td>"
        << escaped(
               timestampText(
                   session.recordContext
                       .analysisLastTimestamp
                   )
               )
        << "</td></tr>"

        << "</tbody></table>"
        << "</div>";

    out
        << "<p class=\"note\">"
        << "Analysis populations intentionally ignore "
           "annotation-only bookmark and finding-status "
           "filters. The visible population preserves the "
           "complete current table filter state."
        << "</p>";

    /*
     * -------------------------------------------------
     * Severity summary
     * -------------------------------------------------
     */

    out
        << "<h3>Severity Summary</h3>";

    appendSeverityTable(
        out,
        session.visibleSeveritySummary,
        session.analysisSeveritySummary
        );

    /*
     * -------------------------------------------------
     * Event activity timeline
     * -------------------------------------------------
     */

    out
        << "<h3>Event Activity Timeline</h3>"
        << "<p class=\"muted\">"
        << "The timeline represents the deterministic "
           "analysis population captured at export time."
        << "</p>";

    appendTimelineSvg(
        out,
        session.timeline
        );

    /*
     * -------------------------------------------------
     * Grouped warning/error analysis
     * -------------------------------------------------
     */

    out
        << "<h3>Grouped Warning/Error Analysis</h3>";

    appendIssueGroups(
        out,
        session.elevatedIssueGroups
        );

    /*
     * -------------------------------------------------
     * Deterministic analytics
     * -------------------------------------------------
     */

    out
        << "<h3>Deterministic Analytics</h3>"
        << "<p class=\"muted\">"
        << "These frequency summaries describe populated "
           "canonical values in the captured analysis population."
        << "</p>";

    appendFrequencyTable(
        out,
        QStringLiteral("Event Codes"),
        session.eventCodeFrequencies
        );

    appendFrequencyTable(
        out,
        QStringLiteral("Entities"),
        session.entityFrequencies
        );

    appendFrequencyTable(
        out,
        QStringLiteral("Subsystems"),
        session.subsystemFrequencies
        );

    /*
     * -------------------------------------------------
     * Timestamp cadence
     * -------------------------------------------------
     */

    out
        << "<h3>Timestamp Cadence</h3>";

    if (
        session
            .burstAnalysis
            .available
        ) {
        appendCadence(
            out,
            session
                .burstAnalysis
                .cadence
            );
    } else {
        out
            << "<p class=\"muted\">"
            << "Timestamp cadence analysis is unavailable "
               "for this source."
            << "</p>";
    }

    /*
     * -------------------------------------------------
     * Burst analysis
     * -------------------------------------------------
     */

    out
        << "<h3>Burst Analysis</h3>";

    appendBurstAnalysis(
        out,
        session.burstAnalysis
        );

    /*
     * -------------------------------------------------
     * Findings and annotations
     * -------------------------------------------------
     */

    out
        << "<h3>Findings and Investigator Annotations</h3>";

    appendInvestigatorAnnotations(
        out,
        session.evidenceRecords
        );

    /*
     * -------------------------------------------------
     * Supporting evidence
     * -------------------------------------------------
     */

    out
        << "<h3>Supporting Evidence</h3>";

    appendEvidence(
        out,
        session,
        supportingEvidenceIncluded
        );

    /*
     * -------------------------------------------------
     * Import and data-quality context
     * -------------------------------------------------
     */

    out
        << "<h3>Import and Data-Quality Context</h3>";

    appendImportContext(
        out,
        session.source,
        technicalAppendixIncluded
        );

    /*
     * -------------------------------------------------
     * Captured filters and source capabilities
     * -------------------------------------------------
     */

    appendFilterScope(
        out,
        session.filters
        );

    appendCapabilities(
        out,
        session.capabilities
        );

    out
        << "</article>";
}

}

QString InvestigationReportHtmlRenderer::render(
    const InvestigationReportSnapshot &snapshot
    ) const
{
    QString html;

    QTextStream out(
        &html
        );

    out
        << "<!DOCTYPE html>"
        << "<html lang=\"en\">"
        << "<head>"
        << "<meta charset=\"utf-8\">"
        << "<meta name=\"viewport\" "
           "content=\"width=device-width, initial-scale=1\">"
        << "<title>"
        << escaped(snapshot.title)
        << "</title>";

    out << R"HTML(
<style>
:root {
    color-scheme: light;
    --page: #f4f6f8;
    --surface: #ffffff;
    --surface-muted: #f8fafc;
    --border: #d7dde5;
    --text: #17202a;
    --muted: #5f6b78;
    --accent: #245b87;
    --accent-soft: #eaf2f8;
    --nav: #18232e;
    --nav-text: #e8eef4;
}

* {
    box-sizing: border-box;
}

html {
    scroll-behavior: smooth;
}

body {
    margin: 0;
    background: var(--page);
    color: var(--text);
    font-family:
        -apple-system,
        BlinkMacSystemFont,
        "Segoe UI",
        Arial,
        sans-serif;
    line-height: 1.5;
}

.report-shell {
    display: grid;
    grid-template-columns: 270px minmax(0, 1fr);
    min-height: 100vh;
}

.report-nav {
    position: sticky;
    top: 0;
    height: 100vh;
    overflow-y: auto;
    padding: 24px 18px;
    background: var(--nav);
    color: var(--nav-text);
}

.nav-brand {
    margin-bottom: 22px;
    font-size: 1.05rem;
    font-weight: 700;
}

.nav-label {
    margin: 18px 8px 6px;
    opacity: 0.65;
    font-size: 0.72rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}

.report-nav a {
    display: block;
    padding: 7px 8px;
    border-radius: 5px;
    color: var(--nav-text);
    text-decoration: none;
    font-size: 0.9rem;
}

.report-nav a:hover {
    background: rgba(255, 255, 255, 0.09);
}

.report-main {
    width: min(1180px, 100%);
    padding: 42px 48px 72px;
}

.report-header,
.report-section {
    margin-bottom: 26px;
    padding: 28px 30px;
    border: 1px solid var(--border);
    border-radius: 10px;
    background: var(--surface);
}

.report-header h1 {
    margin: 0 0 8px;
    font-size: 2rem;
    line-height: 1.2;
}

.report-context {
    white-space: pre-wrap;
}

.section-kicker {
    margin-bottom: 4px;
    color: var(--accent);
    font-size: 0.76rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}

h2 {
    margin: 0 0 8px;
}

h3 {
    margin-top: 26px;
}

.muted {
    color: var(--muted);
}

.metrics {
    display: grid;
    grid-template-columns:
        repeat(auto-fit, minmax(150px, 1fr));
    gap: 12px;
    margin: 20px 0;
}

.metric {
    padding: 14px 16px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: var(--surface-muted);
}

.metric-label {
    margin-bottom: 4px;
    color: var(--muted);
    font-size: 0.78rem;
    font-weight: 600;
    text-transform: uppercase;
}

.metric-value {
    font-size: 1.25rem;
    font-weight: 700;
}

.table-wrap {
    overflow-x: auto;
    margin: 14px 0;
}

table {
    width: 100%;
    border-collapse: collapse;
}

th,
td {
    padding: 9px 10px;
    border-bottom: 1px solid var(--border);
    text-align: left;
    vertical-align: top;
}

thead th {
    background: var(--surface-muted);
    font-size: 0.82rem;
}

td.number {
    text-align: right;
    font-variant-numeric: tabular-nums;
}

.note {
    padding: 12px 14px;
    border-left: 3px solid var(--accent);
    background: var(--accent-soft);
    color: #29465f;
}

details {
    margin: 14px 0;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: var(--surface);
}

summary {
    padding: 12px 14px;
    cursor: pointer;
    font-weight: 650;
}

.details-body {
    padding: 0 14px 14px;
}

.definition-grid {
    display: grid;
    grid-template-columns: minmax(140px, 220px) 1fr;
    gap: 8px 18px;
}

.definition-grid dt {
    color: var(--muted);
    font-weight: 650;
}

.definition-grid dd {
    margin: 0;
}

.comparison-route {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    gap: 16px;
    align-items: center;
    margin: 20px 0;
    padding: 16px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: var(--surface-muted);
}

.comparison-route > div:not(.route-arrow) {
    display: flex;
    flex-direction: column;
}

.route-label {
    color: var(--muted);
    font-size: 0.76rem;
    font-weight: 650;
    text-transform: uppercase;
}

.route-arrow {
    color: var(--accent);
    font-size: 1.5rem;
}

footer {
    padding: 12px 4px;
    color: var(--muted);
    font-size: 0.82rem;
}

.timeline-chart {
    margin: 14px 0 22px;
    padding: 14px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: var(--surface-muted);
}

.timeline-meta {
    margin-bottom: 10px;
    color: var(--muted);
    font-size: 0.82rem;
    font-weight: 600;
}

.timeline-scroll {
    overflow-x: auto;
}

.timeline-svg {
    display: block;
    min-width: 100%;
    height: 220px;
}

.timeline-axis {
    stroke: #8995a3;
    stroke-width: 1;
}

.timeline-axis-label {
    fill: #5f6b78;
    font-size: 10px;
}

.timeline-trace,
.timeline-debug,
.timeline-info {
    fill: #6c8eaa;
}

.timeline-unspecified {
    fill: #a8b0b8;
}

.timeline-warning {
    fill: #d5a72d;
}

.timeline-error {
    fill: #c45b45;
}

.timeline-critical {
    fill: #853c4a;
}

.timeline-legend {
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
    margin-top: 8px;
    color: var(--muted);
    font-size: 0.78rem;
}

.timeline-legend span {
    display: inline-flex;
    align-items: center;
    gap: 5px;
}

.timeline-legend i {
    display: inline-block;
    width: 10px;
    height: 10px;
    border-radius: 2px;
}

.legend-info {
    background: #6c8eaa;
}

.legend-warning {
    background: #d5a72d;
}

.legend-error {
    background: #c45b45;
}

.legend-critical {
    background: #853c4a;
}

.legend-unspecified {
    background: #a8b0b8;
}

.preserve-whitespace {
    white-space: pre-wrap;
    overflow-wrap: anywhere;
}

.evidence-record {
    margin: 10px 0;
}

.evidence-tags {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-bottom: 14px;
}

.evidence-tag {
    padding: 3px 8px;
    border-radius: 999px;
    background: var(--accent-soft);
    color: #29465f;
    font-size: 0.75rem;
    font-weight: 650;
}

.annotation-note {
    padding: 12px 14px;
    border-left: 3px solid var(--accent);
    background: var(--surface-muted);
    white-space: pre-wrap;
    overflow-wrap: anywhere;
}

.raw-source {
    max-height: 420px;
    overflow: auto;
    padding: 14px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: #f6f8fa;
    color: #1f2933;
    font-family:
        "SFMono-Regular",
        Consolas,
        "Liberation Mono",
        monospace;
    font-size: 0.8rem;
    line-height: 1.45;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
}

h4,
h5 {
    margin-bottom: 8px;
}

.technical-code {
    overflow: auto;
    padding: 14px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: #f6f8fa;
    font-family:
        "SFMono-Regular",
        Consolas,
        "Liberation Mono",
        monospace;
    font-size: 0.8rem;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
}

@media (max-width: 860px) {
    .report-shell {
        display: block;
    }

    .report-nav {
        position: sticky;
        z-index: 10;
        height: auto;
        max-height: 40vh;
        padding: 12px 16px;
    }

    .nav-label {
        margin-top: 10px;
    }

    .report-main {
        padding: 24px 16px 48px;
    }

    .report-header,
    .report-section {
        padding: 22px 18px;
    }
}

@media print {
    body {
        background: white;
    }

    .report-shell {
        display: block;
    }

    .report-nav {
        display: none;
    }

    .report-main {
        width: auto;
        padding: 0;
    }

    .report-header,
    .report-section {
        break-inside: avoid;
        border: 0;
        border-bottom: 1px solid #cccccc;
        border-radius: 0;
        padding: 18px 0;
    }

    details {
        break-inside: avoid;
    }

    details > * {
        display: block;
    }
}
</style>
)HTML";

    out
        << "</head><body>"
        << "<div class=\"report-shell\">";

    /*
     * -------------------------------------------------
     * Persistent navigation
     * -------------------------------------------------
     */

    out
        << "<aside class=\"report-nav\">"
        << "<div class=\"nav-brand\">TraceScope Investigation Report</div>"
        << "<nav>"
        << "<a href=\"#overview\">Overview</a>"
        << "<a href=\"#sources\">Sources</a>";

    if (!snapshot.sourceTimeCoverage.isEmpty()) {
        out
            << "<a href=\"#time-coverage\">Time Coverage</a>";
    }

    if (!snapshot.crossSourceChronology.isEmpty()) {
        out
            << "<a href=\"#chronology\">Evidence Chronology</a>";
    }

    if (!snapshot.comparisons.isEmpty()) {
        out
            << "<div class=\"nav-label\">Comparisons</div>";

        for (int index = 0;
             index < snapshot.comparisons.size();
             ++index) {
            out
                << "<a href=\"#"
                << comparisonAnchor(index)
                << "\">"
                << escaped(
                       snapshot.comparisons
                           .at(index)
                           .documentTitle
                       )
                << "</a>";
        }
    }

    if (!snapshot.sessions.isEmpty()) {
        out
            << "<div class=\"nav-label\">Sources</div>";

        for (int index = 0;
             index < snapshot.sessions.size();
             ++index) {
            out
                << "<a href=\"#"
                << sessionAnchor(index)
                << "\">"
                << escaped(
                       snapshot.sessions
                           .at(index)
                           .source
                           .documentTitle
                       )
                << "</a>";
        }
    }

    out
        << "</nav>"
        << "</aside>";

    out
        << "<main class=\"report-main\">";

    /*
     * -------------------------------------------------
     * Overview
     * -------------------------------------------------
     */

    out
        << "<header class=\"report-header\" id=\"overview\">"
        << "<div class=\"section-kicker\">TraceScope Investigation Report</div>"
        << "<h1>"
        << escaped(snapshot.title)
        << "</h1>";

    if (!snapshot.context.trimmed().isEmpty()) {
        out
            << "<p class=\"report-context\">"
            << escaped(snapshot.context)
            << "</p>";
    }

    out
        << "<p class=\"muted\">Generated "
        << escaped(
               timestampText(
                   snapshot.generatedAtUtc
                   )
               )
        << "</p>";

    out
        << "<div class=\"metrics\">";

    appendMetric(
        out,
        QStringLiteral("Sources"),
        QString::number(
            snapshot.sessions.size()
            )
        );

    appendMetric(
        out,
        QStringLiteral("Comparisons"),
        QString::number(
            snapshot.comparisons.size()
            )
        );

    appendMetric(
        out,
        QStringLiteral("Chronology entries"),
        QString::number(
            snapshot.crossSourceChronology.size()
            )
        );

    appendMetric(
        out,
        QStringLiteral("Technical appendix"),
        snapshot.technicalAppendixIncluded
            ? QStringLiteral("Included")
            : QStringLiteral("Not included")
        );

    out
        << "</div>"
        << "</header>";

    /*
     * -------------------------------------------------
     * Source inventory
     * -------------------------------------------------
     */

    out
        << "<section class=\"report-section\" id=\"sources\">"
        << "<div class=\"section-kicker\">Investigation Overview</div>"
        << "<h2>Sources</h2>"
        << "<p class=\"muted\">"
        << "Local workstation source paths are intentionally "
           "omitted from the shareable report snapshot."
        << "</p>";

    if (snapshot.sessions.isEmpty()) {
        out
            << "<p>No full source-session sections were included.</p>";
    } else {
        out
            << "<div class=\"table-wrap\"><table>"
            << "<thead><tr>"
            << "<th>Source</th>"
            << "<th>Importer</th>"
            << "<th>Processed</th>"
            << "<th>Imported</th>"
            << "<th>Skipped</th>"
            << "<th>Truncated</th>"
            << "</tr></thead><tbody>";

        for (const InvestigationReportSessionSnapshot
                 &session : snapshot.sessions) {
            out
                << "<tr><td>"
                << escaped(
                       sourceDisplayName(
                           session.source
                           )
                       )
                << "</td><td>"
                << escaped(
                       session.source.importerId
                       )
                << "</td><td class=\"number\">"
                << session.source.processedRecordCount
                << "</td><td class=\"number\">"
                << session.source.importedRecordCount
                << "</td><td class=\"number\">"
                << session.source.skippedRecordCount
                << "</td><td>"
                << (
                       session.source.sourceTruncated
                           ? "Yes"
                           : "No"
                       )
                << "</td></tr>";
        }

        out
            << "</tbody></table></div>";
    }

    out
        << "</section>";

    /*
     * -------------------------------------------------
     * Cross-source time coverage
     * -------------------------------------------------
     */

    if (!snapshot.sourceTimeCoverage.isEmpty()) {
        out
            << "<section class=\"report-section\" id=\"time-coverage\">"
            << "<div class=\"section-kicker\">Cross-Source Context</div>"
            << "<h2>Time Coverage</h2>"
            << "<p class=\"note\">"
            << "Cross-source chronology reflects timestamps as "
               "imported by TraceScope. Clock synchronization or "
               "clock skew between independent sources is not inferred."
            << "</p>"
            << "<div class=\"table-wrap\"><table>"
            << "<thead><tr>"
            << "<th>Source</th>"
            << "<th>Timestamped records</th>"
            << "<th>First timestamp</th>"
            << "<th>Last timestamp</th>"
            << "</tr></thead><tbody>";

        for (
            const InvestigationReportSourceTimeCoverage
                &coverage
            : snapshot.sourceTimeCoverage
            ) {
            out
                << "<tr><td>"
                << escaped(
                       coverage.sourceName
                       )
                << "</td><td class=\"number\">"
                << coverage.timestampedRecordCount
                << "</td><td>"
                << escaped(
                       timestampText(
                           coverage.firstTimestamp
                           )
                       )
                << "</td><td>"
                << escaped(
                       timestampText(
                           coverage.lastTimestamp
                           )
                       )
                << "</td></tr>";
        }

        out
            << "</tbody></table></div>"
            << "</section>";
    }

    /*
     * -------------------------------------------------
     * Cross-source evidence chronology
     * -------------------------------------------------
     */

    if (!snapshot.crossSourceChronology.isEmpty()) {
        out
            << "<section class=\"report-section\" id=\"chronology\">"
            << "<div class=\"section-kicker\">Cross-Source Evidence</div>"
            << "<h2>Evidence Chronology</h2>"
            << "<p class=\"muted\">"
            << "Only timestamped report evidence appears here. "
               "Untimestamped evidence remains with its source section."
            << "</p>"
            << "<details>"
            << "<summary>Show "
            << snapshot.crossSourceChronology.size()
            << " chronology entries</summary>"
            << "<div class=\"details-body table-wrap\">"
            << "<table>"
            << "<thead><tr>"
            << "<th>Timestamp</th>"
            << "<th>Source</th>"
            << "<th>Severity</th>"
            << "<th>Message</th>"
            << "<th>Context</th>"
            << "</tr></thead><tbody>";

        for (
            const InvestigationReportChronologyEntry
                &entry
            : snapshot.crossSourceChronology
            ) {
            out
                << "<tr><td>"
                << escaped(
                       timestampText(
                           entry.timestamp
                           )
                       )
                << "</td><td>"
                << escaped(
                       entry.sourceName
                       )
                << "</td><td>"
                << escaped(
                       severityText(
                           entry.severity
                           )
                       )
                << "</td><td>"
                << escaped(
                       entry.message.has_value()
                           ? *entry.message
                           : QString()
                       )
                << "</td><td>"
                << escaped(
                       investigationStateText(
                           entry.state,
                           entry.burstEvidence
                           )
                       )
                << "</td></tr>";
        }

        out
            << "</tbody></table>"
            << "</div></details>"
            << "</section>";
    }

    /*
     * -------------------------------------------------
     * Immutable comparisons
     * -------------------------------------------------
     */

    for (int index = 0;
         index < snapshot.comparisons.size();
         ++index) {
        appendComparison(
            out,
            snapshot.comparisons.at(index),
            index
            );
    }

    /*
     * -------------------------------------------------
     * Per-source investigations
     * -------------------------------------------------
     */

    for (int index = 0;
         index < snapshot.sessions.size();
         ++index) {
        appendSession(
            out,
            snapshot.sessions.at(index),
            index,
            snapshot.supportingEvidenceIncluded,
            snapshot.technicalAppendixIncluded
            );
    }

    out
        << "<footer>"
        << "Generated by TraceScope. This report is a static "
           "capture of investigation state and deterministic "
           "analysis at export time."
        << "</footer>"
        << "</main>"
        << "</div>"
        << "</body></html>";

    return html;
}