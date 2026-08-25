#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

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
class QVBoxLayout;
class QPlainTextEdit;
class QGroupBox;
class QChartView;
class QDragEnterEvent;
class QDropEvent;
class QAction;
class QScrollBar;
class QMenu;
class QWidget;
class WorkspaceDocumentHost;
class InvestigationSessionView;
class InvestigationAnalyticsPanel;
class InvestigationEventDetailPanel;
class InvestigationEventPanel;
class InvestigationFilterPanel;
class InvestigationFindingsPanel;
class InvestigationIssueSummaryPanel;
class InvestigationReviewPanel;
class InvestigationTimelinePanel;

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

    InvestigationFilterPanel *filterPanel =
        nullptr;

    InvestigationTimelinePanel *timelinePanel =
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

    InvestigationCsvExporter csvExporter;

    QAction *openAction = nullptr;
    QAction *reloadAction = nullptr;

    QFutureWatcher<ImportResult> *importWatcher =
        nullptr;

    bool hasSeverityData = false;
    bool hasSubsystemData = false;

    QString currentFilePath;

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

    void applyFilters();

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
    void applyTimelineDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
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

    void bindActiveSession();
    void reloadActiveSession();

    void refreshRecentFilesMenu();

    void openRecentFile(
        const QString &filePath
        );

    void drillDownBurst(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        );
};