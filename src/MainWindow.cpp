#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QScrollBar>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableView>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QStringList>
#include <QSizePolicy>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QTime>
#include <QTimer>
#include <QtConcurrentRun>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

#include <algorithm>
#include <utility>

#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "ui/ImportConfigurationDialog.h"

namespace
{
constexpr int SearchDebounceIntervalMs = 600;

void configureEventCountAxis(
    QValueAxis *axis,
    int maxCount
    )
{
    axis->setTitleText(
        QStringLiteral("Events")
        );

    axis->setLabelFormat(
        QStringLiteral("%d")
        );

    axis->setRange(
        0,
        std::max(1, maxCount)
        );

    if (maxCount <= 10) {
        axis->setTickType(
            QValueAxis::TicksDynamic
            );

        axis->setTickAnchor(0);
        axis->setTickInterval(1);

        return;
    }

    axis->setTickType(
        QValueAxis::TicksFixed
        );

    axis->setTickCount(6);
    axis->applyNiceNumbers();
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    summaryLabel(new QLabel("No log file loaded.")),
    eventTable(new QTableView(this)),
    eventDetailText(new QPlainTextEdit(this)),
    issueSummaryTable(new QTableWidget(0, 4)),
    issueSummaryGroup(nullptr),
    investigationController(new InvestigationController(this)),
    timelineChartView(new QChartView(this)),
    levelFilterCombo(new QComboBox(this)),
    subsystemFilterCombo(new QComboBox(this)),
    searchInput(new QLineEdit(this)),
    searchDebounceTimer(new QTimer(this))
{
    setWindowTitle("TraceScope — Qt Telemetry Log Inspector");
    resize(1100, 760);

    setAcceptDrops(true);

    createMenus();
    buildLayout();
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");

    openAction =
        new QAction(
            "&Open Log File...",
            this
            );
    openAction->setShortcut(QKeySequence::Open);

    connect(openAction, &QAction::triggered, this, [this]() {
        openLogFile();
    });

    fileMenu->addAction(openAction);

    auto *exportAction = new QAction("&Export Filtered Results...", this);
    exportAction->setShortcut(QKeySequence::Save);

    connect(exportAction, &QAction::triggered, this, [this]() {
        exportFilteredResults();
    });

    fileMenu->addSeparator();
    fileMenu->addAction(exportAction);

    auto *helpMenu = menuBar()->addMenu("&Help");

    auto *aboutAction = new QAction("&About TraceScope", this);

    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this,
            "About TraceScope",
            "TraceScope is a Qt/C++ telemetry log inspector for loading, filtering, visualizing, and exporting structured diagnostic log files."
            );
    });

    helpMenu->addAction(aboutAction);
}

