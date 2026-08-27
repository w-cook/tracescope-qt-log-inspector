#include "InvestigationComparisonDocument.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QAbstractItemView>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QFileInfo>

#include "../../domain/RecordSeverity.h"

namespace
{

constexpr int MaximumVisibleTableRows = 10;

QString sourceNameFor(
    const InvestigationComparisonSourceSnapshot &source
    )
{
    const QString sourceName =
        source.sourceMetadata
            .sourceName
            .trimmed();

    if (!sourceName.isEmpty()) {
        return sourceName;
    }

    return source.sessionId;
}

QString compactSourceNameFor(
    const InvestigationComparisonSourceSnapshot &source
    )
{
    const QString sourceName =
        source.sourceMetadata
            .sourceName
            .trimmed();

    if (sourceName.isEmpty()) {
        return source.sessionId;
    }

    const QString baseName =
        QFileInfo(sourceName)
            .completeBaseName();

    return baseName.isEmpty()
               ? sourceName
               : baseName;
}

QString documentTitleFor(
    const InvestigationComparisonSnapshot &snapshot
    )
{
    return QStringLiteral(
               "%1 \u2192 %2"
               )
        .arg(
            compactSourceNameFor(
                snapshot.baselineSource()
                ),
            compactSourceNameFor(
                snapshot.comparisonSource()
                )
            );
}

QString formatSignedCount(
    qint64 value
    )
{
    if (value > 0) {
        return QStringLiteral("+%1")
        .arg(value);
    }

    return QString::number(value);
}

QString formatNumber(
    double value
    )
{
    return QString::number(
        value,
        'g',
        6
        );
}

QString formatRate(
    double value
    )
{
    return QString::number(
        value,
        'f',
        value < 10.0
            ? 2
            : 1
        );
}

QString formatSignedRate(
    double value
    )
{
    const QString formatted =
        formatRate(value);

    if (value > 0.0) {
        return QStringLiteral("+%1")
        .arg(formatted);
    }

    return formatted;
}

QString formatDurationMilliseconds(
    qint64 milliseconds
    )
{
    if (milliseconds < 1000) {
        return QStringLiteral("%1 ms")
        .arg(milliseconds);
    }

    if (milliseconds < 60 * 1000) {
        const double seconds =
            static_cast<double>(
                milliseconds
                )
            / 1000.0;

        return QStringLiteral("%1 s")
            .arg(
                seconds,
                0,
                'f',
                seconds < 10.0
                    ? 1
                    : 0
                );
    }

    if (milliseconds
        < 60 * 60 * 1000) {
        const double minutes =
            static_cast<double>(
                milliseconds
                )
            / static_cast<double>(
                60 * 1000
                );

        return QStringLiteral("%1 min")
            .arg(
                minutes,
                0,
                'f',
                minutes < 10.0
                    ? 1
                    : 0
                );
    }

    const double hours =
        static_cast<double>(
            milliseconds
            )
        / static_cast<double>(
            60 * 60 * 1000
            );

    return QStringLiteral("%1 h")
        .arg(
            hours,
            0,
            'f',
            hours < 10.0
                ? 1
                : 0
            );
}

QString formatSignedDuration(
    qint64 milliseconds
    )
{
    if (milliseconds == 0) {
        return formatDurationMilliseconds(
            0
            );
    }

    const bool positive =
        milliseconds > 0;

    const qint64 magnitude =
        positive
            ? milliseconds
            : -milliseconds;

    return QStringLiteral("%1%2")
        .arg(
            positive
                ? QStringLiteral("+")
                : QStringLiteral("-"),
            formatDurationMilliseconds(
                magnitude
                )
            );
}

QString formatTimestamp(
    const QDateTime &timestamp
    )
{
    if (!timestamp.isValid()) {
        return QStringLiteral("Unavailable");
    }

    return timestamp
        .toUTC()
        .toString(
            Qt::ISODate
            );
}

QString formatNumericSummary(
    const InvestigationNumericFieldSummary &summary
    )
{
    return QStringLiteral(
               "min %1   median %2   max %3   (n=%4)"
               )
        .arg(
            formatNumber(
                summary.minimum
                ),
            formatNumber(
                summary.median
                ),
            formatNumber(
                summary.maximum
                ),
            QString::number(
                summary.populatedRecordCount
                )
            );
}

QString formatDominantValue(
    const std::optional<InvestigationValueFrequency>
        &value
    )
{
    if (!value.has_value()) {
        return QStringLiteral("\u2014");
    }

    return QStringLiteral("%1 (%2)")
        .arg(
            value->value,
            QString::number(
                value->count
                )
            );
}

QLabel *makeHeading(
    const QString &text,
    QWidget *parent
    )
{
    auto *label =
        new QLabel(
            text,
            parent
            );

    QFont font =
        label->font();

    font.setBold(
        true
        );

    if (font.pointSizeF() > 0.0) {
        font.setPointSizeF(
            font.pointSizeF()
            + 2.0
            );
    }

    label->setFont(
        font
        );

    return label;
}

QLabel *makeUnavailableLabel(
    const QString &text,
    QWidget *parent
    )
{
    auto *label =
        new QLabel(
            text,
            parent
            );

    label->setWordWrap(
        true
        );

    return label;
}

QTableWidget *makeTable(
    const QStringList &headers,
    QWidget *parent
    )
{
    auto *table =
        new QTableWidget(
            0,
            headers.size(),
            parent
            );

    table->setHorizontalHeaderLabels(
        headers
        );

    table->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    table->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    table->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    table->setAlternatingRowColors(
        true
        );

    table->setWordWrap(
        false
        );

    table->verticalHeader()
        ->setVisible(
            false
            );

    table->verticalHeader()
        ->setDefaultSectionSize(
            24
            );

    table->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
            );

