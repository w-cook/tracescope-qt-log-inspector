#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSet>

#include "domain/TelemetryEvent.h"
#include "parsing/JsonLineLogParser.h"
#include "filtering/TelemetryEventFilter.h"
#include "filtering/TelemetryFilterCriteria.h"
#include "analysis/TelemetryIssueAnalyzer.h"
#include "exporting/TelemetryCsvExporter.h"
#include "analysis/EventTimelineAnalyzer.h"

class QLabel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QVBoxLayout;
class QPlainTextEdit;
class QGroupBox;
class QChartView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *summaryLabel;
    QTableWidget *eventTable;
    QPlainTextEdit *eventDetailText;
    QTableWidget *issueSummaryTable;

    QVector<TelemetryEvent> currentEvents;
    QVector<TelemetryEvent> filteredEvents;

    JsonLineLogParser parser;
    TelemetryEventFilter eventFilter;
    TelemetryIssueAnalyzer issueAnalyzer;
    EventTimelineAnalyzer timelineAnalyzer;
    TelemetryCsvExporter csvExporter;

    QComboBox *levelFilterCombo;
    QComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;

    QString currentFilePath;

    QChartView *timelineChartView;

    void buildLayout();
    void createMenus();
    void openLogFile();
    void loadLogFile(const QString &filePath);
    void populateTable(const QVector<TelemetryEvent> &events);
    void updateSummary(const QVector<TelemetryEvent> &events, const QString &filePath);
    void buildFilterControls(QVBoxLayout *layout);
    TelemetryFilterCriteria currentFilterCriteria() const;
    void applyFilters();
    void refreshSubsystemFilterOptions();
    QGroupBox *buildDetailPanel();
    void updateEventDetailFromSelection();
    void displayEventDetail(const TelemetryEvent &event);
    void clearEventDetail();
    QGroupBox *buildIssueSummaryPanel();
    void updateIssueSummary(const QVector<TelemetryEvent> &events);
    void exportFilteredResults();
    QGroupBox *buildTimelinePanel();
    void updateTimelineChart(const QVector<TelemetryEvent> &events);
};