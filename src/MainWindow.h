#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

#include "exporting/InvestigationCsvExporter.h"
#include "importing/ImportProfile.h"
#include "importing/ImportResult.h"
#include "preferences/FilterPresetStore.h"
#include "preferences/RecentItemsStore.h"
#include "workspace/InvestigationWorkspace.h"
#include "workspace/WorkspacePersistenceState.h"

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

    InvestigationCsvExporter csvExporter;

    QAction *openAction = nullptr;
    QAction *reloadAction = nullptr;
    QAction *compareAction = nullptr;
    QAction *saveWorkspaceAction = nullptr;
    QAction *saveWorkspaceAsAction = nullptr;

    QString currentWorkspacePath;

    QFutureWatcher<ImportResult> *importWatcher =
        nullptr;

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

    void exportFilteredResults();

    void reloadActiveSession();

    void createSessionComparison(
        const QString &preferredBaselineSessionId =
        QString()
        );
    void updateComparisonActionState();

    void refreshRecentFilesMenu();

    void openRecentFile(
        const QString &filePath
        );

    WorkspacePersistenceState
    captureWorkspaceState() const;

    bool saveWorkspaceToFile(
        const QString &filePath
        );

    void saveWorkspace();
    void saveWorkspaceAs();
};