    if (!headers.isEmpty()) {
        table->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
            );
    }

    return table;
}

void fitTableHeight(
    QTableWidget *table
    )
{
    if (table == nullptr) {
        return;
    }

    const int visibleRows =
        std::min(
            table->rowCount(),
            MaximumVisibleTableRows
            );

    const int rowHeight =
        table->verticalHeader()
            ->defaultSectionSize();

    int height =
        table->horizontalHeader()
            ->sizeHint()
            .height();

    height +=
        visibleRows
        * rowHeight;

    height +=
        table->frameWidth()
        * 2;

    /*
     * Leave room for a horizontal scrollbar when
     * a detached/narrow comparison window cannot
     * display every column at once.
     */
    height +=
        table->horizontalScrollBar()
            ->sizeHint()
            .height();

    table->setMinimumHeight(
        height
        );

    table->setMaximumHeight(
        height
        );
}

QTableWidgetItem *countItem(
    qint64 value
    )
{
    auto *item =
        new QTableWidgetItem(
            QString::number(value)
            );

    item->setTextAlignment(
        Qt::AlignRight
        | Qt::AlignVCenter
        );

    return item;
}

QTableWidgetItem *deltaItem(
    qint64 value
    )
{
    auto *item =
        new QTableWidgetItem(
            formatSignedCount(value)
            );

    item->setTextAlignment(
        Qt::AlignRight
        | Qt::AlignVCenter
        );

    return item;
}

void appendValueDifferenceRow(
    QTableWidget *table,
    const InvestigationValueDifference &difference
    )
{
    const int row =
        table->rowCount();

    table->insertRow(
        row
        );

    table->setItem(
        row,
        0,
        new QTableWidgetItem(
            difference.value
            )
        );

    table->setItem(
        row,
        1,
        countItem(
            difference.baselineCount
            )
        );

    table->setItem(
        row,
        2,
        countItem(
            difference.comparisonCount
            )
        );

    table->setItem(
        row,
        3,
        deltaItem(
            difference.delta()
            )
        );
}

