#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QScrollBar>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableView>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QWidget>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QStringList>
#include <QSizePolicy>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QProgressDialog>
#include <QPromise>
#include <QSignalBlocker>
#include <QMargins>
#include <QGraphicsLayout>
#include <QFrame>
#include <QVariant>
#include <QTabBar>
#include <QPushButton>
#include <QDialog>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QDialogButtonBox>
#include <QTextOption>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QColor>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/investigation/InvestigationAnalyticsPanel.h"
#include "ui/investigation/InvestigationEventDetailPanel.h"
#include "ui/investigation/InvestigationEventPanel.h"
#include "ui/investigation/InvestigationFilterPanel.h"
#include "ui/investigation/InvestigationFindingsPanel.h"
#include "ui/investigation/InvestigationIssueSummaryPanel.h"
#include "ui/investigation/InvestigationReviewPanel.h"
#include "ui/investigation/InvestigationSessionSummaryPanel.h"
#include "ui/investigation/InvestigationTimelinePanel.h"
#include "ui/workspace/InvestigationSessionView.h"
#include "ui/workspace/WorkspaceDocumentHost.h"

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
    workspace(new InvestigationWorkspace(this)),
    investigationController(nullptr)
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
            InvestigationSession *session =
                workspace->sessionAt(index);

            if (session == nullptr
                || workspaceDocumentHost
                       == nullptr) {
                return;
            }

            auto *sessionView =
                new InvestigationSessionView(
                    session
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
            if (workspaceDocumentHost
                == nullptr) {
                return;
            }

            /*
             * Tabs are movable, so document position
             * must never be assumed to match workspace
             * session index. Stable document/session IDs
             * are the source of truth.
             */
            const QSignalBlocker blocker(
                workspaceDocumentHost
                );

            for (int documentIndex =
                     workspaceDocumentHost
                         ->documentCount()
                     - 1;
                 documentIndex >= 0;
                 --documentIndex) {
                auto *sessionView =
                    qobject_cast<
                        InvestigationSessionView *>(
                        workspaceDocumentHost
                            ->documentAt(
                                documentIndex
                                )
                        );

                if (sessionView == nullptr) {
                    continue;
                }

                if (workspace->indexOfSession(
                        sessionView
                            ->documentId()
                        )
                    >= 0) {
                    continue;
                }

                /*
                 * Never destroy the shared surface
                 * along with a closing session view.
                 */
                if (surfaceSessionView
                    == sessionView) {
                    sessionView->takeContent();

                    surfaceSessionView =
                        nullptr;
                }

                WorkspaceDocument *removed =
                    workspaceDocumentHost
                        ->removeDocument(
                            sessionView
                                ->documentId()
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
            /*
             * Release the shared surface from whichever
             * session document currently owns it.
             */
            if (surfaceSessionView
                != nullptr) {
                surfaceSessionView
                    ->takeContent();

                surfaceSessionView =
                    nullptr;
            }

            InvestigationSession *session =
                workspace->sessionAt(
                    index
                    );

            if (session != nullptr
                && workspaceDocumentHost
                       != nullptr) {
                {
                    /*
                     * Keep document selection synchronized
                     * without feeding the change back into
                     * InvestigationWorkspace.
                     */
                    const QSignalBlocker blocker(
                        workspaceDocumentHost
                        );

                    workspaceDocumentHost
                        ->setCurrentDocument(
                            session->id()
                            );
                }

                const int documentIndex =
                    workspaceDocumentHost
                        ->indexOfDocument(
                            session->id()
                            );

                auto *sessionView =
                    qobject_cast<
                        InvestigationSessionView *>(
                        workspaceDocumentHost
                            ->documentAt(
                                documentIndex
                                )
                        );

                if (sessionView != nullptr
                    && sessionView->attachContent(
                        investigationSurface
                        )) {
                    surfaceSessionView =
                        sessionView;
                }
            }

            bindActiveSession();
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::sessionReloaded,
        this,
        [this](int index) {
            if (index
                == workspace
                       ->activeSessionIndex()) {
                bindActiveSession();
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

            if (sessionIndex < 0) {
                return;
            }

            workspace->closeSession(
                sessionIndex
                );
        }
        );

    bindActiveSession();
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

    /*
     * During the incremental session-view
     * migration, the existing investigation UI
     * remains one shared surface. The active
     * InvestigationSessionView temporarily owns
     * this widget.
     */
    investigationSurface =
        new QWidget(this);

    auto *layout =
        new QVBoxLayout(
            investigationSurface
            );

    investigationSurface->hide();

    filterPanel =
        new InvestigationFilterPanel(
            &filterPresetStore,
            this
            );

    layout->addWidget(
        filterPanel
        );

    connect(
        filterPanel,
        &InvestigationFilterPanel::
        filterChangeRequested,
        this,
        &MainWindow::applyFilters
        );

    timelinePanel =
        new InvestigationTimelinePanel(
            this
            );

    connect(
        timelinePanel,
        &InvestigationTimelinePanel::
        bucketDrillDownRequested,
        this,
        &MainWindow::
        applyTimelineDrillDown
        );

    /*
     * ---------------------------------------------------------
     * Event list
     * ---------------------------------------------------------
     */
    eventPanel =
        new InvestigationEventPanel(
            this
            );

    connect(
        eventPanel,
        &InvestigationEventPanel::
        selectedRecordChanged,
        this,
        &MainWindow::
        updateEventDetailFromSelection
        );

    connect(
        eventPanel,
        &InvestigationEventPanel::
        customFieldFilterRequested,
        this,
        [this](
            const QString &fieldName,
            const QString &value
            ) {
            if (
                filterPanel != nullptr
                && filterPanel
                       ->addCustomFieldFilter(
                           fieldName,
                           value
                           )
                ) {
                applyFilters();
            }
        }
        );

    /*
     * ---------------------------------------------------------
     * Investigation review surface
     * ---------------------------------------------------------
     *
     * The review container now owns the Issue
     * Summary / Findings / Analytics tab structure
     * and per-session selected-tab state.
     *
     * MainWindow retains only the cross-component
     * orchestration that reaches outside that
     * container.
     */
    reviewPanel =
        new InvestigationReviewPanel(
            this
            );

    issueSummaryPanel =
        reviewPanel->issueSummaryPanel();

    findingsPanel =
        reviewPanel->findingsPanel();

    analyticsPanel =
        reviewPanel->analyticsPanel();

    connect(
        issueSummaryPanel,
        &InvestigationIssueSummaryPanel::
        drillDownRequested,
        this,
        &MainWindow::
        drillDownIssueSummary
        );

    connect(
        findingsPanel,
        &InvestigationFindingsPanel::
        findingActivated,
        this,
        &MainWindow::
        navigateToFinding
        );

    connect(
        analyticsPanel,
        &InvestigationAnalyticsPanel::
        burstDrillDownRequested,
        this,
        &MainWindow::
        drillDownBurst
        );

    /*
     * ---------------------------------------------------------
     * Selected event detail
     * ---------------------------------------------------------
     */
    eventDetailPanel =
        new InvestigationEventDetailPanel(
            this
            );

    connect(
        eventDetailPanel,
        &InvestigationEventDetailPanel::
        findingStatusChangeRequested,
        this,
        &MainWindow::
        updateSelectedEventFindingStatus
        );

    connect(
        eventDetailPanel,
        &InvestigationEventDetailPanel::
        noteEditRequested,
        this,
        &MainWindow::
        editSelectedEventNote
        );

    connect(
        eventDetailPanel,
        &InvestigationEventDetailPanel::
        bookmarkToggleRequested,
        this,
        &MainWindow::
        toggleSelectedEventBookmark
        );

    /*
     * ---------------------------------------------------------
     * Review/detail horizontal splitter
     * ---------------------------------------------------------
     */
    auto *bottomSplitter =
        new QSplitter(
            Qt::Horizontal,
            this
            );

    bottomSplitter->addWidget(
        reviewPanel
        );

    bottomSplitter->addWidget(
        eventDetailPanel
        );

    bottomSplitter->setStretchFactor(
        0,
        0
        );

    bottomSplitter->setStretchFactor(
        1,
        1
        );

    bottomSplitter->setCollapsible(
        0,
        false
        );

    bottomSplitter->setCollapsible(
        1,
        false
        );

    bottomSplitter->setSizes({
        issueSummaryPanel
            ->preferredCompactWidth(),
        1000
    });

    /*
     * Review-tab selection affects only the
     * surrounding layout proportions.
     *
     * Review-tab ownership and persistence now
     * belong to InvestigationReviewPanel.
     */
    connect(
        reviewPanel,
        &InvestigationReviewPanel::
        currentTabChanged,
        this,
        [
            this,
            bottomSplitter
    ](
            InvestigationReviewTab tab
            ) {
            const int totalWidth =
                std::max(
                    1,
                    bottomSplitter->width()
                    );

            const bool wideReviewSelected =
                tab
                    == InvestigationReviewTab::
                    Findings
                || tab
                       == InvestigationReviewTab::
                       Analytics;

            if (wideReviewSelected) {
                /*
                 * Findings and Analytics are genuine
                 * review surfaces and benefit from
                 * more horizontal space. Selected
                 * Event Details remains usable at
                 * roughly forty percent.
                 */
                const int reviewWidth =
                    static_cast<int>(
                        totalWidth * 0.60
                        );

                bottomSplitter->setSizes({
                    reviewWidth,
                    std::max(
                        1,
                        totalWidth - reviewWidth
                        )
                });

                return;
            }

            /*
             * Issue Summary is naturally compact,
             * so return most of the horizontal
             * space to Selected Event Details.
             */
            const int issueWidth =
                std::max(
                    issueSummaryPanel
                        ->preferredCompactWidth(),
                    static_cast<int>(
                        totalWidth * 0.35
                        )
                    );

            bottomSplitter->setSizes({
                issueWidth,
                std::max(
                    1,
                    totalWidth - issueWidth
                    )
            });
        }
        );

    /*
     * ---------------------------------------------------------
     * Primary investigation layout
     * ---------------------------------------------------------
     */
    auto *mainSplitter =
        new QSplitter(
            Qt::Vertical,
            this
            );

    mainSplitter->addWidget(
        timelinePanel
        );

    mainSplitter->addWidget(
        eventPanel
        );

    mainSplitter->addWidget(
        bottomSplitter
        );

    mainSplitter->setStretchFactor(
        0,
        2
        );

    mainSplitter->setStretchFactor(
        1,
        5
        );

    mainSplitter->setStretchFactor(
        2,
        2
        );

    mainSplitter->setCollapsible(
        0,
        false
        );

    mainSplitter->setCollapsible(
        1,
        false
        );

    mainSplitter->setCollapsible(
        2,
        false
        );

    mainSplitter->setSizes(
        QList<int>{
            220,
            350,
            250
        }
        );

    layout->addWidget(
        mainSplitter,
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

void MainWindow::updateSummary(
    const QVector<InvestigationRecord> &records,
    const QString &
    )
{
    if (surfaceSessionView == nullptr
        || surfaceSessionView
               ->summaryPanel()
               == nullptr) {
        return;
    }

    surfaceSessionView
        ->summaryPanel()
        ->refresh(
            records
            );
}

void MainWindow::applyFilters()
{
    if (investigationController == nullptr
        || filterPanel == nullptr) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    const QString selectedRecordId =
        session->selectedRecordId();

    /*
     * The filter component owns the editable
     * filter state; MainWindow owns the coordinated
     * consequences of changing that state.
     */
    filterPanel->applyToSession();

    const int selectedProxyRow =
        !selectedRecordId.isEmpty()
            ? investigationController
                  ->proxyRowForRecordId(
                      selectedRecordId
                      )
            : -1;

    const QVector<InvestigationRecord>
        visibleRecords =
        investigationController
            ->recordsForAnalysis();

    updateSummary(
        visibleRecords,
        currentFilePath
        );

    updateIssueSummary(
        visibleRecords
        );

    if (analyticsPanel != nullptr) {
        analyticsPanel->updateRecords(
            visibleRecords
            );
    }

    if (timelinePanel != nullptr) {
        timelinePanel->updateRecords(
            visibleRecords
            );
    }

    if (eventPanel != nullptr) {
        if (selectedProxyRow >= 0) {
            eventPanel->selectProxyRow(
                selectedProxyRow
                );
        } else {
            eventPanel->clearSelection();

            clearEventDetail();
        }

        eventPanel
            ->refreshNavigationState();
    }
}

void MainWindow::updateEventDetailFromSelection()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (record == nullptr) {
        clearEventDetail();
        return;
    }

    if (eventDetailPanel != nullptr) {
        eventDetailPanel->displayRecord(
            *record
            );
    }

    updateInvestigationStateControls();
}

void MainWindow::clearEventDetail()
{
    if (eventDetailPanel != nullptr) {
        eventDetailPanel->clearRecord();
    }

    updateInvestigationStateControls();
}

const InvestigationRecord *
MainWindow::selectedEventRecord() const
{
    return eventPanel != nullptr
        ? eventPanel->selectedRecord()
        : nullptr;
}

void MainWindow::
    updateInvestigationStateControls()
{
    if (eventDetailPanel == nullptr) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    const InvestigationRecord *record =
        selectedEventRecord();

    if (session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()) {
        eventDetailPanel
            ->clearInvestigationState();

        return;
    }

    const InvestigationRecordState state =
        session
            ->investigationStateStore()
            ->stateForRecord(
                record->recordId
                );

    eventDetailPanel
        ->setInvestigationState(
            state
            );
}

void MainWindow::
    updateSelectedEventFindingStatus()
{
    InvestigationSession *session =
        workspace->activeSession();

    const InvestigationRecord *record =
        selectedEventRecord();

    if (session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()
        || eventDetailPanel == nullptr) {
        return;
    }

    const FindingStatus status =
        eventDetailPanel
            ->selectedFindingStatus();

    session
        ->investigationStateStore()
        ->setFindingStatus(
            record->recordId,
            status
            );

    syncInvestigationStatePresentation();

    updateFindingsPanel();

    if (
        filterPanel != nullptr
        && filterPanel
               ->hasFindingStatusFilter()
        ) {
        applyFilters();
    } else {
        updateInvestigationStateControls();
    }
}

void MainWindow::editSelectedEventNote()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    InvestigationSession *session =
        workspace->activeSession();

    if (record == nullptr
        || session == nullptr) {
        return;
    }

    const QString recordId =
        record->recordId;

    const QString sessionId =
        session->id();

    const InvestigationRecordState state =
        session
            ->investigationStateStore()
            ->stateForRecord(
                recordId
                );

    auto *dialog =
        new QDialog(this);

    dialog->setAttribute(
        Qt::WA_DeleteOnClose
        );

    dialog->setWindowModality(
        Qt::NonModal
        );

    dialog->setModal(
        false
        );

    dialog->setWindowTitle(
        tr("Analyst Note")
        );

    dialog->resize(
        520,
        300
        );

    auto *layout =
        new QVBoxLayout(dialog);

    /*
     * Make it clear which record this modeless
     * editor belongs to, since the analyst is free
     * to select other records while it is open.
     */
    QString recordDescription =
        tr("Source record #%1")
            .arg(
                record->source.recordNumber
                );

    if (record->eventCode.has_value()) {
        recordDescription +=
            tr(" — %1")
                .arg(
                    record->eventCode.value()
                    );
    }

    auto *recordLabel =
        new QLabel(
            recordDescription,
            dialog
            );

    layout->addWidget(
        recordLabel
        );

    auto *noteEdit =
        new QPlainTextEdit(
            dialog
            );

    noteEdit->setPlainText(
        state.note
        );

    /*
     * Wrap only the visual presentation.
     *
     * QPlainTextEdit does not insert newline
     * characters when a line wraps, so resizing
     * the editor simply reflows the text.
     */
    noteEdit->setLineWrapMode(
        QPlainTextEdit::WidgetWidth
        );

    noteEdit->setWordWrapMode(
        QTextOption::WrapAtWordBoundaryOrAnywhere
        );

    noteEdit->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    layout->addWidget(
        noteEdit,
        1
        );

    auto *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Save
                | QDialogButtonBox::Cancel,
            dialog
            );

    layout->addWidget(
        buttonBox
        );

    connect(
        buttonBox,
        &QDialogButtonBox::rejected,
        dialog,
        &QDialog::close
        );

    connect(
        buttonBox,
        &QDialogButtonBox::accepted,
        this,
        [
            this,
            dialog,
            noteEdit,
            sessionId,
            recordId
    ]() {
            const int sessionIndex =
                workspace->indexOfSession(
                    sessionId
                    );

            if (sessionIndex < 0) {
                /*
                 * The source investigation was
                 * closed while this modeless editor
                 * was open.
                 */
                dialog->close();

                return;
            }

            InvestigationSession *targetSession =
                workspace->sessionAt(
                    sessionIndex
                    );

            if (targetSession == nullptr) {
                dialog->close();

                return;
            }

            const QString note =
                noteEdit->toPlainText();

            targetSession
                ->investigationStateStore()
                ->setNote(
                    recordId,
                    note.trimmed().isEmpty()
                        ? QString()
                        : note
                    );

            /*
             * Only the active session can currently
             * affect the visible investigation UI.
             */
            if (
                workspace->activeSession()
                == targetSession
                ) {
                updateInvestigationStateControls();
                updateFindingsPanel();
            }

            dialog->close();
        }
        );

    dialog->show();

    noteEdit->setFocus();
}

void MainWindow::
    syncInvestigationStatePresentation()
{
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr
        || investigationController
               == nullptr) {
        return;
    }

    updateFindingsPanel();

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    investigationController
        ->proxyModel()
        ->setInvestigationStateIndicators(
            stateStore
                ->bookmarkedRecordIds(),
            stateStore
                ->notedRecordIds(),
            stateStore
                ->findingStatuses()
            );
}

