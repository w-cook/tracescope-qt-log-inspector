#pragma once

#include <optional>

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

#include "analysis/EventTimelineAnalyzer.h"
#include "analysis/TelemetryIssueAnalyzer.h"
#include "controllers/InvestigationController.h"
#include "domain/InvestigationRecord.h"
#include "exporting/InvestigationCsvExporter.h"
#include "importing/ImportProfile.h"
#include "importing/ImportResult.h"
#include "preferences/RecentItemsStore.h"
#include "workspace/InvestigationWorkspace.h"

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
class QTimer;
class QAction;
class QScrollBar;
class QTabBar;
class QMenu;

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
    QSettings settings;
    RecentItemsStore recentItemsStore;

    QMenu *recentFilesMenu = nullptr;

    QTabBar *sessionTabBar;
    QLabel *summaryLabel;
    QTableView *eventTable;
    QPlainTextEdit *eventDetailText;
    QTableWidget *issueSummaryTable;
    QGroupBox *issueSummaryGroup;

    InvestigationWorkspace *workspace;
    InvestigationController *investigationController =
        nullptr;

    TelemetryIssueAnalyzer issueAnalyzer;
    EventTimelineAnalyzer timelineAnalyzer;
    InvestigationCsvExporter csvExporter;

    QMetaObject::Connection eventSelectionConnection;

    QComboBox *levelFilterCombo;
    QComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;

    QTimer *searchDebounceTimer;

    QAction *openAction = nullptr;
    QAction *reloadAction = nullptr;

    QFutureWatcher<ImportResult> *importWatcher =
        nullptr;

    bool hasSeverityData = false;
    bool hasSubsystemData = false;

    std::optional<QDateTime> timelineFirstTimestamp;
    std::optional<QDateTime> timelineLastTimestamp;

    QComboBox *timelineIntervalCombo =
        nullptr;

    QScrollBar *timelineScrollBar =
        nullptr;

    QLabel *timelineRangeLabel =
        nullptr;

    bool timelineScaleValid =
        false;

    qint64 timelineScaleIntervalMilliseconds =
        0;

    int timelineScaleMaximum =
        1;

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
        const ImportProfile &profile,
        const QString &reloadSessionId =
        QString()
        );
    void completeLogFileImport(
        const QString &filePath,
        const ImportProfile &profile,
        ImportResult result,
        const QString &reloadSessionId
        );

    void updateSummary(
        const QVector<InvestigationRecord> &records,
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
        const QVector<InvestigationRecord> &records
        );

    void exportFilteredResults();

    QGroupBox *buildTimelinePanel();
    void updateTimelineChart(
        const QVector<InvestigationRecord> &records
        );

    void updateTimelineRangeLabel(
        int scrollValue
        );

    void updateDataCapabilities();

    void bindActiveSession();
    void connectEventTableSelectionModel();
    void reloadActiveSession();

    void refreshRecentFilesMenu();

    void openRecentFile(
        const QString &filePath
        );
};