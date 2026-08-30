#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

#include <functional>
#include <memory>
#include <optional>

#include "exporting/InvestigationCsvExporter.h"
#include "importing/ImportProfile.h"
#include "importing/ImportResult.h"
#include "preferences/FilterPresetStore.h"
#include "preferences/RecentItemsStore.h"
#include "workspace/InvestigationWorkspace.h"
#include "workspace/WorkspacePersistenceState.h"

using ImportCompletionHandler =
    std::function<
        void(
            std::optional<ImportResult>
            )
        >;

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
class QCloseEvent;
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

    void closeEvent(
        QCloseEvent *event
        ) override;

private:
    QSettings settings;
    RecentItemsStore recentItemsStore;
    FilterPresetStore filterPresetStore;

    QMenu *recentFilesMenu = nullptr;
    QMenu *recentWorkspacesMenu = nullptr;

    InvestigationWorkspace *workspace;

    WorkspaceDocumentHost *workspaceDocumentHost =
        nullptr;

    InvestigationCsvExporter csvExporter;

    QAction *openAction = nullptr;
    QAction *reloadAction = nullptr;
    QAction *compareAction = nullptr;
    QAction *saveWorkspaceAction = nullptr;
    QAction *saveWorkspaceAsAction = nullptr;
    QAction *openWorkspaceAction = nullptr;

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
    bool startLogFileImport(
        const QString &filePath,
        const ImportProfile &profile,
        ImportCompletionHandler completion
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

    void refreshRecentWorkspacesMenu();

    void openRecentFile(
        const QString &filePath
        );

    void openRecentWorkspace(
        const QString &filePath
        );

    WorkspacePersistenceState
    captureWorkspaceState() const;

    bool saveWorkspaceToFile(
        const QString &filePath
        );

    void saveWorkspace();
    void saveWorkspaceAs();

    bool resolveWorkspaceSourcePaths(
        WorkspacePersistenceState &state
        );

    struct WorkspaceOpenOperation;

    void openWorkspace(
        const QString &initialFilePath =
        QString()
        );

    void continueWorkspaceOpen(
        const std::shared_ptr<
            WorkspaceOpenOperation
            > &operation
        );

    void installOpenedWorkspace(
        const std::shared_ptr<
            WorkspaceOpenOperation
            > &operation
        );

    void clearCurrentWorkspace();
};