void MainWindow::toggleSelectedEventBookmark()
{
    InvestigationSession *session =
        workspace->activeSession();

    const InvestigationRecord *record =
        selectedEventRecord();

    if (session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()) {
        return;
    }

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    const bool currentlyBookmarked =
        stateStore
            ->stateForRecord(
                record->recordId
                )
            .bookmarked;

    stateStore->setBookmarked(
        record->recordId,
        !currentlyBookmarked
        );

    syncInvestigationStatePresentation();

    if (
        filterPanel != nullptr
        && filterPanel->bookmarksOnly()
        ) {
        applyFilters();
    } else {
        updateInvestigationStateControls();
    }
}

void MainWindow::selectProxyRow(
    int proxyRow
    )
{
    if (eventPanel == nullptr) {
        return;
    }

    eventPanel->selectProxyRow(
        proxyRow
        );
}

void MainWindow::updateIssueSummary(
    const QVector<InvestigationRecord> &records
    )
{
    if (issueSummaryPanel == nullptr) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (
        session == nullptr
        || !session->hasSeverityData()
        || !session->hasSubsystemData()
        ) {
        issueSummaryPanel->clear();

        return;
    }

    issueSummaryPanel->updateRecords(
        records
        );
}

void MainWindow::updateFindingsPanel()
{
    if (findingsPanel == nullptr) {
        return;
    }

    findingsPanel->refresh();
}

