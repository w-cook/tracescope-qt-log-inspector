#include "InvestigationEventPanel.h"

#include <algorithm>
#include <utility>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableView>
#include <QVBoxLayout>

#include "../../controllers/InvestigationController.h"
#include "../../models/InvestigationFilterProxyModel.h"
#include "../../models/InvestigationTableModel.h"
#include "../../workspace/InvestigationSession.h"

InvestigationEventPanel::
    InvestigationEventPanel(
        QWidget *parent
        )
    : QGroupBox(
          tr("Telemetry Events"),
          parent
          ),
    m_table(
        new QTableView(this)
        ),
    m_previousEventButton(
        new QPushButton(
            tr("Previous Event"),
            this
            )
        ),
    m_nextEventButton(
        new QPushButton(
            tr("Next Event"),
            this
            )
        ),
    m_eventPositionLabel(
        new QLabel(this)
        ),
    m_previousIssueButton(
        new QPushButton(
            tr("Previous Issue"),
            this
            )
        ),
    m_nextIssueButton(
        new QPushButton(
            tr("Next Issue"),
            this
            )
        )
{
    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        4
        );

    /*
     * ---------------------------------------------------------
     * Navigation
     * ---------------------------------------------------------
     */

    auto *navigationLayout =
        new QHBoxLayout();

    navigationLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    navigationLayout->setSpacing(
        6
        );

    m_previousEventButton->setToolTip(
        tr(
            "Select the previous visible event"
            )
        );

    m_nextEventButton->setToolTip(
        tr(
            "Select the next visible event"
            )
        );

    m_previousIssueButton->setToolTip(
        tr(
            "Select the previous visible WARN, "
            "ERROR, or CRITICAL event"
            )
        );

    m_nextIssueButton->setToolTip(
        tr(
            "Select the next visible WARN, "
            "ERROR, or CRITICAL event"
            )
        );

    navigationLayout->addStretch();

    navigationLayout->addWidget(
        m_previousEventButton
        );

    navigationLayout->addWidget(
        m_nextEventButton
        );

    navigationLayout->addStretch(
        1
        );

    m_eventPositionLabel->setAlignment(
        Qt::AlignCenter
        );

    navigationLayout->addWidget(
        m_eventPositionLabel
        );

    navigationLayout->addStretch(
        1
        );

    navigationLayout->addWidget(
        m_previousIssueButton
        );

    navigationLayout->addWidget(
        m_nextIssueButton
        );

    layout->addLayout(
        navigationLayout
        );

    /*
     * ---------------------------------------------------------
     * Event table
     * ---------------------------------------------------------
     */

    m_table->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    m_table->setAlternatingRowColors(
        true
        );

    m_table->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_table->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_table->setContextMenuPolicy(
        Qt::CustomContextMenu
        );

    m_table->setSortingEnabled(
        true
        );

    m_table->sortByColumn(
        -1,
        Qt::AscendingOrder
        );

    m_table
        ->horizontalHeader()
        ->setResizeContentsPrecision(
            200
            );

    m_table
        ->horizontalHeader()
        ->setStretchLastSection(
            true
            );

    layout->addWidget(
        m_table
        );

    /*
     * ---------------------------------------------------------
     * Navigation actions
     * ---------------------------------------------------------
     */

    connect(
        m_previousEventButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateAdjacentEvent(
                -1
                );
        }
        );

    connect(
        m_nextEventButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateAdjacentEvent(
                1
                );
        }
        );

    connect(
        m_previousIssueButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateAdjacentIssue(
                -1
                );
        }
        );

    connect(
        m_nextIssueButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateAdjacentIssue(
                1
                );
        }
        );

    /*
     * ---------------------------------------------------------
     * Custom context menu
     * ---------------------------------------------------------
     */

    connect(
        m_table,
        &QTableView::
        customContextMenuRequested,
        this,
        [this](
            const QPoint &position
            ) {
            if (m_session == nullptr) {
                return;
            }

            const QModelIndex proxyIndex =
                m_table->indexAt(
                    position
                    );

            if (!proxyIndex.isValid()) {
                return;
            }

            InvestigationController *controller =
                m_session
                    ->investigationController();

            if (controller == nullptr) {
                return;
            }

            const QString value =
                proxyIndex
                    .data(
                        Qt::DisplayRole
                        )
                    .toString();

            InvestigationFilterProxyModel
                *proxyModel =
                controller->proxyModel();

            InvestigationTableModel
                *sourceModel =
                controller->sourceModel();

            if (proxyModel == nullptr
                || sourceModel == nullptr) {
                return;
            }

            const QModelIndex sourceIndex =
                proxyModel->mapToSource(
                    proxyIndex
                    );

            const bool customColumn =
                sourceIndex.isValid()
                && sourceModel
                       ->isCustomColumn(
                           sourceIndex.column()
                           );

            const QString customField =
                customColumn
                    ? sourceModel
                          ->columnKey(
                              sourceIndex.column()
                              )
                    : QString();

            QMenu menu(
                m_table
                );

            QAction *copyValueAction =
                menu.addAction(
                    tr("Copy Cell Value")
                    );

            QAction *filterValueAction =
                nullptr;

            if (customColumn
                && !customField.isEmpty()
                && !value.isEmpty()) {
                menu.addSeparator();

                filterValueAction =
                    menu.addAction(
                        tr(
                            "Filter by This Value"
                            )
                        );
            }

            QAction *selectedAction =
                menu.exec(
                    m_table
                        ->viewport()
                        ->mapToGlobal(
                            position
                            )
                    );

            if (selectedAction
                == copyValueAction) {
                QApplication::clipboard()
                ->setText(
                    value
                    );

                return;
            }

            if (filterValueAction != nullptr
                && selectedAction
                       == filterValueAction) {
                emit customFieldFilterRequested(
                    customField,
                    value
                    );
            }
        }
        );

    /*
     * ---------------------------------------------------------
     * Per-session column-width persistence
     * ---------------------------------------------------------
     */

    connect(
        m_table->horizontalHeader(),
        &QHeaderView::sectionResized,
        this,
        [this](
            int,
            int,
            int
            ) {
            if (m_session == nullptr
                || m_table->model()
                       == nullptr) {
                return;
            }

            const int columnCount =
                m_table
                    ->horizontalHeader()
                    ->count();

            QVector<int> widths;

            widths.reserve(
                columnCount
                );

            for (
                int column = 0;
                column < columnCount;
                ++column
                ) {
                widths.append(
                    m_table
                        ->columnWidth(
                            column
                            )
                    );
            }

            m_session
                ->setColumnWidths(
                    std::move(widths)
                    );
        }
        );

    refreshPresentation();
}

