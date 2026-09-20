#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPromise>
#include <QProgressDialog>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QtConcurrentRun>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "exporting/InvestigationReportHtmlExporter.h"
#include "exporting/InvestigationReportSnapshotBuilder.h"
#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "importing/SourceFamilyImportService.h"
#include "live/LiveSessionFollowCoordinator.h"
#include "persistence/InvestigationSessionSnapshotFile.h"
#include "sources/RotatedSourceDiscoveryService.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/InvestigationComparisonDialog.h"
#include "ui/InvestigationReportExportDialog.h"
#include "ui/workspace/InvestigationComparisonDocument.h"
#include "ui/workspace/InvestigationReportWorkspaceContext.h"
#include "ui/workspace/InvestigationSessionView.h"
#include "ui/workspace/WorkspaceDocument.h"
#include "ui/workspace/WorkspaceDocumentHost.h"
#include "workspace/HybridInvestigationReconstructionService.h"
#include "workspace/InvestigationComparisonPersistence.h"
#include "workspace/InvestigationComparisonSnapshotBuilder.h"
#include "workspace/InvestigationSessionPersistence.h"
#include "workspace/InvestigationSessionRestorationService.h"
#include "workspace/WorkspaceSavePackageService.h"
#include "workspace/WorkspaceSerialization.h"

namespace
{
QString findingStatusDisplayText(
    FindingStatus status
    )
{
    switch (status) {
    case FindingStatus::Open:
        return QObject::tr("Open");

    case FindingStatus::Resolved:
        return QObject::tr("Resolved");

    case FindingStatus::Dismissed:
        return QObject::tr("Dismissed");

    case FindingStatus::None:
        return QString();
    }

    return QString();
}

class FindingTextDelegate
    : public QStyledItemDelegate
{
public:
    explicit FindingTextDelegate(
        QObject *parent = nullptr
        )
        : QStyledItemDelegate(parent)
    {
    }

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
        ) const override
    {
        QStyleOptionViewItem itemOption(
            option
            );

        initStyleOption(
            &itemOption,
            index
            );

        QStyle *style =
            itemOption.widget != nullptr
                ? itemOption.widget->style()
                : QApplication::style();

        const QString text =
            itemOption.text;

        /*
     * Let Qt paint the normal cell background,
     * selection, focus state, etc., but suppress
     * its single-line text painting.
     */
        itemOption.text.clear();

        style->drawControl(
            QStyle::CE_ItemViewItem,
            &itemOption,
            painter,
            itemOption.widget
            );

        QTextDocument document;

        document.setDocumentMargin(
            0.0
            );

        document.setDefaultFont(
            option.font
            );

        QTextOption textOption;

        textOption.setWrapMode(
            QTextOption::
            WrapAtWordBoundaryOrAnywhere
            );

        document.setDefaultTextOption(
            textOption
            );

        document.setPlainText(
            text
            );

        constexpr int horizontalPadding = 8;
        constexpr int verticalPadding = 4;

        document.setTextWidth(
            std::max(
                1,
                option.rect.width()
                    - horizontalPadding
                )
            );

        /*
     * Choose the text color explicitly rather than
     * relying on QTextDocument to infer it from the
     * view's changing active/inactive palette.
     */
        QPalette::ColorGroup colorGroup;

        if (!(itemOption.state
              & QStyle::State_Enabled)) {
            colorGroup =
                QPalette::Disabled;
        } else if (
            itemOption.state
            & QStyle::State_Active
            ) {
            colorGroup =
                QPalette::Active;
        } else {
            colorGroup =
                QPalette::Inactive;
        }

        const bool selected =
            itemOption.state
            & QStyle::State_Selected;

        const QColor textColor =
            selected
                ? itemOption.palette.color(
                      colorGroup,
                      QPalette::HighlightedText
                      )
                : itemOption.palette.color(
                      colorGroup,
                      QPalette::Text
                      );

        QAbstractTextDocumentLayout::PaintContext
            context;

        context.palette =
            itemOption.palette;

        /*
     * QTextDocument normally uses Text, but setting
     * both roles makes the intended foreground
     * unambiguous across platform styles.
     */
        context.palette.setColor(
            QPalette::Text,
            textColor
            );

        context.palette.setColor(
            QPalette::WindowText,
            textColor
            );

        context.clip =
            QRectF(
                0,
                0,
                document.textWidth(),
                std::max(
                    1,
                    option.rect.height()
                        - verticalPadding
                    )
                );

        painter->save();

        painter->translate(
            option.rect.left() + 4,
            option.rect.top() + 2
            );

        document
            .documentLayout()
            ->draw(
                painter,
                context
                );

        painter->restore();
    }

    QSize sizeHint(
        const QStyleOptionViewItem &option,
        const QModelIndex &index
        ) const override
    {
        QStyleOptionViewItem itemOption(
            option
            );

        initStyleOption(
            &itemOption,
            index
            );

        QTextDocument document;

        document.setDocumentMargin(
            0.0
            );

        document.setDefaultFont(
            itemOption.font
            );

        QTextOption textOption;

        textOption.setWrapMode(
            QTextOption::
            WrapAtWordBoundaryOrAnywhere
            );

        document.setDefaultTextOption(
            textOption
            );

        document.setPlainText(
            itemOption.text
            );

        document.setTextWidth(
            std::max(
                1,
                itemOption.rect.width() - 8
                )
            );

        return QSize(
            itemOption.rect.width(),
            static_cast<int>(
                document.size().height()
                ) + 6
            );
    }
};

QString investigationReportFileName(
    const QString &title
    )
{
    QString baseName =
        title.trimmed();

    if (baseName.isEmpty()) {
        baseName =
            QStringLiteral(
                "investigation-report"
                );
    }

    const QString invalidCharacters =
        QStringLiteral(
            "<>:\"/\\|?*"
            );

    QString safeName;

    safeName.reserve(
        baseName.size()
        );

    bool previousSeparator =
        false;

    for (const QChar character
         : std::as_const(baseName)) {
        const bool invalid =
            character.unicode() < 32
            || invalidCharacters.contains(
                character
                )
            || character.isSpace();

        if (invalid) {
            if (!previousSeparator
                && !safeName.isEmpty()) {
                safeName.append(
                    QLatin1Char('-')
                    );

                previousSeparator =
                    true;
            }

            continue;
        }

        safeName.append(
            character
            );

        previousSeparator =
            false;
    }

    while (
        safeName.endsWith(
            QLatin1Char('-')
            )
        ) {
        safeName.chop(1);
    }

    if (safeName.isEmpty()) {
        safeName =
            QStringLiteral(
                "investigation-report"
                );
    }

    return safeName
           + QStringLiteral(
               ".html"
               );
}

QString investigationReportOutputPath(
    const QString &selectedPath
    )
{
    if (selectedPath.trimmed().isEmpty()) {
        return QString();
    }

    const QFileInfo info(
        selectedPath
        );

    if (!info.suffix().isEmpty()) {
        return selectedPath;
    }

    return selectedPath
           + QStringLiteral(
               ".html"
               );
}

QString resolveWorkspaceSnapshotPath(
    const QString &workspaceFilePath,
    const QString &snapshotReference
    )
{
    const QFileInfo referenceInfo(
        snapshotReference
        );

    if (referenceInfo.isAbsolute()) {
        return QDir::cleanPath(
            referenceInfo.absoluteFilePath()
            );
    }

    if (workspaceFilePath.trimmed().isEmpty()) {
        return {};
    }

    const QDir workspaceDirectory(
        QFileInfo(workspaceFilePath)
            .absolutePath()
        );

    return QDir::cleanPath(
        workspaceDirectory.absoluteFilePath(
            snapshotReference
            )
        );
}

struct ActiveSessionReloadPreparationResult
{
    bool succeeded = false;

    QString sessionId;
    QString errorMessage;

    InvestigationSessionBackingMode mode =
        InvestigationSessionBackingMode::
        SourceBacked;

    InvestigationSessionSnapshot snapshot;

    std::optional<
        HybridInvestigationReconstructionResult>
        hybridReconstruction;
};

struct SnapshotReconnectPreparationResult
{
    bool succeeded = false;

    QString sessionId;
    QString errorMessage;

    InvestigationSessionSnapshot snapshot;

    std::optional<
        HybridInvestigationReconstructionResult>
        reconstruction;
};
}

struct MainWindow::WorkspaceOpenOperation
{
    QString workspacePath;

    WorkspacePersistenceState state;

    int nextSessionIndex = 0;
    int skippedSessionCount = 0;
    int emptySessionCount = 0;

    std::vector<
        std::unique_ptr<InvestigationSession>
        > stagedSessions;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    settings(),
    recentItemsStore(settings),
    filterPresetStore(settings),
    rotatedSourceSettingsStore(settings),
    workspace(new InvestigationWorkspace(this))
{
    setWindowTitle("TraceScope — Qt Telemetry Log Inspector");
    resize(1100, 760);

    setAcceptDrops(true);

    createMenus();
    buildLayout();

    connect(
        workspace,
        &InvestigationWorkspace::sessionAdded,
        this,
        [this](int index) {
            updateComparisonActionState();

            InvestigationSession *session =
                workspace->sessionAt(index);

            WorkspaceDocumentHost *targetHost =
                sessionDocumentTargetHost != nullptr
                    ? sessionDocumentTargetHost
                    : workspaceDocumentHost;

            if (session == nullptr
                || targetHost == nullptr) {
                return;
            }

            auto *sessionView =
                new InvestigationSessionView(
                    session,
                    &filterPresetStore
                    );

            connect(
                sessionView,
                &InvestigationSessionView::
                liveFollowStateChanged,
                this,
                &MainWindow::
                updateReloadActionState
                );

            /*
             * InvestigationWorkspace::addSession()
             * emits sessionAdded before it activates
             * the new session. Avoid letting QTabWidget
             * selection get ahead of workspace state.
             */
            const QSignalBlocker blocker(
                targetHost
                );

            if (!targetHost
                     ->addDocument(
                         sessionView,
                         false
                         )) {
                delete sessionView;
            }
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::sessionClosed,
        this,
        [this](int) {
            updateComparisonActionState();

            if (workspaceDocumentHost
                == nullptr) {
                return;
            }

            /*
             * Document position is independent from
             * workspace session position, and documents
             * may now be either docked or detached.
             *
             * Take a snapshot before removing anything
             * so host membership can safely change while
             * this loop runs.
             */
            const QVector<WorkspaceDocument *>
                documents =
                workspaceDocumentHost
                    ->documents();

            const QSignalBlocker blocker(
                workspaceDocumentHost
                );

            for (WorkspaceDocument *document
                 : documents) {
                auto *sessionView =
                    qobject_cast<
                        InvestigationSessionView *>(
                        document
                        );

                if (sessionView == nullptr) {
                    continue;
                }

                const QString documentId =
                    sessionView->documentId();

                if (workspace->indexOfSession(
                        documentId
                        )
                    >= 0) {
                    continue;
                }

                WorkspaceDocument *removed =
                    workspaceDocumentHost
                        ->removeDocument(
                            documentId
                            );

                if (removed != nullptr) {
                    removed->deleteLater();
                }
            }
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::
        activeSessionChanged,
        this,
        [this](int index) {
            InvestigationSession *session =
                workspace->sessionAt(
                    index
                    );

            if (
                session != nullptr
                && workspaceDocumentHost
                       != nullptr
                ) {
                const QSignalBlocker blocker(
                    workspaceDocumentHost
                    );

                workspaceDocumentHost
                    ->setCurrentDocument(
                        session->id()
                        );
            }

            updateReloadActionState();
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::sessionReloaded,
        this,
        [this](int index) {
            InvestigationSession *session =
                workspace->sessionAt(
                    index
                    );

            if (
                session == nullptr
                || workspaceDocumentHost
                       == nullptr
                ) {
                return;
            }

            WorkspaceDocument *document =
                workspaceDocumentHost
                    ->documentById(
                        session->id()
                        );

            auto *sessionView =
                qobject_cast<
                    InvestigationSessionView *>(
                    document
                    );

            if (sessionView != nullptr) {
                sessionView->refreshSession();
            }
        }
        );

    connect(
        workspaceDocumentHost,
        &WorkspaceDocumentHost::
            currentDocumentChanged,
        this,
        [this](
            const QString &documentId
            ) {
            /*
             * Comparison documents will eventually use
             * the same host. Only IDs belonging to real
             * investigation sessions affect active
             * session state here.
             */
            const int sessionIndex =
                workspace->indexOfSession(
                    documentId
                    );

            if (sessionIndex < 0) {
                return;
            }

            workspace->setActiveSession(
                sessionIndex
                );
        }
        );

    connect(
        workspaceDocumentHost,
        &WorkspaceDocumentHost::
        documentCloseRequested,
        this,
        [this](
            const QString &documentId
            ) {
            const int sessionIndex =
                workspace->indexOfSession(
                    documentId
                    );

            if (sessionIndex >= 0) {
                workspace->closeSession(
                    sessionIndex
                    );

                return;
            }

            /*
             * Non-session workspace documents, such as
             * immutable comparisons, are owned directly
             * by the document workspace rather than by
             * InvestigationWorkspace.
             */
            WorkspaceDocument *document =
                workspaceDocumentHost
                    ->removeDocument(
                        documentId
                        );

            if (document != nullptr) {
                document->deleteLater();
            }
        }
        );

    connect(
        workspaceDocumentHost,
        &WorkspaceDocumentHost::
        documentContextMenuAboutToShow,
        this,
        [this](
            const QString &documentId,
            QMenu *menu
            ) {
            if (menu == nullptr
                || workspace == nullptr
                || workspace->indexOfSession(
                    documentId
                    )
                    < 0) {
                return;
            }

            /*
             * Session-specific source lifecycle actions are
             * available even when this is the only
             * investigation in the workspace.
             */
            menu->addSeparator();

            populateSessionSourceMenu(
                documentId,
                menu
                );

            /*
             * Comparison requires at least two investigation
             * sessions.
             */
            if (workspace->sessionCount()
                < 2) {
                return;
            }

            menu->addSeparator();

            QAction *comparisonAction =
                menu->addAction(
                    tr(
                        "Create Comparison..."
                        )
                    );

            connect(
                comparisonAction,
                &QAction::triggered,
                menu,
                [this, documentId]() {
                    const InvestigationSession
                        *activeSession =
                        workspace
                            ->activeSession();

                    QString preferredBaselineId;

                    if (activeSession != nullptr
                        && activeSession->id()
                            != documentId) {
                        /*
                         * The user is examining the active
                         * session and explicitly clicked
                         * another session:
                         *
                         * clicked -> Baseline
                         * active  -> Comparison
                         */
                        preferredBaselineId =
                            documentId;
                    }

                    createSessionComparison(
                        preferredBaselineId
                        );
                }
                );
        }
        );

    connect(
        workspaceDocumentHost,
        &WorkspaceDocumentHost::
        investigationReportExportRequested,
        this,
        &MainWindow::
        exportInvestigationReport
        );
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");

    openAction =
        new QAction(
            "&Open Log File...",
            this
            );
    openAction->setShortcut(QKeySequence::Open);

    openAction->setShortcutContext(
        Qt::ApplicationShortcut
        );

    connect(openAction, &QAction::triggered, this, [this]() {
        openLogFile();
    });

    fileMenu->addAction(openAction);

    openSnapshotAction =
        new QAction(
            tr(
                "Open Investigation &Snapshot..."
                ),
            this
            );

    connect(
        openSnapshotAction,
        &QAction::triggered,
        this,
        [this]() {
            openInvestigationSnapshot();
        }
        );

    fileMenu->addAction(
        openSnapshotAction
        );

    openWorkspaceAction =
        new QAction(
            tr("Open &Workspace..."),
            this
            );

    connect(
        openWorkspaceAction,
        &QAction::triggered,
        this,
        [this]() {
            openWorkspace();
        }
        );

    fileMenu->addAction(
        openWorkspaceAction
        );

    recentFilesMenu =
        fileMenu->addMenu(
            tr("Recent &Files")
            );

    recentFilesMenu->setToolTipsVisible(
        true
        );

    connect(
        recentFilesMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            refreshRecentFilesMenu();
        }
        );

    refreshRecentFilesMenu();

    recentWorkspacesMenu =
        fileMenu->addMenu(
            tr("Recent &Workspaces")
            );

    recentWorkspacesMenu->setToolTipsVisible(
        true
        );

    connect(
        recentWorkspacesMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            refreshRecentWorkspacesMenu();
        }
        );

    refreshRecentWorkspacesMenu();

    fileMenu->addSeparator();

    saveSnapshotAction =
        new QAction(
            tr(
                "Save Investigation &Snapshot..."
                ),
            this
            );

    saveSnapshotAction->setEnabled(
        false
        );

    connect(
        saveSnapshotAction,
        &QAction::triggered,
        this,
        [this]() {
            saveInvestigationSnapshot();
        }
        );

    fileMenu->addAction(
        saveSnapshotAction
        );

    reloadAction =
        new QAction(
            tr("&Reload Current Session"),
            this
            );

    reloadAction->setShortcut(
        QKeySequence::Refresh
        );

    reloadAction->setEnabled(false);

    connect(
        reloadAction,
        &QAction::triggered,
        this,
        [this]() {
            reloadActiveSession();
        }
        );

    fileMenu->addAction(
        reloadAction
        );

    fileMenu->addSeparator();

    saveWorkspaceAction =
        new QAction(
            tr("&Save Workspace"),
            this
            );

    saveWorkspaceAction->setShortcut(
        QKeySequence::Save
        );

    saveWorkspaceAction->setShortcutContext(
        Qt::ApplicationShortcut
        );

    connect(
        saveWorkspaceAction,
        &QAction::triggered,
        this,
        [this]() {
            saveWorkspace();
        }
        );

    fileMenu->addAction(
        saveWorkspaceAction
        );

    saveWorkspaceAsAction =
        new QAction(
            tr("Save Workspace &As..."),
            this
            );

    saveWorkspaceAsAction->setShortcut(
        QKeySequence::SaveAs
        );

    saveWorkspaceAsAction->setShortcutContext(
        Qt::ApplicationShortcut
        );

    connect(
        saveWorkspaceAsAction,
        &QAction::triggered,
        this,
        [this]() {
            saveWorkspaceAs();
        }
        );

    fileMenu->addAction(
        saveWorkspaceAsAction
        );

    auto *exportingMenu =
        menuBar()->addMenu(
            tr("&Exporting")
            );

    connect(
        exportingMenu,
        &QMenu::aboutToShow,
        this,
        [this, exportingMenu]() {
            exportingMenu->clear();

            WorkspaceDocument *document =
                workspaceDocumentHost != nullptr
                    ? workspaceDocumentHost
                          ->currentDocument()
                    : nullptr;

            if (document != nullptr) {
                document->populateExportMenu(
                    exportingMenu
                    );
            }

            if (
                exportingMenu
                    ->actions()
                    .isEmpty()
                ) {
                QAction *unavailableAction =
                    exportingMenu->addAction(
                        tr(
                            "No export actions available"
                            )
                        );

                unavailableAction->setEnabled(
                    false
                    );
            }
        }
        );

    auto *investigationMenu =
        menuBar()->addMenu(
            tr("&Investigation")
            );

    compareAction =
        new QAction(
            tr("&Compare Sessions..."),
            this
            );

    compareAction->setEnabled(
        false
        );

    connect(
        compareAction,
        &QAction::triggered,
        this,
        [this]() {
            createSessionComparison();
        }
        );

    investigationMenu->addAction(
        compareAction
        );

    auto *helpMenu = menuBar()->addMenu("&Help");

    auto *aboutAction = new QAction("&About TraceScope", this);

    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this,
            "About TraceScope",
            "TraceScope is a Qt/C++ telemetry log inspector for loading, filtering, visualizing, and exporting structured diagnostic log files."
            );
    });

    helpMenu->addAction(aboutAction);
}