void MainWindow::drillDownIssueSummary(
    const QString &subsystem,
    InvestigationIssueDrillDownType type
    )
{
    if (investigationController == nullptr
        || subsystem.isEmpty()) {
        return;
    }

    QStringList targetSeverities;

    switch (type) {
    case InvestigationIssueDrillDownType::
        Warnings:
        targetSeverities = {
            QStringLiteral("WARN")
        };
        break;

    case InvestigationIssueDrillDownType::
        Errors:
        /*
         * Existing grouped analysis intentionally
         * treats CRITICAL as error-class behavior.
         */
        targetSeverities = {
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };
        break;

    case InvestigationIssueDrillDownType::
        AllElevated:
        targetSeverities = {
            QStringLiteral("WARN"),
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };
        break;
    }

    if (
        filterPanel != nullptr
        && filterPanel
               ->configureIssueDrillDown(
                   subsystem,
                   targetSeverities
                   )
        ) {
        applyFilters();
    }
}

void MainWindow::applyTimelineDrillDown(
    const QDateTime &startTimestamp,
    const QDateTime &endTimestamp,
    const QString &severity,
    const QString &subsystem
    )
{
    if (
        filterPanel != nullptr
        && filterPanel
               ->configureTimelineDrillDown(
                   startTimestamp,
                   endTimestamp,
                   severity,
                   subsystem
                   )
        ) {
        applyFilters();
    }
}