QGroupBox *makeSourcesGroup(
    const InvestigationComparisonSnapshot &snapshot,
    QWidget *parent
    )
{
    auto *group =
        new QGroupBox(
            QStringLiteral(
                "Comparison Sources"
                ),
            parent
            );

    auto layoutOwner =
        std::make_unique<QGridLayout>();

    QGridLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    const auto addSource =
        [
            layout,
            group
    ](
            int column,
            const QString &role,
            const InvestigationComparisonSourceSnapshot
                &source
            ) {
            auto *roleLabel =
                new QLabel(
                    role,
                    group
                    );

            QFont roleFont =
                roleLabel->font();

            roleFont.setBold(
                true
                );

            roleLabel->setFont(
                roleFont
                );

            auto *nameLabel =
                new QLabel(
                    sourceNameFor(source),
                    group
                    );

            nameLabel->setTextInteractionFlags(
                Qt::TextSelectableByMouse
                );

            if (!source.sourceMetadata
                     .sourcePath
                     .isEmpty()) {
                nameLabel->setToolTip(
                    source.sourceMetadata
                        .sourcePath
                    );
            }

            QString importedText =
                QStringLiteral(
                    "Imported: %1"
                    )
                    .arg(
                        formatTimestamp(
                            source
                                .sourceMetadata
                                .importedAtUtc
                            )
                        );

            auto *importedLabel =
                new QLabel(
                    importedText,
                    group
                    );

            layout->addWidget(
                roleLabel,
                0,
                column
                );

            layout->addWidget(
                nameLabel,
                1,
                column
                );

            layout->addWidget(
                importedLabel,
                2,
                column
                );

            layout->setColumnStretch(
                column,
                1
                );
        };

    addSource(
        0,
        QStringLiteral("Baseline"),
        snapshot.baselineSource()
        );

    addSource(
        1,
        QStringLiteral("Comparison"),
        snapshot.comparisonSource()
        );

    return group;
}

QGroupBox *makeEventCodeGroup(
    const InvestigationDimensionComparison &comparison,
    QWidget *parent
    )
{
    auto *group =
        new QGroupBox(
            QStringLiteral("Event Codes"),
            parent
            );

    auto layoutOwner =
        std::make_unique<QGridLayout>();

    QGridLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    if (!comparison.comparable()) {
        layout->addWidget(
            makeUnavailableLabel(
                QStringLiteral(
                    "Unavailable — event-code data is not "
                    "populated in both sessions."
                    ),
                group
                )
            );

        return group;
    }

    QVector<InvestigationValueDifference>
        appeared;

    QVector<InvestigationValueDifference>
        disappeared;

    QVector<InvestigationValueDifference>
        changed;

    for (const InvestigationValueDifference &difference
         : comparison.differences) {
        if (difference
                .appearsOnlyInComparison()) {
            appeared.append(
                difference
                );

            continue;
        }

        if (difference
                .appearsOnlyInBaseline()) {
            disappeared.append(
                difference
                );

            continue;
        }

        changed.append(
            difference
            );
    }

    const auto addDifferenceTable =
        [
            layout,
            group
    ](
            const QString &title,
            const QVector<InvestigationValueDifference>
                &differences
            ) {
            if (differences.isEmpty()) {
                return;
            }

            auto *label =
                new QLabel(
                    title,
                    group
                    );

            QFont font =
                label->font();

            font.setBold(
                true
                );

            label->setFont(
                font
                );

            layout->addWidget(
                label
                );

            QTableWidget *table =
                makeTable(
                    {
                        QStringLiteral(
                            "Event Code"
                            ),
                        QStringLiteral(
                            "Baseline"
                            ),
                        QStringLiteral(
                            "Comparison"
                            ),
                        QStringLiteral(
                            "Delta"
                            )
                    },
                    group
                    );

            for (const InvestigationValueDifference
                     &difference
                 : differences) {
                appendValueDifferenceRow(
                    table,
                    difference
                    );
            }

            fitTableHeight(
                table
                );

            layout->addWidget(
                table
                );
        };

    addDifferenceTable(
        QStringLiteral("Appeared"),
        appeared
        );

    addDifferenceTable(
        QStringLiteral("Disappeared"),
        disappeared
        );

    addDifferenceTable(
        QStringLiteral("Changed"),
        changed
        );

    return group;
}