void InvestigationEventPanel::setSession(
    InvestigationSession *session
    )
{
    if (m_session == session
        && session != nullptr) {
        /*
         * A reload keeps the same InvestigationSession
         * object but can replace its records and clear
         * persisted column widths.
         */
        restoreColumnWidths();

        selectRecordId(
            m_session->selectedRecordId()
            );

        refreshPresentation();

        return;
    }

    QObject::disconnect(
        m_selectionConnection
        );

    m_selectionConnection =
        QMetaObject::Connection();

    m_session =
        session;

    /*
     * Model binding can resize header sections.
     * Those changes are synchronization, not a
     * user resize, so they must not overwrite the
     * session's saved width state.
     */
    {
        const QSignalBlocker headerBlocker(
            m_table->horizontalHeader()
            );

        if (m_session == nullptr) {
            m_table->setModel(
                nullptr
                );
        } else {
            m_table->setModel(
                m_session
                    ->investigationController()
                    ->proxyModel()
                );
        }
    }

    connectSelectionModel();

    updateRowHeaderWidth();

    if (m_session == nullptr) {
        refreshPresentation();

        emit selectedRecordChanged();

        return;
    }

    restoreColumnWidths();

    /*
     * Restore only when this record remains visible
     * in the session's current proxy-model result.
     */
    selectRecordId(
        m_session
            ->selectedRecordId()
        );

    refreshPresentation();
}

InvestigationSession *
InvestigationEventPanel::session() const
{
    return m_session;
}

const InvestigationRecord *
    InvestigationEventPanel::
    selectedRecord() const
{
    if (m_session == nullptr
        || m_table->selectionModel()
               == nullptr) {
        return nullptr;
    }

    const QModelIndexList selectedRows =
        m_table
            ->selectionModel()
            ->selectedRows();

    if (selectedRows.isEmpty()) {
        return nullptr;
    }

    return m_session
        ->investigationController()
        ->recordForProxyIndex(
            selectedRows.first()
            );
}