void MainWindow::buildLayout()
{
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    layout->addWidget(summaryLabel);
    buildFilterControls(layout);

    auto *timelineGroup = buildTimelinePanel();

    auto *eventsGroup = new QGroupBox("Telemetry Events", this);
    auto *eventsLayout = new QVBoxLayout(eventsGroup);

    eventTable->setModel(investigationController->proxyModel());

    eventTable->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    eventTable->setAlternatingRowColors(true);

    eventTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    eventTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    eventTable->setSortingEnabled(true);

    eventTable->sortByColumn(
        -1,
        Qt::AscendingOrder
        );

    eventTable->horizontalHeader()->setStretchLastSection(true);

    eventsLayout->addWidget(eventTable);

    issueSummaryGroup =
        buildIssueSummaryPanel();
    auto *detailGroup = buildDetailPanel();

    auto *bottomSplitter = new QSplitter(Qt::Horizontal, this);

    bottomSplitter->addWidget(issueSummaryGroup);
    bottomSplitter->addWidget(detailGroup);

    bottomSplitter->setStretchFactor(0, 0);
    bottomSplitter->setStretchFactor(1, 1);

    bottomSplitter->setCollapsible(0, false);
    bottomSplitter->setCollapsible(1, false);

    bottomSplitter->setSizes({
        issueSummaryTable->minimumWidth() + 30,
        1000
    });

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(timelineGroup);
    mainSplitter->addWidget(eventsGroup);
    mainSplitter->addWidget(bottomSplitter);

    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 5);
    mainSplitter->setStretchFactor(2, 2);

    mainSplitter->setCollapsible(0, false);
    mainSplitter->setCollapsible(1, false);
    mainSplitter->setCollapsible(2, false);

    mainSplitter->setSizes(QList<int>{220, 350, 250});

    layout->addWidget(mainSplitter, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::openLogFile(const QString &initialFilePath)
{
    if (importWatcher != nullptr) {
        QMessageBox::information(
            this,
            tr("Import In Progress"),
            tr(
                "A log file is already being imported. "
                "Wait for the current import to finish "
                "before starting another import."
                )
            );

        return;
    }

    ImportConfigurationDialog dialog(this);

    if (!initialFilePath.isEmpty()) {
        dialog.setSelectedFilePath(
            initialFilePath
            );
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    loadLogFile(
        dialog.selectedFilePath(),
        dialog.configuredProfile()
        );
}

void MainWindow::loadLogFile(
    const QString &filePath,
    const ImportProfile &profile
    )
{
    if (importWatcher != nullptr) {
        return;
    }

    const ImporterRegistry registry =
        createBuiltInImporterRegistry(
            profile
            );

    const std::shared_ptr<ILogImporter> importer =
        registry.importerById(
            profile.importerId
            );

    if (!importer) {
        QMessageBox::warning(
            this,
            tr("Unsupported Import Format"),
            tr(
                "The selected import profile uses "
                "an importer that this version of "
                "TraceScope does not support."
                )
            );

        return;
    }

    auto *watcher =
        new QFutureWatcher<ImportResult>(
            this
            );

    importWatcher =
        watcher;

    if (openAction != nullptr) {
        openAction->setEnabled(false);
    }

    setAcceptDrops(false);

    connect(
        watcher,
        &QFutureWatcher<ImportResult>::finished,
        this,
        [this, watcher, filePath]() {
            const ImportResult result =
                watcher->result();

            if (importWatcher == watcher) {
                importWatcher =
                    nullptr;
            }

            if (openAction != nullptr) {
                openAction->setEnabled(true);
            }

            setAcceptDrops(true);

            watcher->deleteLater();

            completeLogFileImport(
                filePath,
                result
                );
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [importer, filePath]() {
                return importer->importFile(
                    filePath
                    );
            }
            )
        );
}

void MainWindow::completeLogFileImport(
    const QString &filePath,
    const ImportResult &result
    )
{
    /*
     * A cancelled import must never replace
     * the currently loaded investigation with
     * a partial result.
     *
     * Cancellation will be wired to the UI in
     * the next Phase 7 slice.
     */
    if (result.cancelled) {
        return;
    }

    currentFilePath =
        filePath;

    investigationController->setRecords(
        result.records
        );

    if (result.records.isEmpty()) {
        QMessageBox::warning(
            this,
            "No Events Loaded",
            "No telemetry events were loaded. "
            "The file may be empty, malformed, "
            "or unsupported."
            );
    }

    refreshSubsystemFilterOptions();
    updateDataCapabilities();
    applyFilters();

    eventTable->resizeColumnsToContents();

    eventTable
        ->horizontalHeader()
        ->setStretchLastSection(true);
}

void MainWindow::updateSummary(
    const QVector<InvestigationRecord> &records,
    const QString &filePath
    )
{
    int traceCount = 0;
    int debugCount = 0;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;

    if (!hasSeverityData) {
        summaryLabel->setText(
            QString(
                "TOTAL: %1 visible of %2 events from %3"
                )
                .arg(records.size())
                .arg(
                    investigationController
                        ->totalRecordCount()
                    )
                .arg(filePath)
            );

        return;
    }

    for (const InvestigationRecord &record
         : records) {
        if (!record.severity.has_value()) {
            continue;
        }

        switch (record.severity.value()) {
            case RecordSeverity::Trace:
                ++traceCount;
                break;

            case RecordSeverity::Debug:
                ++debugCount;
                break;

            case RecordSeverity::Info:
                ++infoCount;
                break;

            case RecordSeverity::Warning:
                ++warningCount;
                break;

            case RecordSeverity::Error:
                ++errorCount;
                break;

            case RecordSeverity::Critical:
                ++criticalCount;
                break;
        }
    }

    summaryLabel->setText(
        QString(
            "Showing %1 of %2 events from %3 | "
            "TRACE: %4 | DEBUG: %5 | INFO: %6 | "
            "WARN: %7 | ERROR: %8 | CRITICAL: %9"
            )
            .arg(records.size())
            .arg(investigationController->totalRecordCount())
            .arg(filePath)
            .arg(traceCount)
            .arg(debugCount)
            .arg(infoCount)
            .arg(warningCount)
            .arg(errorCount)
            .arg(criticalCount)
        );
}

void MainWindow::buildFilterControls(QVBoxLayout *layout)
{
    levelFilterCombo->addItem("All levels", "");
    levelFilterCombo->addItem("TRACE", "TRACE");
    levelFilterCombo->addItem("DEBUG", "DEBUG");
    levelFilterCombo->addItem("INFO", "INFO");
    levelFilterCombo->addItem("WARN", "WARN");
    levelFilterCombo->addItem("ERROR", "ERROR");
    levelFilterCombo->addItem("CRITICAL", "CRITICAL");

    subsystemFilterCombo->addItem("All subsystems", "");

    subsystemFilterCombo->setMinimumWidth(240);

    subsystemFilterCombo->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon
        );

    subsystemFilterCombo->setMinimumContentsLength(24);

    searchInput->setPlaceholderText(
        "Search canonical fields and custom attributes..."
        );

    searchDebounceTimer->setSingleShot(true);

    searchDebounceTimer->setInterval(
        SearchDebounceIntervalMs
        );

    auto *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(levelFilterCombo);
    filterLayout->addWidget(subsystemFilterCombo);
    filterLayout->addWidget(searchInput);

    layout->addLayout(filterLayout);

    connect(
        levelFilterCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            searchDebounceTimer->stop();
            applyFilters();
        }
        );

    connect(
        subsystemFilterCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            subsystemFilterCombo->setToolTip(
                subsystemFilterCombo
                    ->currentData()
                    .toString()
                );
            searchDebounceTimer->stop();
            applyFilters();
        }
        );

    connect(
        searchInput,
        &QLineEdit::textChanged,
        this,
        [this]() {
            searchDebounceTimer->start();
        }
        );

    connect(
        searchInput,
        &QLineEdit::returnPressed,
        this,
        [this]() {
            searchDebounceTimer->stop();
            applyFilters();
        }
        );

    connect(
        searchDebounceTimer,
        &QTimer::timeout,
        this,
        [this]() {
            applyFilters();
        }
        );
}

