#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSet>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QStringList>
#include <QSizePolicy>
#include <QSplitter>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>
#include <utility>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      summaryLabel(new QLabel("No log file loaded.")),
      eventTable(new QTableWidget(0, 5)),
      eventDetailText(new QPlainTextEdit(this)),
      issueSummaryTable(new QTableWidget(0, 4)),
      timelineChartView(new QChartView(this)),
      levelFilterCombo(new QComboBox(this)),
      subsystemFilterCombo(new QComboBox(this)),
      searchInput(new QLineEdit(this))
{
    setWindowTitle("TraceScope — Qt Telemetry Log Inspector");
    resize(1100, 760);

    createMenus();
    buildLayout();
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");

    auto *openAction = new QAction("&Open Log File...", this);
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

    eventTable->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    eventTable->setColumnCount(6);

    eventTable->setHorizontalHeaderLabels({
        "Timestamp",
        "Level",
        "Subsystem",
        "Event Code",
        "Entity ID",
        "Message"
    });

    eventTable->setAlternatingRowColors(true);
    eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventTable->setSelectionMode(QAbstractItemView::SingleSelection);

    eventsLayout->addWidget(eventTable);

    auto *issueSummaryGroup = buildIssueSummaryPanel();
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

void MainWindow::openLogFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Telemetry Log",
        QString(),
        "Log Files (*.jsonl *.log *.txt);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    loadLogFile(filePath);
}

void MainWindow::loadLogFile(const QString &filePath)
{
    currentEvents = parser.parseFile(filePath);

    if (currentEvents.isEmpty()) {
        QMessageBox::warning(
            this,
            "No Events Loaded",
            "No telemetry events were loaded. The file may be empty, malformed, or unsupported."
            );
    }

    refreshSubsystemFilterOptions();
    applyFilters();

    currentFilePath = filePath;
}

void MainWindow::populateTable(const QVector<TelemetryEvent> &events)
{
    eventTable->setRowCount(events.size());

    for (int row = 0; row < events.size(); ++row) {
        const TelemetryEvent &event = events[row];

        eventTable->setItem(row, 0, new QTableWidgetItem(event.timestamp));
        eventTable->setItem(row, 1, new QTableWidgetItem(event.level));
        eventTable->setItem(row, 2, new QTableWidgetItem(event.subsystem));
        eventTable->setItem(row, 3, new QTableWidgetItem(event.eventCode));
        eventTable->setItem(row, 4, new QTableWidgetItem(event.entityId));
        eventTable->setItem(row, 5, new QTableWidgetItem(event.message));
    }

    eventTable->resizeColumnsToContents();
    eventTable->horizontalHeader()->setStretchLastSection(true);
    eventTable->clearSelection();
    clearEventDetail();
}

void MainWindow::updateSummary(
    const QVector<TelemetryEvent> &events,
    const QString &filePath
    )
{
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;

    for (const TelemetryEvent &event : events) {
        if (event.level == "INFO") {
            ++infoCount;
        } else if (event.level == "WARN") {
            ++warningCount;
        } else if (event.level == "ERROR") {
            ++errorCount;
        }
    }

    summaryLabel->setText(
        QString("Showing %1 of %2 events from %3 | INFO: %4 | WARN: %5 | ERROR: %6")
            .arg(events.size())
            .arg(currentEvents.size())
            .arg(filePath)
            .arg(infoCount)
            .arg(warningCount)
            .arg(errorCount)
        );
}

void MainWindow::buildFilterControls(QVBoxLayout *layout)
{
    levelFilterCombo->addItem("All levels", "");
    levelFilterCombo->addItem("INFO", "INFO");
    levelFilterCombo->addItem("WARN", "WARN");
    levelFilterCombo->addItem("ERROR", "ERROR");

    subsystemFilterCombo->addItem("All subsystems", "");

    searchInput->setPlaceholderText("Search timestamp, level, subsystem, code, message, or entity ID...");

    auto *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(levelFilterCombo);
    filterLayout->addWidget(subsystemFilterCombo);
    filterLayout->addWidget(searchInput);

    layout->addLayout(filterLayout);

    connect(levelFilterCombo, &QComboBox::currentIndexChanged, this, [this]() {
        applyFilters();
    });

    connect(subsystemFilterCombo, &QComboBox::currentIndexChanged, this, [this]() {
        applyFilters();
    });

    connect(searchInput, &QLineEdit::textChanged, this, [this]() {
        applyFilters();
    });
}