void InvestigationEventPanel::clearSelection()
{
    if (m_table->selectionModel()
        != nullptr) {
        m_table->clearSelection();
    }

    /*
     * If there is no selection model there will be
     * no selectionChanged signal to synchronize the
     * session state for us.
     */
    if (m_session != nullptr
        && (
            m_table->selectionModel()
                == nullptr
            || selectedRecord()
                   == nullptr
            )) {
        m_session->setSelectedRecordId(
            QString()
            );
    }

    refreshNavigationState();
}

void InvestigationEventPanel::selectProxyRow(
    int proxyRow
    )
{
    if (m_session == nullptr
        || proxyRow < 0) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr
        || controller->proxyModel()
               == nullptr) {
        return;
    }

    const QModelIndex targetIndex =
        controller
            ->proxyModel()
            ->index(
                proxyRow,
                0
                );

    if (!targetIndex.isValid()) {
        return;
    }

    QItemSelectionModel *selectionModel =
        m_table->selectionModel();

    if (selectionModel == nullptr) {
        return;
    }

    selectionModel->setCurrentIndex(
        targetIndex,
        QItemSelectionModel::
            ClearAndSelect
            | QItemSelectionModel::Rows
        );

    m_table->scrollTo(
        targetIndex,
        QAbstractItemView::
        PositionAtCenter
        );
}

void InvestigationEventPanel::selectRecordId(
    const QString &recordId
    )
{
    if (m_session == nullptr
        || recordId.isEmpty()) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const int proxyRow =
        controller
            ->proxyRowForRecordId(
                recordId
                );

    if (proxyRow < 0) {
        return;
    }

    selectProxyRow(
        proxyRow
        );
}

void InvestigationEventPanel::
    refreshNavigationState()
{
    const bool hasInvestigation =
        m_session != nullptr
        && m_table->model() != nullptr;

    m_previousEventButton->setVisible(
        hasInvestigation
        );

    m_nextEventButton->setVisible(
        hasInvestigation
        );

    m_eventPositionLabel->setVisible(
        hasInvestigation
        );

    const bool hasIssueNavigation =
        hasInvestigation
        && m_session->hasSeverityData();

    m_previousIssueButton->setVisible(
        hasIssueNavigation
        );

    m_nextIssueButton->setVisible(
        hasIssueNavigation
        );

    if (!hasInvestigation) {
        m_previousEventButton->setEnabled(
            false
            );

        m_nextEventButton->setEnabled(
            false
            );

        m_previousIssueButton->setEnabled(
            false
            );

        m_nextIssueButton->setEnabled(
            false
            );

        m_eventPositionLabel->clear();

        return;
    }

    const int visibleCount =
        m_table
            ->model()
            ->rowCount();

    const QModelIndex currentIndex =
        m_table->currentIndex();

    if (!currentIndex.isValid()) {
        const bool hasVisibleEvents =
            visibleCount > 0;

        m_previousEventButton->setEnabled(
            hasVisibleEvents
            );

        m_nextEventButton->setEnabled(
            hasVisibleEvents
            );

        m_previousIssueButton->setEnabled(
            hasVisibleEvents
            && hasIssueNavigation
            );

        m_nextIssueButton->setEnabled(
            hasVisibleEvents
            && hasIssueNavigation
            );

        m_eventPositionLabel->setText(
            tr("%1 visible events")
                .arg(
                    visibleCount
                    )
            );

        return;
    }

    const int currentRow =
        currentIndex.row();

    m_previousEventButton->setEnabled(
        currentRow > 0
        );

    m_nextEventButton->setEnabled(
        currentRow
        < visibleCount - 1
        );

    m_previousIssueButton->setEnabled(
        hasIssueNavigation
        );

    m_nextIssueButton->setEnabled(
        hasIssueNavigation
        );

    QString positionText =
        tr("Event %1 of %2 visible")
            .arg(
                currentRow + 1
                )
            .arg(
                visibleCount
                );

    const InvestigationRecord *record =
        m_session
            ->investigationController()
            ->recordForProxyIndex(
                currentIndex
                );

    if (record != nullptr) {
        positionText +=
            tr(" • Source record %1")
                .arg(
                    record
                        ->source
                        .recordNumber
                    );
    }

    m_eventPositionLabel->setText(
        positionText
        );
}

void InvestigationEventPanel::
    refreshPresentation()
{
    updateRowHeaderWidth();
    refreshNavigationState();
}