void MainWindow::buildLayout()
{
    auto *centralWidget =
        new QWidget(this);

    auto *centralLayout =
        new QVBoxLayout(
            centralWidget
            );

    centralLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    workspaceDocumentHost =
        new WorkspaceDocumentHost(
            centralWidget
            );

    centralLayout->addWidget(
        workspaceDocumentHost,
        1
        );

    setCentralWidget(
        centralWidget
        );
}

int MainWindow::addSessionToWorkspace(
    std::unique_ptr<InvestigationSession> session,
    WorkspaceDocumentHost *targetHost
    )
{
    if (workspace == nullptr
        || !session) {
        return -1;
    }

    /*
     * addSession() emits sessionAdded synchronously.
     * Set the desired presentation host only for the
     * duration of that operation, then restore the
     * previous value so workspace restoration and any
     * future nested use remain deterministic.
     */
    WorkspaceDocumentHost *previousTarget =
        sessionDocumentTargetHost;

    sessionDocumentTargetHost =
        targetHost != nullptr
            ? targetHost
            : workspaceDocumentHost;

    const int sessionIndex =
        workspace->addSession(
            std::move(session)
            );

    sessionDocumentTargetHost =
        previousTarget;

    return sessionIndex;
}

void MainWindow::openLogFile(const QString &initialFilePath)
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || snapshotOpenInProgress
        || sessionReloadInProgress) {
        QMessageBox::information(
            this,
            tr("File Operation In Progress"),
            tr(
                "TraceScope is already importing a log file "
                "or opening a workspace. Wait for the current "
                "operation to finish before opening another "
                "file."
                )
            );

        return;
    }

    ImportConfigurationDialog dialog(
        this,
        &recentItemsStore,
        &rotatedSourceSettingsStore
        );

    if (!initialFilePath.isEmpty()) {
        dialog.setSelectedFilePath(
            initialFilePath
            );
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    sourceFamilyConfiguration
        .includeRotatedSources =
        dialog.includeRotatedSources();

    sourceFamilyConfiguration
        .rotationRule =
        dialog.rotatedSourceRule();

    if (sourceFamilyConfiguration
            .includeRotatedSources) {
        for (const RotatedSourceMatch &match
             : dialog.rotatedSources()) {
            sourceFamilyConfiguration
                .rotatedSourcePaths
                .append(
                    match.filePath
                    );
        }
    }

    loadLogFile(
        dialog.selectedFilePath(),
        dialog.configuredProfile(),
        QString(),
        std::move(
            sourceFamilyConfiguration
            )
        );
}

void MainWindow::openInvestigationSnapshot(
    const QString &initialFilePath
    )
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress
        || snapshotOpenInProgress) {
        QMessageBox::information(
            this,
            tr("File Operation In Progress"),
            tr(
                "TraceScope is already importing, "
                "restoring, or opening another file. "
                "Wait for the current operation to "
                "finish before opening an investigation "
                "snapshot."
                )
            );

        return;
    }

    QString filePath =
        initialFilePath;

    if (filePath.isEmpty()) {
        filePath =
            QFileDialog::getOpenFileName(
                this,
                tr(
                    "Open Investigation Snapshot"
                    ),
                QString(),
                tr(
                    "TraceScope Investigation Snapshot "
                    "(*.tsinv);;"
                    "All Files (*)"
                    )
                );
    }

    if (filePath.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(
        filePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        QMessageBox::warning(
            this,
            tr(
                "Open Investigation Snapshot Failed"
                ),
            tr(
                "The selected investigation snapshot "
                "does not exist or is not a file."
                )
            );

        return;
    }

    const QString absoluteSnapshotPath =
        fileInfo.absoluteFilePath();

    /*
     * Standalone snapshot opening intentionally uses
     * the same SnapshotBacked restoration path used
     * when restoring a saved workspace.
     *
     * The absolute snapshot reference means no
     * workspace-relative resolution is required.
     */
    PersistedInvestigationSession
        persistedSession;

    persistedSession.sessionId =
        QUuid::createUuid().toString(
            QUuid::WithoutBraces
            );

    persistedSession.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SnapshotBacked;

    persistedSession
        .backing
        .snapshotReference =
        absoluteSnapshotPath;

    auto *watcher =
        new QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>(
            this
            );

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Opening %1...\n"
                "Loading investigation snapshot..."
                )
                .arg(
                    fileInfo.fileName()
                    ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr(
            "Open Investigation Snapshot"
            )
        );

    progressDialog->setWindowModality(
        Qt::NonModal
        );

    progressDialog->setMinimumDuration(
        500
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    setSnapshotOpenInProgress(
        true
        );

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        cancel
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        finished,
        this,
        [
            this,
            watcher,
            progressDialog,
            persistedSession
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            progressDialog->hide();
            progressDialog->deleteLater();

            if (cancelled) {
                watcher->deleteLater();

                setSnapshotOpenInProgress(
                    false
                    );

                return;
            }

            if (watcher
                    ->future()
                    .resultCount()
                <= 0) {
                watcher->deleteLater();

                setSnapshotOpenInProgress(
                    false
                    );

                QMessageBox::warning(
                    this,
                    tr(
                        "Open Investigation Snapshot Failed"
                        ),
                    tr(
                        "TraceScope did not receive a "
                        "snapshot restoration result."
                        )
                    );

                return;
            }

            InvestigationSessionRestorationPreparationResult
                preparation =
                watcher->result();

            watcher->deleteLater();

            if (!preparation.succeeded) {
                const QString reason =
                    preparation
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "The selected investigation "
                              "snapshot could not be loaded."
                              )
                        : preparation.errorMessage;

                setSnapshotOpenInProgress(
                    false
                    );

                QMessageBox::warning(
                    this,
                    tr(
                        "Open Investigation Snapshot Failed"
                        ),
                    tr(
                        "TraceScope was unable to open "
                        "the selected investigation "
                        "snapshot.\n\n"
                        "Reason:\n%1"
                        )
                        .arg(
                            reason
                            )
                    );

                return;
            }

            /*
             * InvestigationSession and its Qt model
             * graph are materialized back on the UI
             * thread.
             */
            InvestigationSessionRestorationService
                restorationService;

            InvestigationSessionRestorationResult
                restoration =
                restorationService.materialize(
                    persistedSession,
                    std::move(
                        preparation
                        )
                    );

            if (!restoration.succeeded
                || restoration.session
                       == nullptr) {
                const QString reason =
                    restoration
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "The loaded investigation "
                              "snapshot could not be "
                              "materialized."
                              )
                        : restoration.errorMessage;

                setSnapshotOpenInProgress(
                    false
                    );

                QMessageBox::warning(
                    this,
                    tr(
                        "Open Investigation Snapshot Failed"
                        ),
                    tr(
                        "TraceScope was unable to "
                        "materialize the selected "
                        "investigation snapshot.\n\n"
                        "Reason:\n%1"
                        )
                        .arg(
                            reason
                            )
                    );

                return;
            }

            const bool emptySnapshot =
                restoration
                    .session
                    ->importedRecordCount()
                == 0;

            addSessionToWorkspace(
                std::move(
                    restoration.session
                    )
                );

            setSnapshotOpenInProgress(
                false
                );

            if (emptySnapshot) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Empty Investigation Snapshot"
                        ),
                    tr(
                        "The investigation snapshot "
                        "opened successfully, but it "
                        "contains no investigation "
                        "records."
                        )
                    );
            }
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                persistedSession
            ](
                QPromise<
                    InvestigationSessionRestorationPreparationResult>
                    &promise
                ) {
                /*
                 * Snapshot deserialization currently has
                 * no incremental byte-progress callback,
                 * so this operation remains indeterminate.
                 */
                promise.setProgressRange(
                    0,
                    0
                    );

                InvestigationSessionRestorationService
                    restorationService;

                InvestigationSessionRestorationPreparationResult
                    preparation =
                    restorationService.prepare(
                        persistedSession,
                        QString()
                        );

                if (promise.isCanceled()) {
                    return;
                }

                promise.addResult(
                    std::move(
                        preparation
                        )
                    );
            }
            )
        );
}

void MainWindow::saveInvestigationSnapshot()
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress
        || snapshotOpenInProgress) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    /*
     * Freeze the investigation evidence immediately.
     *
     * Unlike Preserve as Snapshot Only, this operation
     * does not change the runtime backing mode. Live
     * following therefore does not need to remain paused
     * while the user chooses a destination or while the
     * standalone snapshot is written.
     */
    const InvestigationSessionSnapshot snapshot =
        session->captureSnapshot();

    QString suggestedFileName =
        session
            ->sourceMetadata()
            .sourceName
            .trimmed();

    if (suggestedFileName.isEmpty()) {
        suggestedFileName =
            QStringLiteral(
                "investigation"
                );
    } else {
        suggestedFileName =
            QFileInfo(
                suggestedFileName
                )
                .completeBaseName();

        if (suggestedFileName.isEmpty()) {
            suggestedFileName =
                QStringLiteral(
                    "investigation"
                    );
        }
    }

    suggestedFileName +=
        QStringLiteral(
            ".tsinv"
            );

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            tr(
                "Save Investigation Snapshot"
                ),
            suggestedFileName,
            tr(
                "TraceScope Investigation Snapshot "
                "(*.tsinv);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    if (QFileInfo(filePath)
            .suffix()
            .compare(
                QStringLiteral(
                    "tsinv"
                    ),
                Qt::CaseInsensitive
                )
        != 0) {
        filePath +=
            QStringLiteral(
                ".tsinv"
                );
    }

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        InvestigationSessionSnapshotFile()
            .save(
                filePath,
                snapshot
                );

    if (!saveResult.isSuccess()) {
        QMessageBox::warning(
            this,
            tr(
                "Save Investigation Snapshot Failed"
                ),
            saveResult
                    .errorMessage
                    .isEmpty()
                ? tr(
                      "TraceScope could not save "
                      "the investigation snapshot."
                      )
                : saveResult.errorMessage
            );

        return;
    }

    QMessageBox::information(
        this,
        tr(
            "Investigation Snapshot Saved"
            ),
        tr(
            "The current investigation evidence was "
            "saved as a standalone snapshot.\n\n"
            "%1\n\n"
            "The open investigation and its backing "
            "mode were not changed."
            )
            .arg(
                QFileInfo(filePath)
                    .absoluteFilePath()
                )
        );
}

void MainWindow::loadLogFile(
    const QString &filePath,
    const ImportProfile &profile,
    const QString &reloadSessionId,
    SourceFamilyConfiguration
        sourceFamilyConfiguration
    )
{
    const QString activeFilePath =
        QFileInfo(filePath)
            .absoluteFilePath();

    std::optional<
        SourceFamilyConfiguration>
        resolvedConfiguration =
        resolveSourceFamilyConfiguration(
            activeFilePath,
            std::move(
                sourceFamilyConfiguration
                )
            );

    if (!resolvedConfiguration
             .has_value()) {
        return;
    }

    SourceFamilyConfiguration
        resolved =
        std::move(
            resolvedConfiguration.value()
            );

    const QStringList orderedSourcePaths =
        resolved.orderedSourcePaths(
            activeFilePath
            );

    const qint64 initialLiveFollowByteOffset =
        LiveSessionFollowCoordinator::
        captureInitialReadOffset(
            activeFilePath,
            profile
            );

    startLogFileImport(
        activeFilePath,
        orderedSourcePaths,
        profile,
        [
            this,
            activeFilePath,
            profile,
            reloadSessionId,
            initialLiveFollowByteOffset,
            resolved =
            std::move(resolved)
        ](
            std::optional<ImportResult> result
            ) mutable {
            if (!result.has_value()) {
                return;
            }

            completeLogFileImport(
                activeFilePath,
                profile,
                std::move(resolved),
                initialLiveFollowByteOffset,
                std::move(
                    result.value()
                    ),
                reloadSessionId
                );
        }
        );
}

bool MainWindow::startLogFileImport(
    const QString &activeFilePath,
    const QStringList &orderedSourcePaths,
    const ImportProfile &profile,
    ImportCompletionHandler completion
    )
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || snapshotOpenInProgress
        || sessionReloadInProgress) {
        return false;
    }

    const ImporterRegistry registry =
        createBuiltInImporterRegistry(
            profile
            );

    const std::shared_ptr<ILogImporter> importer =
        registry.importerById(
            profile.importerId
            );

    if (!importer) {
        QMessageBox::warning(
            this,
            tr("Unsupported Import Format"),
            tr(
                "The selected import profile uses "
                "an importer that this version of "
                "TraceScope does not support."
                )
            );

        return false;
    }

    auto *watcher =
        new QFutureWatcher<ImportResult>(
            this
            );

    importWatcher =
        watcher;

    updateReloadActionState();

    const QString displayFileName =
        QFileInfo(activeFilePath)
            .fileName();

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Importing %1...\n"
                "Preparing import..."
                )
                .arg(
                    displayFileName
                    ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr("Import Log File")
        );

    progressDialog->setWindowModality(
        Qt::NonModal
        );

    progressDialog->setMinimumDuration(
        500
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    if (openAction != nullptr) {
        openAction->setEnabled(false);
    }

    if (openSnapshotAction != nullptr) {
        openSnapshotAction->setEnabled(false);
    }

    if (openWorkspaceAction != nullptr) {
        openWorkspaceAction->setEnabled(false);
    }

    setAcceptDrops(false);

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<ImportResult>::cancel
        );

    connect(
        watcher,
        &QFutureWatcher<ImportResult>::
        progressRangeChanged,
        progressDialog,
        &QProgressDialog::setRange
        );

    connect(
        watcher,
        &QFutureWatcher<ImportResult>::
        progressValueChanged,
        progressDialog,
        &QProgressDialog::setValue
        );

    connect(
        watcher,
        &QFutureWatcher<ImportResult>::
        progressTextChanged,
        this,
        [
            progressDialog,
            displayFileName
        ](
            const QString &progressText
            ) {
            QString label =
                tr(
                    "Importing %1..."
                    )
                    .arg(
                        displayFileName
                        );

            if (!progressText.isEmpty()) {
                label +=
                    QStringLiteral("\n")
                    + progressText;
            }

            progressDialog->setLabelText(
                label
                );
        }
        );

    connect(
        watcher,
        &QFutureWatcher<ImportResult>::finished,
        this,
        [
            this,
            watcher,
            progressDialog,
            completion
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            progressDialog->hide();
            progressDialog->deleteLater();

            if (importWatcher == watcher) {
                importWatcher =
                    nullptr;
            }

            if (openAction != nullptr) {
                openAction->setEnabled(true);
            }

            if (openSnapshotAction != nullptr) {
                openSnapshotAction->setEnabled(true);
            }

            if (openWorkspaceAction != nullptr) {
                openWorkspaceAction->setEnabled(true);
            }

            setAcceptDrops(true);

            updateReloadActionState();

            if (cancelled) {
                watcher->deleteLater();

                if (completion) {
                    completion(
                        std::nullopt
                        );
                }

                return;
            }

            ImportResult result =
                watcher->result();

            watcher->deleteLater();

            if (completion) {
                completion(
                    std::optional<ImportResult>(
                        std::move(result)
                        )
                    );
            }
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                importer,
                orderedSourcePaths
            ](
                QPromise<ImportResult> &promise
                ) {
                /*
                 * Stay indeterminate until the
                 * importer reports measurable
                 * source progress.
                 */
                promise.setProgressRange(
                    0,
                    0
                    );

                bool determinateProgress =
                    false;

                ImportExecutionContext
                    executionContext;

                executionContext
                    .isCancellationRequested =
                    [&promise]() {
                        return promise.isCanceled();
                    };

                executionContext.reportProgress =
                    [
                        &promise,
                        &determinateProgress
                    ](
                        const ImportProgress &progress
                        ) {
                        if (progress.totalBytes
                            <= 0) {
                            return;
                        }

                        if (!determinateProgress) {
                            promise.setProgressRange(
                                0,
                                100
                                );

                            determinateProgress =
                                true;
                        }

                        const double
                            percentageValue =
                            100.0
                            * static_cast<double>(
                                progress
                                    .bytesProcessed
                                )
                            / static_cast<double>(
                                progress
                                    .totalBytes
                                );

                        const int percentage =
                            std::clamp(
                                static_cast<int>(
                                    percentageValue
                                    ),
                                0,
                                100
                                );

                        const QString progressText =
                            QStringLiteral(
                                "%1 records processed"
                                )
                                .arg(
                                    progress
                                        .processedRecordCount
                                    );

                        promise
                            .setProgressValueAndText(
                                percentage,
                                progressText
                                );
                    };

                SourceFamilyImportOptions options;

                options.maxProcessedRecords =
                    ILogImporter::
                    UnlimitedRecordLimit;

                options.recordLimitMode =
                    SourceFamilyRecordLimitMode::
                    Sequential;

                ImportResult result =
                    SourceFamilyImportService()
                        .importFiles(
                            orderedSourcePaths,
                            *importer,
                            options,
                            executionContext
                            );

                /*
                 * A cancelled future intentionally
                 * publishes no partial result.
                 */
                if (promise.isCanceled()) {
                    return;
                }

                if (determinateProgress) {
                    promise
                        .setProgressValueAndText(
                            100,
                            QStringLiteral(
                                "%1 records processed"
                                )
                                .arg(
                                    result
                                        .processedRecordCount
                                    )
                            );
                }

                promise.addResult(
                    std::move(result)
                    );
            }
            )
        );

    return true;
}

