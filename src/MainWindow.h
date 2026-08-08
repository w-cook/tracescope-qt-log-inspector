#pragma once

#include <QMainWindow>
#include <QVector>

#include "domain/InvestigationRecord.h"
#include "domain/TelemetryEvent.h"
#include "parsing/JsonLineLogParser.h"
#include "models/InvestigationTableModel.h"
#include "models/InvestigationFilterProxyModel.h"
#include "analysis/TelemetryIssueAnalyzer.h"
#include "exporting/TelemetryCsvExporter.h"
#include "analysis/EventTimelineAnalyzer.h"

class QLabel;
class QTableView;
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
    QTableView *eventTable;
    QPlainTextEdit *eventDetailText;
    QTableWidget *issueSummaryTable;

    InvestigationTableModel *eventModel;
    InvestigationFilterProxyModel *eventProxyModel;

    JsonLineLogParser parser;
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

    void updateSummary(
        const QVector<TelemetryEvent> &events,
        const QString &filePath
        );

    void buildFilterControls(QVBoxLayout *layout);
    void applyFilters();
    void refreshSubsystemFilterOptions();

    QVector<InvestigationRecord> visibleRecords() const;

    QGroupBox *buildDetailPanel();
    void updateEventDetailFromSelection();
    void displayEventDetail(
        const InvestigationRecord &record
        );
    void clearEventDetail();

    QGroupBox *buildIssueSummaryPanel();
    void updateIssueSummary(
        const QVector<TelemetryEvent> &events
        );

    void exportFilteredResults();

    QGroupBox *buildTimelinePanel();
    void updateTimelineChart(
        const QVector<TelemetryEvent> &events
        );
};