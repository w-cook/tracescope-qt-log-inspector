#include "MainWindow.h"

#include <QLabel>
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

    buildLayout();
}

void MainWindow::buildLayout()
{
    eventTable->setHorizontalHeaderLabels({
        "Timestamp",
        "Level",
        "Subsystem",
        "Event Code",
        "Message"
    });

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    layout->addWidget(summaryLabel);
    layout->addWidget(eventTable);

    setCentralWidget(central);
}