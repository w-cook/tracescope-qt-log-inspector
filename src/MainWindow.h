#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSet>

#include "domain/TelemetryEvent.h"
#include "parsing/JsonLineLogParser.h"
#include "filtering/TelemetryEventFilter.h"
#include "filtering/TelemetryFilterCriteria.h"

class QLabel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QVBoxLayout;

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

    QVector<TelemetryEvent> filteredEvents;

    TelemetryEventFilter eventFilter;

    QComboBox *levelFilterCombo;
    QComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;

    QString currentFilePath;

    void buildLayout();
    void createMenus();
    void openLogFile();
    void loadLogFile(const QString &filePath);
    void populateTable(const QVector<TelemetryEvent> &events);
    void updateSummary(const QVector<TelemetryEvent> &events, const QString &filePath);
    void buildFilterControls(QVBoxLayout *layout);
    void applyFilters();
    void refreshSubsystemFilterOptions();
    TelemetryFilterCriteria currentFilterCriteria() const;
};