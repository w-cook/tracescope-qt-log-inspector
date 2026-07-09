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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      summaryLabel(new QLabel("No log file loaded.")),
      eventTable(new QTableWidget(0, 5))
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
    layout->addWidget(eventTable);

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

    populateTable(currentEvents);
    updateSummary(currentEvents, filePath);
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
        QString("Loaded %1 events from %2 | INFO: %3 | WARN: %4 | ERROR: %5")
            .arg(events.size())
            .arg(filePath)
            .arg(infoCount)
            .arg(warningCount)
            .arg(errorCount)
        );
}