QGroupBox *makeSeverityGroup(
    const InvestigationSeverityComparison &comparison,
    QWidget *parent
    )
{
    auto *group =
        new QGroupBox(
            QStringLiteral("Severity"),
            parent
            );

    auto layoutOwner =
        std::make_unique<QGridLayout>();

    QGridLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    if (!comparison.comparable()) {
        layout->addWidget(
            makeUnavailableLabel(
                QStringLiteral(
                    "Unavailable — severity data is not "
                    "populated in both sessions."
                    ),
                group
                )
            );

        return group;
    }

    QTableWidget *table =
        makeTable(
            {
                QStringLiteral("Severity"),
                QStringLiteral("Baseline"),
                QStringLiteral("Comparison"),
                QStringLiteral("Delta")
            },
            group
            );

    for (const InvestigationSeverityDifference
             &difference
         : comparison.differences) {
        const int row =
            table->rowCount();

        table->insertRow(
            row
            );

        table->setItem(
            row,
            0,
            new QTableWidgetItem(
                recordSeverityToString(
                    difference.severity
                    )
                )
            );

        table->setItem(
            row,
            1,
            countItem(
                difference.baselineCount
                )
            );

        table->setItem(
            row,
            2,
            countItem(
                difference.comparisonCount
                )
            );

        table->setItem(
            row,
            3,
            deltaItem(
                difference.delta()
                )
            );
    }

    fitTableHeight(
        table
        );

    layout->addWidget(
        table
        );

    return group;
}

QGroupBox *makeDimensionGroup(
    const QString &title,
    const QString &valueHeader,
    const QString &unavailableText,
    const InvestigationDimensionComparison &comparison,
    QWidget *parent
    )
{
    auto *group =
        new QGroupBox(
            title,
            parent
            );

    auto layoutOwner =
        std::make_unique<QGridLayout>();

    QGridLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    if (!comparison.comparable()) {
        layout->addWidget(
            makeUnavailableLabel(
                unavailableText,
                group
                )
            );

        return group;
    }

    QTableWidget *table =
        makeTable(
            {
                valueHeader,
                QStringLiteral("Baseline"),
                QStringLiteral("Comparison"),
                QStringLiteral("Delta")
            },
            group
            );

    for (const InvestigationValueDifference &difference
         : comparison.differences) {
        appendValueDifferenceRow(
            table,
            difference
            );
    }

    fitTableHeight(
        table
        );

    layout->addWidget(
        table
        );

    return group;
}

