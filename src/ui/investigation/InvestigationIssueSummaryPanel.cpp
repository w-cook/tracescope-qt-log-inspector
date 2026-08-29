#include "InvestigationIssueSummaryPanel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include <algorithm>

#include "../../analysis/TelemetryIssueGroup.h"

InvestigationIssueSummaryPanel::
    InvestigationIssueSummaryPanel(
        QWidget *parent
        )
    : QGroupBox(
          tr("Grouped Warnings and Errors"),
          parent
          ),
    m_table(
        new QTableWidget(
            0,
            4,
            this
            )
        )
{
    m_table->setHorizontalHeaderLabels({
        tr("Subsystem"),
        tr("Warnings"),
        tr("Errors"),
        tr("Total")
    });

    m_table
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
            );

    m_table
        ->horizontalHeader()
        ->setStretchLastSection(
            false
            );

    m_table->setAlternatingRowColors(
        true
        );

    m_table->setSelectionBehavior(
        QAbstractItemView::SelectItems
        );

    m_table->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_table->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_table->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAsNeeded
        );

    m_table->setToolTip(
        tr(
            "Double-click a summary value to filter "
            "the investigation to the represented "
            "issues."
            )
        );

    connect(
        m_table,
        &QTableWidget::cellDoubleClicked,
        this,
        [this](
            int row,
            int column
            ) {
            handleCellDoubleClicked(
                row,
                column
                );
        },
        Qt::QueuedConnection
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->addWidget(
        m_table
        );

    updateMinimumTableWidth();
}

void InvestigationIssueSummaryPanel::
    updateRecords(
        const QVector<InvestigationRecord> &records
        )
{
    const QVector<TelemetryIssueGroup> groups =
        m_analyzer
            .groupWarningsAndErrorsBySubsystem(
                records
                );

    m_table->setRowCount(
        groups.size()
        );

    for (
        int row = 0;
        row < groups.size();
        ++row
        ) {
        const TelemetryIssueGroup &group =
            groups.at(row);

        auto *subsystemItem =
            new QTableWidgetItem(
                group.subsystem
                );

        auto *warningItem =
            new QTableWidgetItem(
                QString::number(
                    group.warningCount
                    )
                );

        auto *errorItem =
            new QTableWidgetItem(
                QString::number(
                    group.errorCount
                    )
                );

        auto *totalItem =
            new QTableWidgetItem(
                QString::number(
                    group.totalCount()
                    )
                );

        /*
         * The analyzer uses this presentation value
         * for records without a subsystem. The exact
         * subsystem filter cannot currently express
         * "missing subsystem", so no drill-down should
         * be advertised for this row.
         */
        if (group.subsystem
            == QStringLiteral(
                "(No subsystem)"
                )) {
            const QString unavailableToolTip =
                tr(
                    "Drill-down is unavailable because "
                    "these records do not contain a "
                    "subsystem value."
                    );

            subsystemItem->setToolTip(
                unavailableToolTip
                );

            warningItem->setToolTip(
                unavailableToolTip
                );

            errorItem->setToolTip(
                unavailableToolTip
                );

            totalItem->setToolTip(
                unavailableToolTip
                );
        } else {
            subsystemItem->setToolTip(
                tr(
                    "Double-click to show "
                    "warning/error-class events "
                    "for this subsystem."
                    )
                );

            warningItem->setToolTip(
                group.warningCount > 0
                    ? tr(
                          "Double-click to show "
                          "warnings for this subsystem."
                          )
                    : tr(
                          "No warnings are currently "
                          "visible for this subsystem."
                          )
                );

            errorItem->setToolTip(
                group.errorCount > 0
                    ? tr(
                          "Double-click to show errors "
                          "and critical events for this "
                          "subsystem."
                          )
                    : tr(
                          "No errors or critical events "
                          "are currently visible for "
                          "this subsystem."
                          )
                );

            totalItem->setToolTip(
                tr(
                    "Double-click to show all "
                    "warning/error-class events "
                    "for this subsystem."
                    )
                );
        }

        m_table->setItem(
            row,
            0,
            subsystemItem
            );

        m_table->setItem(
            row,
            1,
            warningItem
            );

        m_table->setItem(
            row,
            2,
            errorItem
            );

        m_table->setItem(
            row,
            3,
            totalItem
            );
    }

    m_table
        ->horizontalHeader()
        ->resizeSections(
            QHeaderView::ResizeToContents
            );

    updateMinimumTableWidth();
}