void InvestigationEventPanel::focusTable()
{
    m_table->setFocus();
}

void InvestigationEventPanel::
    connectSelectionModel()
{
    QObject::disconnect(
        m_selectionConnection
        );

    if (m_table->selectionModel()
        == nullptr) {
        m_selectionConnection =
            QMetaObject::Connection();

        return;
    }

    m_selectionConnection =
        connect(
            m_table->selectionModel(),
            &QItemSelectionModel::
            selectionChanged,
            this,
            [this](
                const QItemSelection &,
                const QItemSelection &
                ) {
                if (m_session != nullptr) {
                    const InvestigationRecord
                        *record =
                        selectedRecord();

                    m_session
                        ->setSelectedRecordId(
                            record != nullptr
                                ? record
                                      ->recordId
                                : QString()
                            );
                }

                emit selectedRecordChanged();

                refreshNavigationState();
            }
            );
}

void InvestigationEventPanel::
    navigateAdjacentEvent(
        int direction
        )
{
    if (m_session == nullptr
        || m_table->model()
               == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const QModelIndex currentIndex =
        m_table->currentIndex();

    const int currentProxyRow =
        currentIndex.isValid()
            ? currentIndex.row()
            : -1;

    const int targetProxyRow =
        controller
            ->adjacentVisibleProxyRow(
                currentProxyRow,
                direction
                );

    selectProxyRow(
        targetProxyRow
        );
}

void InvestigationEventPanel::
    navigateAdjacentIssue(
        int direction
        )
{
    if (m_session == nullptr
        || m_table->model()
               == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    int currentProxyRow =
        -1;

    const QModelIndex currentIndex =
        m_table->currentIndex();

    if (currentIndex.isValid()) {
        currentProxyRow =
            currentIndex.row();
    }

    const int targetProxyRow =
        controller
            ->adjacentIssueProxyRow(
                currentProxyRow,
                direction
                );

    selectProxyRow(
        targetProxyRow
        );
}

void InvestigationEventPanel::
    updateRowHeaderWidth()
{
    QHeaderView *header =
        m_table->verticalHeader();

    if (header == nullptr) {
        return;
    }

    int maximumRowNumber =
        1;

    if (m_session != nullptr) {
        InvestigationController *controller =
            m_session
                ->investigationController();

        if (controller != nullptr) {
            maximumRowNumber =
                std::max(
                    1,
                    controller
                        ->totalRecordCount()
                    );
        }
    }

    /*
     * Always reserve room for the bookmark
     * indicator so filtering cannot make the
     * row-header width jump.
     */
    const QString widestExpectedText =
        QStringLiteral("★ %1")
            .arg(
                maximumRowNumber
                );

    const int textWidth =
        header
            ->fontMetrics()
            .horizontalAdvance(
                widestExpectedText
                );

    header->setFixedWidth(
        textWidth + 8
        );
}

void InvestigationEventPanel::
    restoreColumnWidths()
{
    if (m_session == nullptr
        || m_table->model()
               == nullptr) {
        return;
    }

    const QVector<int> columnWidths =
        m_session->columnWidths();

    const int columnCount =
        m_table
            ->horizontalHeader()
            ->count();

    if (columnWidths.size()
        == columnCount) {
        const QSignalBlocker headerBlocker(
            m_table->horizontalHeader()
            );

        for (
            int column = 0;
            column < columnCount;
            ++column
            ) {
            m_table->setColumnWidth(
                column,
                columnWidths[column]
                );
        }
    } else {
        QVector<int> measuredWidths;

        measuredWidths.reserve(
            columnCount
            );

        {
            const QSignalBlocker headerBlocker(
                m_table->horizontalHeader()
                );

            m_table
                ->resizeColumnsToContents();

            for (
                int column = 0;
                column < columnCount;
                ++column
                ) {
                measuredWidths.append(
                    m_table
                        ->columnWidth(
                            column
                            )
                    );
            }
        }

        m_session->setColumnWidths(
            std::move(
                measuredWidths
                )
            );
    }

    /*
     * Preserve the existing behavior in which the
     * final column consumes unused horizontal space.
     */
    {
        const QSignalBlocker headerBlocker(
            m_table->horizontalHeader()
            );

        m_table
            ->horizontalHeader()
            ->setStretchLastSection(
                true
                );
    }
}