TelemetryFilterCriteria MainWindow::currentFilterCriteria() const
{
    TelemetryFilterCriteria criteria;
    criteria.level = levelFilterCombo->currentData().toString();
    criteria.subsystem = subsystemFilterCombo->currentData().toString();
    criteria.searchText = searchInput->text();

    return criteria;
}

void MainWindow::applyFilters()
{
    filteredEvents = eventFilter.apply(currentEvents, currentFilterCriteria());

    populateTable(filteredEvents);
    updateSummary(filteredEvents, currentFilePath);
    updateIssueSummary(filteredEvents);
    updateTimelineChart(filteredEvents);
}

void MainWindow::refreshSubsystemFilterOptions()
{
    const QString selectedSubsystem = subsystemFilterCombo->currentData().toString();

    subsystemFilterCombo->blockSignals(true);
    subsystemFilterCombo->clear();
    subsystemFilterCombo->addItem("All subsystems", "");

    QSet<QString> subsystems;

    for (const TelemetryEvent &event : std::as_const(currentEvents)) {
        if (!event.subsystem.isEmpty()) {
            subsystems.insert(event.subsystem);
        }
    }

    const QList<QString> sortedSubsystems = QList<QString>(subsystems.begin(), subsystems.end());

    for (const QString &subsystem : sortedSubsystems) {
        subsystemFilterCombo->addItem(subsystem, subsystem);
    }

    const int previousIndex = subsystemFilterCombo->findData(selectedSubsystem);

    if (previousIndex >= 0) {
        subsystemFilterCombo->setCurrentIndex(previousIndex);
    }

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
        eventTable,
        &QTableWidget::itemSelectionChanged,
        this,
        &MainWindow::updateEventDetailFromSelection
        );

    return detailGroup;
}

void MainWindow::updateEventDetailFromSelection()
{
    const int row = eventTable->currentRow();

    if (row < 0 || row >= filteredEvents.size()) {
        clearEventDetail();
        return;
    }

    displayEventDetail(filteredEvents[row]);
}

void MainWindow::displayEventDetail(const TelemetryEvent &event)
{
    QStringList lines;

    lines << "Timestamp: " + event.timestamp;
    lines << "Level: " + event.level;
    lines << "Subsystem: " + event.subsystem;
    lines << "Event Code: " + event.eventCode;
    lines << "Entity ID: " + event.entityId;
    lines << "";
    lines << "Message:";
    lines << event.message;

    eventDetailText->setPlainText(lines.join("\n"));
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

void MainWindow::updateIssueSummary(const QVector<TelemetryEvent> &events)
{
    const auto groups = issueAnalyzer.groupWarningsAndErrorsBySubsystem(events);

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
    if (filteredEvents.isEmpty()) {
        QMessageBox::information(
            this,
            "No Events to Export",
            "There are no currently visible telemetry events to export."
            );

        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Filtered Telemetry Events",
        "filtered-telemetry-events.csv",
        "CSV Files (*.csv);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    const bool exported = csvExporter.exportToFile(filteredEvents, filePath);

    if (!exported) {
        QMessageBox::warning(
            this,
            "Export Failed",
            "TraceScope could not export the filtered telemetry events."
            );

        return;
    }

    QMessageBox::information(
        this,
        "Export Complete",
        QString("Exported %1 telemetry events.").arg(filteredEvents.size())
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

    auto *timelineLayout = new QVBoxLayout(timelineGroup);
    timelineLayout->addWidget(timelineChartView);

    return timelineGroup;
}

void MainWindow::updateTimelineChart(const QVector<TelemetryEvent> &events)
{
    const auto buckets = timelineAnalyzer.groupEventsByMinute(events);

    if (buckets.isEmpty()) {
        auto *chart = new QChart();
        chart->setTitle("No events to display");
        timelineChartView->setChart(chart);
        return;
    }

    auto *infoSet = new QBarSet("INFO");
    auto *warnSet = new QBarSet("WARN");
    auto *errorSet = new QBarSet("ERROR");

    QStringList categories;

    for (const EventCountBucket &bucket : buckets) {
        categories << bucket.label;
        *infoSet << bucket.infoCount;
        *warnSet << bucket.warningCount;
        *errorSet << bucket.errorCount;
    }

    auto *series = new QBarSeries();
    series->append(infoSet);
    series->append(warnSet);
    series->append(errorSet);

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Filtered Event Counts by Minute");
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->setAlignment(Qt::AlignRight);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setTitleText("Events");
    axisY->setLabelFormat("%d");

    int maxTotal = 1;

    for (const EventCountBucket &bucket : buckets) {
        maxTotal = std::max(maxTotal, bucket.totalCount());
    }

    axisY->setRange(0, maxTotal);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    timelineChartView->setChart(chart);
}