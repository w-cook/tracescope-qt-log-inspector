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
class MultiSelectFilterComboBox;
class CustomFieldFilterEditor;
class QPushButton;
class QCheckBox;
class QDateTimeEdit;
class QWidget;
class QDialog;

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

    MultiSelectFilterComboBox *levelFilterCombo;
    MultiSelectFilterComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;
    QPushButton *resetFiltersButton;

    QPushButton *customFiltersButton;
    QDialog *customFiltersDialog;

    QPushButton *timeRangeButton;
    QDialog *timeRangeDialog;

    MultiSelectFilterComboBox *eventCodeFilterCombo;
    MultiSelectFilterComboBox *entityFilterCombo;

    CustomFieldFilterEditor *customFieldFilterEditor;

    QWidget *eventCodeFilterWidget = nullptr;
    QWidget *entityFilterWidget = nullptr;

    bool hasEventCodeData = false;
    bool hasEntityData = false;
    bool hasCustomFieldData = false;

    QWidget *timeRangeFilterWidget = nullptr;

    QCheckBox *timeRangeStartCheckBox = nullptr;
    QDateTimeEdit *timeRangeStartEdit = nullptr;

    QCheckBox *timeRangeEndCheckBox = nullptr;
    QDateTimeEdit *timeRangeEndEdit = nullptr;

    bool hasTimestampData = false;

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
    void resetFilters();
    void refreshSubsystemFilterOptions();
    void refreshCanonicalFilterOptions();

    void updateCustomFiltersButton();

    void updateTimeRangeButton();

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

    std::optional<QDateTime>
    effectiveTimelineFirstTimestamp() const;

    std::optional<QDateTime>
    effectiveTimelineLastTimestamp() const;

    void updateDataCapabilities();

    void bindActiveSession();
    void connectEventTableSelectionModel();
    void reloadActiveSession();

    void refreshRecentFilesMenu();

    void openRecentFile(
        const QString &filePath
        );

    void resizeCustomFiltersDialogToContents();
};