void MainWindow::applyFilters()
{
    investigationController->setFilters(
        levelFilterCombo->currentData().toString(),
        subsystemFilterCombo->currentData().toString(),
        searchInput->text()
        );

    eventTable->clearSelection();
    clearEventDetail();

    const QVector<InvestigationRecord>
        visibleRecords =
        investigationController->visibleRecords();

    updateSummary(
        visibleRecords,
        currentFilePath
        );

    updateIssueSummary(
        visibleRecords
        );

    updateTimelineChart(
        visibleRecords
        );
}

void MainWindow::refreshSubsystemFilterOptions()
{
    const QString selectedSubsystem =
        subsystemFilterCombo
            ->currentData()
            .toString();

    subsystemFilterCombo->blockSignals(true);
    subsystemFilterCombo->clear();

    subsystemFilterCombo->addItem(
        "All subsystems",
        ""
        );

    subsystemFilterCombo->setItemData(
        0,
        QStringLiteral("All subsystems"),
        Qt::ToolTipRole
        );

    const QStringList subsystems =
        investigationController
            ->availableSubsystems();

    int widestTextWidth =
        subsystemFilterCombo
            ->fontMetrics()
            .horizontalAdvance(
                QStringLiteral(
                    "All subsystems"
                    )
                );

    for (const QString &subsystem
         : subsystems) {
        subsystemFilterCombo->addItem(
            subsystem,
            subsystem
            );

        const int itemIndex =
            subsystemFilterCombo->count() - 1;

        subsystemFilterCombo->setItemData(
            itemIndex,
            subsystem,
            Qt::ToolTipRole
            );

        widestTextWidth =
            std::max(
                widestTextWidth,
                subsystemFilterCombo
                    ->fontMetrics()
                    .horizontalAdvance(
                        subsystem
                        )
                );
    }

    /*
     * Keep the filter control itself compact,
     * but let its popup expand enough to show
     * long subsystem/logger names clearly.
     */
    const int popupWidth =
        std::clamp(
            widestTextWidth + 40,
            240,
            650
            );

    subsystemFilterCombo
        ->view()
        ->setMinimumWidth(
            popupWidth
            );

    const int previousIndex =
        subsystemFilterCombo->findData(
            selectedSubsystem
            );

    if (previousIndex >= 0) {
        subsystemFilterCombo->setCurrentIndex(
            previousIndex
            );
    }

    subsystemFilterCombo->setToolTip(
        subsystemFilterCombo
            ->currentData()
            .toString()
        );

    subsystemFilterCombo->blockSignals(false);
}