void MainWindow::navigateToFinding(
    const QString &recordId
    )
{
    if (investigationController == nullptr
        || recordId.isEmpty()) {
        return;
    }

    const QVector<InvestigationRecord> &records =
        investigationController
            ->allRecords();

    const InvestigationRecord *targetRecord =
        nullptr;

    for (const InvestigationRecord &record
         : records) {
        if (record.recordId
            == recordId) {
            targetRecord =
                &record;

            break;
        }
    }

    if (targetRecord == nullptr) {
        return;
    }

    int proxyRow =
        investigationController
            ->proxyRowForRecordId(
                recordId
                );

    /*
     * If the record is already visible, navigation
     * should not disturb the analyst's filters.
     */
    if (proxyRow >= 0) {
        selectProxyRow(
            proxyRow
            );

        if (eventPanel != nullptr) {
            eventPanel->focusTable();
        }

        return;
    }

    /*
     * Otherwise relax only the criteria preventing
     * this finding's source record from appearing.
     */
    revealFindingRecord(
        *targetRecord
        );

    proxyRow =
        investigationController
            ->proxyRowForRecordId(
                recordId
                );

    if (proxyRow < 0) {
        return;
    }

    selectProxyRow(
        proxyRow
        );

    if (eventPanel != nullptr) {
        eventPanel->focusTable();
    }
}