void MainWindow::completeLogFileImport(
    const QString &filePath,
    const ImportProfile &profile,
    SourceFamilyConfiguration
        sourceFamilyConfiguration,
    qint64 initialLiveFollowByteOffset,
    ImportResult result,
    const QString &reloadSessionId
    )
{
    if (result.cancelled) {
        return;
    }

    const bool noRecordsLoaded =
        result.records.isEmpty();

    if (reloadSessionId.isEmpty()) {
        auto session =
            std::make_unique<
                InvestigationSession>(
                filePath,
                profile,
                std::move(result),
                sourceFamilyConfiguration
                );

        session->setInitialLiveFollowByteOffset(
            initialLiveFollowByteOffset
            );

        addSessionToWorkspace(
            std::move(session)
            );
    } else {
        const int sessionIndex =
            workspace->indexOfSession(
                reloadSessionId
                );

        InvestigationSession *session =
            sessionIndex >= 0
                ? workspace->sessionAt(
                      sessionIndex
                      )
                : nullptr;

        if (session != nullptr) {
            session
                ->setSourceFamilyConfiguration(
                    sourceFamilyConfiguration
                    );
        }

        workspace->reloadSession(
            reloadSessionId,
            std::move(result)
            );

        if (session != nullptr) {
            session->setInitialLiveFollowByteOffset(
                initialLiveFollowByteOffset
                );
        }
    }

    /*
     * Only reaching this point means the actual
     * import completed and was installed.
     */
    rotatedSourceSettingsStore
        .rememberSourceConfiguration(
            {
                filePath,
                sourceFamilyConfiguration
                    .rotationRule,
                sourceFamilyConfiguration
                    .includeRotatedSources
            }
            );

    recentItemsStore.addRecentFile(
        filePath
        );

    refreshRecentFilesMenu();

    if (noRecordsLoaded) {
        QMessageBox::warning(
            this,
            "No Events Loaded",
            "No telemetry events were loaded. "
            "The file may be empty, malformed, "
            "or unsupported."
            );
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || snapshotOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    if (!event->mimeData()->hasUrls()) {
        return;
    }

    const QList<QUrl> urls =
        event->mimeData()->urls();

    if (urls.size() != 1
        || !urls.first().isLocalFile()) {
        return;
    }

    openLogFile(
        urls.first().toLocalFile()
        );

    event->acceptProposedAction();
}

void MainWindow::closeEvent(
    QCloseEvent *event
    )
{
    /*
     * Detached workspace windows are independent
     * top-level windows. Tear down the complete
     * document workspace before accepting closure
     * of the primary application window so no
     * detached TraceScope windows remain alive.
     */
    clearCurrentWorkspace();

    QMainWindow::closeEvent(
        event
        );
}

void MainWindow::reloadActiveSession()
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    const InvestigationSessionBackingMode mode =
        session->backing().mode();

    /*
     * SourceBacked retains the existing import path.
     * The external source remains authoritative.
     */
    if (mode
        == InvestigationSessionBackingMode::
        SourceBacked) {
        const QString sessionId =
            session->id();

        /*
         * Capture all SourceBacked continuity state on the
         * UI thread before preparation begins.
         *
         * This includes logical source identity, generation,
         * physical identity, source-family configuration,
         * and the active import profile.
         */
        PersistedInvestigationSession
            persistedSession =
            InvestigationSessionPersistence::capture(
                *session
                );

        auto *watcher =
            new QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>(
                this
                );

        auto *progressDialog =
            new QProgressDialog(
                tr(
                    "Reloading Source-backed "
                    "investigation...\n"
                    "Preparing source evidence..."
                    ),
                tr("Cancel"),
                0,
                0,
                this
                );

        progressDialog->setWindowTitle(
            tr(
                "Reload Investigation"
                )
            );

        progressDialog->setWindowModality(
            Qt::WindowModal
            );

        progressDialog->setMinimumDuration(
            250
            );

        progressDialog->setAutoClose(
            false
            );

        progressDialog->setAutoReset(
            false
            );

        setSessionReloadInProgress(
            true
            );

        connect(
            progressDialog,
            &QProgressDialog::canceled,
            watcher,
            &QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>::
            cancel
            );

        connect(
            watcher,
            &QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>::
            progressRangeChanged,
            progressDialog,
            &QProgressDialog::setRange
            );

        connect(
            watcher,
            &QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>::
            progressValueChanged,
            progressDialog,
            &QProgressDialog::setValue
            );

        connect(
            watcher,
            &QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>::
            progressTextChanged,
            this,
            [
                progressDialog
            ](
                const QString &progressText
                ) {
                QString label =
                    QObject::tr(
                        "Reloading Source-backed "
                        "investigation..."
                        );

                if (!progressText.isEmpty()) {
                    label +=
                        QStringLiteral("\n")
                        + progressText;
                }

                progressDialog->setLabelText(
                    label
                    );
            }
            );

        connect(
            watcher,
            &QFutureWatcher<
                InvestigationSessionRestorationPreparationResult>::
            finished,
            this,
            [
                this,
                watcher,
                progressDialog,
                sessionId
            ]() mutable {
                const bool cancelled =
                    watcher->isCanceled();

                progressDialog->hide();
                progressDialog->deleteLater();

                setSessionReloadInProgress(
                    false
                    );

                if (cancelled) {
                    watcher->deleteLater();
                    return;
                }

                if (watcher
                        ->future()
                        .resultCount()
                    <= 0) {
                    watcher->deleteLater();

                    QMessageBox::warning(
                        this,
                        tr(
                            "Reload Investigation Failed"
                            ),
                        tr(
                            "TraceScope did not receive a "
                            "source preparation result. "
                            "The open investigation was "
                            "not changed."
                            )
                        );

                    return;
                }

                InvestigationSessionRestorationPreparationResult
                    preparation =
                    watcher->result();

                watcher->deleteLater();

                if (!preparation.succeeded
                    || !preparation
                            .preparedData
                            .has_value()) {
                    QMessageBox::warning(
                        this,
                        tr(
                            "Reload Investigation Failed"
                            ),
                        preparation
                                .errorMessage
                                .isEmpty()
                            ? tr(
                                  "TraceScope could not "
                                  "prepare the Source-backed "
                                  "investigation for reload."
                                  )
                            : preparation.errorMessage
                        );

                    return;
                }

                PreparedSourceBackedInvestigationSession
                    *prepared =
                    std::get_if<
                        PreparedSourceBackedInvestigationSession>(
                        &*preparation.preparedData
                        );

                if (prepared == nullptr) {
                    QMessageBox::warning(
                        this,
                        tr(
                            "Reload Investigation Failed"
                            ),
                        tr(
                            "Source preparation did not "
                            "produce a Source-backed "
                            "candidate."
                            )
                        );

                    return;
                }

                InvestigationExternalSourceBinding
                    preparedBinding;

                preparedBinding.sourcePath =
                    std::move(
                        prepared->sourcePath
                        );

                preparedBinding.logicalSourceKey =
                    std::move(
                        prepared->logicalSourceKey
                        );

                preparedBinding
                    .sourceFamilyConfiguration =
                    std::move(
                        prepared
                            ->sourceFamilyConfiguration
                        );

                preparedBinding.sourceGeneration =
                    prepared->sourceGeneration;

                preparedBinding.sourceIdentity =
                    std::move(
                        prepared->sourceIdentity
                        );

                const bool noRecordsLoaded =
                    prepared
                        ->importResult
                        .records
                        .isEmpty();

                const QString sourcePath =
                    preparedBinding.sourcePath;

                const SourceFamilyConfiguration
                    sourceFamilyConfiguration =
                    preparedBinding
                        .sourceFamilyConfiguration;

                const InvestigationSessionSourceReloadResult
                    result =
                    workspace
                        ->applyPreparedSourceBackedReload(
                            sessionId,
                            std::move(
                                preparedBinding
                                ),
                            std::move(
                                prepared->importProfile
                                ),
                            std::move(
                                prepared->importResult
                                ),
                            prepared
                                ->initialLiveFollowByteOffset
                            );

                if (!result.succeeded) {
                    QMessageBox::warning(
                        this,
                        tr(
                            "Reload Investigation Failed"
                            ),
                        result
                                .errorMessage
                                .isEmpty()
                            ? tr(
                                  "The prepared source "
                                  "reload could not be "
                                  "applied."
                                  )
                            : result.errorMessage
                        );

                    return;
                }

                /*
                 * Preserve the same recent-source and
                 * rotation-setting behavior as the old
                 * loadLogFile() reload path.
                 */
                rotatedSourceSettingsStore
                    .rememberSourceConfiguration(
                        {
                            sourcePath,
                            sourceFamilyConfiguration
                                .rotationRule,
                            sourceFamilyConfiguration
                                .includeRotatedSources
                        }
                        );

                recentItemsStore.addRecentFile(
                    sourcePath
                    );

                refreshRecentFilesMenu();

                if (noRecordsLoaded) {
                    QMessageBox::warning(
                        this,
                        tr(
                            "No Events Loaded"
                            ),
                        tr(
                            "No telemetry events were "
                            "loaded from the authoritative "
                            "source."
                            )
                        );
                }
            }
            );

        watcher->setFuture(
            QtConcurrent::run(
                [
                    persistedSession =
                    std::move(
                        persistedSession
                        )
                ](
                    QPromise<
                        InvestigationSessionRestorationPreparationResult>
                        &promise
                    ) mutable {
                    promise.setProgressRange(
                        0,
                        0
                        );

                    bool determinateProgress =
                        false;

                    ImportExecutionContext
                        executionContext;

                    executionContext
                        .isCancellationRequested =
                        [&promise]() {
                            return promise.isCanceled();
                        };

                    executionContext.reportProgress =
                        [
                            &promise,
                            &determinateProgress
                        ](
                            const ImportProgress &progress
                            ) {
                            if (progress.totalBytes
                                <= 0) {
                                return;
                            }

                            if (!determinateProgress) {
                                promise.setProgressRange(
                                    0,
                                    100
                                    );

                                determinateProgress =
                                    true;
                            }

                            const double
                                importFraction =
                                static_cast<double>(
                                    progress
                                        .bytesProcessed
                                    )
                                / static_cast<double>(
                                    progress
                                        .totalBytes
                                    );

                            /*
                             * As with Use Source as
                             * Authoritative, reserve the
                             * final five percent for
                             * identity verification and
                             * candidate finalization.
                             */
                            const int percentage =
                                std::clamp(
                                    static_cast<int>(
                                        95.0
                                        * importFraction
                                        ),
                                    0,
                                    95
                                    );

                            const QString progressText =
                                percentage >= 95
                                    ? QStringLiteral(
                                          "%1 records "
                                          "processed; "
                                          "finalizing source "
                                          "verification..."
                                          )
                                          .arg(
                                              progress
                                                  .processedRecordCount
                                              )
                                    : QStringLiteral(
                                          "%1 records "
                                          "processed"
                                          )
                                          .arg(
                                              progress
                                                  .processedRecordCount
                                              );

                            promise
                                .setProgressValueAndText(
                                    percentage,
                                    progressText
                                    );
                        };

                    InvestigationSessionRestorationService
                        restorationService;

                    InvestigationSessionRestorationPreparationResult
                        preparation =
                        restorationService.prepare(
                            persistedSession,
                            QString(),
                            executionContext
                            );

                    if (promise.isCanceled()) {
                        return;
                    }

                    promise.addResult(
                        std::move(
                            preparation
                            )
                        );
                }
                )
            );

        return;
    }

    const InvestigationSnapshotBinding
        *snapshotBinding =
        session->backing().snapshot();

    if (snapshotBinding == nullptr
        || snapshotBinding
               ->snapshotPath
               .trimmed()
               .isEmpty()) {
        QMessageBox::warning(
            this,
            tr("Reload Investigation Failed"),
            tr(
                "This investigation does not have "
                "a valid durable snapshot backing."
                )
            );

        return;
    }

    const QString sessionId =
        session->id();

    const QString snapshotPath =
        snapshotBinding->snapshotPath;

    std::optional<
        InvestigationExternalSourceBinding>
        externalSourceBinding;

    if (mode
        == InvestigationSessionBackingMode::
        Hybrid) {
        const InvestigationExternalSourceBinding
            *externalSource =
            session->backing().externalSource();

        if (externalSource == nullptr) {
            QMessageBox::warning(
                this,
                tr("Reload Investigation Failed"),
                tr(
                    "The Hybrid investigation does "
                    "not contain its external source "
                    "binding."
                    )
                );

            return;
        }

        externalSourceBinding =
            *externalSource;
    }

    auto *watcher =
        new QFutureWatcher<
            ActiveSessionReloadPreparationResult>(
            this
            );

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Reloading investigation...\n"
                "Preparing durable evidence..."
                ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr("Reload Investigation")
        );

    progressDialog->setWindowModality(
        Qt::WindowModal
        );

    progressDialog->setMinimumDuration(
        250
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    setSessionReloadInProgress(
        true
        );

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<
            ActiveSessionReloadPreparationResult>::
        cancel
        );

    connect(
        watcher,
        &QFutureWatcher<
            ActiveSessionReloadPreparationResult>::
        finished,
        this,
        [
            this,
            watcher,
            progressDialog
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            progressDialog->hide();
            progressDialog->deleteLater();

            setSessionReloadInProgress(
                false
                );

            if (cancelled) {
                watcher->deleteLater();
                return;
            }

            ActiveSessionReloadPreparationResult
                preparation =
                watcher->result();

            watcher->deleteLater();

            if (!preparation.succeeded) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Reload Investigation Failed"
                        ),
                    preparation
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "TraceScope could not "
                              "prepare the investigation "
                              "for reload."
                              )
                        : preparation.errorMessage
                    );

                return;
            }

            if (preparation.mode
                == InvestigationSessionBackingMode::
                SnapshotBacked) {
                if (!workspace
                         ->applySnapshotReload(
                             preparation.sessionId,
                             std::move(
                                 preparation.snapshot
                                 )
                             )) {
                    QMessageBox::warning(
                        this,
                        tr(
                            "Reload Investigation Failed"
                            ),
                        tr(
                            "The prepared snapshot "
                            "could not be applied to "
                            "the open investigation."
                            )
                        );
                }

                return;
            }

            if (preparation.mode
                    != InvestigationSessionBackingMode::
                    Hybrid
                || !preparation
                        .hybridReconstruction
                        .has_value()) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Reload Investigation Failed"
                        ),
                    tr(
                        "Hybrid reload preparation "
                        "did not produce a complete "
                        "reconstruction."
                        )
                    );

                return;
            }

            InvestigationSessionHybridReloadResult
                result =
                workspace->applyHybridReload(
                    preparation.sessionId,
                    std::move(
                        preparation.snapshot
                        ),
                    std::move(
                        *preparation
                             .hybridReconstruction
                        )
                    );

            if (!result.succeeded) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Reload Investigation Failed"
                        ),
                    result.errorMessage.isEmpty()
                        ? tr(
                              "The prepared Hybrid "
                              "reload could not be "
                              "applied."
                              )
                        : result.errorMessage
                    );
            }
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                sessionId,
                snapshotPath,
                mode,
                externalSourceBinding
            ](
                QPromise<
                    ActiveSessionReloadPreparationResult>
                    &promise
                ) {
                ActiveSessionReloadPreparationResult
                    preparation;

                preparation.sessionId =
                    sessionId;

                preparation.mode =
                    mode;

                InvestigationSessionSnapshotLoadResult
                    loadResult =
                    InvestigationSessionSnapshotFile()
                        .load(
                            snapshotPath
                            );

                if (!loadResult.isSuccess()) {
                    preparation.errorMessage =
                        loadResult
                                .errorMessage
                                .isEmpty()
                            ? QStringLiteral(
                                  "The durable "
                                  "investigation "
                                  "snapshot could not "
                                  "be loaded."
                                  )
                            : loadResult.errorMessage;

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                if (promise.isCanceled()) {
                    return;
                }

                preparation.snapshot =
                    std::move(
                        *loadResult.snapshot
                        );

                if (mode
                    == InvestigationSessionBackingMode::
                    SnapshotBacked) {
                    preparation.succeeded =
                        true;

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                if (mode
                        != InvestigationSessionBackingMode::
                        Hybrid
                    || !externalSourceBinding
                            .has_value()) {
                    preparation.errorMessage =
                        QStringLiteral(
                            "Hybrid reload preparation "
                            "is missing its external "
                            "source binding."
                            );

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                HybridInvestigationReconstructionResult
                    reconstruction;

                const QFileInfo sourceInfo(
                    externalSourceBinding
                        ->sourcePath
                    );

                /*
                 * Missing source is not a Hybrid
                 * failure. The snapshot remains the
                 * durable evidence floor.
                 */
                if (!sourceInfo.exists()
                    || !sourceInfo.isFile()) {
                    reconstruction.succeeded =
                        true;

                    reconstruction
                        .externalSourceAvailable =
                        false;

                    reconstruction
                        .externalSourceBinding =
                        *externalSourceBinding;

                    reconstruction
                        .importResult
                        .records =
                        preparation
                            .snapshot
                            .records;

                    reconstruction
                        .importResult
                        .diagnostics =
                        preparation
                            .snapshot
                            .diagnostics;

                    reconstruction
                        .importResult
                        .processedRecordCount =
                        preparation
                            .snapshot
                            .processedRecordCount;

                    reconstruction
                        .importResult
                        .sourceTruncated =
                        preparation
                            .snapshot
                            .sourceTruncated;
                } else {
                    ImporterRegistry registry =
                        createBuiltInImporterRegistry(
                            preparation
                                .snapshot
                                .importProfile
                            );

                    const std::shared_ptr<
                        ILogImporter>
                        importer =
                        registry.importerById(
                            preparation
                                .snapshot
                                .importProfile
                                .importerId
                            );

                    if (!importer) {
                        preparation.errorMessage =
                            QStringLiteral(
                                "No importer is "
                                "available for the "
                                "Hybrid investigation's "
                                "snapshot profile."
                                );

                        promise.addResult(
                            std::move(preparation)
                            );

                        return;
                    }

                    ImportExecutionContext
                        executionContext;

                    executionContext
                        .isCancellationRequested =
                        [&promise]() {
                            return
                                promise.isCanceled();
                        };

                    reconstruction =
                        HybridInvestigationReconstructionService()
                            .reconstruct(
                                preparation.snapshot,
                                *externalSourceBinding,
                                *importer,
                                executionContext
                                );
                }

                if (promise.isCanceled()) {
                    return;
                }

                if (!reconstruction.succeeded) {
                    preparation.errorMessage =
                        reconstruction
                                .errorMessage
                                .isEmpty()
                            ? QStringLiteral(
                                  "The Hybrid "
                                  "investigation could "
                                  "not be reconstructed."
                                  )
                            : reconstruction
                                  .errorMessage;

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                preparation.hybridReconstruction =
                    std::move(
                        reconstruction
                        );

                preparation.succeeded =
                    true;

                promise.addResult(
                    std::move(preparation)
                    );
            }
            )
        );
}

void MainWindow::
    reconnectSnapshotBackedSession(
        const QString &sessionId
        )
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    const int sessionIndex =
        workspace->indexOfSession(
            sessionId
            );

    if (sessionIndex < 0) {
        return;
    }

    InvestigationSession *session =
        workspace->sessionAt(
            sessionIndex
            );

    if (session == nullptr
        || session->backing().mode()
               != InvestigationSessionBackingMode::
               SnapshotBacked) {
        return;
    }

    const InvestigationSnapshotBinding
        *snapshotBinding =
        session->backing().snapshot();

    const InvestigationExternalSourceBinding
        *reconnectHint =
        session->reconnectSourceHint();

    if (snapshotBinding == nullptr
        || snapshotBinding
               ->snapshotPath
               .trimmed()
               .isEmpty()) {
        QMessageBox::warning(
            this,
            tr("Reconnect Source Failed"),
            tr(
                "This investigation does not have "
                "a valid durable snapshot backing."
                )
            );

        return;
    }

    if (reconnectHint == nullptr
        || reconnectHint
               ->sourcePath
               .trimmed()
               .isEmpty()) {
        QMessageBox::information(
            this,
            tr("Reconnect Source"),
            tr(
                "This Snapshot-backed investigation "
                "does not retain enough source "
                "continuity information to reconnect "
                "safely."
                )
            );

        return;
    }

    const QString sourcePath =
        QFileInfo(
            reconnectHint->sourcePath
            ).absoluteFilePath();

    const QFileInfo sourceInfo(
        sourcePath
        );

    if (!sourceInfo.exists()
        || !sourceInfo.isFile()) {
        QMessageBox::information(
            this,
            tr("Source Not Available"),
            tr(
                "The recorded source is not currently "
                "available:\n\n%1\n\n"
                "Restore the source at its original "
                "path and try reconnecting again."
                )
                .arg(sourcePath)
            );

        return;
    }

    const QString snapshotPath =
        snapshotBinding->snapshotPath;

    const InvestigationExternalSourceBinding
        externalSourceBinding =
        *reconnectHint;

    auto *watcher =
        new QFutureWatcher<
            SnapshotReconnectPreparationResult>(
            this
            );

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Reconnecting source...\n"
                "Loading saved evidence and "
                "replaying the external source..."
                ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr("Reconnect Source")
        );

    progressDialog->setWindowModality(
        Qt::WindowModal
        );

    progressDialog->setMinimumDuration(
        250
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    setSessionReloadInProgress(
        true
        );

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<
            SnapshotReconnectPreparationResult>::
        cancel
        );

    connect(
        watcher,
        &QFutureWatcher<
            SnapshotReconnectPreparationResult>::
        finished,
        this,
        [
            this,
            watcher,
            progressDialog
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            progressDialog->hide();
            progressDialog->deleteLater();

            setSessionReloadInProgress(
                false
                );

            if (cancelled) {
                watcher->deleteLater();
                return;
            }

            SnapshotReconnectPreparationResult
                preparation =
                watcher->result();

            watcher->deleteLater();

            if (!preparation.succeeded
                || !preparation
                        .reconstruction
                        .has_value()) {
                QMessageBox::warning(
                    this,
                    tr("Reconnect Source Failed"),
                    preparation
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "TraceScope could not "
                              "prepare the source "
                              "reconnection."
                              )
                        : preparation.errorMessage
                    );

                return;
            }

            InvestigationSessionHybridReloadResult
                result =
                workspace->applyHybridReconnect(
                    preparation.sessionId,
                    std::move(
                        preparation.snapshot
                        ),
                    std::move(
                        *preparation
                             .reconstruction
                        )
                    );

            if (!result.succeeded) {
                QMessageBox::warning(
                    this,
                    tr("Reconnect Source Failed"),
                    result.errorMessage.isEmpty()
                        ? tr(
                              "The prepared source "
                              "reconnection could not "
                              "be applied."
                              )
                        : result.errorMessage
                    );

                return;
            }

            /*
             * A successful reconnect is now Hybrid.
             * sessionReloaded has already refreshed
             * the document presentation and tab
             * accessory.
             */
            updateReloadActionState();
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                sessionId,
                snapshotPath,
                externalSourceBinding
            ](
                QPromise<
                    SnapshotReconnectPreparationResult>
                    &promise
                ) {
                SnapshotReconnectPreparationResult
                    preparation;

                preparation.sessionId =
                    sessionId;

                InvestigationSessionSnapshotLoadResult
                    loadResult =
                    InvestigationSessionSnapshotFile()
                        .load(
                            snapshotPath
                            );

                if (!loadResult.isSuccess()) {
                    preparation.errorMessage =
                        loadResult
                                .errorMessage
                                .isEmpty()
                            ? QStringLiteral(
                                  "The durable "
                                  "investigation snapshot "
                                  "could not be loaded."
                                  )
                            : loadResult.errorMessage;

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                if (promise.isCanceled()) {
                    return;
                }

                preparation.snapshot =
                    std::move(
                        *loadResult.snapshot
                        );

                ImporterRegistry registry =
                    createBuiltInImporterRegistry(
                        preparation
                            .snapshot
                            .importProfile
                        );

                const std::shared_ptr<
                    ILogImporter>
                    importer =
                    registry.importerById(
                        preparation
                            .snapshot
                            .importProfile
                            .importerId
                        );

                if (!importer) {
                    preparation.errorMessage =
                        QStringLiteral(
                            "No importer is available "
                            "for the saved investigation "
                            "profile."
                            );

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                ImportExecutionContext
                    executionContext;

                executionContext
                    .isCancellationRequested =
                    [&promise]() {
                        return promise.isCanceled();
                    };

                HybridInvestigationReconstructionResult
                    reconstruction =
                    HybridInvestigationReconstructionService()
                        .reconstruct(
                            preparation.snapshot,
                            externalSourceBinding,
                            *importer,
                            executionContext
                            );

                if (promise.isCanceled()) {
                    return;
                }

                if (!reconstruction.succeeded) {
                    preparation.errorMessage =
                        reconstruction
                                .errorMessage
                                .isEmpty()
                            ? QStringLiteral(
                                  "The recorded source "
                                  "could not be replayed "
                                  "against the saved "
                                  "investigation."
                                  )
                            : reconstruction
                                  .errorMessage;

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                if (!reconstruction
                         .externalSourceAvailable) {
                    preparation.errorMessage =
                        QStringLiteral(
                            "The recorded source became "
                            "unavailable while "
                            "reconnection was being "
                            "prepared."
                            );

                    promise.addResult(
                        std::move(preparation)
                        );

                    return;
                }

                preparation.reconstruction =
                    std::move(
                        reconstruction
                        );

                preparation.succeeded =
                    true;

                promise.addResult(
                    std::move(preparation)
                    );
            }
            )
        );
}