QGroupBox *makeCustomFieldsGroup(
    const InvestigationCustomFieldComparison &comparison,
    QWidget *parent
    )
{
    if (comparison
            .categoricalFields
            .isEmpty()
        && comparison
               .numericFields
               .isEmpty()) {
        return nullptr;
    }

    auto *group =
        new QGroupBox(
            QStringLiteral("Custom Fields"),
            parent
            );

    auto layoutOwner =
        std::make_unique<QVBoxLayout>();

    QVBoxLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    if (!comparison
             .categoricalFields
             .isEmpty()) {
        auto *label =
            new QLabel(
                QStringLiteral(
                    "Appeared / Disappeared Values"
                    ),
                group
                );

        QFont font =
            label->font();

        font.setBold(
            true
            );

        label->setFont(
            font
            );

        layout->addWidget(
            label
            );

        QTableWidget *table =
            makeTable(
                {
                    QStringLiteral("Field"),
                    QStringLiteral("Change"),
                    QStringLiteral("Value"),
                    QStringLiteral("Baseline"),
                    QStringLiteral("Comparison")
                },
                group
                );

        for (
            const InvestigationCategoricalCustomFieldComparison
                &field
            : comparison.categoricalFields
            ) {
            for (
                const InvestigationValueDifference
                    &difference
                : field.changedValues
                ) {
                const int row =
                    table->rowCount();

                table->insertRow(
                    row
                    );

                table->setItem(
                    row,
                    0,
                    new QTableWidgetItem(
                        field.fieldName
                        )
                    );

                const QString change =
                    difference
                            .appearsOnlyInComparison()
                        ? QStringLiteral(
                              "Appeared"
                              )
                        : QStringLiteral(
                              "Disappeared"
                              );

                table->setItem(
                    row,
                    1,
                    new QTableWidgetItem(
                        change
                        )
                    );

                table->setItem(
                    row,
                    2,
                    new QTableWidgetItem(
                        difference.value
                        )
                    );

                table->setItem(
                    row,
                    3,
                    countItem(
                        difference.baselineCount
                        )
                    );

                table->setItem(
                    row,
                    4,
                    countItem(
                        difference.comparisonCount
                        )
                    );
            }
        }

        fitTableHeight(
            table
            );

        layout->addWidget(
            table
            );
    }

    if (!comparison
             .numericFields
             .isEmpty()) {
        auto *label =
            new QLabel(
                QStringLiteral(
                    "Numeric Summaries"
                    ),
                group
                );

        QFont font =
            label->font();

        font.setBold(
            true
            );

        label->setFont(
            font
            );

        layout->addWidget(
            label
            );

        QTableWidget *table =
            makeTable(
                {
                    QStringLiteral("Field"),
                    QStringLiteral("Baseline"),
                    QStringLiteral("Comparison")
                },
                group
                );

        table->horizontalHeader()
            ->setSectionResizeMode(
                1,
                QHeaderView::Stretch
                );

        table->horizontalHeader()
            ->setSectionResizeMode(
                2,
                QHeaderView::Stretch
                );

        for (
            const InvestigationNumericCustomFieldComparison
                &field
            : comparison.numericFields
            ) {
            const int row =
                table->rowCount();

            table->insertRow(
                row
                );

            table->setItem(
                row,
                0,
                new QTableWidgetItem(
                    field.fieldName
                    )
                );

            table->setItem(
                row,
                1,
                new QTableWidgetItem(
                    formatNumericSummary(
                        field.baseline
                        )
                    )
                );

            table->setItem(
                row,
                2,
                new QTableWidgetItem(
                    formatNumericSummary(
                        field.comparison
                        )
                    )
                );
        }

        fitTableHeight(
            table
            );

        layout->addWidget(
            table
            );
    }

    return group;
}

QString formatBurstSettings(
    const BurstDetectionSettings &settings
    )
{
    return QStringLiteral(
               "Shared settings: window %1; elevated threshold %2; "
               "Error/Critical threshold %3; merge gap %4"
               )
        .arg(
            formatDurationMilliseconds(
                settings.windowMilliseconds
                ),
            QString::number(
                settings.elevatedEventThreshold
                ),
            QString::number(
                settings.errorCriticalThreshold
                ),
            formatDurationMilliseconds(
                settings.mergeGapMilliseconds
                )
            );
}

