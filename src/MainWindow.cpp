#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
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
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "exporting/InvestigationReportHtmlExporter.h"
#include "exporting/InvestigationReportSnapshotBuilder.h"
#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "importing/SourceFamilyImportService.h"
#include "live/LiveSessionFollowCoordinator.h"
#include "sources/RotatedSourceDiscoveryService.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/InvestigationComparisonDialog.h"
#include "ui/InvestigationReportExportDialog.h"
#include "ui/workspace/InvestigationComparisonDocument.h"
#include "ui/workspace/InvestigationReportWorkspaceContext.h"
#include "ui/workspace/InvestigationSessionView.h"
#include "ui/workspace/WorkspaceDocument.h"
#include "ui/workspace/WorkspaceDocumentHost.h"
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
    if (importWatcher != nullptr
        || workspaceOpenInProgress) {
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
        || workspaceOpenInProgress) {
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

        workspace->addSession(
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
        || workspaceOpenInProgress) {
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
        || workspaceOpenInProgress) {
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
        session->id(),
        session
            ->sourceFamilyConfiguration()
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

void MainWindow::setWorkspaceOpenInProgress(
    bool inProgress
    )
{
    workspaceOpenInProgress =
        inProgress;

    const bool fileOperationAvailable =
        !workspaceOpenInProgress
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

    setAcceptDrops(
        fileOperationAvailable
        );

    updateReloadActionState();
}

void MainWindow::
    updateReloadActionState()
{
    if (reloadAction == nullptr) {
        return;
    }

    InvestigationSession *session =
        workspace != nullptr
            ? workspace->activeSession()
            : nullptr;

    bool liveFollowActive =
        false;

    if (session != nullptr) {
        const LiveSessionFollowCoordinator
            *coordinator =
            session
                ->liveFollowCoordinator();

        liveFollowActive =
            coordinator != nullptr
            && !coordinator
                    ->state()
                    .isStopped();
    }

    reloadAction->setEnabled(
        session != nullptr
        && importWatcher == nullptr
        && !workspaceOpenInProgress
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
    const QString &filePath
    )
{
    WorkspacePersistenceState state =
        captureWorkspaceState();

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

            session->updateSnapshotBackingPath(
                snapshotPathIterator.value()
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
                .externalSourceBinding
                .reset();

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
        || workspaceOpenInProgress) {
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