void MainWindow::redefineSessionSourcePath(
    const QString &sessionId
    )
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    const int sessionIndex =
        workspace->indexOfSession(
            sessionId
            );

    if (sessionIndex < 0) {
        return;
    }

    InvestigationSession *session =
        workspace->sessionAt(
            sessionIndex
            );

    if (session == nullptr) {
        return;
    }

    const InvestigationExternalSourceBinding
        *sourceBinding =
        session
            ->backing()
            .externalSource();

    if (sourceBinding == nullptr) {
        sourceBinding =
            session->reconnectSourceHint();
    }

    if (sourceBinding == nullptr
        || !sourceBinding
                ->sourceIdentity
                .has_value()) {
        QMessageBox::information(
            this,
            tr("Redefine Source Path"),
            tr(
                "This investigation does not retain "
                "enough physical source identity "
                "information to verify a relocated "
                "source safely."
                )
            );

        return;
    }

    const QString currentSourcePath =
        sourceBinding->sourcePath;

    QString initialDirectory;

    if (!currentSourcePath
             .trimmed()
             .isEmpty()) {
        initialDirectory =
            QFileInfo(
                currentSourcePath
                ).absolutePath();
    }

    const QString candidatePath =
        QFileDialog::getOpenFileName(
            this,
            tr("Redefine Source Path"),
            initialDirectory,
            tr("All Files (*)")
            );

    if (candidatePath.isEmpty()) {
        return;
    }

    const InvestigationSessionSourceRelocationResult
        result =
        workspace->redefineSourcePath(
            sessionId,
            candidatePath
            );

    if (!result.succeeded) {
        QMessageBox::warning(
            this,
            tr("Redefine Source Path Failed"),
            result.errorMessage.isEmpty()
                ? tr(
                      "The selected file could not be "
                      "verified as the same logical "
                      "source."
                      )
                : result.errorMessage
            );

        return;
    }

    QMessageBox::information(
        this,
        tr("Source Path Redefined"),
        tr(
            "TraceScope verified the selected file "
            "against the investigation's saved source "
            "identity and updated the physical source "
            "location.\n\n"
            "The logical source identity and source "
            "generation were preserved."
            )
        );
}