QGroupBox *MainWindow::buildDetailPanel()
{
    auto *detailGroup =
        new QGroupBox("Selected Event Details", this);

    eventDetailText->setReadOnly(true);
    eventDetailText->setPlaceholderText(
        "Select a telemetry event to view its details."
        );

    auto *detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->addWidget(eventDetailText);

    connect(
        eventTable->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](
            const QItemSelection &,
            const QItemSelection &
            ) {
            updateEventDetailFromSelection();
        }
        );

    return detailGroup;
}

void MainWindow::updateEventDetailFromSelection()
{
    const QModelIndexList selectedRows =
        eventTable->selectionModel()
            ->selectedRows();

    if (selectedRows.isEmpty()) {
        clearEventDetail();
        return;
    }

    const InvestigationRecord *record =
        investigationController
            ->recordForProxyIndex(
                selectedRows.first()
                );

    if (record == nullptr) {
        clearEventDetail();
        return;
    }

    displayEventDetail(*record);
}

void MainWindow::displayEventDetail(
    const InvestigationRecord &record
    )
{
    QStringList lines;

    lines << "Timestamp: "
                 + (
                     record.timestamp.has_value()
                         ? record.timestamp->toString(
                               Qt::ISODateWithMs
                               )
                         : QString()
                     );

    lines << "Level: "
                 + (
                     record.severity.has_value()
                         ? recordSeverityToString(
                               record.severity.value()
                               )
                         : QString()
                     );

    lines << "Subsystem: "
                 + record.subsystem.value_or(
                     QString()
                     );

    lines << "Event Code: "
                 + record.eventCode.value_or(
                     QString()
                     );

    lines << "Entity ID: "
                 + record.entityId.value_or(
                     QString()
                     );

    lines << "";
    lines << "Message:";
    lines << record.message.value_or(
        QString()
        );

    if (!record.customAttributes.isEmpty()) {
        lines << "";
        lines << "Custom Attributes:";

        QStringList attributeKeys =
            record.customAttributes.keys();

        std::sort(
            attributeKeys.begin(),
            attributeKeys.end(),
            [](const QString &left, const QString &right) {
                return left.compare(
                           right,
                           Qt::CaseInsensitive
                           ) < 0;
            }
            );

        for (
            const QString &key :
            std::as_const(attributeKeys)
            ) {
            lines << QString("%1: %2")
            .arg(
                key,
                record.customAttributes
                    .value(key)
                    .toString()
                );
        }
    }

    eventDetailText->setPlainText(
        lines.join("\n")
        );
}

void MainWindow::clearEventDetail()
{
    eventDetailText->clear();
}

QGroupBox *MainWindow::buildIssueSummaryPanel()
{
    auto *issueSummaryGroup =
        new QGroupBox("Grouped Warnings and Errors", this);

    issueSummaryTable->setColumnCount(4);
    issueSummaryTable->setHorizontalHeaderLabels({
        "Subsystem",
        "Warnings",
        "Errors",
        "Total"
    });

    issueSummaryTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
        );

    issueSummaryTable->horizontalHeader()->setStretchLastSection(false);
    issueSummaryTable->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents
        );

    const int requiredTableWidth =
        issueSummaryTable->verticalHeader()->width()
        + issueSummaryTable->horizontalHeader()->length()
        + issueSummaryTable->frameWidth() * 2
        + issueSummaryTable->verticalScrollBar()->sizeHint().width()
        + 8;

    issueSummaryTable->setMinimumWidth(requiredTableWidth);
    issueSummaryTable->setAlternatingRowColors(true);

    auto *issueLayout = new QVBoxLayout(issueSummaryGroup);
    issueLayout->addWidget(issueSummaryTable);

    return issueSummaryGroup;
}

