#pragma once

#include <QMainWindow>
#include <QVector>

#include "domain/TelemetryEvent.h"
#include "parsing/JsonLineLogParser.h"

class QLabel;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *summaryLabel;
    QTableWidget *eventTable;

    JsonLineLogParser parser;
    QVector<TelemetryEvent> currentEvents;

    void buildLayout();
    void createMenus();
    void openLogFile();
    void loadLogFile(const QString &filePath);
    void populateTable(const QVector<TelemetryEvent> &events);
    void updateSummary(const QVector<TelemetryEvent> &events, const QString &filePath);
};