void MainWindow::useSourceAsAuthoritative(
    const QString &sessionId
    )
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress
        || snapshotOpenInProgress) {
        return;
    }

    const int sessionIndex =
        workspace->indexOfSession(
            sessionId
            );

    if (sessionIndex < 0) {
        return;
    }

    InvestigationSession *session =
        workspace->sessionAt(
            sessionIndex
            );

    if (session == nullptr
        || session->backing().mode()
               != InvestigationSessionBackingMode::
               Hybrid) {
        return;
    }

    const InvestigationExternalSourceBinding
        *externalSource =
        session
            ->backing()
            .externalSource();

    if (externalSource == nullptr
        || externalSource
               ->sourcePath
               .trimmed()
               .isEmpty()) {
        QMessageBox::warning(
            this,
            tr(
                "Use Source as Authoritative Failed"
                ),
            tr(
                "This Hybrid investigation does not "
                "have a valid connected external "
                "source."
                )
            );

        return;
    }

    /*
     * This is intentionally destructive with respect
     * to the open investigation's evidence set.
     *
     * Snapshot-only evidence will no longer be part of
     * the investigation once the external source becomes
     * authoritative.
     */
    const QMessageBox::StandardButton choice =
        QMessageBox::warning(
            this,
            tr(
                "Use Source as Authoritative"
                ),
            tr(
                "TraceScope will rebuild this "
                "investigation entirely from the "
                "connected external source.\n\n"
                "Evidence that exists only in the "
                "durable snapshot will be removed from "
                "the open investigation. Bookmarks, "
                "notes, findings, and selection tied "
                "only to removed records will also no "
                "longer apply.\n\n"
                "The snapshot file itself will NOT be "
                "deleted.\n\n"
                "Continue?"
                ),
            QMessageBox::Yes
                | QMessageBox::Cancel,
            QMessageBox::Cancel
            );

    if (choice != QMessageBox::Yes) {
        return;
    }

    /*
     * Capture the current Hybrid persistence model,
     * then transform only this detached candidate into
     * a SourceBacked descriptor.
     *
     * The live InvestigationSession remains completely
     * untouched during preparation.
     */
    PersistedInvestigationSession persistedSession =
        InvestigationSessionPersistence::capture(
            *session
            );

    if (!persistedSession
             .backing
             .externalSourceBinding
             .has_value()) {
        QMessageBox::warning(
            this,
            tr(
                "Use Source as Authoritative Failed"
                ),
            tr(
                "TraceScope could not capture the "
                "Hybrid investigation's external "
                "source binding."
                )
            );

        return;
    }

    persistedSession.backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    /*
     * This candidate must represent the external source
     * independently. The current snapshot must not
     * participate in source preparation.
     */
    persistedSession
        .backing
        .snapshotReference
        .reset();

    /*
     * Hybrid normally gets its authoritative import
     * profile from the snapshot. SourceBacked
     * restoration expects that profile directly on
     * the backing descriptor.
     */
    persistedSession
        .backing
        .sourceImportProfile =
        session->importProfile();

    /*
     * Keep the temporary compatibility mirror coherent
     * even though schema-v2 preparation reads the
     * normalized backing model.
     */
    persistedSession.importProfile =
        session->importProfile();

    auto *watcher =
        new QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>(
            this
            );

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Rebuilding investigation from "
                "the authoritative source...\n"
                "Preparing source evidence..."
                ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr(
            "Use Source as Authoritative"
            )
        );

    progressDialog->setWindowModality(
        Qt::WindowModal
        );

    progressDialog->setMinimumDuration(
        250
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    setSessionReloadInProgress(
        true
        );

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        cancel
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressRangeChanged,
        progressDialog,
        &QProgressDialog::setRange
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressValueChanged,
        progressDialog,
        &QProgressDialog::setValue
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressTextChanged,
        this,
        [
            progressDialog
        ](
            const QString &progressText
            ) {
            QString label =
                QObject::tr(
                    "Rebuilding investigation from "
                    "the authoritative source..."
                    );

            if (!progressText.isEmpty()) {
                label +=
                    QStringLiteral("\n")
                    + progressText;
            }

            progressDialog->setLabelText(
                label
                );
        }
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        finished,
        this,
        [
            this,
            watcher,
            progressDialog,
            sessionId
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            /*
             * Destroy the progress dialog before opening any
             * modal completion/error dialog.
             *
             * deleteLater() is not sufficient here because the
             * following QMessageBox starts its own nested event
             * loop. The progress dialog can remain alive and
             * visible in front of that message box until the
             * user explicitly dismisses it.
             */
            progressDialog->close();
            delete progressDialog;

            setSessionReloadInProgress(
                false
                );

            if (cancelled) {
                watcher->deleteLater();
                return;
            }

            if (watcher
                    ->future()
                    .resultCount()
                <= 0) {
                watcher->deleteLater();

                QMessageBox::warning(
                    this,
                    tr(
                        "Use Source as Authoritative "
                        "Failed"
                        ),
                    tr(
                        "TraceScope did not receive a "
                        "source preparation result. The "
                        "current Hybrid investigation "
                        "was not changed."
                        )
                    );

                return;
            }

            InvestigationSessionRestorationPreparationResult
                preparation =
                watcher->result();

            watcher->deleteLater();

            if (!preparation.succeeded
                || !preparation
                        .preparedData
                        .has_value()) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Use Source as Authoritative "
                        "Failed"
                        ),
                    preparation
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "TraceScope could not "
                              "prepare a complete "
                              "Source-backed "
                              "investigation. The "
                              "current Hybrid "
                              "investigation was not "
                              "changed."
                              )
                        : preparation.errorMessage
                    );

                return;
            }

            PreparedSourceBackedInvestigationSession
                *prepared =
                std::get_if<
                    PreparedSourceBackedInvestigationSession>(
                    &*preparation.preparedData
                    );

            if (prepared == nullptr) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Use Source as Authoritative "
                        "Failed"
                        ),
                    tr(
                        "Source preparation completed "
                        "without producing a complete "
                        "Source-backed candidate. The "
                        "current Hybrid investigation "
                        "was not changed."
                        )
                    );

                return;
            }

            InvestigationExternalSourceBinding
                preparedBinding;

            preparedBinding.sourcePath =
                std::move(
                    prepared->sourcePath
                    );

            preparedBinding.logicalSourceKey =
                std::move(
                    prepared->logicalSourceKey
                    );

            preparedBinding
                .sourceFamilyConfiguration =
                std::move(
                    prepared
                        ->sourceFamilyConfiguration
                    );

            preparedBinding.sourceGeneration =
                prepared->sourceGeneration;

            preparedBinding.sourceIdentity =
                std::move(
                    prepared->sourceIdentity
                    );

            const bool noRecordsLoaded =
                prepared
                    ->importResult
                    .records
                    .isEmpty();

            const InvestigationSessionSourceAuthoritativeResult
                result =
                workspace
                    ->applySourceAuthoritativeTransition(
                        sessionId,
                        std::move(
                            preparedBinding
                            ),
                        std::move(
                            prepared->importProfile
                            ),
                        std::move(
                            prepared->importResult
                            ),
                        prepared
                            ->initialLiveFollowByteOffset
                        );

            if (!result.succeeded) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Use Source as Authoritative "
                        "Failed"
                        ),
                    result
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "The prepared source "
                              "could not be committed "
                              "to the open "
                              "investigation."
                              )
                        : result.errorMessage
                    );

                return;
            }

            if (noRecordsLoaded) {
                QMessageBox::warning(
                    this,
                    tr(
                        "Source is Now Authoritative"
                        ),
                    tr(
                        "The investigation is now "
                        "Source-backed, but the "
                        "authoritative source produced "
                        "no investigation records.\n\n"
                        "The previous snapshot file was "
                        "not deleted."
                        )
                    );

                return;
            }

            QMessageBox::information(
                this,
                tr(
                    "Source is Now Authoritative"
                    ),
                tr(
                    "The investigation was rebuilt "
                    "from the connected external source "
                    "and is now Source-backed.\n\n"
                    "The previous snapshot file was "
                    "not deleted."
                    )
                );
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                persistedSession =
                std::move(
                    persistedSession
                    )
            ](
                QPromise<
                    InvestigationSessionRestorationPreparationResult>
                    &promise
                ) mutable {
                promise.setProgressRange(
                    0,
                    0
                    );

                bool determinateProgress = false;

                ImportExecutionContext
                    executionContext;

                executionContext
                    .isCancellationRequested =
                    [&promise]() {
                        return promise.isCanceled();
                    };

                executionContext.reportProgress =
                    [
                        &promise,
                        &determinateProgress
                ](
                        const ImportProgress &progress
                        ) {
                        if (progress.totalBytes
                            <= 0) {
                            return;
                        }

                        if (!determinateProgress) {
                            promise.setProgressRange(
                                0,
                                100
                                );

                            determinateProgress =
                                true;
                        }

                        /*
                         * Source import is only part of the preparation
                         * operation. Leave the final five percent for source
                         * identity verification, rebasing, and candidate
                         * finalization so the dialog never claims completion
                         * while preparation is still running.
                         */
                        const double importFraction =
                            static_cast<double>(
                                progress.bytesProcessed
                                )
                            / static_cast<double>(
                                progress.totalBytes
                                );

                        const int percentage =
                            std::clamp(
                                static_cast<int>(
                                    95.0 * importFraction
                                    ),
                                0,
                                95
                                );

                        const QString progressText =
                            percentage >= 95
                                ? QStringLiteral(
                                      "%1 records processed; "
                                      "finalizing source verification..."
                                      )
                                      .arg(
                                          progress
                                              .processedRecordCount
                                          )
                                : QStringLiteral(
                                      "%1 records processed"
                                      )
                                      .arg(
                                          progress
                                              .processedRecordCount
                                          );

                        promise
                            .setProgressValueAndText(
                                percentage,
                                progressText
                                );
                    };

                InvestigationSessionRestorationService
                    restorationService;

                /*
                 * SourceBacked preparation needs no
                 * workspace-relative path resolution.
                 * The external source binding already
                 * contains its absolute physical path.
                 */
                InvestigationSessionRestorationPreparationResult
                    preparation =
                    restorationService.prepare(
                        persistedSession,
                        QString(),
                        executionContext
                        );

                if (promise.isCanceled()) {
                    return;
                }

                promise.addResult(
                    std::move(
                        preparation
                        )
                    );
            }
            )
        );
}

void MainWindow::openSourceLocation(
    const QString &sourcePath
    )
{
    if (sourcePath.trimmed().isEmpty()) {
        return;
    }

    const QFileInfo sourceInfo(
        sourcePath
        );

    if (!sourceInfo.exists()) {
        QMessageBox::information(
            this,
            tr("Source Not Available"),
            tr(
                "The source file is no longer "
                "available at:\n\n%1"
                )
                .arg(sourcePath)
            );

        return;
    }

    const QString directoryPath =
        sourceInfo.absolutePath();

    if (!QDesktopServices::openUrl(
            QUrl::fromLocalFile(
                directoryPath
                )
            )) {
        QMessageBox::warning(
            this,
            tr("Open Source Location Failed"),
            tr(
                "TraceScope could not open the "
                "source directory:\n\n%1"
                )
                .arg(directoryPath)
            );
    }
}

void MainWindow::populateSessionSourceMenu(
    const QString &sessionId,
    QMenu *menu
    )
{
    if (workspace == nullptr
        || menu == nullptr) {
        return;
    }

    const int sessionIndex =
        workspace->indexOfSession(
            sessionId
            );

    if (sessionIndex < 0) {
        return;
    }

    InvestigationSession *session =
        workspace->sessionAt(
            sessionIndex
            );

    if (session == nullptr) {
        return;
    }

    QMenu *sourceMenu =
        menu->addMenu(
            tr("Source")
            );

    sourceMenu->setToolTipsVisible(
        true
        );

    const InvestigationSessionBacking
        &backing =
        session->backing();

    switch (backing.mode()) {
    case InvestigationSessionBackingMode::
        SourceBacked: {
        const InvestigationExternalSourceBinding
            *source =
            backing.externalSource();

        if (source == nullptr
            || source->sourcePath
                   .trimmed()
                   .isEmpty()) {
            sourceMenu->setEnabled(
                false
                );

            return;
        }

        /*
         * Lifecycle transition:
         *
         * SourceBacked -> SnapshotBacked
         */
        QAction *snapshotOnlyAction =
            sourceMenu->addAction(
                tr(
                    "Preserve as Snapshot Only..."
                    )
                );

        snapshotOnlyAction->setToolTip(
            tr(
                "Capture all currently admitted evidence into "
                "a fresh durable snapshot and convert this "
                "investigation to Snapshot-backed"
                )
            );

        connect(
            snapshotOnlyAction,
            &QAction::triggered,
            sourceMenu,
            [
                this,
                sessionId
            ]() {
                preserveSessionAsSnapshotOnly(
                    sessionId
                    );
            }
            );

        sourceMenu->addSeparator();

        /*
         * External-source management.
         */
        QAction *redefineAction =
            sourceMenu->addAction(
                tr(
                    "Redefine Source Path..."
                    )
                );

        const bool canRedefine =
            source
                ->sourceIdentity
                .has_value();

        redefineAction->setEnabled(
            canRedefine
            );

        redefineAction->setToolTip(
            canRedefine
                ? tr(
                      "Select a relocated copy of this source. "
                      "TraceScope will update the path only if "
                      "the file can be verified against the "
                      "saved physical source identity."
                      )
                : tr(
                      "This investigation does not retain "
                      "enough physical source identity "
                      "information to verify relocation."
                      )
            );

        if (canRedefine) {
            connect(
                redefineAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    sessionId
                ]() {
                    redefineSessionSourcePath(
                        sessionId
                        );
                }
                );
        }

        QAction *openAction =
            sourceMenu->addAction(
                tr(
                    "Open Source Location"
                    )
                );

        openAction->setToolTip(
            tr(
                "Open the directory containing the "
                "authoritative external source"
                )
            );

        const QString sourcePath =
            source->sourcePath;

        connect(
            openAction,
            &QAction::triggered,
            sourceMenu,
            [
                this,
                sourcePath
            ]() {
                openSourceLocation(
                    sourcePath
                    );
            }
            );

        break;
    }

    case InvestigationSessionBackingMode::
        SnapshotBacked: {
        const InvestigationExternalSourceBinding
            *reconnectHint =
            session->reconnectSourceHint();

        /*
         * Lifecycle transition:
         *
         * SnapshotBacked -> Hybrid
         */
        QAction *reconnectAction =
            sourceMenu->addAction(
                tr(
                    "Reconnect Source"
                    )
                );

        const bool canReconnect =
            reconnectHint != nullptr
            && !reconnectHint
                    ->sourcePath
                    .trimmed()
                    .isEmpty();

        reconnectAction->setEnabled(
            canReconnect
            );

        if (canReconnect) {
            const QString reconnectPath =
                reconnectHint->sourcePath;

            reconnectAction->setToolTip(
                tr(
                    "Reconnect this investigation to its "
                    "previously recorded external source "
                    "and restore Hybrid backing:\n%1"
                    )
                    .arg(
                        reconnectPath
                        )
                );

            connect(
                reconnectAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    sessionId
                ]() {
                    reconnectSnapshotBackedSession(
                        sessionId
                        );
                }
                );
        } else {
            reconnectAction->setToolTip(
                tr(
                    "This investigation does not retain "
                    "source continuity metadata required "
                    "for safe reconnection."
                    )
                );
        }

        sourceMenu->addSeparator();

        /*
         * Dormant external-source management.
         */
        QAction *redefineAction =
            sourceMenu->addAction(
                tr(
                    "Redefine Source Path..."
                    )
                );

        const bool canRedefine =
            reconnectHint != nullptr
            && reconnectHint
                   ->sourceIdentity
                   .has_value();

        redefineAction->setEnabled(
            canRedefine
            );

        redefineAction->setToolTip(
            canRedefine
                ? tr(
                      "Select the relocated external source "
                      "represented by this investigation's "
                      "saved continuity information."
                      )
                : tr(
                      "This investigation does not retain "
                      "enough physical source identity "
                      "information to verify relocation."
                      )
            );

        if (canRedefine) {
            connect(
                redefineAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    sessionId
                ]() {
                    redefineSessionSourcePath(
                        sessionId
                        );
                }
                );
        }

        /*
         * Original source provenance is informational.
         * Only expose the location action while that
         * physical source currently exists.
         */
        const QString originalSourcePath =
            session
                ->sourceMetadata()
                .sourcePath;

        const QFileInfo sourceInfo(
            originalSourcePath
            );

        if (!originalSourcePath
                 .trimmed()
                 .isEmpty()
            && sourceInfo.exists()
            && sourceInfo.isFile()) {
            QAction *openAction =
                sourceMenu->addAction(
                    tr(
                        "Open Original Source Location"
                        )
                    );

            openAction->setToolTip(
                tr(
                    "Open the directory containing the "
                    "original external source represented "
                    "by this Snapshot-backed investigation"
                    )
                );

            connect(
                openAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    originalSourcePath
                ]() {
                    openSourceLocation(
                        originalSourcePath
                        );
                }
                );
        }

        break;
    }

    case InvestigationSessionBackingMode::
        Hybrid: {
        const InvestigationExternalSourceBinding
            *source =
            backing.externalSource();

        if (source == nullptr
            || source->sourcePath
                   .trimmed()
                   .isEmpty()) {
            sourceMenu->setEnabled(
                false
                );

            return;
        }

        /*
         * Lifecycle transitions:
         *
         * Hybrid -> SourceBacked
         * Hybrid -> SnapshotBacked
         */
        QAction *sourceAuthoritativeAction =
            sourceMenu->addAction(
                tr(
                    "Use Source as Authoritative..."
                    )
                );

        sourceAuthoritativeAction->setToolTip(
            tr(
                "Rebuild the investigation entirely from "
                "the connected external source and convert "
                "it to Source-backed"
                )
            );

        connect(
            sourceAuthoritativeAction,
            &QAction::triggered,
            sourceMenu,
            [
                this,
                sessionId
            ]() {
                useSourceAsAuthoritative(
                    sessionId
                    );
            }
            );

        QAction *snapshotOnlyAction =
            sourceMenu->addAction(
                tr(
                    "Preserve as Snapshot Only..."
                    )
                );

        snapshotOnlyAction->setToolTip(
            tr(
                "Capture all currently admitted evidence into "
                "a fresh durable snapshot and convert this "
                "investigation to Snapshot-backed"
                )
            );

        connect(
            snapshotOnlyAction,
            &QAction::triggered,
            sourceMenu,
            [
                this,
                sessionId
            ]() {
                preserveSessionAsSnapshotOnly(
                    sessionId
                    );
            }
            );

        sourceMenu->addSeparator();

        /*
         * Connected external-source management.
         */
        QAction *redefineAction =
            sourceMenu->addAction(
                tr(
                    "Redefine Source Path..."
                    )
                );

        const bool canRedefine =
            source
                ->sourceIdentity
                .has_value();

        redefineAction->setEnabled(
            canRedefine
            );

        redefineAction->setToolTip(
            canRedefine
                ? tr(
                      "Select a relocated copy of the connected "
                      "source. TraceScope will update the path "
                      "only after verifying source identity."
                      )
                : tr(
                      "This investigation does not retain "
                      "enough physical source identity "
                      "information to verify relocation."
                      )
            );

        if (canRedefine) {
            connect(
                redefineAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    sessionId
                ]() {
                    redefineSessionSourcePath(
                        sessionId
                        );
                }
                );
        }

        const QString sourcePath =
            source->sourcePath;

        const QFileInfo sourceInfo(
            sourcePath
            );

        if (sourceInfo.exists()
            && sourceInfo.isFile()) {
            QAction *openAction =
                sourceMenu->addAction(
                    tr(
                        "Open Source Location"
                        )
                    );

            openAction->setToolTip(
                tr(
                    "Open the directory containing the "
                    "Hybrid investigation's connected "
                    "external source"
                    )
                );

            connect(
                openAction,
                &QAction::triggered,
                sourceMenu,
                [
                    this,
                    sourcePath
                ]() {
                    openSourceLocation(
                        sourcePath
                        );
                }
                );
        }

        break;
    }
    }
}

void MainWindow::
    createSessionComparison(
        const QString &preferredBaselineSessionId
        )
{
    if (workspace == nullptr
        || workspaceDocumentHost
               == nullptr
        || workspace->sessionCount()
               < 2) {
        return;
    }

    const InvestigationSession *activeSession =
        workspace->activeSession();

    if (activeSession == nullptr) {
        return;
    }

    /*
     * The active investigation is the session the
     * user is currently examining, so it defaults
     * to Comparison.
     */
    const QString initialComparisonSessionId =
        activeSession->id();

    QString initialBaselineSessionId =
        preferredBaselineSessionId;

    /*
     * A context-clicked inactive session can provide
     * the tentative Baseline.
     *
     * Context-clicking the active session must not
     * reverse the orientation. Likewise, an invalid
     * preferred ID is ignored and the dialog chooses
     * another open session as the Baseline.
     */
    if (initialBaselineSessionId
            == initialComparisonSessionId
        || workspace->indexOfSession(
               initialBaselineSessionId
               )
               < 0) {
        initialBaselineSessionId.clear();
    }

    InvestigationComparisonDialog dialog(
        workspace,
        initialBaselineSessionId,
        initialComparisonSessionId,
        this
        );

    if (
        dialog.exec()
        != QDialog::Accepted
        ) {
        return;
    }

    const QString baselineSessionId =
        dialog.baselineSessionId();

    const QString comparisonSessionId =
        dialog.comparisonSessionId();

    const int baselineIndex =
        workspace->indexOfSession(
            baselineSessionId
            );

    const int comparisonIndex =
        workspace->indexOfSession(
            comparisonSessionId
            );

    if (baselineIndex < 0
        || comparisonIndex < 0
        || baselineIndex
               == comparisonIndex) {
        return;
    }

    const InvestigationSession *baselineSession =
        workspace->sessionAt(
            baselineIndex
            );

    const InvestigationSession *comparisonSession =
        workspace->sessionAt(
            comparisonIndex
            );

    if (baselineSession == nullptr
        || comparisonSession == nullptr) {
        return;
    }

    InvestigationComparisonSnapshotBuilder builder;

    InvestigationComparisonSnapshot snapshot =
        builder.build(
            *baselineSession,
            *comparisonSession,
            dialog.burstSettings()
            );

    auto *document =
        new InvestigationComparisonDocument(
            std::move(snapshot)
            );

    if (!workspaceDocumentHost
             ->addDocument(
                 document,
                 true
                 )) {
        delete document;
    }
}