void InvestigationIssueSummaryPanel::clear()
{
    m_table->setRowCount(
        0
        );

    updateMinimumTableWidth();
}

int InvestigationIssueSummaryPanel::
    preferredCompactWidth() const
{
    const int preferredTableWidth =
        m_table
            ->verticalHeader()
            ->width()
        + m_table
              ->horizontalHeader()
              ->length()
        + m_table
                  ->frameWidth()
              * 2
        + m_table
              ->verticalScrollBar()
              ->sizeHint()
              .width()
        + 8;

    return preferredTableWidth
           + 30;
}

InvestigationTablePresentationState
    InvestigationIssueSummaryPanel::
    capturePresentationState() const
{
    InvestigationTablePresentationState
        state;

    if (m_table == nullptr) {
        return state;
    }

    const QModelIndex currentIndex =
        m_table->currentIndex();

    if (currentIndex.isValid()) {
        state.currentRow =
            currentIndex.row();

        state.currentColumn =
            currentIndex.column();
    }

    if (QScrollBar *horizontal =
        m_table->horizontalScrollBar();
        horizontal != nullptr) {
        state.scroll.horizontalValue =
            horizontal->value();
    }

    if (QScrollBar *vertical =
        m_table->verticalScrollBar();
        vertical != nullptr) {
        state.scroll.verticalValue =
            vertical->value();
    }

    return state;
}

void InvestigationIssueSummaryPanel::
    restorePresentationState(
        const InvestigationTablePresentationState
            &state
        )
{
    if (m_table == nullptr) {
        return;
    }

    m_table->clearSelection();

    if (state.currentRow >= 0
        && state.currentRow
               < m_table->rowCount()
        && state.currentColumn >= 0
        && state.currentColumn
               < m_table->columnCount()) {
        m_table->setCurrentCell(
            state.currentRow,
            state.currentColumn
            );
    }

    if (QScrollBar *horizontal =
        m_table->horizontalScrollBar();
        horizontal != nullptr) {
        horizontal->setValue(
            std::clamp(
                state.scroll.horizontalValue,
                horizontal->minimum(),
                horizontal->maximum()
                )
            );
    }

    if (QScrollBar *vertical =
        m_table->verticalScrollBar();
        vertical != nullptr) {
        vertical->setValue(
            std::clamp(
                state.scroll.verticalValue,
                vertical->minimum(),
                vertical->maximum()
                )
            );
    }
}

void InvestigationIssueSummaryPanel::
    handleCellDoubleClicked(
        int row,
        int column
        )
{
    if (row < 0
        || row >= m_table->rowCount()
        || column < 0
        || column >= m_table->columnCount()) {
        return;
    }

    QTableWidgetItem *subsystemItem =
        m_table->item(
            row,
            0
            );

    if (subsystemItem == nullptr) {
        return;
    }

    const QString subsystem =
        subsystemItem->text();

    /*
     * The current exact-value subsystem filter
     * cannot represent a missing subsystem.
     */
    if (subsystem
        == QStringLiteral(
            "(No subsystem)"
            )) {
        return;
    }

    InvestigationIssueDrillDownType type;

    switch (column) {
    case 0:
    case 3:
        type =
            InvestigationIssueDrillDownType::
            AllElevated;
        break;

    case 1:
    {
        QTableWidgetItem *warningItem =
            m_table->item(
                row,
                1
                );

        if (warningItem == nullptr
            || warningItem
                       ->text()
                       .toInt() <= 0) {
            return;
        }

        type =
            InvestigationIssueDrillDownType::
            Warnings;

        break;
    }

    case 2:
    {
        QTableWidgetItem *errorItem =
            m_table->item(
                row,
                2
                );

        if (errorItem == nullptr
            || errorItem
                       ->text()
                       .toInt() <= 0) {
            return;
        }

        type =
            InvestigationIssueDrillDownType::
            Errors;

        break;
    }

    default:
        return;
    }

    emit drillDownRequested(
        subsystem,
        type
        );
}

void InvestigationIssueSummaryPanel::
    updateMinimumTableWidth()
{
    m_table
        ->horizontalHeader()
        ->resizeSections(
            QHeaderView::ResizeToContents
            );

    /*
     * Column contents should influence the preferred
     * compact width, but must not become a hard
     * minimum for the entire workspace document.
     *
     * In narrow or detached windows the table may
     * scroll horizontally instead.
     */
    m_table->setMinimumWidth(
        0
        );
}