QGroupBox *makeBurstGroup(
    const InvestigationComparisonSnapshot &snapshot,
    QWidget *parent
    )
{
    if (!snapshot
             .burstComparisonRequested()) {
        return nullptr;
    }

    auto *group =
        new QGroupBox(
            QStringLiteral("Burst Comparison"),
            parent
            );

    auto layoutOwner =
        std::make_unique<QGridLayout>();

    QGridLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    if (snapshot
            .requestedBurstSettings()
            .has_value()) {
        auto *settingsLabel =
            new QLabel(
                formatBurstSettings(
                    snapshot
                        .requestedBurstSettings()
                        .value()
                    ),
                group
                );

        settingsLabel->setWordWrap(
            true
            );

        layout->addWidget(
            settingsLabel
            );
    }

    if (!snapshot
             .analysis()
             .bursts
             .has_value()) {
        layout->addWidget(
            makeUnavailableLabel(
                QStringLiteral(
                    "Unavailable — the requested shared burst "
                    "settings were not valid."
                    ),
                group
                )
            );

        return group;
    }

    const InvestigationBurstComparison &comparison =
        snapshot
            .analysis()
            .bursts
            .value();

    if (!comparison.comparable()) {
        layout->addWidget(
            makeUnavailableLabel(
                QStringLiteral(
                    "Unavailable — both sessions require usable "
                    "timestamp and severity data for burst comparison."
                    ),
                group
                )
            );

        return group;
    }

    QTableWidget *table =
        makeTable(
            {
                QStringLiteral("Metric"),
                QStringLiteral("Baseline"),
                QStringLiteral("Comparison")
            },
            group
            );

    const auto appendRow =
        [
            table
    ](
            const QString &metric,
            const QString &baseline,
            const QString &comparisonValue
            ) {
            const int row =
                table->rowCount();

            table->insertRow(
                row
                );

            table->setItem(
                row,
                0,
                new QTableWidgetItem(
                    metric
                    )
                );

            table->setItem(
                row,
                1,
                new QTableWidgetItem(
                    baseline
                    )
                );

            table->setItem(
                row,
                2,
                new QTableWidgetItem(
                    comparisonValue
                    )
                );
        };

    appendRow(
        QStringLiteral("Detected Bursts"),
        QString::number(
            comparison
                .baseline
                .burstCount
            ),
        QString::number(
            comparison
                .comparison
                .burstCount
            )
        );

    appendRow(
        QStringLiteral(
            "Elevated Records in Bursts"
            ),
        QString::number(
            comparison
                .baseline
                .elevatedRecordCountInBursts
            ),
        QString::number(
            comparison
                .comparison
                .elevatedRecordCountInBursts
            )
        );

    appendRow(
        QStringLiteral(
            "Peak Burst Elevated Count"
            ),
        QString::number(
            comparison
                .baseline
                .peakBurstElevatedCount
            ),
        QString::number(
            comparison
                .comparison
                .peakBurstElevatedCount
            )
        );

    appendRow(
        QStringLiteral(
            "Longest Burst"
            ),
        formatDurationMilliseconds(
            comparison
                .baseline
                .longestBurstDurationMilliseconds
            ),
        formatDurationMilliseconds(
            comparison
                .comparison
                .longestBurstDurationMilliseconds
            )
        );

    appendRow(
        QStringLiteral(
            "Dominant Subsystem"
            ),
        formatDominantValue(
            comparison
                .baseline
                .dominantSubsystem
            ),
        formatDominantValue(
            comparison
                .comparison
                .dominantSubsystem
            )
        );

    appendRow(
        QStringLiteral(
            "Dominant Event Code"
            ),
        formatDominantValue(
            comparison
                .baseline
                .dominantEventCode
            ),
        formatDominantValue(
            comparison
                .comparison
                .dominantEventCode
            )
        );

    appendRow(
        QStringLiteral(
            "Dominant Entity"
            ),
        formatDominantValue(
            comparison
                .baseline
                .dominantEntity
            ),
        formatDominantValue(
            comparison
                .comparison
                .dominantEntity
            )
        );

    fitTableHeight(
        table
        );

    layout->addWidget(
        table
        );

    return group;
}