void MainWindow::
    updateComparisonActionState()
{
    if (compareAction == nullptr) {
        return;
    }

    compareAction->setEnabled(
        workspace != nullptr
        && workspace->sessionCount()
               >= 2
        );
}

void MainWindow::setWorkspaceOpenInProgress(
    bool inProgress
    )
{
    workspaceOpenInProgress =
        inProgress;

    const bool fileOperationAvailable =
        !workspaceOpenInProgress
        && !sessionReloadInProgress
        && !snapshotOpenInProgress
        && importWatcher == nullptr;

    if (openAction != nullptr) {
        openAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openWorkspaceAction != nullptr) {
        openWorkspaceAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openSnapshotAction != nullptr) {
        openSnapshotAction->setEnabled(
            fileOperationAvailable
            );
    }

    setAcceptDrops(
        fileOperationAvailable
        );

    updateReloadActionState();
}

void MainWindow::setSessionReloadInProgress(
    bool inProgress
    )
{
    sessionReloadInProgress =
        inProgress;
    
    const bool fileOperationAvailable =
        !workspaceOpenInProgress
        && !sessionReloadInProgress
        && !snapshotOpenInProgress
        && importWatcher == nullptr;

    if (openAction != nullptr) {
        openAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openWorkspaceAction != nullptr) {
        openWorkspaceAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openSnapshotAction != nullptr) {
        openSnapshotAction->setEnabled(
            fileOperationAvailable
            );
    }

    setAcceptDrops(
        fileOperationAvailable
        );

    updateReloadActionState();
}

void MainWindow::setSnapshotOpenInProgress(
    bool inProgress
    )
{
    snapshotOpenInProgress =
        inProgress;

    const bool fileOperationAvailable =
        !workspaceOpenInProgress
        && !sessionReloadInProgress
        && !snapshotOpenInProgress
        && importWatcher == nullptr;

    if (openAction != nullptr) {
        openAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openSnapshotAction != nullptr) {
        openSnapshotAction->setEnabled(
            fileOperationAvailable
            );
    }

    if (openWorkspaceAction != nullptr) {
        openWorkspaceAction->setEnabled(
            fileOperationAvailable
            );
    }

    setAcceptDrops(
        fileOperationAvailable
        );

    updateReloadActionState();
}

void MainWindow::updateReloadActionState()
{
    InvestigationSession *session =
        workspace != nullptr
            ? workspace->activeSession()
            : nullptr;

    const bool fileOperationAvailable =
        importWatcher == nullptr
        && !workspaceOpenInProgress
        && !sessionReloadInProgress
        && !snapshotOpenInProgress;

    if (saveSnapshotAction != nullptr) {
        saveSnapshotAction->setEnabled(
            session != nullptr
            && fileOperationAvailable
            );
    }

    if (reloadAction == nullptr) {
        return;
    }

    bool liveFollowActive = false;

    if (session != nullptr) {
        const LiveSessionFollowCoordinator
            *coordinator =
            session->liveFollowCoordinator();

        liveFollowActive =
            coordinator != nullptr
            && !coordinator
                    ->state()
                    .isStopped();
    }

    reloadAction->setEnabled(
        session != nullptr
        && fileOperationAvailable
        && !liveFollowActive
        );
}

void MainWindow::refreshRecentFilesMenu()
{
    if (recentFilesMenu == nullptr) {
        return;
    }

    recentFilesMenu->clear();

    const QStringList recentFiles =
        recentItemsStore.recentFiles();

    int validItemCount = 0;

    for (const QString &filePath
         : recentFiles) {
        const QFileInfo fileInfo(filePath);

        if (!fileInfo.exists()
            || !fileInfo.isFile()) {
            recentItemsStore
                .removeRecentFile(
                    filePath
                    );

            continue;
        }

        QAction *action =
            recentFilesMenu->addAction(
                fileInfo.fileName()
                );

        action->setToolTip(
            filePath
            );

        connect(
            action,
            &QAction::triggered,
            this,
            [this, filePath]() {
                openRecentFile(
                    filePath
                    );
            }
            );

        ++validItemCount;
    }

    recentFilesMenu->setEnabled(
        validItemCount > 0
        );
}

void MainWindow::refreshRecentWorkspacesMenu()
{
    if (recentWorkspacesMenu == nullptr) {
        return;
    }

    recentWorkspacesMenu->clear();

    const QStringList recentWorkspaces =
        recentItemsStore.recentWorkspaces();

    int validItemCount = 0;

    for (const QString &filePath
         : recentWorkspaces) {
        const QFileInfo fileInfo(
            filePath
            );

        if (!fileInfo.exists()
            || !fileInfo.isFile()) {
            recentItemsStore
                .removeRecentWorkspace(
                    filePath
                    );

            continue;
        }

        QAction *action =
            recentWorkspacesMenu
                ->addAction(
                    fileInfo.fileName()
                    );

        action->setToolTip(
            filePath
            );

        connect(
            action,
            &QAction::triggered,
            this,
            [this, filePath]() {
                openRecentWorkspace(
                    filePath
                    );
            }
            );

        ++validItemCount;
    }

    recentWorkspacesMenu->setEnabled(
        validItemCount > 0
        );
}

void MainWindow::openRecentFile(
    const QString &filePath
    )
{
    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        recentItemsStore
            .removeRecentFile(
                filePath
                );

        refreshRecentFilesMenu();

        QMessageBox::warning(
            this,
            tr("Recent File Not Found"),
            tr(
                "The recent log file no longer "
                "exists at:\n%1"
                )
                .arg(filePath)
            );

        return;
    }

    openLogFile(
        filePath
        );
}

void MainWindow::openRecentWorkspace(
    const QString &filePath
    )
{
    const QFileInfo fileInfo(
        filePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        recentItemsStore
            .removeRecentWorkspace(
                filePath
                );

        refreshRecentWorkspacesMenu();

        QMessageBox::warning(
            this,
            tr(
                "Recent Workspace Not Found"
                ),
            tr(
                "The recent TraceScope workspace "
                "no longer exists at:\n%1"
                )
                .arg(
                    filePath
                    )
            );

        return;
    }

    openWorkspace(
        filePath
        );
}

WorkspacePersistenceState
MainWindow::captureWorkspaceState() const
{
    WorkspacePersistenceState state;

    if (workspace == nullptr
        || workspaceDocumentHost
               == nullptr) {
        return state;
    }

    for (int index = 0;
         index < workspace->sessionCount();
         ++index) {
        const InvestigationSession *session =
            workspace->sessionAt(
                index
                );

        if (session == nullptr) {
            continue;
        }

        InvestigationSessionPresentationState
            presentationState;

        WorkspaceDocument *document =
            workspaceDocumentHost
                ->documentById(
                    session->id()
                    );

        const auto *sessionView =
            qobject_cast<
                const InvestigationSessionView *>(
                document
                );

        if (sessionView != nullptr) {
            presentationState =
                sessionView
                    ->capturePresentationState();
        }

        state.sessions.append(
            InvestigationSessionPersistence
            ::capture(
                *session,
                presentationState
                )
            );
    }

    const QVector<WorkspaceDocument *>
        documents =
        workspaceDocumentHost->documents();

    for (WorkspaceDocument *document
         : documents) {
        const auto *comparisonDocument =
            qobject_cast<
                const InvestigationComparisonDocument *>(
                document
                );

        if (comparisonDocument == nullptr) {
            continue;
        }

        state.comparisons.append(
            InvestigationComparisonPersistence::
            capture(
                comparisonDocument
                    ->snapshot(),
                comparisonDocument
                    ->capturePresentationState()
                )
            );
    }

    state.documentLayout =
        workspaceDocumentHost
            ->captureLayoutState();

    const QRect normalMainGeometry =
        normalGeometry();

    state.documentLayout
        .mainWindowGeometry =
        isMaximized()
                && normalMainGeometry.isValid()
            ? normalMainGeometry
            : geometry();

    state.documentLayout
        .mainWindowMaximized =
        isMaximized();

    return state;
}

bool MainWindow::saveWorkspaceToFile(
    const QString &filePath,
    const QString &snapshotOnlySessionId,
    QString *snapshotOnlyPath
    )
{
    WorkspacePersistenceState state =
        captureWorkspaceState();

    if (snapshotOnlyPath != nullptr) {
        snapshotOnlyPath->clear();
    }

    /*
     * A source-lifecycle transition is being persisted
     * atomically with this workspace save.
     *
     * Runtime backing has not changed yet. Only the staged
     * persisted representation becomes SnapshotBacked.
     */
    if (!snapshotOnlySessionId.isEmpty()) {
        bool targetFound = false;

        for (PersistedInvestigationSession
                 &persistedSession
             : state.sessions) {
            if (persistedSession.sessionId
                != snapshotOnlySessionId) {
                continue;
            }

            targetFound = true;

            const auto mode =
                persistedSession
                    .backing
                    .mode;

            if (mode
                    != PersistedInvestigationSessionBackingMode::
                    SourceBacked
                && mode
                       != PersistedInvestigationSessionBackingMode::
                       Hybrid) {
                QMessageBox::warning(
                    this,
                    tr("Preserve Snapshot Failed"),
                    tr(
                        "The selected investigation is "
                        "not currently source-backed."
                        )
                    );

                return false;
            }

            /*
             * Keep externalSourceBinding.
             *
             * In SnapshotBacked persistence it becomes the
             * dormant reconnect hint rather than an active
             * dependency.
             */
            persistedSession.backing.mode =
                PersistedInvestigationSessionBackingMode::
                SnapshotBacked;

            /*
             * SnapshotBacked uses the .tsinv import profile
             * as authoritative.
             */
            persistedSession
                .backing
                .sourceImportProfile
                .reset();

            break;
        }

        if (!targetFound) {
            QMessageBox::warning(
                this,
                tr("Preserve Snapshot Failed"),
                tr(
                    "The investigation could not be found "
                    "in the workspace being saved."
                    )
                );

            return false;
        }
    }

    QVector<WorkspaceSessionSnapshotSaveItem>
        sessionSnapshots;

    if (workspace != nullptr) {
        sessionSnapshots.reserve(
            workspace->sessionCount()
            );

        /*
         * Capture immutable normalized evidence on the
         * UI thread before any package files are written.
         *
         * This gives every .tsinv a stable save-time
         * record set even if live following continues
         * after this method returns.
         */
        for (int index = 0;
             index < workspace->sessionCount();
             ++index) {
            const InvestigationSession *session =
                workspace->sessionAt(
                    index
                    );

            if (session == nullptr) {
                continue;
            }

            WorkspaceSessionSnapshotSaveItem item;

            item.sessionId =
                session->id();

            item.snapshot =
                session->captureSnapshot();

            sessionSnapshots.append(
                std::move(item)
                );
        }
    }

    if (state.sessions.size()
        != sessionSnapshots.size()) {
        QMessageBox::warning(
            this,
            tr("Save Workspace Failed"),
            tr(
                "TraceScope could not capture a "
                "consistent snapshot for every open "
                "investigation."
                )
            );

        return false;
    }

    WorkspaceSavePackageResult saveResult =
        WorkspaceSavePackageService()
            .save(
                filePath,
                std::move(state),
                std::move(sessionSnapshots)
                );

    if (!saveResult.succeeded) {
        const QString reason =
            saveResult.errorMessage.isEmpty()
                ? tr(
                      "The workspace package could "
                      "not be saved successfully."
                      )
                : saveResult.errorMessage;

        QMessageBox::warning(
            this,
            tr("Save Workspace Failed"),
            tr(
                "TraceScope could not save the "
                "workspace.\n\n"
                "Reason:\n%1"
                )
                .arg(reason)
            );

        return false;
    }

    /*
     * The manifest is now committed successfully.
     *
     * SnapshotBacked and Hybrid runtime sessions must
     * follow the newly committed durable snapshot path
     * so future Hybrid reloads use exactly the snapshot
     * referenced by the saved workspace.
     *
     * SourceBacked sessions intentionally reject this
     * update because their newly written .tsinv is a
     * save-time recovery fallback, not a runtime mode
     * transition.
     */
    if (workspace != nullptr) {
        for (int index = 0;
             index < workspace->sessionCount();
             ++index) {
            InvestigationSession *session =
                workspace->sessionAt(
                    index
                    );

            if (session == nullptr) {
                continue;
            }

            const auto snapshotPathIterator =
                saveResult
                    .snapshotPathsBySessionId
                    .constFind(
                        session->id()
                        );

            if (snapshotPathIterator
                == saveResult
                       .snapshotPathsBySessionId
                       .constEnd()) {
                continue;
            }

            const QString newSnapshotPath =
                snapshotPathIterator.value();

            /*
             * The target session's persisted backing has
             * already been committed as SnapshotBacked.
             *
             * Do not change its runtime backing here.
             * preserveSessionAsSnapshotOnly() will perform
             * that transition after this save returns.
             */
            if (session->id()
                == snapshotOnlySessionId) {
                if (snapshotOnlyPath != nullptr) {
                    *snapshotOnlyPath =
                        newSnapshotPath;
                }

                continue;
            }

            /*
             * Existing SnapshotBacked / Hybrid sessions
             * follow the newly committed workspace-owned
             * snapshot as before.
             *
             * SourceBacked sessions reject this update,
             * which is intentional: their saved snapshot
             * remains only a recovery fallback.
             */
            session->updateSnapshotBackingPath(
                newSnapshotPath
                );
        }
    }

    currentWorkspacePath =
        QFileInfo(filePath)
            .absoluteFilePath();

    recentItemsStore.addRecentWorkspace(
        currentWorkspacePath
        );

    refreshRecentWorkspacesMenu();

    return true;
}

void MainWindow::saveWorkspace()
{
    if (currentWorkspacePath.isEmpty()) {
        saveWorkspaceAs();
        return;
    }

    saveWorkspaceToFile(
        currentWorkspacePath
        );
}

void MainWindow::saveWorkspaceAs()
{
    QString initialPath =
        currentWorkspacePath;

    if (initialPath.isEmpty()) {
        initialPath =
            QStringLiteral(
                "tracescope-workspace.tsw"
                );
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            tr("Save TraceScope Workspace"),
            initialPath,
            tr(
                "TraceScope Workspace (*.tsw);;"
                "Legacy TraceScope Workspace (*.json);;"
                "All Files (*)"
                )
            );

    if (filePath.isEmpty()) {
        return;
    }

    saveWorkspaceToFile(
        filePath
        );
}