void MainWindow::updateIssueSummary(const QVector<InvestigationRecord> &records)
{
    if (!hasSeverityData
        || !hasSubsystemData) {
        issueSummaryTable->setRowCount(0);
        return;
    }

    const auto groups = issueAnalyzer.groupWarningsAndErrorsBySubsystem(records);

    issueSummaryTable->setRowCount(groups.size());

    for (int row = 0; row < groups.size(); ++row) {
        const TelemetryIssueGroup &group = groups[row];

        issueSummaryTable->setItem(row, 0, new QTableWidgetItem(group.subsystem));
        issueSummaryTable->setItem(
            row,
            1,
            new QTableWidgetItem(QString::number(group.warningCount))
            );
        issueSummaryTable->setItem(
            row,
            2,
            new QTableWidgetItem(QString::number(group.errorCount))
            );
        issueSummaryTable->setItem(
            row,
            3,
            new QTableWidgetItem(QString::number(group.totalCount()))
            );
    }

    issueSummaryTable->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents
        );

    const int requiredTableWidth =
        issueSummaryTable->verticalHeader()->width()
        + issueSummaryTable->horizontalHeader()->length()
        + issueSummaryTable->frameWidth() * 2
        + issueSummaryTable->verticalScrollBar()->sizeHint().width()
        + 8;

    issueSummaryTable->setMinimumWidth(requiredTableWidth);
}

void MainWindow::exportFilteredResults()
{
    const QVector<InvestigationRecord> records =
        investigationController->visibleRecords();

    if (records.isEmpty()) {
        QMessageBox::information(
            this,
            "No Records to Export",
            "There are no currently visible records to export."
            );

        return;
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Export Filtered Records",
            "filtered-investigation-records.csv",
            "CSV Files (*.csv);;All Files (*)"
            );

    if (filePath.isEmpty()) {
        return;
    }

    const bool exported =
        csvExporter.exportToFile(
            records,
            filePath
            );

    if (!exported) {
        QMessageBox::warning(
            this,
            "Export Failed",
            "TraceScope could not export the filtered records."
            );

        return;
    }

    QMessageBox::information(
        this,
        "Export Complete",
        QString(
            "Exported %1 records."
            ).arg(records.size())
        );
}

QGroupBox *MainWindow::buildTimelinePanel()
{
    auto *timelineGroup =
        new QGroupBox("Event Counts Over Time", this);

    timelineChartView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    timelineChartView->setMinimumHeight(190);
    timelineChartView->setRenderHint(QPainter::Antialiasing);

    timelineChartView->setAcceptDrops(false);
    timelineChartView->viewport()->setAcceptDrops(false);

    auto *timelineLayout = new QVBoxLayout(timelineGroup);
    timelineLayout->addWidget(timelineChartView);

    return timelineGroup;
}

