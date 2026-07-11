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
class QPlainTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *summaryLabel;
    QTableWidget *eventTable;
    QPlainTextEdit *eventDetailText;

    QVector<TelemetryEvent> currentEvents;
    QVector<TelemetryEvent> filteredEvents;

    JsonLineLogParser parser;
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
    void buildDetailPanel(QVBoxLayout *layout);
    void updateEventDetailFromSelection();
    void displayEventDetail(const TelemetryEvent &event);
    void clearEventDetail();
    TelemetryFilterCriteria currentFilterCriteria() const;
};