MainWindow::WorkspaceSessionRecoveryOutcome
MainWindow::resolveWorkspaceSessionRecovery(
    PersistedInvestigationSession
        &persistedSession,
    const QString &workspaceFilePath
    )
{
    /*
     * Snapshot-backed sessions have no external source
     * dependency.
     *
     * Hybrid sessions may also restore successfully
     * when their external continuation source is
     * unavailable because the durable snapshot remains
     * the evidence floor.
     *
     * Only SourceBacked sessions require source-file
     * recovery before restoration preparation begins.
     */
    if (persistedSession.backing.mode
        != PersistedInvestigationSessionBackingMode::
        SourceBacked) {
        return
            WorkspaceSessionRecoveryOutcome::
            Ready;
    }

    if (!persistedSession
             .backing
             .externalSourceBinding
             .has_value()) {
        /*
         * This is malformed persistence rather than a
         * normal missing-file recovery case.
         *
         * Let InvestigationSessionRestorationService
         * produce the authoritative validation error.
         */
        return
            WorkspaceSessionRecoveryOutcome::
            Ready;
    }

    PersistedInvestigationExternalSourceBinding
        &binding =
        *persistedSession
             .backing
             .externalSourceBinding;

    bool snapshotFallbackAvailable =
        persistedSession
            .backing
            .snapshotReference
            .has_value();

    QString snapshotFallbackPath;

    if (snapshotFallbackAvailable) {
        snapshotFallbackPath =
            resolveWorkspaceSnapshotPath(
                workspaceFilePath,
                *persistedSession
                     .backing
                     .snapshotReference
                );

        const QFileInfo snapshotInfo(
            snapshotFallbackPath
            );

        snapshotFallbackAvailable =
            !snapshotFallbackPath.isEmpty()
            && snapshotInfo.exists()
            && snapshotInfo.isFile();
    }

    bool sourceAvailable =
        !binding.sourcePath
             .trimmed()
             .isEmpty();

    if (sourceAvailable) {
        const QFileInfo sourceInfo(
            binding.sourcePath
            );

        sourceAvailable =
            sourceInfo.exists()
            && sourceInfo.isFile();
    }

    while (!sourceAvailable) {
        QMessageBox prompt(this);

        prompt.setIcon(
            QMessageBox::Warning
            );

        prompt.setWindowTitle(
            tr(
                "Workspace Source File Missing"
                )
            );

        prompt.setText(
            tr(
                "A Source-backed investigation "
                "depends on an external source file "
                "that could not be found."
                )
            );

        if (snapshotFallbackAvailable) {
            prompt.setInformativeText(
                tr(
                    "Saved source location:\n%1\n\n"
                    "This workspace also contains a durable "
                    "snapshot of the investigation as it "
                    "existed when the workspace was saved.\n\n"
                    "Open the saved snapshot to recover that "
                    "evidence without the original source, "
                    "locate the source file, skip the session, "
                    "or cancel opening the workspace."
                    )
                    .arg(
                        binding.sourcePath
                        )
                );
        } else {
            prompt.setInformativeText(
                tr(
                    "Saved location:\n%1\n\n"
                    "Locate the source file to restore this "
                    "session, skip the session, or cancel "
                    "opening the workspace."
                    )
                    .arg(
                        binding.sourcePath
                                .isEmpty()
                            ? tr("(no saved path)")
                            : binding.sourcePath
                        )
                );
        }

        QPushButton *snapshotButton =
            nullptr;

        if (snapshotFallbackAvailable) {
            snapshotButton =
                prompt.addButton(
                    tr("Open Saved Snapshot"),
                    QMessageBox::AcceptRole
                    );
        }

        QPushButton *locateButton =
            prompt.addButton(
                tr("Locate File..."),
                snapshotFallbackAvailable
                    ? QMessageBox::ActionRole
                    : QMessageBox::AcceptRole
                );

        QPushButton *skipButton =
            prompt.addButton(
                tr("Skip Session"),
                QMessageBox::ActionRole
                );

        QPushButton *cancelButton =
            prompt.addButton(
                QMessageBox::Cancel
                );

        prompt.setDefaultButton(
            snapshotFallbackAvailable
                ? snapshotButton
                : locateButton
            );

        prompt.exec();

        if (prompt.clickedButton()
            == cancelButton) {
            return
                WorkspaceSessionRecoveryOutcome::
                Abort;
        }

        if (snapshotButton != nullptr
            && prompt.clickedButton()
                   == snapshotButton) {
            /*
             * This is an explicit recovery decision.
             *
             * The persisted SourceBacked session becomes
             * SnapshotBacked for this restoration. The saved
             * normalized evidence is now authoritative, and
             * the unavailable external source is no longer a
             * runtime dependency.
             */
            persistedSession.backing.mode =
                PersistedInvestigationSessionBackingMode::
                SnapshotBacked;

            persistedSession
                .backing
                .sourceImportProfile
                .reset();

            return
                WorkspaceSessionRecoveryOutcome::
                Ready;
        }

        if (prompt.clickedButton()
            == skipButton) {
            return
                WorkspaceSessionRecoveryOutcome::
                SkipSession;
        }

        if (prompt.clickedButton()
            != locateButton) {
            continue;
        }

        const QFileInfo missingInfo(
            binding.sourcePath
            );

        const QString replacementPath =
            QFileDialog::getOpenFileName(
                this,
                tr(
                    "Locate Workspace Source File"
                    ),
                missingInfo.absolutePath(),
                tr("All Files (*)")
                );

        /*
         * Cancelling the file picker returns to the
         * recovery prompt rather than silently skipping
         * the investigation.
         */
        if (replacementPath.isEmpty()) {
            continue;
        }

        const QFileInfo replacementInfo(
            replacementPath
            );

        if (!replacementInfo.exists()
            || !replacementInfo.isFile()) {
            QMessageBox::warning(
                this,
                tr("Source File Not Found"),
                tr(
                    "The selected source file could "
                    "not be opened. Choose another "
                    "file, skip this session, or "
                    "cancel workspace opening."
                    )
                );

            continue;
        }

        binding.sourcePath =
            replacementInfo
                .absoluteFilePath();

        /*
         * Keep the temporary schema-v1 compatibility
         * mirror synchronized until the legacy open
         * path is removed completely.
         */
        persistedSession.sourcePath =
            binding.sourcePath;

        sourceAvailable = true;
    }

    /*
     * Source-family persistence contains only the
     * logical rotation rule. Restoration itself will
     * rediscover physical rotated members.
     *
     * We perform this lightweight discovery here only
     * so the existing user-facing Active Source Only /
     * Skip / Cancel recovery choice is preserved.
     */
    SourceFamilyConfiguration
        &familyConfiguration =
        binding.sourceFamilyConfiguration;

    if (!familyConfiguration
             .includeRotatedSources) {
        return
            WorkspaceSessionRecoveryOutcome::
            Ready;
    }

    const RotatedSourceDiscoveryResult discovery =
        RotatedSourceDiscoveryService::discover(
            binding.sourcePath,
            familyConfiguration.rotationRule
            );

    if (!discovery.succeeded) {
        QMessageBox::warning(
            this,
            tr("Source Family Discovery Failed"),
            tr(
                "TraceScope could not rediscover the "
                "rotated source family configured for "
                "this workspace session.\n\n%1"
                )
                .arg(
                    discovery.errorMessage
                    )
            );

        return
            WorkspaceSessionRecoveryOutcome::
            Abort;
    }

    if (!discovery.rotatedSources.isEmpty()) {
        return
            WorkspaceSessionRecoveryOutcome::
            Ready;
    }

    QMessageBox prompt(this);

    prompt.setIcon(
        QMessageBox::Warning
        );

    prompt.setWindowTitle(
        tr(
            "Workspace Rotated Sources Not Found"
            )
        );

    prompt.setText(
        tr(
            "The active source file exists, but none "
            "of the configured rotated source files "
            "can currently be rediscovered."
            )
        );

    prompt.setInformativeText(
        tr(
            "Active source:\n%1\n\n"
            "You can restore this Source-backed "
            "investigation using only the active "
            "source, skip the session, or cancel "
            "opening the workspace."
            )
            .arg(
                binding.sourcePath
                )
        );

    QPushButton *activeOnlyButton =
        prompt.addButton(
            tr("Open Active Source Only"),
            QMessageBox::AcceptRole
            );

    QPushButton *skipButton =
        prompt.addButton(
            tr("Skip Session"),
            QMessageBox::ActionRole
            );

    QPushButton *cancelButton =
        prompt.addButton(
            QMessageBox::Cancel
            );

    prompt.setDefaultButton(
        activeOnlyButton
        );

    prompt.exec();

    if (prompt.clickedButton()
        == cancelButton) {
        return
            WorkspaceSessionRecoveryOutcome::
            Abort;
    }

    if (prompt.clickedButton()
        == skipButton) {
        return
            WorkspaceSessionRecoveryOutcome::
            SkipSession;
    }

    if (prompt.clickedButton()
        != activeOnlyButton) {
        return
            WorkspaceSessionRecoveryOutcome::
            Abort;
    }

    /*
     * The user deliberately changed this restored
     * SourceBacked session to active-only.
     *
     * Persist that logical choice in the staged state
     * so Reload and a later Save do not immediately
     * expect the unavailable rotations again.
     */
    familyConfiguration.includeRotatedSources =
        false;

    familyConfiguration
        .rotatedSourcePaths
        .clear();

    /*
     * Temporary compatibility mirror. This disappears
     * with the legacy open code in the next step.
     */
    persistedSession
        .sourceFamilyConfiguration
        .includeRotatedSources =
        false;

    return
        WorkspaceSessionRecoveryOutcome::
        Ready;
}

void MainWindow::openWorkspace(const QString &initialFilePath)
{
    if (importWatcher != nullptr
        || workspaceOpenInProgress
        || snapshotOpenInProgress
        || sessionReloadInProgress) {
        QMessageBox::information(
            this,
            tr("File Operation In Progress"),
            tr(
                "TraceScope is already importing a log file "
                "or opening a workspace. Wait for the current "
                "operation to finish before opening another "
                "workspace."
                )
            );

        return;
    }

    QString filePath =
        initialFilePath;

    if (filePath.isEmpty()) {
        filePath =
            QFileDialog::getOpenFileName(
                this,
                tr("Open TraceScope Workspace"),
                currentWorkspacePath,
                tr(
                    "TraceScope Workspace (*.tsw *.json);;"
                    "All Files (*)"
                    )
                );
    }

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(
        filePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        QMessageBox::warning(
            this,
            tr("Open Workspace Failed"),
            tr(
                "TraceScope could not open the "
                "selected workspace file."
                )
            );

        return;
    }

    const QByteArray json =
        file.readAll();

    file.close();

    const WorkspaceSerializer serializer;

    WorkspaceDeserializationResult result =
        serializer.deserialize(
            json
            );

    if (!result.isSuccess()) {
        const QString reason =
            result.errorMessage.isEmpty()
                ? tr(
                      "The selected file is not a "
                      "valid TraceScope workspace."
                      )
                : result.errorMessage;

        QMessageBox::warning(
            this,
            tr("Open Workspace Failed"),
            tr(
                "TraceScope was unable to load the "
                "selected workspace.\n\n"
                "Reason:\n%1"
                )
                .arg(
                    reason
                    )
            );

        return;
    }

    WorkspacePersistenceState state =
        std::move(
            result.workspace.value()
            );

    /*
     * Don't silently destroy the investigation the
     * user is currently working in.
     */
    if (workspaceDocumentHost != nullptr
        && !workspaceDocumentHost
                ->documents()
                .isEmpty()) {
        const QMessageBox::StandardButton choice =
            QMessageBox::question(
                this,
                tr("Replace Current Workspace"),
                tr(
                    "Opening this workspace will "
                    "replace the workspace that is "
                    "currently open.\n\n"
                    "Save the current workspace first "
                    "if you want to keep any changes.\n\n"
                    "Continue?"
                    ),
                QMessageBox::Yes
                    | QMessageBox::Cancel,
                QMessageBox::Cancel
                );

        if (choice != QMessageBox::Yes) {
            return;
        }
    }

    int skippedSessionCount = 0;

    /*
     * Resolve all user-facing recovery choices before any
     * asynchronous preparation begins.
     *
     * Cancelling here leaves the currently installed
     * workspace completely untouched.
     */
    for (int index = 0;
         index < state.sessions.size();) {
        PersistedInvestigationSession
            &persistedSession =
            state.sessions[index];

        const WorkspaceSessionRecoveryOutcome
            recoveryOutcome =
            resolveWorkspaceSessionRecovery(
                persistedSession,
                filePath
                );

        if (recoveryOutcome
            == WorkspaceSessionRecoveryOutcome::
            Abort) {
            return;
        }

        if (recoveryOutcome
            == WorkspaceSessionRecoveryOutcome::
            SkipSession) {
            state.sessions.removeAt(
                index
                );

            ++skippedSessionCount;

            /*
             * The next session now occupies this index.
             */
            continue;
        }

        ++index;
    }

    auto operation =
        std::make_shared<
            WorkspaceOpenOperation
            >();

    operation->workspacePath =
        QFileInfo(filePath)
            .absoluteFilePath();

    operation->skippedSessionCount =
        skippedSessionCount;

    operation->state =
        std::move(
            state
            );

    operation->stagedSessions.reserve(
        static_cast<std::size_t>(
            operation->state
                .sessions
                .size()
            )
        );

    setWorkspaceOpenInProgress(
        true
        );

    continueWorkspaceOpen(
        operation
        );
}

void MainWindow::continueWorkspaceOpen(
    const std::shared_ptr<
        WorkspaceOpenOperation
        > &operation
    )
{
    if (operation == nullptr) {
        setWorkspaceOpenInProgress(
            false
            );

        return;
    }

    /*
     * Every recoverable investigation has now been
     * prepared and materialized successfully.
     *
     * This remains the first point where the currently
     * installed workspace is allowed to change.
     */
    if (operation->nextSessionIndex
        >= operation->state
               .sessions
               .size()) {
        installOpenedWorkspace(
            operation
            );

        setWorkspaceOpenInProgress(
            false
            );

        return;
    }

    const int sessionIndex =
        operation->nextSessionIndex;

    /*
     * Copy persistence data into the worker boundary.
     *
     * The worker must not retain references into the
     * mutable WorkspaceOpenOperation state.
     */
    const PersistedInvestigationSession
        persistedSession =
        operation->state
            .sessions
            .at(
                sessionIndex
                );

    const int totalSessionCount =
        operation->state
            .sessions
            .size();

    auto *watcher =
        new QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>(
            this
            );

    auto *progressDialog =
        new QProgressDialog(
            tr(
                "Restoring workspace session "
                "%1 of %2...\n"
                "Preparing investigation..."
                )
                .arg(
                    sessionIndex + 1
                    )
                .arg(
                    totalSessionCount
                    ),
            tr("Cancel"),
            0,
            0,
            this
            );

    progressDialog->setWindowTitle(
        tr("Open Workspace")
        );

    progressDialog->setWindowModality(
        Qt::NonModal
        );

    progressDialog->setMinimumDuration(
        500
        );

    progressDialog->setAutoClose(
        false
        );

    progressDialog->setAutoReset(
        false
        );

    connect(
        progressDialog,
        &QProgressDialog::canceled,
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        cancel
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressRangeChanged,
        progressDialog,
        &QProgressDialog::setRange
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressValueChanged,
        progressDialog,
        &QProgressDialog::setValue
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        progressTextChanged,
        this,
        [
            progressDialog,
            sessionIndex,
            totalSessionCount
        ](
            const QString &progressText
            ) {
            QString label =
                tr(
                    "Restoring workspace session "
                    "%1 of %2..."
                    )
                    .arg(
                        sessionIndex + 1
                        )
                    .arg(
                        totalSessionCount
                        );

            if (!progressText.isEmpty()) {
                label +=
                    QStringLiteral("\n")
                    + progressText;
            }

            progressDialog->setLabelText(
                label
                );
        }
        );

    connect(
        watcher,
        &QFutureWatcher<
            InvestigationSessionRestorationPreparationResult>::
        finished,
        this,
        [
            this,
            watcher,
            progressDialog,
            operation,
            sessionIndex
        ]() mutable {
            const bool cancelled =
                watcher->isCanceled();

            progressDialog->hide();
            progressDialog->deleteLater();

            /*
             * A cancelled staged restoration aborts the
             * entire workspace-open operation.
             *
             * Nothing has been installed yet, so the
             * current workspace remains untouched.
             */
            if (cancelled) {
                watcher->deleteLater();

                setWorkspaceOpenInProgress(
                    false
                    );

                return;
            }

            if (watcher
                    ->future()
                    .resultCount()
                <= 0) {
                watcher->deleteLater();

                QMessageBox::warning(
                    this,
                    tr("Open Workspace Failed"),
                    tr(
                        "TraceScope did not receive a "
                        "restoration result for one of "
                        "the workspace sessions."
                        )
                    );

                setWorkspaceOpenInProgress(
                    false
                    );

                return;
            }

            InvestigationSessionRestorationPreparationResult
                preparation =
                watcher->result();

            watcher->deleteLater();

            if (!preparation.succeeded) {
                const QString reason =
                    preparation
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "The investigation could "
                              "not be prepared."
                              )
                        : preparation.errorMessage;

                QMessageBox::warning(
                    this,
                    tr("Open Workspace Failed"),
                    tr(
                        "TraceScope was unable to "
                        "restore one of the workspace "
                        "investigations.\n\n"
                        "Reason:\n%1"
                        )
                        .arg(
                            reason
                            )
                    );

                setWorkspaceOpenInProgress(
                    false
                    );

                return;
            }

            /*
             * We are now back on MainWindow's thread.
             *
             * InvestigationSession, InvestigationController,
             * and the Qt models are deliberately created
             * here rather than on the worker thread.
             */
            InvestigationSessionRestorationService
                restorationService;

            InvestigationSessionRestorationResult
                restoration =
                restorationService.materialize(
                    operation->state
                        .sessions
                        .at(
                            sessionIndex
                            ),
                    std::move(
                        preparation
                        )
                    );

            if (!restoration.succeeded
                || restoration.session
                       == nullptr) {
                const QString reason =
                    restoration
                            .errorMessage
                            .isEmpty()
                        ? tr(
                              "The prepared investigation "
                              "could not be materialized."
                              )
                        : restoration.errorMessage;

                QMessageBox::warning(
                    this,
                    tr("Open Workspace Failed"),
                    tr(
                        "TraceScope was unable to "
                        "materialize one of the "
                        "workspace investigations.\n\n"
                        "Reason:\n%1"
                        )
                        .arg(
                            reason
                            )
                    );

                setWorkspaceOpenInProgress(
                    false
                    );

                return;
            }

            if (restoration
                    .session
                    ->importedRecordCount()
                == 0) {
                ++operation
                      ->emptySessionCount;
            }

            /*
             * Stage the fully constructed session, but
             * do not install it yet.
             *
             * If a later session fails, destruction of
             * WorkspaceOpenOperation discards every
             * staged session and the user's existing
             * workspace remains intact.
             */
            operation
                ->stagedSessions
                .push_back(
                    std::move(
                        restoration.session
                        )
                    );

            ++operation
                  ->nextSessionIndex;

            continueWorkspaceOpen(
                operation
                );
        }
        );

    watcher->setFuture(
        QtConcurrent::run(
            [
                persistedSession,
                workspacePath =
                operation->workspacePath
    ](
                QPromise<
                    InvestigationSessionRestorationPreparationResult>
                    &promise
                ) {
                /*
                 * Snapshot-only restoration has no
                 * byte-progress stream, so begin in
                 * indeterminate mode.
                 */
                promise.setProgressRange(
                    0,
                    0
                    );

                bool determinateProgress =
                    false;

                ImportExecutionContext
                    executionContext;

                executionContext
                    .isCancellationRequested =
                    [&promise]() {
                        return promise.isCanceled();
                    };

                executionContext.reportProgress =
                    [
                        &promise,
                        &determinateProgress
                    ](
                        const ImportProgress &progress
                        ) {
                        if (progress.totalBytes
                            <= 0) {
                            return;
                        }

                        if (!determinateProgress) {
                            promise.setProgressRange(
                                0,
                                100
                                );

                            determinateProgress =
                                true;
                        }

                        const double percentageValue =
                            100.0
                            * static_cast<double>(
                                progress.bytesProcessed
                                )
                            / static_cast<double>(
                                progress.totalBytes
                                );

                        const int percentage =
                            std::clamp(
                                static_cast<int>(
                                    percentageValue
                                    ),
                                0,
                                100
                                );

                        const QString progressText =
                            QStringLiteral(
                                "%1 records processed"
                                )
                                .arg(
                                    progress
                                        .processedRecordCount
                                    );

                        promise
                            .setProgressValueAndText(
                                percentage,
                                progressText
                                );
                    };

                InvestigationSessionRestorationService
                    restorationService;

                InvestigationSessionRestorationPreparationResult
                    preparation =
                    restorationService.prepare(
                        persistedSession,
                        workspacePath,
                        executionContext
                        );

                /*
                 * A cancelled future intentionally
                 * publishes no partial preparation.
                 */
                if (promise.isCanceled()) {
                    return;
                }

                if (determinateProgress) {
                    promise
                        .setProgressValueAndText(
                            100,
                            QStringLiteral(
                                "Investigation prepared"
                                )
                            );
                }

                promise.addResult(
                    std::move(
                        preparation
                        )
                    );
            }
            )
        );
}