void MainWindow::updateTimelineChart(
    const QVector<InvestigationRecord> &records
    )
{
    if (!timelineFirstMinute.has_value()
        || !timelineLastMinute.has_value()) {
        auto *chart = new QChart();

        chart->setTitle(
            "No events to display"
            );

        timelineChartView->setChart(
            chart
            );

        return;
    }

    const auto buckets =
        timelineAnalyzer.groupRecordsByMinute(
            records,
            *timelineFirstMinute,
            *timelineLastMinute
            );

    if (buckets.isEmpty()) {
        auto *chart = new QChart();
        chart->setTitle("No events to display");
        timelineChartView->setChart(chart);
        return;
    }

    const bool showSeveritySeries =
        hasSeverityData;

    if (!showSeveritySeries) {
        auto *totalSet =
            new QBarSet(
                "TOTAL"
                );

        QStringList categories;

        int maxCount = 1;

        for (const EventCountBucket &bucket
             : buckets) {
            categories << bucket.label;

            *totalSet
                << bucket.totalCount();

            maxCount =
                std::max(
                    maxCount,
                    bucket.totalCount()
                    );
        }

        auto *series =
            new QBarSeries();

        series->append(
            totalSet
            );

        auto *chart =
            new QChart();

        chart->addSeries(
            series
            );

        chart->setTitle(
            "Filtered Event Counts by Minute"
            );

        chart->setAnimationOptions(
            QChart::NoAnimation
            );

        /*
     * A single TOTAL series does not need a
     * legend explaining what the only bar means.
     */
        chart->legend()->setVisible(
            false
            );

        auto *axisX =
            new QBarCategoryAxis();

        axisX->append(
            categories
            );

        chart->addAxis(
            axisX,
            Qt::AlignBottom
            );

        series->attachAxis(
            axisX
            );

        auto *axisY =
            new QValueAxis();

        configureEventCountAxis(
            axisY,
            maxCount
            );

        chart->addAxis(
            axisY,
            Qt::AlignLeft
            );

        series->attachAxis(
            axisY
            );

        timelineChartView->setChart(
            chart
            );

        return;
    }

    auto *traceSet =
        new QBarSet("TRACE");

    auto *debugSet =
        new QBarSet("DEBUG");

    auto *infoSet =
        new QBarSet("INFO");

    auto *warnSet =
        new QBarSet("WARN");

    auto *errorSet =
        new QBarSet("ERROR");

    auto *criticalSet =
        new QBarSet("CRITICAL");

    bool hasUnspecifiedEvents = false;

    for (const EventCountBucket &bucket : buckets) {
        if (bucket.unspecifiedCount > 0) {
            hasUnspecifiedEvents = true;
            break;
        }
    }

    QBarSet *unspecifiedSet = nullptr;

    if (hasUnspecifiedEvents) {
        unspecifiedSet =
            new QBarSet(
                "UNSPECIFIED"
                );
    }

    QStringList categories;

    for (const EventCountBucket &bucket : buckets) {
        categories << bucket.label;
        *traceSet
            << bucket.traceCount;

        *debugSet
            << bucket.debugCount;

        *infoSet
            << bucket.infoCount;

        *warnSet
            << bucket.warningCount;

        *errorSet
            << bucket.errorCount;

        *criticalSet
            << bucket.criticalCount;

        if (unspecifiedSet != nullptr) {
            *unspecifiedSet
                << bucket.unspecifiedCount;
        }
    }

    auto *series = new QBarSeries();
    series->append(traceSet);
    series->append(debugSet);
    series->append(infoSet);
    series->append(warnSet);
    series->append(errorSet);
    series->append(criticalSet);

    if (unspecifiedSet != nullptr) {
        series->append(
            unspecifiedSet
            );
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Filtered Event Counts by Minute");
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();

    int maxCount = 1;

    for (const EventCountBucket &bucket : buckets) {
        maxCount =
            std::max(
                maxCount,
                bucket.traceCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.debugCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.infoCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.warningCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.errorCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.criticalCount
                );

        maxCount =
            std::max(
                maxCount,
                bucket.unspecifiedCount
                );
    }

    configureEventCountAxis(
        axisY,
        maxCount
        );

    chart->addAxis(
        axisY,
        Qt::AlignLeft
        );

    series->attachAxis(
        axisY
        );

    timelineChartView->setChart(
        chart
        );
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (importWatcher != nullptr) {
        return;
    }

    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (importWatcher != nullptr) {
        return;
    }

    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    openLogFile(
        urls.first().toLocalFile()
        );

    event->acceptProposedAction();
}

void MainWindow::updateDataCapabilities()
{
    hasSeverityData = false;
    hasSubsystemData = false;

    timelineFirstMinute.reset();
    timelineLastMinute.reset();

    const QVector<InvestigationRecord> &records =
        investigationController->allRecords();
    
    for (const InvestigationRecord &record
         : records) {
        hasSeverityData =
            hasSeverityData
            || record.severity.has_value();

        hasSubsystemData =
            hasSubsystemData
            || record.subsystem.has_value();

        if (!record.timestamp.has_value()) {
            continue;
        }

        QDateTime minute =
            record.timestamp->toUTC();

        minute.setTime(
            QTime(
                minute.time().hour(),
                minute.time().minute()
                )
            );

        if (!timelineFirstMinute.has_value()
            || minute <
                   *timelineFirstMinute) {
            timelineFirstMinute =
                minute;
        }

        if (!timelineLastMinute.has_value()
            || minute >
                   *timelineLastMinute) {
            timelineLastMinute =
                minute;
        }
    }

    /*
     * Clear filters that no longer apply before
     * hiding their controls.
     */
    if (!hasSeverityData) {
        levelFilterCombo->blockSignals(true);
        levelFilterCombo->setCurrentIndex(0);
        levelFilterCombo->blockSignals(false);
    }

    if (!hasSubsystemData) {
        subsystemFilterCombo->blockSignals(true);
        subsystemFilterCombo->setCurrentIndex(0);
        subsystemFilterCombo->blockSignals(false);
    }

    levelFilterCombo->setVisible(
        hasSeverityData
        );

    subsystemFilterCombo->setVisible(
        hasSubsystemData
        );

    /*
     * Warning/error grouping only has useful
     * meaning when both concepts are available.
     */
    if (issueSummaryGroup != nullptr) {
        issueSummaryGroup->setVisible(
            hasSeverityData
            && hasSubsystemData
            );
    }
}