QGroupBox *makeSessionContextGroup(
    const InvestigationSessionComparison &comparison,
    QWidget *parent
    )
{
    auto *group =
        new QGroupBox(
            QStringLiteral("Session Context"),
            parent
            );

    auto layoutOwner =
        std::make_unique<QVBoxLayout>();

    QVBoxLayout *layout =
        layoutOwner.get();

    group->setLayout(
        layoutOwner.release()
        );

    QTableWidget *table =
        makeTable(
            {
                QStringLiteral("Metric"),
                QStringLiteral("Baseline"),
                QStringLiteral("Comparison"),
                QStringLiteral("Delta")
            },
            group
            );

    const auto appendRow =
        [
            table
    ](
            const QString &metric,
            const QString &baseline,
            const QString &comparisonValue,
            const QString &delta
            ) {
            const int row =
                table->rowCount();

            table->insertRow(
                row
                );

            table->setItem(
                row,
                0,
                new QTableWidgetItem(
                    metric
                    )
                );

            table->setItem(
                row,
                1,
                new QTableWidgetItem(
                    baseline
                    )
                );

            table->setItem(
                row,
                2,
                new QTableWidgetItem(
                    comparisonValue
                    )
                );

            table->setItem(
                row,
                3,
                new QTableWidgetItem(
                    delta
                    )
                );
        };

    appendRow(
        QStringLiteral("Total Records"),
        QString::number(
            comparison
                .totalRecords
                .baselineCount
            ),
        QString::number(
            comparison
                .totalRecords
                .comparisonCount
            ),
        formatSignedCount(
            comparison
                .totalRecords
                .delta()
            )
        );

    const bool durationComparable =
        comparison
            .baselineTiming
            .available()
        && comparison
               .comparisonTiming
               .available();

    appendRow(
        QStringLiteral("Duration"),
        comparison
                .baselineTiming
                .available()
            ? formatDurationMilliseconds(
                  comparison
                      .baselineTiming
                      .durationMilliseconds
                  )
            : QStringLiteral(
                  "Unavailable"
                  ),
        comparison
                .comparisonTiming
                .available()
            ? formatDurationMilliseconds(
                  comparison
                      .comparisonTiming
                      .durationMilliseconds
                  )
            : QStringLiteral(
                  "Unavailable"
                  ),
        durationComparable
            ? formatSignedDuration(
                  comparison
                      .comparisonTiming
                      .durationMilliseconds
                  - comparison
                        .baselineTiming
                        .durationMilliseconds
                  )
            : QStringLiteral(
                  "Unavailable"
                  )
        );

    const bool rateComparable =
        comparison
            .baselineTiming
            .rateAvailable()
        && comparison
               .comparisonTiming
               .rateAvailable();

    appendRow(
        QStringLiteral("Records / Minute"),
        comparison
                .baselineTiming
                .rateAvailable()
            ? formatRate(
                  comparison
                      .baselineTiming
                      .recordsPerMinute
                      .value()
                  )
            : QStringLiteral(
                  "Unavailable"
                  ),
        comparison
                .comparisonTiming
                .rateAvailable()
            ? formatRate(
                  comparison
                      .comparisonTiming
                      .recordsPerMinute
                      .value()
                  )
            : QStringLiteral(
                  "Unavailable"
                  ),
        rateComparable
            ? formatSignedRate(
                  comparison
                      .comparisonTiming
                      .recordsPerMinute
                      .value()
                  - comparison
                        .baselineTiming
                        .recordsPerMinute
                        .value()
                  )
            : QStringLiteral(
                  "Unavailable"
                  )
        );

    fitTableHeight(
        table
        );

    layout->addWidget(
        table
        );

    return group;
}

}