void MainWindow::clearCurrentWorkspace()
{
    if (workspace == nullptr
        || workspaceDocumentHost
               == nullptr) {
        return;
    }

    /*
     * Closing sessions through InvestigationWorkspace
     * lets the existing sessionClosed connection
     * remove their InvestigationSessionView documents
     * correctly, including detached ones.
     */
    while (workspace->sessionCount() > 0) {
        workspace->closeSession(
            workspace->sessionCount()
            - 1
            );
    }

    /*
     * Session documents are now gone. Remove any
     * remaining non-session documents, currently
     * immutable comparison documents.
     */
    const QVector<WorkspaceDocument *>
        remainingDocuments =
        workspaceDocumentHost
            ->documents();

    for (WorkspaceDocument *document
         : remainingDocuments) {
        if (document == nullptr) {
            continue;
        }

        WorkspaceDocument *removed =
            workspaceDocumentHost
                ->removeDocument(
                    document->documentId()
                    );

        if (removed != nullptr) {
            removed->deleteLater();
        }
    }
}

void MainWindow::installOpenedWorkspace(
    const std::shared_ptr<
        WorkspaceOpenOperation
        > &operation
    )
{
    if (operation == nullptr
        || workspace == nullptr
        || workspaceDocumentHost
               == nullptr) {
        return;
    }

    /*
     * Every recoverable source has now completed its
     * import. This is the first point where replacing
     * the existing workspace is safe.
     */
    clearCurrentWorkspace();

    for (int index = 0;
         index < operation->state
                     .sessions
                     .size();
         ++index) {
        if (index
            >= static_cast<int>(
                operation->stagedSessions
                    .size()
                )) {
            break;
        }

        const PersistedInvestigationSession
            &persistedSession =
            operation->state
                .sessions
                .at(index);

        std::unique_ptr<InvestigationSession>
            session =
            std::move(
                operation->stagedSessions[
                    static_cast<
                        std::size_t
                        >(index)
                ]
                );

        if (!session) {
            continue;
        }

        const QString sessionId =
            session->id();

        addSessionToWorkspace(
            std::move(
                session
                )
            );

        /*
         * sessionAdded has synchronously constructed
         * the InvestigationSessionView, so visual
         * presentation state can now be restored.
         */
        WorkspaceDocument *document =
            workspaceDocumentHost
                ->documentById(
                    sessionId
                    );

        auto *sessionView =
            qobject_cast<
                InvestigationSessionView *>(
                document
                );

        if (sessionView != nullptr) {
            sessionView
                ->restorePresentationState(
                    persistedSession
                        .presentationState
                    );
        }
    }

    /*
     * Comparison snapshots are persisted independently
     * of the live sessions. Restore them even when one
     * of their original source sessions was skipped
     * because its source file is unavailable.
     */
    for (
        const PersistedInvestigationComparison
            &persistedComparison
        : std::as_const(operation->state.comparisons)
        ) {
        InvestigationComparisonSnapshot snapshot =
            InvestigationComparisonPersistence::
            restore(
                persistedComparison
                );

        auto *document =
            new InvestigationComparisonDocument(
                std::move(
                    snapshot
                    )
                );

        if (!workspaceDocumentHost
                 ->addDocument(
                     document,
                     false
                     )) {
            delete document;
        }
    }

    const WorkspaceDocumentLayoutState
        &layoutState =
        operation->state.documentLayout;

    if (layoutState
            .mainWindowGeometry
            .isValid()) {
        /*
         * Normalize first so setGeometry() establishes
         * the saved normal position/size even if the
         * currently running window happens to be
         * maximized.
         */
        showNormal();

        setGeometry(
            layoutState
                .mainWindowGeometry
            );

        if (layoutState
                .mainWindowMaximized) {
            showMaximized();
        }
    }

    /*
     * All documents must exist before restoring
     * tab order, detached groups, window geometry,
     * local current tabs, and global active document.
     *
     * Missing document IDs are already deliberately
     * ignored by WorkspaceDocumentHost.
     */
    workspaceDocumentHost
        ->restoreLayoutState(
            layoutState
            );

    for (
        const PersistedInvestigationComparison
            &persistedComparison
        : std::as_const(operation->state.comparisons)
        ) {
        WorkspaceDocument *document =
            workspaceDocumentHost
                ->documentById(
                    persistedComparison
                        .comparisonId
                    );

        auto *comparisonDocument =
            qobject_cast<
                InvestigationComparisonDocument *>(
                document
                );

        if (comparisonDocument != nullptr) {
            comparisonDocument
                ->restorePresentationState(
                    persistedComparison
                        .presentationState
                    );
        }
    }

    currentWorkspacePath =
        operation->workspacePath;

    recentItemsStore.addRecentWorkspace(
        currentWorkspacePath
        );

    refreshRecentWorkspacesMenu();

    if (operation->skippedSessionCount > 0
        || operation->emptySessionCount > 0) {
        QStringList messages;

        if (operation->skippedSessionCount > 0) {
            messages.append(
                tr(
                    "%1 unavailable investigation session(s) "
                    "were skipped."
                    )
                    .arg(
                        operation
                            ->skippedSessionCount
                        )
                );
        }

        if (operation->emptySessionCount > 0) {
            messages.append(
                tr(
                    "%1 restored investigation session(s) "
                    "loaded no events."
                    )
                    .arg(
                        operation
                            ->emptySessionCount
                        )
                );
        }

        QMessageBox::warning(
            this,
            tr(
                "Workspace Partially Restored"
                ),
            messages.join(
                QStringLiteral("\n")
                )
            );
    }
}

void MainWindow::exportInvestigationReport(
    const QString &originDocumentId
    )
{
    if (workspaceDocumentHost == nullptr) {
        return;
    }

    /*
     * Resolve the complete logical workspace rather
     * than only the tab group containing the origin
     * document. Detached documents therefore remain
     * eligible report sources.
     */
    const InvestigationReportWorkspaceContextBuilder
        contextBuilder;

    InvestigationReportWorkspaceContext context =
        contextBuilder.build(
            *workspaceDocumentHost,
            originDocumentId
            );

    if (!context.hasReportDocuments()) {
        return;
    }

    /*
     * Parent the workflow to the originating document's
     * window where possible. This keeps export dialogs
     * associated with a detached window when that is
     * where the command originated.
     */
    WorkspaceDocument *originDocument =
        workspaceDocumentHost
            ->documentById(
                originDocumentId
                );

    QWidget *dialogParent =
        originDocument != nullptr
            ? originDocument->window()
            : this;

    InvestigationReportExportDialog dialog(
        context.sessionSelections,
        context.comparisonSelections,
        context.origin,
        context.suggestedTitle,
        dialogParent
        );

    if (
        dialog.exec()
        != QDialog::Accepted
        ) {
        return;
    }

    const InvestigationReportConfiguration
        configuration =
        dialog.configuration();

    if (!configuration.hasSelectedDocuments()) {
        return;
    }

    /*
     * -------------------------------------------------
     * Immutable export boundary
     * -------------------------------------------------
     *
     * Capture the report immediately after the user
     * confirms its configuration and BEFORE opening
     * the save-file dialog.
     *
     * Everything after this point operates only on
     * this frozen snapshot. Mutable widgets, filters,
     * annotations, live session state, or comparison
     * documents cannot alter the report while the
     * user is choosing a destination.
     */
    const QDateTime generatedAtUtc =
        QDateTime::currentDateTimeUtc();

    const InvestigationReportSnapshotBuilder
        snapshotBuilder;

    const InvestigationReportSnapshot snapshot =
        snapshotBuilder.build(
            configuration,
            context.sessionInputs,
            context.comparisonInputs,
            generatedAtUtc
            );

    if (
        snapshot.sessions.isEmpty()
        && snapshot.comparisons.isEmpty()
        ) {
        QMessageBox::warning(
            dialogParent,
            tr(
                "Investigation Report Unavailable"
                ),
            tr(
                "The selected investigation documents "
                "are no longer available for export."
                )
            );

        return;
    }

    const QString selectedPath =
        QFileDialog::getSaveFileName(
            dialogParent,
            tr(
                "Export Investigation Report"
                ),
            investigationReportFileName(
                snapshot.title
                ),
            tr(
                "HTML Files (*.html);;"
                "All Files (*)"
                )
            );

    if (selectedPath.isEmpty()) {
        return;
    }

    const QString filePath =
        investigationReportOutputPath(
            selectedPath
            );

    const InvestigationReportHtmlExporter
        exporter;

    if (!exporter.exportToFile(
            snapshot,
            filePath
            )) {
        QMessageBox::warning(
            dialogParent,
            tr(
                "Investigation Report Export Failed"
                ),
            tr(
                "TraceScope could not write the "
                "investigation report to:\n\n%1"
                )
                .arg(filePath)
            );

        return;
    }

    QMessageBox::information(
        dialogParent,
        tr(
            "Investigation Report Exported"
            ),
        tr(
            "Exported the investigation report to:"
            "\n\n%1"
            )
            .arg(filePath)
        );
}

std::optional<SourceFamilyConfiguration>
MainWindow::resolveSourceFamilyConfiguration(
    const QString &activeFilePath,
    SourceFamilyConfiguration configuration
    )
{
    configuration.rotatedSourcePaths.clear();

    if (!configuration
             .includeRotatedSources) {
        return configuration;
    }

    const RotatedSourceDiscoveryResult
        discovery =
        RotatedSourceDiscoveryService::discover(
            activeFilePath,
            configuration.rotationRule
            );

    if (!discovery.succeeded) {
        QMessageBox::warning(
            this,
            tr("Source Family Discovery Failed"),
            tr(
                "TraceScope could not discover the "
                "rotated files configured for this "
                "source.\n\n%1"
                )
                .arg(
                    discovery.errorMessage
                    )
            );

        return std::nullopt;
    }

    if (discovery.rotatedSources.isEmpty()) {
        QMessageBox::warning(
            this,
            tr("Rotated Sources Not Found"),
            tr(
                "This import is configured to include "
                "rotated files, but no rotated files "
                "currently match the selected source "
                "family rule."
                )
            );

        return std::nullopt;
    }

    for (const RotatedSourceMatch &match
         : discovery.rotatedSources) {
        configuration
            .rotatedSourcePaths
            .append(
                QFileInfo(
                    match.filePath
                    )
                    .absoluteFilePath()
                );
    }

    return configuration;
}

void MainWindow::
    preserveSessionAsSnapshotOnly(
        const QString &sessionId
        )
{
    if (workspace == nullptr
        || importWatcher != nullptr
        || workspaceOpenInProgress
        || sessionReloadInProgress) {
        return;
    }

    const int sessionIndex =
        workspace->indexOfSession(
            sessionId
            );

    if (sessionIndex < 0) {
        return;
    }

    InvestigationSession *session =
        workspace->sessionAt(
            sessionIndex
            );

    if (session == nullptr) {
        return;
    }

    const InvestigationSessionBackingMode mode =
        session->backing().mode();

    if (mode
            != InvestigationSessionBackingMode::
            SourceBacked
        && mode
               != InvestigationSessionBackingMode::
               Hybrid) {
        return;
    }

    QString standaloneSnapshotPath;

    /*
     * Without a saved workspace there is no
     * application-owned sidecar directory.
     *
     * The user therefore owns this standalone .tsinv
     * artifact and chooses where it lives.
     */
    if (currentWorkspacePath.isEmpty()) {
        standaloneSnapshotPath =
            QFileDialog::getSaveFileName(
                this,
                tr(
                    "Save Investigation Snapshot"
                    ),
                QStringLiteral(
                    "investigation.tsinv"
                    ),
                tr(
                    "TraceScope Investigation Snapshot "
                    "(*.tsinv);;All Files (*)"
                    )
                );

        if (standaloneSnapshotPath.isEmpty()) {
            return;
        }

        if (QFileInfo(
                standaloneSnapshotPath
                )
                .suffix()
                .compare(
                    QStringLiteral("tsinv"),
                    Qt::CaseInsensitive
                    )
            != 0) {
            standaloneSnapshotPath +=
                QStringLiteral(".tsinv");
        }
    }

    LiveSessionFollowCoordinator
        *liveCoordinator =
        session->liveFollowCoordinator();

    const bool wasFollowing =
        liveCoordinator != nullptr
        && liveCoordinator
               ->state()
               .isFollowing();

    /*
     * Establish a stable capture boundary.
     *
     * A live session must not admit records after the
     * snapshot has been captured but before that
     * snapshot becomes authoritative.
     */
    if (wasFollowing
        && !liveCoordinator->pause()) {
        QMessageBox::warning(
            this,
            tr("Preserve Snapshot Failed"),
            tr(
                "TraceScope could not pause live "
                "following long enough to capture a "
                "stable investigation snapshot."
                )
            );

        return;
    }

    const InvestigationSessionSnapshot snapshot =
        session->captureSnapshot();

    auto resumeAfterFailure =
        [
            liveCoordinator,
            wasFollowing
    ]() {
            if (wasFollowing
                && liveCoordinator != nullptr) {
                liveCoordinator->resume();
            }
        };

    QString committedSnapshotPath;

    if (!currentWorkspacePath.isEmpty()) {
        /*
         * The workspace package owns this snapshot.
         *
         * The staged manifest is written as
         * SnapshotBacked before runtime state changes.
         */
        if (!saveWorkspaceToFile(
                currentWorkspacePath,
                sessionId,
                &committedSnapshotPath
                )) {
            resumeAfterFailure();
            return;
        }

        if (committedSnapshotPath.isEmpty()) {
            resumeAfterFailure();

            QMessageBox::warning(
                this,
                tr("Preserve Snapshot Failed"),
                tr(
                    "The workspace was saved, but "
                    "TraceScope could not identify the "
                    "new investigation snapshot."
                    )
                );

            return;
        }
    } else {
        /*
         * Standalone snapshot ownership belongs to the
         * user. TraceScope must never garbage-collect
         * this path merely because a future workspace
         * creates its own managed copy.
         */
        const InvestigationSessionSnapshotSaveResult
            saveResult =
            InvestigationSessionSnapshotFile()
                .save(
                    standaloneSnapshotPath,
                    snapshot
                    );

        if (!saveResult.isSuccess()) {
            resumeAfterFailure();

            QMessageBox::warning(
                this,
                tr("Save Investigation Snapshot Failed"),
                saveResult.errorMessage.isEmpty()
                    ? tr(
                          "TraceScope could not save "
                          "the investigation snapshot."
                          )
                    : saveResult.errorMessage
                );

            return;
        }

        committedSnapshotPath =
            QFileInfo(
                standaloneSnapshotPath
                ).absoluteFilePath();
    }

    /*
     * Persistence has succeeded.
     *
     * This runtime mutation performs no I/O and is the
     * final commit boundary for the open investigation.
     */
    if (!workspace
             ->applySnapshotOnlyTransition(
                 sessionId,
                 committedSnapshotPath,
                 snapshot.sourceFidelity
                 )) {
        resumeAfterFailure();

        QMessageBox::critical(
            this,
            tr("Preserve Snapshot Failed"),
            tr(
                "The snapshot was saved successfully, "
                "but the open investigation could not "
                "be switched to Snapshot-backed mode."
                )
            );

        return;
    }

    /*
     * Do not resume live following after success.
     *
     * SnapshotBacked intentionally has no active
     * external-source dependency.
     */
    updateReloadActionState();
}