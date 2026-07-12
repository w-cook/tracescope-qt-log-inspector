#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
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
#include <utility>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      summaryLabel(new QLabel("No log file loaded.")),
      eventTable(new QTableWidget(0, 5)),
      eventDetailText(new QPlainTextEdit(this)),
      issueSummaryTable(new QTableWidget(0, 4)),
      levelFilterCombo(new QComboBox(this)),
      subsystemFilterCombo(new QComboBox(this)),
      searchInput(new QLineEdit(this))
{
    setWindowTitle("TraceScope — Qt Telemetry Log Inspector");
    resize(1100, 700);

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
}

void MainWindow::buildLayout()
{
    eventTable->setHorizontalHeaderLabels({
        "Timestamp",
        "Level",
        "Subsystem",
        "Event Code",
        "Entity ID",
        "Message"
    });

    eventTable->horizontalHeader()->setStretchLastSection(true);
    eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    layout->addWidget(summaryLabel);
    buildFilterControls(layout);
    layout->addWidget(eventTable);
    buildIssueSummaryPanel(layout);
    buildDetailPanel(layout);

    setCentralWidget(central);
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

void MainWindow::buildDetailPanel(QVBoxLayout *layout)
{
    auto *detailGroup = new QGroupBox("Selected Event Details", this);

    eventDetailText->setReadOnly(true);
    eventDetailText->setPlaceholderText("Select an event row to inspect details.");
    eventDetailText->setMinimumHeight(140);

    auto *detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->addWidget(eventDetailText);

    layout->addWidget(detailGroup);

    connect(eventTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        updateEventDetailFromSelection();
    });
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

void MainWindow::buildIssueSummaryPanel(QVBoxLayout *layout)
{
    auto *issueGroup = new QGroupBox("Grouped Warnings and Errors", this);

    issueSummaryTable->setHorizontalHeaderLabels({
        "Subsystem",
        "Warnings",
        "Errors",
        "Total"
    });

    issueSummaryTable->horizontalHeader()->setStretchLastSection(true);
    issueSummaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    issueSummaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    issueSummaryTable->setMaximumHeight(180);

    auto *issueLayout = new QVBoxLayout(issueGroup);
    issueLayout->addWidget(issueSummaryTable);

    layout->addWidget(issueGroup);
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

    issueSummaryTable->resizeColumnsToContents();
    issueSummaryTable->horizontalHeader()->setStretchLastSection(true);
}