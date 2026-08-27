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

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/InvestigationComparisonDialog.h"
#include "ui/workspace/InvestigationComparisonDocument.h"
#include "ui/workspace/InvestigationSessionView.h"
#include "ui/workspace/WorkspaceDocumentHost.h"
#include "workspace/InvestigationComparisonSnapshotBuilder.h"

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

    connect(openAction, &QAction::triggered, this, [this]() {
        openLogFile();
    });

    fileMenu->addAction(openAction);

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

    auto *exportAction = new QAction("&Export Filtered Results...", this);
    exportAction->setShortcut(QKeySequence::Save);

    connect(exportAction, &QAction::triggered, this, [this]() {
        exportFilteredResults();
    });

    fileMenu->addSeparator();
    fileMenu->addAction(exportAction);

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
    if (importWatcher != nullptr) {
        return;
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

        return;
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
        [progressDialog, displayFileName](
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
            filePath,
            profile,
            reloadSessionId
        ]() {
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

            setAcceptDrops(true);

            if (cancelled) {
                watcher->deleteLater();
                return;
            }

            ImportResult result =
                watcher->result();

            watcher->deleteLater();

            if (reloadAction != nullptr) {
                reloadAction->setEnabled(
                    workspace->activeSession()
                    != nullptr
                    );
            }

            completeLogFileImport(
                filePath,
                profile,
                std::move(result),
                reloadSessionId
                );
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
                        if (progress.totalBytes <= 0) {
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

void MainWindow::exportFilteredResults()
{
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        QMessageBox::information(
            this,
            "No Records to Export",
            "There are no currently visible records to export."
            );

        return;
    }

    InvestigationController *controller =
        session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const QVector<InvestigationRecord> records =
        controller->visibleRecords();

    if (records.isEmpty()) {
        QMessageBox::information(
            this,
            "No Records to Export",
            "There are no currently visible records to export."
            );

        return;
    }

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Export Filtered Records",
            "filtered-investigation-records.csv",
            "CSV Files (*.csv);;All Files (*)"
            );

    if (filePath.isEmpty()) {
        return;
    }

    const bool exported =
        csvExporter.exportToFile(
            records,
            filePath
            );

    if (!exported) {
        QMessageBox::warning(
            this,
            "Export Failed",
            "TraceScope could not export the filtered records."
            );

        return;
    }

    QMessageBox::information(
        this,
        "Export Complete",
        QString(
            "Exported %1 records."
            ).arg(records.size())
        );
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