InvestigationComparisonDocument::
    InvestigationComparisonDocument(
        InvestigationComparisonSnapshot snapshot,
        QWidget *parent
        )
    : WorkspaceDocument(
          snapshot.id(),
          documentTitleFor(
              snapshot
              ),
          parent
          ),
    m_snapshot(
        std::move(snapshot)
        )
{
    auto *outerLayout =
        new QVBoxLayout(
            this
            );

    outerLayout->setContentsMargins(
        6,
        4,
        6,
        6
        );

    auto *scrollArea =
        new QScrollArea(
            this
            );

    scrollArea->setWidgetResizable(
        true
        );

    scrollArea->setFrameShape(
        QFrame::NoFrame
        );

    auto *content =
        new QWidget(
            scrollArea
            );

    auto *contentLayout =
        new QVBoxLayout(
            content
            );

    contentLayout->setContentsMargins(
        4,
        4,
        4,
        4
        );

    contentLayout->setSpacing(
        8
        );

    contentLayout->addWidget(
        makeSourcesGroup(
            m_snapshot,
            content
            )
        );

    auto *orientationLabel =
        new QLabel(
            QStringLiteral(
                "All deltas are Comparison \u2212 Baseline. "
                "This document is an immutable snapshot of the "
                "complete imported sessions."
                ),
            content
            );

    orientationLabel->setWordWrap(
        true
        );

    contentLayout->addWidget(
        orientationLabel
        );

    contentLayout->addWidget(
        makeHeading(
            QStringLiteral(
                "Key Differences"
                ),
            content
            )
        );

    bool hasDisplayedDifference =
        false;

    const InvestigationSessionComparison &analysis =
        m_snapshot.analysis();

    /*
     * Event codes.
     *
     * An unavailable dimension is displayed
     * explicitly. A comparable dimension with no
     * differences is omitted.
     */
    if (!analysis
             .eventCodes
             .comparable()
        || !analysis
                .eventCodes
                .differences
                .isEmpty()) {
        contentLayout->addWidget(
            makeEventCodeGroup(
                analysis.eventCodes,
                content
                )
            );

        hasDisplayedDifference =
            true;
    }

    if (!analysis
             .severity
             .comparable()
        || !analysis
                .severity
                .differences
                .isEmpty()) {
        contentLayout->addWidget(
            makeSeverityGroup(
                analysis.severity,
                content
                )
            );

        hasDisplayedDifference =
            true;
    }

    if (!analysis
             .elevatedSubsystems
             .comparable()
        || !analysis
                .elevatedSubsystems
                .differences
                .isEmpty()) {
        contentLayout->addWidget(
            makeDimensionGroup(
                QStringLiteral(
                    "Elevated Activity by Subsystem"
                    ),
                QStringLiteral(
                    "Subsystem"
                    ),
                QStringLiteral(
                    "Unavailable — subsystem and severity data "
                    "are not populated sufficiently in both sessions."
                    ),
                analysis.elevatedSubsystems,
                content
                )
            );

        hasDisplayedDifference =
            true;
    }

    if (!analysis
             .elevatedEntities
             .comparable()
        || !analysis
                .elevatedEntities
                .differences
                .isEmpty()) {
        contentLayout->addWidget(
            makeDimensionGroup(
                QStringLiteral(
                    "Elevated Activity by Entity"
                    ),
                QStringLiteral(
                    "Entity"
                    ),
                QStringLiteral(
                    "Unavailable — entity and severity data "
                    "are not populated sufficiently in both sessions."
                    ),
                analysis.elevatedEntities,
                content
                )
            );

        hasDisplayedDifference =
            true;
    }

    if (QGroupBox *customFields =
        makeCustomFieldsGroup(
            analysis.customFields,
            content
            );
        customFields != nullptr) {
        contentLayout->addWidget(
            customFields
            );

        hasDisplayedDifference =
            true;
    }

    if (!hasDisplayedDifference) {
        auto *noDifferencesLabel =
            new QLabel(
                QStringLiteral(
                    "No key differences were detected in the "
                    "comparable investigation dimensions."
                    ),
                content
                );

        noDifferencesLabel->setWordWrap(
            true
            );

        contentLayout->addWidget(
            noDifferencesLabel
            );
    }

    if (QGroupBox *bursts =
        makeBurstGroup(
            m_snapshot,
            content
            );
        bursts != nullptr) {
        contentLayout->addWidget(
            makeHeading(
                QStringLiteral(
                    "Burst Comparison"
                    ),
                content
                )
            );

        contentLayout->addWidget(
            bursts
            );
    }

    contentLayout->addWidget(
        makeHeading(
            QStringLiteral(
                "Session Context"
                ),
            content
            )
        );

    contentLayout->addWidget(
        makeSessionContextGroup(
            analysis,
            content
            )
        );

    contentLayout->addStretch(
        1
        );

    scrollArea->setWidget(
        content
        );

    outerLayout->addWidget(
        scrollArea
        );
}

const InvestigationComparisonSnapshot &
    InvestigationComparisonDocument::
    snapshot() const
{
    return m_snapshot;
}