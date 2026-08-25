#pragma once

#include <optional>

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

#include "analysis/EventTimelineAnalyzer.h"
#include "analysis/InvestigationAnalyticsAnalyzer.h"
#include "controllers/InvestigationController.h"
#include "domain/InvestigationRecord.h"
#include "exporting/InvestigationCsvExporter.h"
#include "importing/ImportProfile.h"
#include "importing/ImportResult.h"
#include "preferences/FilterPresetStore.h"
#include "preferences/RecentItemsStore.h"
#include "workspace/InvestigationWorkspace.h"

class QLabel;
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
class QMenu;
class MultiSelectFilterComboBox;
class CustomFieldFilterEditor;
class QPushButton;
class QCheckBox;
class QDateTimeEdit;
class QWidget;
class QDialog;
class WorkspaceDocumentHost;
class InvestigationSessionView;
class InvestigationAnalyticsPanel;
class InvestigationEventDetailPanel;
class InvestigationEventPanel;
class InvestigationFindingsPanel;
class InvestigationIssueSummaryPanel;
class InvestigationReviewPanel;

enum class InvestigationIssueDrillDownType;

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
    FilterPresetStore filterPresetStore;

    QMenu *recentFilesMenu = nullptr;

    InvestigationWorkspace *workspace;

    WorkspaceDocumentHost *workspaceDocumentHost =
        nullptr;

    QWidget *investigationSurface =
        nullptr;

    /*
     * Transitional owner of the shared investigation
     * surface while the session UI is being extracted
     * into independently renderable components.
     */
    InvestigationSessionView *surfaceSessionView =
        nullptr;

    InvestigationEventPanel *eventPanel =
        nullptr;

    InvestigationReviewPanel *reviewPanel =
        nullptr;

    InvestigationIssueSummaryPanel
        *issueSummaryPanel = nullptr;

    InvestigationFindingsPanel *findingsPanel =
        nullptr;

    InvestigationAnalyticsPanel *analyticsPanel =
        nullptr;

    InvestigationEventDetailPanel
        *eventDetailPanel = nullptr;

    InvestigationController *investigationController =
        nullptr;

    EventTimelineAnalyzer timelineAnalyzer;
    InvestigationAnalyticsAnalyzer analyticsAnalyzer;

    InvestigationCsvExporter csvExporter;

    MultiSelectFilterComboBox *levelFilterCombo;
    MultiSelectFilterComboBox *subsystemFilterCombo;
    QLineEdit *searchInput;
    QPushButton *resetFiltersButton;

    QCheckBox *bookmarksOnlyCheckBox;

    QPushButton *filterPresetsButton;
    QMenu *filterPresetsMenu;

    QPushButton *customFiltersButton;
    QDialog *customFiltersDialog;

    QPushButton *timeRangeButton;
    QDialog *timeRangeDialog;

    MultiSelectFilterComboBox *eventCodeFilterCombo;
    MultiSelectFilterComboBox *entityFilterCombo;
    MultiSelectFilterComboBox *findingStatusFilterCombo;

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

    QWidget *timelineBreakdownWidget =
        nullptr;

    QComboBox *timelineBreakdownCombo =
        nullptr;

    QWidget *timelineSubsystemShowWidget =
        nullptr;

    QComboBox *timelineSubsystemLimitCombo =
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

    InvestigationFilterPreset currentFilterPreset(
        const QString &name
        ) const;

    void applyFilterPreset(
        const InvestigationFilterPreset &preset
        );

    void refreshFilterPresetsMenu();

    void updateCustomFiltersButton();

    void updateTimeRangeButton();

    void updateTimelineBreakdownControls();

    void updateEventDetailFromSelection();
    void clearEventDetail();

    const InvestigationRecord *
    selectedEventRecord() const;

    void toggleSelectedEventBookmark();

    void updateInvestigationStateControls();
    void updateSelectedEventFindingStatus();
    void editSelectedEventNote();

    void syncInvestigationStatePresentation();

    void selectProxyRow(
        int proxyRow
        );

    void updateIssueSummary(
        const QVector<InvestigationRecord> &records
        );
    void drillDownIssueSummary(
        const QString &subsystem,
        InvestigationIssueDrillDownType type
        );
    void drillDownTimelineBucket(
        int visibleBucketIndex,
        const QString &severity,
        const QString &subsystem
        );

    void updateFindingsPanel();

    void navigateToFinding(
        const QString &recordId
        );

    void revealFindingRecord(
        const InvestigationRecord &record
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
    void reloadActiveSession();

    void refreshRecentFilesMenu();

    void openRecentFile(
        const QString &filePath
        );

    void resizeCustomFiltersDialogToContents();

    void drillDownBurst(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        );
};