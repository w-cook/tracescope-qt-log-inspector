#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QVector>
#include <QFutureWatcher>
#include <QSettings>

#include <functional>
#include <memory>
#include <optional>

#include "importing/ImportProfile.h"
#include "importing/ImportResult.h"
#include "preferences/FilterPresetStore.h"
#include "preferences/RecentItemsStore.h"
#include "preferences/RotatedSourceSettingsStore.h"
#include "sources/SourceFamilyConfiguration.h"
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
    RotatedSourceSettingsStore rotatedSourceSettingsStore;

    QMenu *recentFilesMenu = nullptr;
    QMenu *recentWorkspacesMenu = nullptr;

    InvestigationWorkspace *workspace;

    WorkspaceDocumentHost *workspaceDocumentHost =
        nullptr;

    /*
     * InvestigationWorkspace::addSession() emits
     * sessionAdded synchronously before activating the
     * new session. This temporary target tells the
     * sessionAdded handler which document host should
     * receive the new InvestigationSessionView.
     *
     * nullptr means the root document host.
     */
    WorkspaceDocumentHost *sessionDocumentTargetHost =
        nullptr;

    QAction *openAction = nullptr;
    QAction *openSnapshotAction = nullptr;
    QAction *saveSnapshotAction = nullptr;
    QAction *reloadAction = nullptr;
    QAction *compareAction = nullptr;
    QAction *saveWorkspaceAction = nullptr;
    QAction *saveWorkspaceAsAction = nullptr;
    QAction *openWorkspaceAction = nullptr;

    QString currentWorkspacePath;

    QFutureWatcher<ImportResult> *importWatcher =
        nullptr;

    bool workspaceOpenInProgress = false;
    bool sessionReloadInProgress = false;
    bool snapshotOpenInProgress = false;

    void buildLayout();
    void createMenus();
    int addSessionToWorkspace(
        std::unique_ptr<InvestigationSession> session,
        WorkspaceDocumentHost *targetHost = nullptr
        );
    void openLogFile(
        const QString &initialFilePath =
        QString(),
        WorkspaceDocumentHost *targetHost =
        nullptr
        );

    void openInvestigationSnapshot(
        const QString &initialFilePath =
        QString(),
        WorkspaceDocumentHost *targetHost =
        nullptr
        );

    void saveInvestigationSnapshot();

    void loadLogFile(
        const QString &filePath,
        const ImportProfile &profile,
        const QString &reloadSessionId =
        QString(),
        SourceFamilyConfiguration
            sourceFamilyConfiguration = {},
        WorkspaceDocumentHost *targetHost =
        nullptr
        );

    bool startLogFileImport(
        const QString &activeFilePath,
        const QStringList &orderedSourcePaths,
        const ImportProfile &profile,
        ImportCompletionHandler completion
        );

    void completeLogFileImport(
        const QString &filePath,
        const ImportProfile &profile,
        SourceFamilyConfiguration
            sourceFamilyConfiguration,
        qint64 initialLiveFollowByteOffset,
        ImportResult result,
        const QString &reloadSessionId,
        WorkspaceDocumentHost *targetHost =
        nullptr
        );

    void reloadActiveSession();

    void reconnectSnapshotBackedSession(
        const QString &sessionId
        );

    void redefineSessionSourcePath(
        const QString &sessionId
        );

    void useSourceAsAuthoritative(
        const QString &sessionId
        );

    void populateSessionSourceMenu(
        const QString &sessionId,
        QMenu *menu
        );

    void openSourceLocation(
        const QString &sourcePath
        );

    void createSessionComparison(
        const QString &preferredBaselineSessionId =
        QString()
        );
    void updateComparisonActionState();

    void updateReloadActionState();

    void refreshRecentFilesMenu();

    void refreshRecentWorkspacesMenu();

    void openRecentFile(
        const QString &filePath
        );

    void openRecentWorkspace(
        const QString &filePath
        );

    void setWorkspaceOpenInProgress(
        bool inProgress
        );

    void setSessionReloadInProgress(
        bool inProgress
        );

    void setSnapshotOpenInProgress(
        bool inProgress
        );

    enum class WorkspaceSessionRecoveryOutcome
    {
        Ready,
        SkipSession,
        Abort
    };

    WorkspaceSessionRecoveryOutcome
    resolveWorkspaceSessionRecovery(
        PersistedInvestigationSession
            &persistedSession,
        const QString &workspaceFilePath
        );

    WorkspacePersistenceState
    captureWorkspaceState() const;

    bool saveWorkspaceToFile(
        const QString &filePath,
        const QString &snapshotOnlySessionId =
        QString(),
        QString *snapshotOnlyPath =
        nullptr
        );

    void saveWorkspace();
    void saveWorkspaceAs();

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

    void exportInvestigationReport(
        const QString &originDocumentId
        );

    std::optional<SourceFamilyConfiguration>
    resolveSourceFamilyConfiguration(
        const QString &activeFilePath,
        SourceFamilyConfiguration configuration
        );

    void preserveSessionAsSnapshotOnly(
        const QString &sessionId
        );
};