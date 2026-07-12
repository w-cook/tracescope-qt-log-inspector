#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSet>

#include "domain/TelemetryEvent.h"
#include "parsing/JsonLineLogParser.h"
#include "filtering/TelemetryEventFilter.h"
#include "filtering/TelemetryFilterCriteria.h"
#include "analysis/TelemetryIssueAnalyzer.h"

class QLabel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QVBoxLayout;
class QPlainTextEdit;
class QGroupBox;

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
    void buildIssueSummaryPanel(QVBoxLayout *layout);
    void updateIssueSummary(const QVector<TelemetryEvent> &events);
    TelemetryFilterCriteria currentFilterCriteria() const;
};