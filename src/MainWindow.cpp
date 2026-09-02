#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QPlainTextEdit>
#include <QStringList>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QProgressDialog>
#include <QPromise>
#include <QSignalBlocker>
#include <QVariant>
#include <QDialog>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QSaveFile>
#include <QPushButton>
#include <QFile>
#include <QCloseEvent>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/InvestigationComparisonDialog.h"
#include "ui/workspace/InvestigationComparisonDocument.h"
#include "ui/workspace/InvestigationSessionView.h"
#include "ui/workspace/WorkspaceDocument.h"
#include "ui/workspace/WorkspaceDocumentHost.h"
#include "workspace/InvestigationComparisonPersistence.h"
#include "workspace/InvestigationComparisonSnapshotBuilder.h"
#include "workspace/InvestigationSessionPersistence.h"
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

            if (session == nullptr
                || workspaceDocumentHost
                       == nullptr) {
                return;
            }

            auto *sessionView =
                new InvestigationSessionView(
                    session,
                    &filterPresetStore
                    );

            /*
             * InvestigationWorkspace::addSession()
             * emits sessionAdded before it activates
             * the new session. Avoid letting QTabWidget
             * selection get ahead of workspace state.
             */
            const QSignalBlocker blocker(
                workspaceDocumentHost
                );

            if (!workspaceDocumentHost
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

            if (reloadAction != nullptr) {
                reloadAction->setEnabled(
                    session != nullptr
                    && importWatcher == nullptr
                    );
            }
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
                || workspace->sessionCount()
                       < 2
                || workspace->indexOfSession(
                       documentId
                       )
                       < 0) {
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
                     * The user is examining the
                     * active session and explicitly
                     * clicked another session:
                     *
                     * clicked  -> Baseline
                     * active   -> Comparison
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

void MainWindow::openLogFile(const QString &initialFilePath)
{
    if (importWatcher != nullptr) {
        QMessageBox::information(
            this,
            tr("Import In Progress"),
            tr(
                "A log file is already being imported. "
                "Wait for the current import to finish "
                "before starting another import."
                )
            );

        return;
    }

    ImportConfigurationDialog dialog(
        this,
        &recentItemsStore
        );

    if (!initialFilePath.isEmpty()) {
        dialog.setSelectedFilePath(
            initialFilePath
            );
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    loadLogFile(
        dialog.selectedFilePath(),
        dialog.configuredProfile()
        );
}

void MainWindow::loadLogFile(
    const QString &filePath,
    const ImportProfile &profile,
    const QString &reloadSessionId
    )
{
    startLogFileImport(
        filePath,
        profile,
        [
            this,
            filePath,
            profile,
            reloadSessionId
    ](
            std::optional<ImportResult> result
            ) {
            if (!result.has_value()) {
                return;
            }

            completeLogFileImport(
                filePath,
                profile,
                std::move(
                    result.value()
                    ),
                reloadSessionId
                );
        }
        );
}

bool MainWindow::startLogFileImport(
    const QString &filePath,
    const ImportProfile &profile,
    ImportCompletionHandler completion
    )
{
    if (importWatcher != nullptr) {
        return false;
    }

    if (reloadAction != nullptr) {
        reloadAction->setEnabled(false);
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

    const QString displayFileName =
        QFileInfo(filePath)
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

            if (openWorkspaceAction != nullptr) {
                openWorkspaceAction->setEnabled(true);
            }

            setAcceptDrops(true);

            /*
             * Restore normal session-reload
             * availability regardless of whether the
             * import completed or was cancelled.
             */
            if (reloadAction != nullptr) {
                reloadAction->setEnabled(
                    workspace != nullptr
                    && workspace->activeSession()
                           != nullptr
                    );
            }

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
                filePath
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

                ImportResult result =
                    importer->importFile(
                        filePath,
                        ILogImporter::
                        UnlimitedRecordLimit,
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
                std::move(result)
                );

        workspace->addSession(
            std::move(session)
            );
    } else {
        workspace->reloadSession(
            reloadSessionId,
            std::move(result)
            );
    }

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
    if (importWatcher != nullptr) {
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
    if (importWatcher != nullptr) {
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
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    loadLogFile(
        session
            ->sourceMetadata()
            .sourcePath,
        session->importProfile(),
        session->id()
        );
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
    const QString &filePath
    )
{
    const WorkspacePersistenceState state =
        captureWorkspaceState();

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(
            state
            );

    QSaveFile file(
        filePath
        );

    if (!file.open(
            QIODevice::WriteOnly
            )) {
        QMessageBox::warning(
            this,
            tr("Save Workspace Failed"),
            tr(
                "TraceScope could not open the "
                "selected workspace file for writing."
                )
            );

        return false;
    }

    if (file.write(json)
            != json.size()
        || !file.commit()) {
        file.cancelWriting();

        QMessageBox::warning(
            this,
            tr("Save Workspace Failed"),
            tr(
                "TraceScope could not save the "
                "workspace successfully."
                )
            );

        return false;
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
                "tracescope-workspace.json"
                );
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            tr("Save TraceScope Workspace"),
            initialPath,
            tr(
                "TraceScope Workspace (*.json);;"
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

bool MainWindow::resolveWorkspaceSourcePaths(
    WorkspacePersistenceState &state
    )
{
    QVector<PersistedInvestigationSession>
        recoverableSessions;

    recoverableSessions.reserve(
        state.sessions.size()
        );

    for (PersistedInvestigationSession
             persistedSession
         : std::as_const(state.sessions)) {
        const QFileInfo sourceInfo(
            persistedSession.sourcePath
            );

        if (sourceInfo.exists()
            && sourceInfo.isFile()) {
            recoverableSessions.append(
                std::move(
                    persistedSession
                    )
                );

            continue;
        }

        bool resolved =
            false;

        bool skipped =
            false;

        while (!resolved
               && !skipped) {
            QMessageBox prompt(
                this
                );

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
                    "A source file used by this "
                    "workspace could not be found."
                    )
                );

            prompt.setInformativeText(
                tr(
                    "Saved location:\n%1\n\n"
                    "Locate the file to restore this "
                    "session, or skip the session and "
                    "continue opening the rest of the "
                    "workspace."
                    )
                    .arg(
                        persistedSession
                            .sourcePath
                        )
                );

            QPushButton *locateButton =
                prompt.addButton(
                    tr("Locate File..."),
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
                locateButton
                );

            prompt.exec();

            if (prompt.clickedButton()
                == cancelButton) {
                return false;
            }

            if (prompt.clickedButton()
                == skipButton) {
                skipped =
                    true;

                continue;
            }

            if (prompt.clickedButton()
                != locateButton) {
                continue;
            }

            const QFileInfo missingInfo(
                persistedSession.sourcePath
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
             * Cancelling the picker returns to the
             * recovery prompt instead of silently
             * treating the source as skipped.
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
                    tr(
                        "Source File Not Found"
                        ),
                    tr(
                        "The selected source file "
                        "could not be opened. Choose "
                        "another file or skip this "
                        "session."
                        )
                    );

                continue;
            }

            persistedSession.sourcePath =
                replacementInfo
                    .absoluteFilePath();

            recoverableSessions.append(
                std::move(
                    persistedSession
                    )
                );

            resolved =
                true;
        }
    }

    state.sessions =
        std::move(
            recoverableSessions
            );

    return true;
}

void MainWindow::openWorkspace(const QString &initialFilePath)
{
    if (importWatcher != nullptr) {
        QMessageBox::information(
            this,
            tr("Import In Progress"),
            tr(
                "A log file is already being imported. "
                "Wait for the current import to finish "
                "before opening a workspace."
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
                    "TraceScope Workspace (*.json);;"
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

    const int originalSessionCount =
        state.sessions.size();

    /*
     * Missing paths are resolved before any import
     * begins. Cancelling recovery leaves the current
     * workspace completely untouched.
     */
    if (!resolveWorkspaceSourcePaths(
            state
            )) {
        return;
    }

    auto operation =
        std::make_shared<
            WorkspaceOpenOperation
            >();

    operation->workspacePath =
        QFileInfo(filePath)
            .absoluteFilePath();

    operation->skippedSessionCount =
        originalSessionCount
        - state.sessions.size();

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
        return;
    }

    if (operation->nextSessionIndex
        >= operation->state
               .sessions
               .size()) {
        installOpenedWorkspace(
            operation
            );

        return;
    }

    const int sessionIndex =
        operation->nextSessionIndex;

    const PersistedInvestigationSession
        &persistedSession =
        operation->state
            .sessions
            .at(
                sessionIndex
                );

    const bool started =
        startLogFileImport(
            persistedSession.sourcePath,
            persistedSession.importProfile,
            [
                this,
                operation,
                sessionIndex
            ](
                std::optional<ImportResult>
                    result
                ) {
                /*
                 * A cancelled import aborts the open
                 * operation. Nothing live has been
                 * replaced yet.
                 */
                if (!result.has_value()
                    || result->cancelled) {
                    return;
                }

                const PersistedInvestigationSession
                    &persistedSession =
                    operation->state
                        .sessions
                        .at(
                            sessionIndex
                            );

                if (result->records.isEmpty()) {
                    ++operation
                          ->emptySessionCount;
                }

                auto session =
                    std::make_unique<
                        InvestigationSession
                        >(
                        persistedSession
                            .sessionId,
                        persistedSession
                            .sourcePath,
                        persistedSession
                            .importProfile,
                        std::move(
                            result.value()
                            )
                        );

                /*
                 * Restore bookmarks, notes, findings,
                 * filters, selected record, and other
                 * session/domain persistence before
                 * the session ever enters the live
                 * workspace.
                 */
                InvestigationSessionPersistence::
                    restoreState(
                        persistedSession,
                        *session
                        );

                operation->stagedSessions
                    .push_back(
                        std::move(
                            session
                            )
                        );

                ++operation
                      ->nextSessionIndex;

                continueWorkspaceOpen(
                    operation
                    );
            }
            );

    /*
     * An unsupported importer, or another inability
     * to start the import, aborts restoration while
     * preserving the currently open workspace.
     *
     * startLogFileImport() already presents the
     * appropriate error message.
     */
    if (!started) {
        return;
    }
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

        workspace->addSession(
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
                    "%1 unavailable source session(s) "
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
                    "%1 restored source session(s) "
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