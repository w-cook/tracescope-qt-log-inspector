#pragma once

#include <QMainWindow>
#include <QVector>

#include "domain/InvestigationRecord.h"
#include "domain/TelemetryEvent.h"
#include "importing/ImportProfile.h"
#include "controllers/InvestigationController.h"
#include "analysis/TelemetryIssueAnalyzer.h"
#include "exporting/InvestigationCsvExporter.h"
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
class QDragEnterEvent;
class QDropEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(
        QDragEnterEvent *event
        ) override;

    void dropEvent(
        QDropEvent *event
        ) override;

private:
    QLabel *summaryLabel;
    QTableView *eventTable;
    QPlainTextEdit *eventDetailText;
    QTableWidget *issueSummaryTable;
    QGroupBox *issueSummaryGroup;

    InvestigationController *investigationController;

    TelemetryIssueAnalyzer issueAnalyzer;
    EventTimelineAnalyzer timelineAnalyzer;
    InvestigationCsvExporter csvExporter;

    QComboBox *levelFilterCombo;
    QComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;

    bool hasSeverityData = false;
    bool hasSubsystemData = false;

    QString currentFilePath;

    QChartView *timelineChartView;

    void buildLayout();
    void createMenus();
    void openLogFile(
        const QString &initialFilePath =
        QString()
        );
    void loadLogFile(
        const QString &filePath,
        const ImportProfile &profile
        );

    void updateSummary(
        const QVector<TelemetryEvent> &events,
        const QString &filePath
        );

    void buildFilterControls(QVBoxLayout *layout);
    void applyFilters();
    void refreshSubsystemFilterOptions();

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
        const QVector<TelemetryEvent> &events,
        const QVector<TelemetryEvent> &rangeEvents
        );

    void updateDataCapabilities();
};