void MainWindow::revealFindingRecord(
    const InvestigationRecord &record
    )
{
    if (investigationController == nullptr) {
        return;
    }

    InvestigationFilterProxyModel *proxyModel =
        investigationController
            ->proxyModel();

    const InvestigationFilterMatch match =
        proxyModel->filterMatchForRecord(
            record
            );

    if (match.allMatch()) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr
        || filterPanel == nullptr) {
        return;
    }

    const InvestigationRecordState state =
        session
            ->investigationStateStore()
            ->stateForRecord(
                record.recordId
                );

    if (
        filterPanel->revealRecord(
            record,
            match,
            state
            )
        ) {
        applyFilters();
    }
}

void MainWindow::exportFilteredResults()
{
    if (investigationController == nullptr) {
        QMessageBox::information(
            this,
            "No Records to Export",
            "There are no currently visible records to export."
            );

        return;
    }

    const QVector<InvestigationRecord> records =
        investigationController->visibleRecords();

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

void MainWindow::bindActiveSession()
{
    InvestigationSession *session =
        workspace->activeSession();

    /*
     * ---------------------------------------------------------
     * No active investigation
     * ---------------------------------------------------------
     */
    if (session == nullptr) {
        if (reloadAction != nullptr) {
            reloadAction->setEnabled(
                false
                );
        }

        investigationController =
            nullptr;

        currentFilePath.clear();

        if (filterPanel != nullptr) {
            filterPanel->setSession(
                nullptr
                );
        }

        if (eventPanel != nullptr) {
            eventPanel->setSession(
                nullptr
                );
        }

        if (timelinePanel != nullptr) {
            timelinePanel->setSession(
                nullptr
                );
        }

        if (reviewPanel != nullptr) {
            reviewPanel->setSession(
                nullptr
                );

            reviewPanel
                ->setIssueSummaryAvailable(
                    false
                    );

            reviewPanel->setVisible(
                false
                );
        }

        clearEventDetail();

        if (issueSummaryPanel != nullptr) {
            issueSummaryPanel->clear();
        }

        return;
    }

    /*
     * ---------------------------------------------------------
     * Active investigation
     * ---------------------------------------------------------
     */
    if (reloadAction != nullptr) {
        reloadAction->setEnabled(
            importWatcher == nullptr
            );
    }

    investigationController =
        session->investigationController();

    currentFilePath =
        session
            ->sourceMetadata()
            .sourcePath;

    if (filterPanel != nullptr) {
        filterPanel->setSession(
            session
            );
    }

    if (eventPanel != nullptr) {
        eventPanel->setSession(
            session
            );
    }

    if (timelinePanel != nullptr) {
        timelinePanel->setSession(
            session
            );
    }

    if (reviewPanel != nullptr) {
        reviewPanel->setSession(
            session
            );
    }

    /*
     * Restore bookmark/note/finding indicators
     * before recalculating the filtered result.
     */
    syncInvestigationStatePresentation();

    if (reviewPanel != nullptr) {
        reviewPanel
            ->setIssueSummaryAvailable(
                session->hasSeverityData()
                && session
                       ->hasSubsystemData()
                );

        reviewPanel->setVisible(
            true
            );

        reviewPanel
            ->restoreSelectedTab();
    }

    /*
     * InvestigationFilterPanel has already restored
     * the active session's independent filter
     * controls from its proxy-model state.
     *
     * Apply them once and refresh all dependent
     * investigation surfaces.
     */
    applyFilters();

    if (eventPanel != nullptr) {
        eventPanel
            ->refreshNavigationState();
    }
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

void MainWindow::drillDownBurst(
    const QDateTime &startTimestamp,
    const QDateTime &endTimestamp
    )
{
    if (
        filterPanel == nullptr
        || !filterPanel
                ->configureBurstDrillDown(
                    startTimestamp,
                    endTimestamp
                    )
        ) {
        return;
    }

    applyFilters();

    if (eventPanel != nullptr) {
        eventPanel->focusTable();
    }
}