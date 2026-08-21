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
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
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
#include <QTimer>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QProgressDialog>
#include <QPromise>
#include <QSignalBlocker>
#include <QMargins>
#include <QGraphicsLayout>
#include <QFrame>
#include <QVariant>
#include <QTimeZone>
#include <QTabBar>
#include <QPushButton>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QInputDialog>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "importing/BuiltInImporterRegistry.h"
#include "importing/ILogImporter.h"
#include "ui/ImportConfigurationDialog.h"
#include "ui/CustomFieldFilterEditor.h"
#include "ui/MultiSelectFilterComboBox.h"

namespace
{
constexpr int SearchDebounceIntervalMs = 600;

constexpr int TimelineAutoTargetBuckets = 20;

constexpr int TimelineVisibleBucketCount = 20;

constexpr int TimelineScaledScrollMaximum = 1'000'000;

constexpr qint64 MillisecondsPerSecond = 1'000;

constexpr qint64 MillisecondsPerMinute = 60 * MillisecondsPerSecond;

constexpr qint64 MillisecondsPerHour = 60 * MillisecondsPerMinute;

constexpr qint64 MillisecondsPerDay = 24 * MillisecondsPerHour;

qint64 automaticTimelineIntervalMilliseconds(
    const QDateTime &firstTimestamp,
    const QDateTime &lastTimestamp
    )
{
    if (!firstTimestamp.isValid()
        || !lastTimestamp.isValid()
        || firstTimestamp > lastTimestamp) {
        return MillisecondsPerMinute;
    }

    const qint64 spanMilliseconds =
        std::max<qint64>(
            1,
            firstTimestamp.msecsTo(
                lastTimestamp
                )
                + 1
            );

    const QList<qint64> candidates {
        1,
        10,
        100,
        500,

        1 * MillisecondsPerSecond,
        5 * MillisecondsPerSecond,
        15 * MillisecondsPerSecond,
        30 * MillisecondsPerSecond,

        1 * MillisecondsPerMinute,
        5 * MillisecondsPerMinute,
        15 * MillisecondsPerMinute,
        30 * MillisecondsPerMinute,

        1 * MillisecondsPerHour,
        3 * MillisecondsPerHour,
        6 * MillisecondsPerHour,
        12 * MillisecondsPerHour,

        1 * MillisecondsPerDay,
        3 * MillisecondsPerDay,
        7 * MillisecondsPerDay
    };

    for (const qint64 intervalMilliseconds
         : candidates) {
        const qint64 bucketCount =
            (
                spanMilliseconds
                + intervalMilliseconds
                - 1
                )
            / intervalMilliseconds;

        if (bucketCount
            <= TimelineAutoTargetBuckets) {
            return intervalMilliseconds;
        }
    }

    /*
     * For unusually long investigations,
     * continue scaling in whole-day units.
     */
    const qint64 targetMilliseconds =
        (
            spanMilliseconds
            + TimelineAutoTargetBuckets
            - 1
            )
        / TimelineAutoTargetBuckets;

    const qint64 wholeDays =
        std::max<qint64>(
            1,
            (
                targetMilliseconds
                + MillisecondsPerDay
                - 1
                )
                / MillisecondsPerDay
            );

    return wholeDays
           * MillisecondsPerDay;
}

void configureEventCountAxis(
    QValueAxis *axis,
    int maxCount
    )
{
    const int effectiveMax =
        std::max(
            1,
            maxCount
            );

    axis->setLabelFormat(
        QStringLiteral("%d")
        );

    axis->setTruncateLabels(
        false
        );

    /*
     * Keep the Y range tied directly to the
     * observed data maximum. Do not use
     * applyNiceNumbers(), because it can expand
     * the range and consume valuable vertical
     * plot space.
     */
    axis->setRange(
        0,
        effectiveMax
        );

    axis->setTickType(
        QValueAxis::TicksDynamic
        );

    axis->setTickAnchor(
        0
        );

    if (effectiveMax <= 10) {
        axis->setTickInterval(
            1
            );

        return;
    }

    /*
     * Aim for roughly five intervals while
     * retaining integer event-count labels.
     */
    const int tickInterval =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    static_cast<double>(
                        effectiveMax
                        )
                    / 5.0
                    )
                )
            );

    axis->setTickInterval(
        tickInterval
        );
}

int timelineScrollMaximum(
    qint64 totalBucketCount
    )
{
    const qint64 maximumStartBucketIndex =
        std::max<qint64>(
            0,
            totalBucketCount
                - TimelineVisibleBucketCount
            );

    if (maximumStartBucketIndex
        <= std::numeric_limits<int>::max()) {
        return static_cast<int>(
            maximumStartBucketIndex
            );
    }

    return TimelineScaledScrollMaximum;
}

qint64 timelineStartBucketIndex(
    qint64 totalBucketCount,
    int scrollValue
    )
{
    const qint64 maximumStartBucketIndex =
        std::max<qint64>(
            0,
            totalBucketCount
                - TimelineVisibleBucketCount
            );

    if (maximumStartBucketIndex <= 0) {
        return 0;
    }

    if (maximumStartBucketIndex
        <= std::numeric_limits<int>::max()) {
        return std::clamp<qint64>(
            scrollValue,
            0,
            maximumStartBucketIndex
            );
    }

    const int clampedScrollValue =
        std::clamp(
            scrollValue,
            0,
            TimelineScaledScrollMaximum
            );

    const long double fraction =
        static_cast<long double>(
            clampedScrollValue
            )
        / static_cast<long double>(
            TimelineScaledScrollMaximum
            );

    return std::clamp<qint64>(
        static_cast<qint64>(
            std::llround(
                fraction
                * static_cast<long double>(
                    maximumStartBucketIndex
                    )
                )
            ),
        0,
        maximumStartBucketIndex
        );
}

qint64 normalizedTimelineBucketEpoch(
    const QDateTime &timestamp,
    qint64 intervalMilliseconds
    )
{
    const qint64 epochMilliseconds =
        timestamp.toMSecsSinceEpoch();

    qint64 remainder =
        epochMilliseconds
        % intervalMilliseconds;

    if (remainder < 0) {
        remainder +=
            intervalMilliseconds;
    }

    return epochMilliseconds
           - remainder;
}

QString timelineDisplayLabel(
    const QString &canonicalLabel,
    qint64 intervalMilliseconds
    )
{
    /*
     * Fine-resolution windows contain at most
     * twenty adjacent buckets, so the surrounding
     * visible-range label supplies the full date
     * and time context.
     */
    if (intervalMilliseconds < 1000) {
        /*
         * HH:mm:ss.zzz -> ss.zzz
         *
         * This remains unique within a twenty
         * bucket sub-second window.
         */
        return canonicalLabel.right(
            6
            );
    }

    /*
     * Keep complete second-scale labels. Even a
     * 30-second interval spans several minutes
     * within a twenty-bucket window, so HH:mm:ss
     * is useful context.
     */
    return canonicalLabel;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    settings(),
    recentItemsStore(settings),
    filterPresetStore(settings),
    sessionTabBar(new QTabBar(this)),
    summaryLabel(new QLabel("No log file loaded.")),
    eventTable(new QTableView(this)),
    previousEventButton(
        new QPushButton(
            tr("Previous Event"),
            this
            )
        ),
    nextEventButton(
        new QPushButton(
            tr("Next Event"),
            this
            )
        ),
    eventPositionLabel(
        new QLabel(
            this
            )
        ),
    previousIssueButton(
        new QPushButton(
            tr("Previous Issue"),
            this
            )
        ),
    nextIssueButton(
        new QPushButton(
            tr("Next Issue"),
            this
            )
        ),
    eventDetailText(new QPlainTextEdit(this)),
    bookmarkButton(
        new QPushButton(
            tr("Bookmark Event"),
            this
            )
        ),
    issueSummaryTable(new QTableWidget(0, 4)),
    issueSummaryGroup(nullptr),
    workspace(new InvestigationWorkspace(this)),
    investigationController(nullptr),
    timelineChartView(new QChartView(this)),
    levelFilterCombo(new MultiSelectFilterComboBox(this)),
    resetFiltersButton(
        new QPushButton(
            tr("Reset Filters"),
            this
            )
        ),
    filterPresetsButton(
        new QPushButton(
            tr("Presets"),
            this
            )
        ),
    filterPresetsMenu(
        new QMenu(this)
        ),
    subsystemFilterCombo(new MultiSelectFilterComboBox(this)),
    eventCodeFilterCombo(new MultiSelectFilterComboBox(this)),
    entityFilterCombo(new MultiSelectFilterComboBox(this)),
    customFieldFilterEditor(new CustomFieldFilterEditor(this)),
    customFiltersButton(
        new QPushButton(
            tr("Custom Filters"),
            this
            )
        ),
    customFiltersDialog(
        new QDialog(this)
        ),
    timeRangeButton(
        new QPushButton(
            tr("Time Range"),
            this
            )
        ),
    timeRangeDialog(
        new QDialog(this)
        ),
    searchInput(new QLineEdit(this)),
    searchDebounceTimer(new QTimer(this))
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

            if (session == nullptr) {
                return;
            }

            const QSignalBlocker blocker(
                sessionTabBar
                );

            sessionTabBar->insertTab(
                index,
                session
                    ->sourceMetadata()
                    .sourceName
                );

            sessionTabBar->setTabToolTip(
                index,
                session
                    ->sourceMetadata()
                    .sourcePath
                );

            sessionTabBar->setVisible(true);
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::sessionClosed,
        this,
        [this](int index) {
            const QSignalBlocker blocker(
                sessionTabBar
                );

            sessionTabBar->removeTab(index);

            sessionTabBar->setVisible(
                workspace->sessionCount() > 0
                );
        }
        );

    connect(
        workspace,
        &InvestigationWorkspace::activeSessionChanged,
        this,
        [this](int index) {
            {
                const QSignalBlocker blocker(
                    sessionTabBar
                    );

                sessionTabBar->setCurrentIndex(
                    index
                    );
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
        sessionTabBar,
        &QTabBar::currentChanged,
        this,
        [this](int index) {
            if (index < 0) {
                return;
            }

            workspace->setActiveSession(
                index
                );
        }
        );

    connect(
        sessionTabBar,
        &QTabBar::tabCloseRequested,
        this,
        [this](int index) {
            workspace->closeSession(
                index
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
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    sessionTabBar->setTabsClosable(true);

    sessionTabBar->setExpanding(false);

    sessionTabBar->setDocumentMode(true);

    sessionTabBar->setUsesScrollButtons(true);

    sessionTabBar->setElideMode(
        Qt::ElideMiddle
        );

    sessionTabBar->setVisible(false);

    layout->addWidget(
        sessionTabBar
        );

    layout->addWidget(summaryLabel);
    buildFilterControls(layout);

    auto *timelineGroup = buildTimelinePanel();

    auto *eventsGroup = new QGroupBox("Telemetry Events", this);
    auto *eventsLayout = new QVBoxLayout(eventsGroup);

    auto *eventNavigationLayout =
        new QHBoxLayout();

    eventNavigationLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    eventNavigationLayout->setSpacing(
        6
        );

    previousEventButton->setToolTip(
        tr(
            "Select the previous visible event"
            )
        );

    nextEventButton->setToolTip(
        tr(
            "Select the next visible event"
            )
        );

    previousIssueButton->setToolTip(
        tr(
            "Select the previous visible WARN, "
            "ERROR, or CRITICAL event"
            )
        );

    nextIssueButton->setToolTip(
        tr(
            "Select the next visible WARN, "
            "ERROR, or CRITICAL event"
            )
        );

    eventNavigationLayout->addStretch();

    eventNavigationLayout->addWidget(
        previousEventButton
        );

    eventNavigationLayout->addWidget(
        nextEventButton
        );

    eventNavigationLayout->addStretch(
        1
        );

    eventPositionLabel->setAlignment(
        Qt::AlignCenter
        );

    eventNavigationLayout->addWidget(
        eventPositionLabel
        );

    eventNavigationLayout->addStretch(
        1
        );

    eventNavigationLayout->addWidget(
        previousIssueButton
        );

    eventNavigationLayout->addWidget(
        nextIssueButton
        );

    eventsLayout->addLayout(
        eventNavigationLayout
        );

    eventsLayout->setSpacing(
        4
        );

    eventTable->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    eventTable->setAlternatingRowColors(true);

    eventTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    eventTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    eventTable->setContextMenuPolicy(
        Qt::CustomContextMenu
        );

    connect(
        eventTable,
        &QTableView::customContextMenuRequested,
        this,
        [this](
            const QPoint &position
            ) {
            const QModelIndex proxyIndex =
                eventTable->indexAt(
                    position
                    );

            if (!proxyIndex.isValid()
                || investigationController
                       == nullptr) {
                return;
            }

            const QString value =
                proxyIndex
                    .data(Qt::DisplayRole)
                    .toString();

            InvestigationFilterProxyModel
                *proxyModel =
                investigationController
                    ->proxyModel();

            InvestigationTableModel
                *sourceModel =
                investigationController
                    ->sourceModel();

            const QModelIndex sourceIndex =
                proxyModel->mapToSource(
                    proxyIndex
                    );

            const bool customColumn =
                sourceIndex.isValid()
                && sourceModel
                       ->isCustomColumn(
                           sourceIndex.column()
                           );

            const QString customField =
                customColumn
                    ? sourceModel
                          ->columnKey(
                              sourceIndex.column()
                              )
                    : QString();

            QMenu menu(
                eventTable
                );

            QAction *copyValueAction =
                menu.addAction(
                    tr("Copy Cell Value")
                    );

            QAction *filterValueAction =
                nullptr;

            /*
             * Exact custom-field filtering only makes
             * sense when both a custom field and a
             * concrete displayed value are present.
             */
            if (customColumn
                && !customField.isEmpty()
                && !value.isEmpty()) {
                menu.addSeparator();

                filterValueAction =
                    menu.addAction(
                        tr("Filter by This Value")
                        );
            }

            QAction *selectedAction =
                menu.exec(
                    eventTable
                        ->viewport()
                        ->mapToGlobal(
                            position
                            )
                    );

            if (selectedAction
                == copyValueAction) {
                QApplication::clipboard()
                ->setText(
                    value
                    );

                return;
            }

            if (filterValueAction != nullptr
                && selectedAction
                       == filterValueAction) {
                customFieldFilterEditor
                    ->addFilter(
                        customField,
                        value
                        );
            }
        }
        );

    eventTable->setSortingEnabled(true);

    eventTable->sortByColumn(
        -1,
        Qt::AscendingOrder
        );

    eventTable
        ->horizontalHeader()
        ->setResizeContentsPrecision(
            200
            );

    eventTable->horizontalHeader()->setStretchLastSection(true);

    connect(
        previousEventButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateToAdjacentEvent(
                -1
                );
        }
        );

    connect(
        nextEventButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateToAdjacentEvent(
                1
                );
        }
        );

    connect(
        previousIssueButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateToAdjacentIssue(
                -1
                );
        }
        );

    connect(
        nextIssueButton,
        &QPushButton::clicked,
        this,
        [this]() {
            navigateToAdjacentIssue(
                1
                );
        }
        );

    previousIssueButton->setVisible(
        false
        );

    nextIssueButton->setVisible(
        false
        );

    connect(
        eventTable->horizontalHeader(),
        &QHeaderView::sectionResized,
        this,
        [this](
            int,
            int,
            int
            ) {
            InvestigationSession *session =
                workspace->activeSession();

            if (session == nullptr
                || eventTable->model() == nullptr) {
                return;
            }

            const int columnCount =
                eventTable
                    ->horizontalHeader()
                    ->count();

            QVector<int> widths;

            widths.reserve(
                columnCount
                );

            for (
                int column = 0;
                column < columnCount;
                ++column
                ) {
                widths.append(
                    eventTable->columnWidth(
                        column
                        )
                    );
            }

            session->setColumnWidths(
                std::move(widths)
                );
        }
        );

    eventsLayout->addWidget(eventTable);

    issueSummaryGroup =
        buildIssueSummaryPanel();
    auto *detailGroup = buildDetailPanel();

    auto *bottomSplitter = new QSplitter(Qt::Horizontal, this);

    bottomSplitter->addWidget(issueSummaryGroup);
    bottomSplitter->addWidget(detailGroup);

    bottomSplitter->setStretchFactor(0, 0);
    bottomSplitter->setStretchFactor(1, 1);

    bottomSplitter->setCollapsible(0, false);
    bottomSplitter->setCollapsible(1, false);

    bottomSplitter->setSizes({
        issueSummaryTable->minimumWidth() + 30,
        1000
    });

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(timelineGroup);
    mainSplitter->addWidget(eventsGroup);
    mainSplitter->addWidget(bottomSplitter);

    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 5);
    mainSplitter->setStretchFactor(2, 2);

    mainSplitter->setCollapsible(0, false);
    mainSplitter->setCollapsible(1, false);
    mainSplitter->setCollapsible(2, false);

    mainSplitter->setSizes(QList<int>{220, 350, 250});

    layout->addWidget(mainSplitter, 1);

    setCentralWidget(centralWidget);
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
    const QString &filePath
    )
{
    int traceCount = 0;
    int debugCount = 0;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;

    if (!hasSeverityData) {
        summaryLabel->setText(
            QString(
                "TOTAL: %1 visible of %2 events from %3"
                )
                .arg(records.size())
                .arg(
                    investigationController
                        ->totalRecordCount()
                    )
                .arg(filePath)
            );

        return;
    }

    for (const InvestigationRecord &record
         : records) {
        if (!record.severity.has_value()) {
            continue;
        }

        switch (record.severity.value()) {
            case RecordSeverity::Trace:
                ++traceCount;
                break;

            case RecordSeverity::Debug:
                ++debugCount;
                break;

            case RecordSeverity::Info:
                ++infoCount;
                break;

            case RecordSeverity::Warning:
                ++warningCount;
                break;

            case RecordSeverity::Error:
                ++errorCount;
                break;

            case RecordSeverity::Critical:
                ++criticalCount;
                break;
        }
    }

    summaryLabel->setText(
        QString(
            "Showing %1 of %2 events from %3 | "
            "TRACE: %4 | DEBUG: %5 | INFO: %6 | "
            "WARN: %7 | ERROR: %8 | CRITICAL: %9"
            )
            .arg(records.size())
            .arg(investigationController->totalRecordCount())
            .arg(filePath)
            .arg(traceCount)
            .arg(debugCount)
            .arg(infoCount)
            .arg(warningCount)
            .arg(errorCount)
            .arg(criticalCount)
        );
}

void MainWindow::buildFilterControls(
    QVBoxLayout *layout
    )
{
    /*
     * ---------------------------------------------------------
     * Primary categorical filter controls
     * ---------------------------------------------------------
     */

    levelFilterCombo->setEmptySelectionText(
        tr("All severities")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("TRACE"),
        QStringLiteral("TRACE")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("DEBUG"),
        QStringLiteral("DEBUG")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("INFO"),
        QStringLiteral("INFO")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("WARN"),
        QStringLiteral("WARN")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("ERROR"),
        QStringLiteral("ERROR")
        );

    levelFilterCombo->addFilterItem(
        QStringLiteral("CRITICAL"),
        QStringLiteral("CRITICAL")
        );

    levelFilterCombo->setMinimumWidth(
        150
        );

    subsystemFilterCombo
        ->setEmptySelectionText(
            tr("All subsystems")
            );

    subsystemFilterCombo->setMinimumWidth(
        190
        );

    subsystemFilterCombo->setSizeAdjustPolicy(
        QComboBox::
        AdjustToMinimumContentsLengthWithIcon
        );

    subsystemFilterCombo
        ->setMinimumContentsLength(
            18
            );

    /*
     * Event-code filter.
     */
    eventCodeFilterWidget =
        new QWidget(this);

    auto *eventCodeLayout =
        new QHBoxLayout(
            eventCodeFilterWidget
            );

    eventCodeLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    eventCodeLayout->setSpacing(
        4
        );

    auto *eventCodeLabel =
        new QLabel(
            tr("Event code:"),
            eventCodeFilterWidget
            );

    eventCodeFilterCombo
        ->setEmptySelectionText(
            tr("All event codes")
            );

    eventCodeFilterCombo->setMinimumWidth(
        170
        );

    eventCodeLayout->addWidget(
        eventCodeLabel
        );

    eventCodeLayout->addWidget(
        eventCodeFilterCombo
        );

    /*
     * Entity filter.
     */
    entityFilterWidget =
        new QWidget(this);

    auto *entityLayout =
        new QHBoxLayout(
            entityFilterWidget
            );

    entityLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    entityLayout->setSpacing(
        4
        );

    auto *entityLabel =
        new QLabel(
            tr("Entity:"),
            entityFilterWidget
            );

    entityFilterCombo
        ->setEmptySelectionText(
            tr("All entities")
            );

    entityFilterCombo->setMinimumWidth(
        170
        );

    entityLayout->addWidget(
        entityLabel
        );

    entityLayout->addWidget(
        entityFilterCombo
        );

    /*
     * ---------------------------------------------------------
     * Row 1
     *
     * Severity | Subsystem | Event Code | Entity
     * ---------------------------------------------------------
     */

    auto *primaryFilterLayout =
        new QHBoxLayout();

    primaryFilterLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    primaryFilterLayout->setSpacing(
        8
        );

    primaryFilterLayout->addWidget(
        levelFilterCombo
        );

    primaryFilterLayout->addWidget(
        subsystemFilterCombo
        );

    primaryFilterLayout->addWidget(
        eventCodeFilterWidget
        );

    primaryFilterLayout->addWidget(
        entityFilterWidget
        );

    primaryFilterLayout->addStretch();

    layout->addLayout(
        primaryFilterLayout
        );

    /*
     * Event-code/entity controls are capability
     * driven and begin hidden.
     */
    eventCodeFilterWidget->setVisible(
        false
        );

    entityFilterWidget->setVisible(
        false
        );

    /*
     * ---------------------------------------------------------
     * Search
     * ---------------------------------------------------------
     */

    searchInput->setPlaceholderText(
        tr(
            "Search canonical fields and "
            "custom attributes..."
            )
        );

    searchDebounceTimer->setSingleShot(
        true
        );

    searchDebounceTimer->setInterval(
        SearchDebounceIntervalMs
        );

    /*
     * ---------------------------------------------------------
     * Time-range dialog
     * ---------------------------------------------------------
     */

    timeRangeFilterWidget =
        new QWidget(
            timeRangeDialog
            );

    auto *timeRangeLayout =
        new QHBoxLayout(
            timeRangeFilterWidget
            );

    timeRangeLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    timeRangeLayout->setSpacing(
        6
        );

    auto *timeRangeLabel =
        new QLabel(
            tr("Time range (UTC):"),
            timeRangeFilterWidget
            );

    timeRangeStartCheckBox =
        new QCheckBox(
            tr("From"),
            timeRangeFilterWidget
            );

    timeRangeStartEdit =
        new QDateTimeEdit(
            timeRangeFilterWidget
            );

    timeRangeEndCheckBox =
        new QCheckBox(
            tr("To"),
            timeRangeFilterWidget
            );

    timeRangeEndEdit =
        new QDateTimeEdit(
            timeRangeFilterWidget
            );

    for (
        QDateTimeEdit *edit
        : {
            timeRangeStartEdit,
            timeRangeEndEdit
        }
        ) {
        edit->setDisplayFormat(
            QStringLiteral(
                "yyyy-MM-dd HH:mm:ss.zzz"
                )
            );

        edit->setTimeZone(
            QTimeZone::UTC
            );

        edit->setKeyboardTracking(
            true
            );

        edit->setEnabled(
            false
            );

        edit->setMinimumWidth(
            210
            );
    }

    timeRangeLayout->addWidget(
        timeRangeLabel
        );

    timeRangeLayout->addWidget(
        timeRangeStartCheckBox
        );

    timeRangeLayout->addWidget(
        timeRangeStartEdit
        );

    timeRangeLayout->addSpacing(
        12
        );

    timeRangeLayout->addWidget(
        timeRangeEndCheckBox
        );

    timeRangeLayout->addWidget(
        timeRangeEndEdit
        );

    timeRangeLayout->addStretch();

    timeRangeDialog->setWindowTitle(
        tr("Time Range Filter")
        );

    timeRangeDialog->setModal(
        false
        );

    timeRangeDialog->setWindowModality(
        Qt::NonModal
        );

    auto *timeDialogLayout =
        new QVBoxLayout(
            timeRangeDialog
            );

    timeDialogLayout->addWidget(
        timeRangeFilterWidget
        );

    auto *timeCloseButton =
        new QPushButton(
            tr("Close"),
            timeRangeDialog
            );

    timeDialogLayout->addWidget(
        timeCloseButton,
        0,
        Qt::AlignRight
        );

    /*
     * ---------------------------------------------------------
     * Custom-field filter dialog
     * ---------------------------------------------------------
     */

    customFiltersDialog->setWindowTitle(
        tr("Custom Field Filters")
        );

    customFiltersDialog->setModal(
        false
        );

    customFiltersDialog->setWindowModality(
        Qt::NonModal
        );

    customFiltersDialog->setMinimumWidth(
        600
        );

    auto *customDialogLayout =
        new QVBoxLayout(
            customFiltersDialog
            );

    customDialogLayout->addWidget(
        customFieldFilterEditor
        );

    auto *customCloseButton =
        new QPushButton(
            tr("Close"),
            customFiltersDialog
            );

    customDialogLayout->addWidget(
        customCloseButton,
        0,
        Qt::AlignRight
        );

    /*
     * ---------------------------------------------------------
     * Row 2
     *
     * Search | Time Range | Custom Filters | Reset
     * ---------------------------------------------------------
     */

    auto *secondaryFilterLayout =
        new QHBoxLayout();

    secondaryFilterLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    secondaryFilterLayout->setSpacing(
        8
        );

    secondaryFilterLayout->addWidget(
        searchInput,
        1
        );

    secondaryFilterLayout->addWidget(
        timeRangeButton
        );

    secondaryFilterLayout->addWidget(
        customFiltersButton
        );

    secondaryFilterLayout->addWidget(
        filterPresetsButton
        );

    filterPresetsButton->setMenu(
        filterPresetsMenu
        );

    secondaryFilterLayout->addWidget(
        resetFiltersButton
        );

    layout->addLayout(
        secondaryFilterLayout
        );

    /*
     * These controls only become available when
     * the active investigation supports them.
     */
    timeRangeButton->setVisible(
        false
        );

    customFiltersButton->setVisible(
        false
        );

    resetFiltersButton->setEnabled(
        false
        );

    resetFiltersButton->setVisible(
        false
        );

    filterPresetsButton->setVisible(
        false
        );

    /*
     * ---------------------------------------------------------
     * Filter connections
     * ---------------------------------------------------------
     */

    connect(
        levelFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        [this]() {
            searchDebounceTimer->stop();

            applyFilters();
        }
        );

    connect(
        subsystemFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        [this]() {
            searchDebounceTimer->stop();

            applyFilters();
        }
        );

    connect(
        eventCodeFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        [this]() {
            applyFilters();
        }
        );

    connect(
        entityFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        [this]() {
            applyFilters();
        }
        );

    connect(
        filterPresetsMenu,
        &QMenu::aboutToShow,
        this,
        [this]() {
            refreshFilterPresetsMenu();
        }
        );

    /*
     * ---------------------------------------------------------
     * Search connections
     * ---------------------------------------------------------
     */

    connect(
        searchInput,
        &QLineEdit::textChanged,
        this,
        [this]() {
            searchDebounceTimer->start();
        }
        );

    connect(
        searchInput,
        &QLineEdit::returnPressed,
        this,
        [this]() {
            searchDebounceTimer->stop();

            applyFilters();
        }
        );

    connect(
        searchDebounceTimer,
        &QTimer::timeout,
        this,
        [this]() {
            applyFilters();
        }
        );

    /*
     * ---------------------------------------------------------
     * Reset
     * ---------------------------------------------------------
     */

    connect(
        resetFiltersButton,
        &QPushButton::clicked,
        this,
        [this]() {
            resetFilters();
        }
        );

    /*
     * ---------------------------------------------------------
     * Time-range dialog
     * ---------------------------------------------------------
     */

    connect(
        timeRangeButton,
        &QPushButton::clicked,
        this,
        [this]() {
            timeRangeDialog->show();

            timeRangeDialog->raise();

            timeRangeDialog
                ->activateWindow();
        }
        );

    connect(
        timeCloseButton,
        &QPushButton::clicked,
        timeRangeDialog,
        &QDialog::close
        );

    /*
     * ---------------------------------------------------------
     * Custom-field dialog
     * ---------------------------------------------------------
     */

    connect(
        customFiltersButton,
        &QPushButton::clicked,
        this,
        [this]() {
            customFiltersDialog->show();

            customFiltersDialog->raise();

            customFiltersDialog
                ->activateWindow();
        }
        );

    connect(
        customCloseButton,
        &QPushButton::clicked,
        customFiltersDialog,
        &QDialog::close
        );

    /*
     * Establish a compact initial size based on the
     * editor's currently visible contents. Do not
     * impose a fixed vertical size.
     */
    customFiltersDialog->adjustSize();

    connect(
        customFieldFilterEditor,
        &CustomFieldFilterEditor::
        filtersChanged,
        this,
        [this]() {
            updateCustomFiltersButton();

            resizeCustomFiltersDialogToContents();

            applyFilters();
        }
        );

    /*
     * ---------------------------------------------------------
     * Time-range filtering
     * ---------------------------------------------------------
     */

    connect(
        timeRangeStartCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            timeRangeStartEdit->setEnabled(
                checked
                );

            if (
                checked
                && timeRangeEndCheckBox
                       ->isChecked()
                && timeRangeStartEdit
                           ->dateTime()
                       > timeRangeEndEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    timeRangeEndEdit
                    );

                timeRangeEndEdit->setDateTime(
                    timeRangeStartEdit
                        ->dateTime()
                    );
            }

            updateTimeRangeButton();

            applyFilters();
        }
        );

    connect(
        timeRangeEndCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            timeRangeEndEdit->setEnabled(
                checked
                );

            if (
                checked
                && timeRangeStartCheckBox
                       ->isChecked()
                && timeRangeEndEdit
                           ->dateTime()
                       < timeRangeStartEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    timeRangeStartEdit
                    );

                timeRangeStartEdit->setDateTime(
                    timeRangeEndEdit
                        ->dateTime()
                    );
            }

            updateTimeRangeButton();

            applyFilters();
        }
        );

    connect(
        timeRangeStartEdit,
        &QDateTimeEdit::dateTimeChanged,
        this,
        [this](
            const QDateTime &dateTime
            ) {
            if (
                !timeRangeStartCheckBox
                     ->isChecked()
                ) {
                return;
            }

            if (
                timeRangeEndCheckBox
                    ->isChecked()
                && dateTime
                       > timeRangeEndEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    timeRangeEndEdit
                    );

                timeRangeEndEdit->setDateTime(
                    dateTime
                    );
            }

            updateTimeRangeButton();

            applyFilters();
        }
        );

    connect(
        timeRangeEndEdit,
        &QDateTimeEdit::dateTimeChanged,
        this,
        [this](
            const QDateTime &dateTime
            ) {
            if (
                !timeRangeEndCheckBox
                     ->isChecked()
                ) {
                return;
            }

            if (
                timeRangeStartCheckBox
                    ->isChecked()
                && dateTime
                       < timeRangeStartEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    timeRangeStartEdit
                    );

                timeRangeStartEdit->setDateTime(
                    dateTime
                    );
            }

            updateTimeRangeButton();

            applyFilters();
        }
        );

    /*
     * Establish the initial button summaries.
     */
    updateTimeRangeButton();
    updateCustomFiltersButton();
}

void MainWindow::applyFilters()
{
    if (investigationController == nullptr) {
        return;
    }

    timelineScaleValid =
        false;

    const std::optional<QDateTime>
        timeRangeStart =
        timeRangeStartCheckBox != nullptr
                && timeRangeStartCheckBox
                       ->isChecked()
            ? std::optional<QDateTime>(
                  timeRangeStartEdit
                      ->dateTime()
                  )
            : std::nullopt;

    const std::optional<QDateTime>
        timeRangeEnd =
        timeRangeEndCheckBox != nullptr
                && timeRangeEndCheckBox
                       ->isChecked()
            ? std::optional<QDateTime>(
                  timeRangeEndEdit
                      ->dateTime()
                  )
            : std::nullopt;

    investigationController
        ->setFilterState(
            levelFilterCombo
                ->selectedValues(),
            subsystemFilterCombo
                ->selectedValues(),
            searchInput->text(),
            eventCodeFilterCombo
                ->selectedValues(),
            entityFilterCombo
                ->selectedValues(),
            timeRangeStart,
            timeRangeEnd,
            customFieldFilterEditor
                ->filters()
            );

    eventTable->clearSelection();
    clearEventDetail();

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

    updateTimelineChart(
        visibleRecords
        );

    updateEventNavigationState();
}

void MainWindow::resetFilters()
{
    if (investigationController == nullptr) {
        return;
    }

    searchDebounceTimer->stop();

    {
        const QSignalBlocker severityBlocker(
            levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            subsystemFilterCombo
            );

        const QSignalBlocker searchBlocker(
            searchInput
            );

        const QSignalBlocker eventCodeBlocker(
            eventCodeFilterCombo
            );

        const QSignalBlocker entityBlocker(
            entityFilterCombo
            );

        const QSignalBlocker customFieldBlocker(
            customFieldFilterEditor
            );

        const QSignalBlocker timeStartCheckBlocker(
            timeRangeStartCheckBox
            );

        const QSignalBlocker timeStartEditBlocker(
            timeRangeStartEdit
            );

        const QSignalBlocker timeEndCheckBlocker(
            timeRangeEndCheckBox
            );

        const QSignalBlocker timeEndEditBlocker(
            timeRangeEndEdit
            );

        levelFilterCombo->clearSelection();

        subsystemFilterCombo->clearSelection();

        searchInput->clear();

        eventCodeFilterCombo->clearSelection();

        entityFilterCombo->clearSelection();

        customFieldFilterEditor->clearFilters();

        timeRangeStartCheckBox->setChecked(
            false
            );

        timeRangeEndCheckBox->setChecked(
            false
            );

        timeRangeStartEdit->setEnabled(
            false
            );

        timeRangeEndEdit->setEnabled(
            false
            );

        if (timelineFirstTimestamp.has_value()) {
            timeRangeStartEdit->setDateTime(
                timelineFirstTimestamp.value()
                );
        }

        if (timelineLastTimestamp.has_value()) {
            timeRangeEndEdit->setDateTime(
                timelineLastTimestamp.value()
                );
        }
    }

    updateTimeRangeButton();
    updateCustomFiltersButton();

    resizeCustomFiltersDialogToContents();

    applyFilters();
}

InvestigationFilterPreset
MainWindow::currentFilterPreset(
    const QString &name
    ) const
{
    InvestigationFilterPreset preset;

    preset.name =
        name.trimmed();

    if (investigationController == nullptr) {
        return preset;
    }

    InvestigationFilterProxyModel *proxyModel =
        investigationController
            ->proxyModel();

    preset.severities =
        proxyModel->severityFilters();

    preset.subsystems =
        proxyModel->subsystemFilters();

    preset.searchText =
        proxyModel->searchText();

    preset.eventCodes =
        proxyModel->eventCodeFilters();

    preset.entityIds =
        proxyModel->entityFilters();

    preset.timeRangeStart =
        proxyModel->timeRangeStart();

    preset.timeRangeEnd =
        proxyModel->timeRangeEnd();

    preset.customFieldFilters =
        proxyModel->customFieldFilters();

    return preset;
}

void MainWindow::applyFilterPreset(
    const InvestigationFilterPreset &preset
    )
{
    if (investigationController == nullptr) {
        return;
    }

    searchDebounceTimer->stop();

    /*
     * Update every control as one transaction.
     * Some preset criteria may not exist in the
     * active investigation. Multi-select controls
     * naturally ignore unavailable values, while
     * CustomFieldFilterEditor removes unavailable
     * custom-field criteria.
     */
    {
        const QSignalBlocker severityBlocker(
            levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            subsystemFilterCombo
            );

        const QSignalBlocker searchBlocker(
            searchInput
            );

        const QSignalBlocker eventCodeBlocker(
            eventCodeFilterCombo
            );

        const QSignalBlocker entityBlocker(
            entityFilterCombo
            );

        const QSignalBlocker customFieldBlocker(
            customFieldFilterEditor
            );

        const QSignalBlocker timeStartCheckBlocker(
            timeRangeStartCheckBox
            );

        const QSignalBlocker timeStartEditBlocker(
            timeRangeStartEdit
            );

        const QSignalBlocker timeEndCheckBlocker(
            timeRangeEndCheckBox
            );

        const QSignalBlocker timeEndEditBlocker(
            timeRangeEndEdit
            );

        levelFilterCombo->setSelectedValues(
            hasSeverityData
                ? preset.severities
                : QStringList()
            );

        subsystemFilterCombo->setSelectedValues(
            hasSubsystemData
                ? preset.subsystems
                : QStringList()
            );

        eventCodeFilterCombo->setSelectedValues(
            hasEventCodeData
                ? preset.eventCodes
                : QStringList()
            );

        entityFilterCombo->setSelectedValues(
            hasEntityData
                ? preset.entityIds
                : QStringList()
            );

        customFieldFilterEditor->setFilters(
            hasCustomFieldData
                ? preset.customFieldFilters
                : CustomFieldFilterMap()
            );

        searchInput->setText(
            preset.searchText
            );

        if (hasTimestampData) {
            const bool hasStart =
                preset.timeRangeStart.has_value();

            const bool hasEnd =
                preset.timeRangeEnd.has_value();

            timeRangeStartCheckBox->setChecked(
                hasStart
                );

            timeRangeEndCheckBox->setChecked(
                hasEnd
                );

            timeRangeStartEdit->setEnabled(
                hasStart
                );

            timeRangeEndEdit->setEnabled(
                hasEnd
                );

            timeRangeStartEdit->setDateTime(
                hasStart
                    ? preset
                          .timeRangeStart
                          .value()
                    : timelineFirstTimestamp
                          .value()
                );

            timeRangeEndEdit->setDateTime(
                hasEnd
                    ? preset
                          .timeRangeEnd
                          .value()
                    : timelineLastTimestamp
                          .value()
                );
        } else {
            timeRangeStartCheckBox->setChecked(
                false
                );

            timeRangeEndCheckBox->setChecked(
                false
                );

            timeRangeStartEdit->setEnabled(
                false
                );

            timeRangeEndEdit->setEnabled(
                false
                );
        }
    }

    updateTimeRangeButton();
    updateCustomFiltersButton();

    resizeCustomFiltersDialogToContents();

    /*
     * Apply the fully restored preset once rather
     * than producing intermediate filtered states.
     */
    applyFilters();
}

void MainWindow::refreshFilterPresetsMenu()
{
    if (filterPresetsMenu == nullptr) {
        return;
    }

    filterPresetsMenu->clear();

    QAction *saveAction =
        filterPresetsMenu->addAction(
            tr("Save Current Filters...")
            );

    saveAction->setEnabled(
        investigationController != nullptr
        );

    connect(
        saveAction,
        &QAction::triggered,
        this,
        [this]() {
            bool accepted = false;

            const QString name =
                QInputDialog::getText(
                    this,
                    tr("Save Filter Preset"),
                    tr("Preset name:"),
                    QLineEdit::Normal,
                    QString(),
                    &accepted
                    )
                    .trimmed();

            if (!accepted
                || name.isEmpty()) {
                return;
            }

            const QVector<
                InvestigationFilterPreset>
                existingPresets =
                filterPresetStore.presets();

            bool existingName = false;

            for (
                const InvestigationFilterPreset
                    &existing
                : existingPresets
                ) {
                if (existing.name.compare(
                        name,
                        Qt::CaseInsensitive
                        )
                    == 0) {
                    existingName = true;
                    break;
                }
            }

            if (existingName) {
                const QMessageBox::StandardButton
                    overwrite =
                    QMessageBox::question(
                        this,
                        tr(
                            "Replace Filter Preset"
                            ),
                        tr(
                            "A filter preset named "
                            "\"%1\" already exists.\n\n"
                            "Replace it with the "
                            "current filters?"
                            )
                            .arg(name),
                        QMessageBox::Yes
                            | QMessageBox::No,
                        QMessageBox::No
                        );

                if (overwrite
                    != QMessageBox::Yes) {
                    return;
                }
            }

            filterPresetStore.savePreset(
                currentFilterPreset(name)
                );
        }
        );

    filterPresetsMenu->addSeparator();

    const QVector<InvestigationFilterPreset>
        presets =
        filterPresetStore.presets();

    if (presets.isEmpty()) {
        QAction *emptyAction =
            filterPresetsMenu->addAction(
                tr("No Saved Presets")
                );

        emptyAction->setEnabled(
            false
            );

        return;
    }

    /*
     * Saved presets are reusable application-level
     * investigation shortcuts.
     */
    for (const InvestigationFilterPreset &preset
         : presets) {
        QAction *presetAction =
            filterPresetsMenu->addAction(
                preset.name
                );

        presetAction->setEnabled(
            investigationController != nullptr
            );

        connect(
            presetAction,
            &QAction::triggered,
            this,
            [this, preset]() {
                applyFilterPreset(
                    preset
                    );
            }
            );
    }

    filterPresetsMenu->addSeparator();

    QMenu *deleteMenu =
        filterPresetsMenu->addMenu(
            tr("Delete Preset")
            );

    for (const InvestigationFilterPreset &preset
         : presets) {
        QAction *deleteAction =
            deleteMenu->addAction(
                preset.name
                );

        connect(
            deleteAction,
            &QAction::triggered,
            this,
            [this, preset]() {
                const QMessageBox::StandardButton
                    confirmation =
                    QMessageBox::question(
                        this,
                        tr(
                            "Delete Filter Preset"
                            ),
                        tr(
                            "Delete the filter preset "
                            "\"%1\"?"
                            )
                            .arg(
                                preset.name
                                ),
                        QMessageBox::Yes
                            | QMessageBox::No,
                        QMessageBox::No
                        );

                if (confirmation
                    != QMessageBox::Yes) {
                    return;
                }

                filterPresetStore.removePreset(
                    preset.name
                    );
            }
            );
    }
}

void MainWindow::
    refreshSubsystemFilterOptions()
{
    if (investigationController
        == nullptr) {
        return;
    }

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    const QStringList selectedSubsystems =
        subsystemFilterCombo
            ->selectedValues();

    const QSignalBlocker blocker(
        subsystemFilterCombo
        );

    subsystemFilterCombo->clear();

    const QStringList &subsystems =
        session->availableSubsystems();

    int widestTextWidth =
        subsystemFilterCombo
            ->fontMetrics()
            .horizontalAdvance(
                QStringLiteral(
                    "All subsystems"
                    )
                );

    for (const QString &subsystem
         : subsystems) {
        subsystemFilterCombo
            ->addFilterItem(
                subsystem,
                subsystem
                );

        widestTextWidth =
            std::max(
                widestTextWidth,
                subsystemFilterCombo
                    ->fontMetrics()
                    .horizontalAdvance(
                        subsystem
                        )
                );
    }

    const int popupWidth =
        std::clamp(
            widestTextWidth + 40,
            240,
            650
            );

    subsystemFilterCombo
        ->view()
        ->setMinimumWidth(
            popupWidth
            );

    subsystemFilterCombo
        ->setSelectedValues(
            selectedSubsystems
            );
}

void MainWindow::refreshCanonicalFilterOptions()
{
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    auto populateFilter =
        [](
            MultiSelectFilterComboBox *combo,
            const QStringList &values
            ) {
            const QSignalBlocker blocker(
                combo
                );

            combo->clear();

            int widestTextWidth = 0;

            for (const QString &value
                 : values) {
                combo->addFilterItem(
                    value,
                    value
                    );

                widestTextWidth =
                    std::max(
                        widestTextWidth,
                        combo
                            ->fontMetrics()
                            .horizontalAdvance(
                                value
                                )
                        );
            }

            combo->clearSelection();

            combo->view()
                ->setMinimumWidth(
                    std::clamp(
                        widestTextWidth + 50,
                        220,
                        700
                        )
                    );
        };

    populateFilter(
        eventCodeFilterCombo,
        session->availableEventCodes()
        );

    populateFilter(
        entityFilterCombo,
        session->availableEntities()
        );
}

void MainWindow::
    updateCustomFiltersButton()
{
    if (customFieldFilterEditor == nullptr) {
        return;
    }

    const CustomFieldFilterMap &filters =
        customFieldFilterEditor
            ->filters();

    int criterionCount = 0;

    for (
        auto iterator =
        filters.constBegin();
        iterator != filters.constEnd();
        ++iterator
        ) {
        criterionCount +=
            iterator.value().size();
    }

    if (criterionCount == 0) {
        customFiltersButton->setText(
            tr("Custom Filters")
            );

        customFiltersButton->setToolTip(
            tr("No custom-field filters active")
            );

        return;
    }

    customFiltersButton->setText(
        tr("Custom Filters (%1)")
            .arg(criterionCount)
        );

    QStringList descriptions;

    for (
        auto iterator =
        filters.constBegin();
        iterator != filters.constEnd();
        ++iterator
        ) {
        for (const QString &value
             : iterator.value()) {
            descriptions.append(
                QStringLiteral("%1 = %2")
                    .arg(
                        iterator.key(),
                        value
                        )
                );
        }
    }

    customFiltersButton->setToolTip(
        descriptions.join(
            QStringLiteral("\n")
            )
        );
}

void MainWindow::
    updateTimeRangeButton()
{
    const bool hasStart =
        timeRangeStartCheckBox
        && timeRangeStartCheckBox
               ->isChecked();

    const bool hasEnd =
        timeRangeEndCheckBox
        && timeRangeEndCheckBox
               ->isChecked();

    if (!hasStart && !hasEnd) {
        timeRangeButton->setText(
            tr("Time Range")
            );

        timeRangeButton->setToolTip(
            tr("No time-range filter active")
            );

        return;
    }

    QStringList parts;

    if (hasStart) {
        parts.append(
            tr("From %1")
                .arg(
                    timeRangeStartEdit
                        ->dateTime()
                        .toString(
                            QStringLiteral(
                                "yyyy-MM-dd HH:mm:ss.zzz"
                                )
                            )
                    )
            );
    }

    if (hasEnd) {
        parts.append(
            tr("To %1")
                .arg(
                    timeRangeEndEdit
                        ->dateTime()
                        .toString(
                            QStringLiteral(
                                "yyyy-MM-dd HH:mm:ss.zzz"
                                )
                            )
                    )
            );
    }

    timeRangeButton->setText(
        tr("Time Range (Active)")
        );

    timeRangeButton->setToolTip(
        parts.join(
            QStringLiteral("\n")
            )
        );
}

QGroupBox *MainWindow::buildDetailPanel()
{
    auto *detailGroup =
        new QGroupBox("Selected Event Details", this);

    eventDetailText->setReadOnly(true);
    eventDetailText->setPlaceholderText(
        "Select a telemetry event to view its details."
        );

    auto *detailLayout = new QVBoxLayout(detailGroup);

    auto *bookmarkLayout =
        new QHBoxLayout();

    bookmarkLayout->addStretch();

    bookmarkButton->setEnabled(false);

    bookmarkLayout->addWidget(
        bookmarkButton
        );

    detailLayout->addLayout(
        bookmarkLayout
        );

    connect(
        bookmarkButton,
        &QPushButton::clicked,
        this,
        [this]() {
            toggleSelectedEventBookmark();
        }
        );

    detailLayout->addWidget(eventDetailText);

    return detailGroup;
}

void MainWindow::updateEventDetailFromSelection()
{
    const InvestigationRecord *record =
        selectedEventRecord();

    if (record == nullptr) {
        clearEventDetail();
        return;
    }

    displayEventDetail(
        *record
        );

    updateBookmarkButton();
}

void MainWindow::displayEventDetail(
    const InvestigationRecord &record
    )
{
    QStringList lines;

    lines << "Timestamp: "
                 + (
                     record.timestamp.has_value()
                         ? record.timestamp->toString(
                               Qt::ISODateWithMs
                               )
                         : QString()
                     );

    lines << "Level: "
                 + (
                     record.severity.has_value()
                         ? recordSeverityToString(
                               record.severity.value()
                               )
                         : QString()
                     );

    lines << "Subsystem: "
                 + record.subsystem.value_or(
                     QString()
                     );

    lines << "Event Code: "
                 + record.eventCode.value_or(
                     QString()
                     );

    lines << "Entity ID: "
                 + record.entityId.value_or(
                     QString()
                     );

    lines << "";
    lines << "Message:";
    lines << record.message.value_or(
        QString()
        );

    if (!record.customAttributes.isEmpty()) {
        lines << "";
        lines << "Custom Attributes:";

        QStringList attributeKeys =
            record.customAttributes.keys();

        std::sort(
            attributeKeys.begin(),
            attributeKeys.end(),
            [](const QString &left, const QString &right) {
                return left.compare(
                           right,
                           Qt::CaseInsensitive
                           ) < 0;
            }
            );

        for (
            const QString &key :
            std::as_const(attributeKeys)
            ) {
            lines << QString("%1: %2")
            .arg(
                key,
                record.customAttributes
                    .value(key)
                    .toString()
                );
        }
    }

    eventDetailText->setPlainText(
        lines.join("\n")
        );
}

void MainWindow::clearEventDetail()
{
    eventDetailText->clear();

    bookmarkButton->setEnabled(false);

    bookmarkButton->setText(
        tr("Bookmark Event")
        );
}

const InvestigationRecord *
MainWindow::selectedEventRecord() const
{
    if (investigationController == nullptr
        || eventTable->selectionModel()
               == nullptr) {
        return nullptr;
    }

    const QModelIndexList selectedRows =
        eventTable
            ->selectionModel()
            ->selectedRows();

    if (selectedRows.isEmpty()) {
        return nullptr;
    }

    return investigationController
        ->recordForProxyIndex(
            selectedRows.first()
            );
}

void MainWindow::updateBookmarkButton()
{
    InvestigationSession *session =
        workspace->activeSession();

    const InvestigationRecord *record =
        selectedEventRecord();

    if (session == nullptr
        || record == nullptr
        || record->recordId.isEmpty()) {
        bookmarkButton->setEnabled(false);

        bookmarkButton->setText(
            tr("Bookmark Event")
            );

        return;
    }

    const InvestigationRecordState state =
        session
            ->investigationStateStore()
            ->stateForRecord(
                record->recordId
                );

    bookmarkButton->setEnabled(true);

    bookmarkButton->setText(
        state.bookmarked
            ? tr("Remove Bookmark")
            : tr("Bookmark Event")
        );
}

void MainWindow::syncBookmarkPresentation()
{
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr
        || investigationController
               == nullptr) {
        return;
    }

    investigationController
        ->proxyModel()
        ->setBookmarkedRecordIds(
            session
                ->investigationStateStore()
                ->bookmarkedRecordIds()
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

    syncBookmarkPresentation();
    updateBookmarkButton();
}

void MainWindow::navigateToAdjacentIssue(
    int direction
    )
{
    if (investigationController == nullptr
        || eventTable->model() == nullptr) {
        return;
    }

    int currentProxyRow = -1;

    const QModelIndex currentIndex =
        eventTable->currentIndex();

    if (currentIndex.isValid()) {
        currentProxyRow =
            currentIndex.row();
    }

    const int targetProxyRow =
        investigationController
            ->adjacentIssueProxyRow(
                currentProxyRow,
                direction
                );

    selectProxyRow(
        targetProxyRow
        );
}

void MainWindow::navigateToAdjacentEvent(
    int direction
    )
{
    if (investigationController == nullptr
        || eventTable->model() == nullptr) {
        return;
    }

    const QModelIndex currentIndex =
        eventTable->currentIndex();

    const int currentProxyRow =
        currentIndex.isValid()
            ? currentIndex.row()
            : -1;

    const int targetProxyRow =
        investigationController
            ->adjacentVisibleProxyRow(
                currentProxyRow,
                direction
                );

    selectProxyRow(
        targetProxyRow
        );
}

void MainWindow::selectProxyRow(
    int proxyRow
    )
{
    if (investigationController == nullptr
        || proxyRow < 0) {
        return;
    }

    const QModelIndex targetIndex =
        investigationController
            ->proxyModel()
            ->index(
                proxyRow,
                0
                );

    if (!targetIndex.isValid()) {
        return;
    }

    QItemSelectionModel *selectionModel =
        eventTable->selectionModel();

    if (selectionModel == nullptr) {
        return;
    }

    selectionModel->setCurrentIndex(
        targetIndex,
        QItemSelectionModel::ClearAndSelect
            | QItemSelectionModel::Rows
        );

    eventTable->scrollTo(
        targetIndex,
        QAbstractItemView::PositionAtCenter
        );
}

void MainWindow::updateEventNavigationState()
{
    const bool hasInvestigation =
        investigationController != nullptr
        && eventTable->model() != nullptr;

    previousEventButton->setVisible(
        hasInvestigation
        );

    nextEventButton->setVisible(
        hasInvestigation
        );

    eventPositionLabel->setVisible(
        hasInvestigation
        );

    if (!hasInvestigation) {
        previousEventButton->setEnabled(
            false
            );

        nextEventButton->setEnabled(
            false
            );

        eventPositionLabel->clear();

        return;
    }

    const int visibleCount =
        eventTable->model()->rowCount();

    const QModelIndex currentIndex =
        eventTable->currentIndex();

    if (!currentIndex.isValid()) {
        const bool hasVisibleEvents =
            visibleCount > 0;

        previousEventButton->setEnabled(
            hasVisibleEvents
            );

        nextEventButton->setEnabled(
            hasVisibleEvents
            );

        eventPositionLabel->setText(
            tr("%1 visible events")
                .arg(
                    visibleCount
                    )
            );

        return;
    }

    const int currentRow =
        currentIndex.row();

    previousEventButton->setEnabled(
        currentRow > 0
        );

    nextEventButton->setEnabled(
        currentRow
        < visibleCount - 1
        );

    QString positionText =
        tr("Event %1 of %2 visible")
            .arg(
                currentRow + 1
                )
            .arg(
                visibleCount
                );

    const InvestigationRecord *record =
        investigationController
            ->recordForProxyIndex(
                currentIndex
                );

    if (record != nullptr) {
        positionText +=
            tr(" • Source record %1")
                .arg(
                    record
                        ->source
                        .recordNumber
                    );
    }

    eventPositionLabel->setText(
        positionText
        );
}

QGroupBox *MainWindow::buildIssueSummaryPanel()
{
    auto *issueSummaryGroup =
        new QGroupBox("Grouped Warnings and Errors", this);

    issueSummaryTable->setColumnCount(4);
    issueSummaryTable->setHorizontalHeaderLabels({
        "Subsystem",
        "Warnings",
        "Errors",
        "Total"
    });

    issueSummaryTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
        );

    issueSummaryTable->horizontalHeader()->setStretchLastSection(false);
    issueSummaryTable->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents
        );

    const int requiredTableWidth =
        issueSummaryTable->verticalHeader()->width()
        + issueSummaryTable->horizontalHeader()->length()
        + issueSummaryTable->frameWidth() * 2
        + issueSummaryTable->verticalScrollBar()->sizeHint().width()
        + 8;

    issueSummaryTable->setMinimumWidth(requiredTableWidth);
    issueSummaryTable->setAlternatingRowColors(true);

    issueSummaryTable->setSelectionBehavior(
        QAbstractItemView::SelectItems
        );

    issueSummaryTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    issueSummaryTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    issueSummaryTable->setToolTip(
        tr(
            "Double-click a summary value to filter "
            "the investigation to the represented issues."
            )
        );

    connect(
        issueSummaryTable,
        &QTableWidget::cellDoubleClicked,
        this,
        [this](
            int row,
            int column
            ) {
            drillDownIssueSummary(
                row,
                column
                );
        },
        Qt::QueuedConnection
        );

    auto *issueLayout = new QVBoxLayout(issueSummaryGroup);
    issueLayout->addWidget(issueSummaryTable);

    return issueSummaryGroup;
}

void MainWindow::updateIssueSummary(
    const QVector<InvestigationRecord> &records
    )
{
    if (!hasSeverityData
        || !hasSubsystemData) {
        issueSummaryTable->setRowCount(
            0
            );

        return;
    }

    const QVector<TelemetryIssueGroup> groups =
        issueAnalyzer
            .groupWarningsAndErrorsBySubsystem(
                records
                );

    issueSummaryTable->setRowCount(
        groups.size()
        );

    for (int row = 0;
         row < groups.size();
         ++row) {
        const TelemetryIssueGroup &group =
            groups[row];

        auto *subsystemItem =
            new QTableWidgetItem(
                group.subsystem
                );

        auto *warningItem =
            new QTableWidgetItem(
                QString::number(
                    group.warningCount
                    )
                );

        auto *errorItem =
            new QTableWidgetItem(
                QString::number(
                    group.errorCount
                    )
                );

        auto *totalItem =
            new QTableWidgetItem(
                QString::number(
                    group.totalCount()
                    )
                );

        /*
         * Records without a subsystem are grouped
         * together by the analyzer for presentation,
         * but the exact-value subsystem filter cannot
         * currently express "missing subsystem".
         *
         * Do not advertise a drill-down interaction
         * that cannot be reproduced by the active
         * filter controls.
         */
        if (group.subsystem
            == QStringLiteral("(No subsystem)")) {
            const QString unavailableToolTip =
                tr(
                    "Drill-down is unavailable because "
                    "these records do not contain a "
                    "subsystem value."
                    );

            subsystemItem->setToolTip(
                unavailableToolTip
                );

            warningItem->setToolTip(
                unavailableToolTip
                );

            errorItem->setToolTip(
                unavailableToolTip
                );

            totalItem->setToolTip(
                unavailableToolTip
                );
        } else {
            subsystemItem->setToolTip(
                tr(
                    "Double-click to show "
                    "warning/error-class events "
                    "for this subsystem."
                    )
                );

            warningItem->setToolTip(
                group.warningCount > 0
                    ? tr(
                          "Double-click to show "
                          "warnings for this subsystem."
                          )
                    : tr(
                          "No warnings are currently "
                          "visible for this subsystem."
                          )
                );

            errorItem->setToolTip(
                group.errorCount > 0
                    ? tr(
                          "Double-click to show errors "
                          "and critical events for this "
                          "subsystem."
                          )
                    : tr(
                          "No errors or critical events "
                          "are currently visible for "
                          "this subsystem."
                          )
                );

            totalItem->setToolTip(
                tr(
                    "Double-click to show all "
                    "warning/error-class events "
                    "for this subsystem."
                    )
                );
        }

        issueSummaryTable->setItem(
            row,
            0,
            subsystemItem
            );

        issueSummaryTable->setItem(
            row,
            1,
            warningItem
            );

        issueSummaryTable->setItem(
            row,
            2,
            errorItem
            );

        issueSummaryTable->setItem(
            row,
            3,
            totalItem
            );
    }

    issueSummaryTable
        ->horizontalHeader()
        ->resizeSections(
            QHeaderView::ResizeToContents
            );

    const int requiredTableWidth =
        issueSummaryTable
            ->verticalHeader()
            ->width()
        + issueSummaryTable
              ->horizontalHeader()
              ->length()
        + issueSummaryTable
                  ->frameWidth()
              * 2
        + issueSummaryTable
              ->verticalScrollBar()
              ->sizeHint()
              .width()
        + 8;

    issueSummaryTable->setMinimumWidth(
        requiredTableWidth
        );
}

void MainWindow::drillDownIssueSummary(
    int row,
    int column
    )
{
    if (investigationController == nullptr
        || row < 0
        || row >= issueSummaryTable->rowCount()
        || column < 0
        || column >= issueSummaryTable->columnCount()) {
        return;
    }

    QTableWidgetItem *subsystemItem =
        issueSummaryTable->item(
            row,
            0
            );

    if (subsystemItem == nullptr) {
        return;
    }

    const QString subsystem =
        subsystemItem->text();

    /*
     * The grouped analyzer uses this display label
     * for records without a subsystem. The current
     * exact-value subsystem filter cannot express
     * "missing subsystem", so do not pretend this
     * drill-down can reproduce that group.
     */
    if (subsystem
        == QStringLiteral("(No subsystem)")) {
        return;
    }

    QStringList targetSeverities;

    switch (column) {
    case 1:
    {
        QTableWidgetItem *warningItem =
            issueSummaryTable->item(
                row,
                1
                );

        if (warningItem == nullptr
            || warningItem
                       ->text()
                       .toInt() <= 0) {
            return;
        }

        targetSeverities = {
            QStringLiteral("WARN")
    };

        break;
    }

    case 2:
    {
        QTableWidgetItem *errorItem =
            issueSummaryTable->item(
                row,
                2
                );

        if (errorItem == nullptr
            || errorItem
                   ->text()
                   .toInt() <= 0) {
            return;
        }

        /*
         * Existing grouped analysis intentionally
         * counts CRITICAL together with ERROR.
         */
        targetSeverities = {
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };

        break;
    }

    case 0:
    case 3:
        targetSeverities = {
            QStringLiteral("WARN"),
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        };
        break;

    default:
        return;
    }

    /*
     * Drill-down must narrow the current result
     * set rather than silently broaden an existing
     * severity filter.
     */
    const QStringList currentSeverities =
        levelFilterCombo
            ->selectedValues();

    if (!currentSeverities.isEmpty()) {
        QStringList intersection;

        for (
            auto severityIterator =
            targetSeverities.cbegin();
            severityIterator
            != targetSeverities.cend();
            ++severityIterator
            ) {
            if (currentSeverities.contains(
                    *severityIterator
                    )) {
                intersection.append(
                    *severityIterator
                    );
            }
        }

        if (intersection.isEmpty()) {
            return;
        }

        targetSeverities =
            std::move(
                intersection
                );
    }

    searchDebounceTimer->stop();

    /*
     * Synchronize both affected controls before
     * applying the complete filter state once.
     */
    {
        const QSignalBlocker severityBlocker(
            levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            subsystemFilterCombo
            );

        levelFilterCombo->setSelectedValues(
            targetSeverities
            );

        subsystemFilterCombo->setSelectedValues(
            {
                subsystem
            }
            );
    }

    applyFilters();
}

void MainWindow::drillDownTimelineBucket(
    int visibleBucketIndex,
    const QString &severity
    )
{
    if (investigationController == nullptr
        || !hasTimestampData
        || visibleBucketIndex < 0) {
        return;
    }

    const std::optional<QDateTime>
        effectiveFirstTimestamp =
        effectiveTimelineFirstTimestamp();

    const std::optional<QDateTime>
        effectiveLastTimestamp =
        effectiveTimelineLastTimestamp();

    if (!effectiveFirstTimestamp.has_value()
        || !effectiveLastTimestamp.has_value()
        || effectiveFirstTimestamp.value()
               > effectiveLastTimestamp.value()) {
        return;
    }

    const qint64 requestedIntervalMilliseconds =
        timelineIntervalCombo != nullptr
            ? timelineIntervalCombo
                  ->currentData()
                  .toLongLong()
            : 0;

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  *effectiveFirstTimestamp,
                  *effectiveLastTimestamp
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        return;
    }

    const qint64 totalBucketCount =
        timelineAnalyzer
            .intervalBucketCountMilliseconds(
                *effectiveFirstTimestamp,
                *effectiveLastTimestamp,
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        return;
    }

    qint64 startBucketIndex = 0;

    if (!automaticInterval) {
        const int scrollValue =
            timelineScrollBar != nullptr
                ? timelineScrollBar->value()
                : 0;

        startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                scrollValue
                );
    }

    const qint64 absoluteBucketIndex =
        startBucketIndex
        + visibleBucketIndex;

    if (absoluteBucketIndex < 0
        || absoluteBucketIndex
               >= totalBucketCount) {
        return;
    }

    const qint64 firstBucketEpoch =
        normalizedTimelineBucketEpoch(
            *effectiveFirstTimestamp,
            intervalMilliseconds
            );

    qint64 bucketStartEpoch =
        firstBucketEpoch
        + absoluteBucketIndex
              * intervalMilliseconds;

    qint64 bucketEndEpoch =
        bucketStartEpoch
        + intervalMilliseconds
        - 1;

    /*
     * The first and last logical buckets can extend
     * outside an already active investigation time
     * range. Drill-down must only narrow the current
     * result set, never broaden it.
     */
    bucketStartEpoch =
        std::max(
            bucketStartEpoch,
            effectiveFirstTimestamp
                ->toMSecsSinceEpoch()
            );

    bucketEndEpoch =
        std::min(
            bucketEndEpoch,
            effectiveLastTimestamp
                ->toMSecsSinceEpoch()
            );

    if (bucketStartEpoch
        > bucketEndEpoch) {
        return;
    }

    /*
     * A severity-aware bar represents one specific
     * severity. If an existing severity filter is
     * active, only allow drill-down when the clicked
     * severity is already included in that result.
     */
    if (!severity.isEmpty()) {
        const QStringList currentSeverities =
            levelFilterCombo
                ->selectedValues();

        if (!currentSeverities.isEmpty()
            && !currentSeverities.contains(
                severity
                )) {
            return;
        }
    }

    const QDateTime bucketStart =
        QDateTime::fromMSecsSinceEpoch(
            bucketStartEpoch,
            QTimeZone::UTC
            );

    const QDateTime bucketEnd =
        QDateTime::fromMSecsSinceEpoch(
            bucketEndEpoch,
            QTimeZone::UTC
            );

    searchDebounceTimer->stop();

    /*
     * Synchronize every affected control first so
     * the drill-down becomes one logical filter
     * application.
     */
    {
        const QSignalBlocker severityBlocker(
            levelFilterCombo
            );

        const QSignalBlocker startCheckBlocker(
            timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            timeRangeEndEdit
            );

        if (!severity.isEmpty()) {
            levelFilterCombo->setSelectedValues(
                QStringList {
                    severity
                }
                );
        }

        timeRangeStartEdit->setDateTime(
            bucketStart
            );

        timeRangeEndEdit->setDateTime(
            bucketEnd
            );

        timeRangeStartCheckBox->setChecked(
            true
            );

        timeRangeEndCheckBox->setChecked(
            true
            );

        timeRangeStartEdit->setEnabled(
            true
            );

        timeRangeEndEdit->setEnabled(
            true
            );
    }

    updateTimeRangeButton();

    applyFilters();
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

QGroupBox *MainWindow::buildTimelinePanel()
{
    auto *timelineGroup =
        new QGroupBox(
            tr("Event Counts Over Time"),
            this
            );

    auto *timelineLayout =
        new QVBoxLayout(
            timelineGroup
            );

    timelineLayout->setContentsMargins(
        4,
        2,
        4,
        2
        );

    timelineLayout->setSpacing(
        2
        );

    auto *controlsLayout =
        new QHBoxLayout();

    controlsLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    controlsLayout->setSpacing(
        6
        );

    auto *intervalLabel =
        new QLabel(
            tr("Bucket size:"),
            timelineGroup
            );

    timelineIntervalCombo =
        new QComboBox(
            timelineGroup
            );

    timelineIntervalCombo->addItem(
        tr("Auto"),
        QVariant::fromValue<qint64>(
            0
            )
        );

    timelineIntervalCombo->addItem(
        tr("1 ms"),
        QVariant::fromValue<qint64>(
            1
            )
        );

    timelineIntervalCombo->addItem(
        tr("10 ms"),
        QVariant::fromValue<qint64>(
            10
            )
        );

    timelineIntervalCombo->addItem(
        tr("100 ms"),
        QVariant::fromValue<qint64>(
            100
            )
        );

    timelineIntervalCombo->addItem(
        tr("500 ms"),
        QVariant::fromValue<qint64>(
            500
            )
        );

    timelineIntervalCombo->addItem(
        tr("1 second"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerSecond
            )
        );

    timelineIntervalCombo->addItem(
        tr("5 seconds"),
        QVariant::fromValue<qint64>(
            5 * MillisecondsPerSecond
            )
        );

    timelineIntervalCombo->addItem(
        tr("15 seconds"),
        QVariant::fromValue<qint64>(
            15 * MillisecondsPerSecond
            )
        );

    timelineIntervalCombo->addItem(
        tr("30 seconds"),
        QVariant::fromValue<qint64>(
            30 * MillisecondsPerSecond
            )
        );

    timelineIntervalCombo->addItem(
        tr("1 minute"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerMinute
            )
        );

    timelineIntervalCombo->addItem(
        tr("5 minutes"),
        QVariant::fromValue<qint64>(
            5 * MillisecondsPerMinute
            )
        );

    timelineIntervalCombo->addItem(
        tr("15 minutes"),
        QVariant::fromValue<qint64>(
            15 * MillisecondsPerMinute
            )
        );

    timelineIntervalCombo->addItem(
        tr("30 minutes"),
        QVariant::fromValue<qint64>(
            30 * MillisecondsPerMinute
            )
        );

    timelineIntervalCombo->addItem(
        tr("1 hour"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerHour
            )
        );

    timelineIntervalCombo->addItem(
        tr("3 hours"),
        QVariant::fromValue<qint64>(
            3 * MillisecondsPerHour
            )
        );

    timelineIntervalCombo->addItem(
        tr("6 hours"),
        QVariant::fromValue<qint64>(
            6 * MillisecondsPerHour
            )
        );

    timelineIntervalCombo->addItem(
        tr("1 day"),
        QVariant::fromValue<qint64>(
            1 * MillisecondsPerDay
            )
        );

    int intervalPopupWidth = 0;

    for (int index = 0;
         index < timelineIntervalCombo->count();
         ++index) {
        intervalPopupWidth =
            std::max(
                intervalPopupWidth,
                timelineIntervalCombo
                    ->fontMetrics()
                    .horizontalAdvance(
                        timelineIntervalCombo
                            ->itemText(index)
                        )
                );
    }

    timelineIntervalCombo
        ->view()
        ->setMinimumWidth(
            intervalPopupWidth + 40
            );

    timelineIntervalCombo->setToolTip(
        tr(
            "Auto chooses a readable interval for "
            "the complete investigation. Selecting "
            "a specific interval preserves that "
            "resolution and lets you navigate "
            "through the timeline horizontally."
            )
        );

    controlsLayout->addWidget(
        intervalLabel
        );

    controlsLayout->addWidget(
        timelineIntervalCombo
        );

    timelineRangeLabel =
        new QLabel(
            tr("Visible: —"),
            timelineGroup
            );

    timelineRangeLabel
        ->setTextInteractionFlags(
            Qt::TextSelectableByMouse
            );

    timelineRangeLabel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
        );

    controlsLayout->addSpacing(
        12
        );

    controlsLayout->addWidget(
        timelineRangeLabel,
        1
        );

    timelineLayout->addLayout(
        controlsLayout
        );

    timelineChartView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Ignored
        );

    timelineChartView->setMinimumHeight(
        0
        );

    timelineChartView->setFrameShape(
        QFrame::NoFrame
        );

    timelineChartView->setRenderHint(
        QPainter::Antialiasing
        );

    timelineChartView
        ->setHorizontalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff
            );

    timelineChartView
        ->setVerticalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff
            );

    timelineChartView->setAcceptDrops(
        false
        );

    timelineChartView->setToolTip(
        tr(
            "Double-click a bar to filter the "
            "investigation to that time bucket. "
            "Severity bars also filter to the "
            "selected severity."
            )
        );

    timelineChartView
        ->viewport()
        ->setAcceptDrops(
            false
            );

    timelineLayout->addWidget(
        timelineChartView,
        1
        );

    timelineScrollBar =
        new QScrollBar(
            Qt::Horizontal,
            timelineGroup
            );

    timelineScrollBar->setVisible(
        false
        );

    timelineScrollBar->setSingleStep(
        1
        );

    /*
     * Fine-resolution windows may require scanning
     * a large record collection. Do not redraw the
     * chart continuously while the user drags the
     * scrollbar thumb.
     */
    timelineScrollBar->setTracking(
        false
        );

    timelineLayout->addWidget(
        timelineScrollBar
        );

    connect(
        timelineScrollBar,
        &QScrollBar::valueChanged,
        this,
        [this](int) {
            const QVector<InvestigationRecord>
                visibleRecords =
                investigationController
                    ->visibleRecords();

            updateTimelineChart(
                visibleRecords
                );
        }
        );

    connect(
        timelineScrollBar,
        &QScrollBar::sliderMoved,
        this,
        [this](int value) {
            updateTimelineRangeLabel(
                value
                );
        }
        );

    connect(
        timelineIntervalCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            timelineScaleValid =
                false;

            if (timelineScrollBar != nullptr) {
                const QSignalBlocker blocker(
                    timelineScrollBar
                    );

                timelineScrollBar->setValue(
                    0
                    );
            }

            const QVector<InvestigationRecord>
                visibleRecords =
                investigationController
                    ->visibleRecords();

            updateTimelineChart(
                visibleRecords
                );
        }
        );

    return timelineGroup;
}

void MainWindow::updateTimelineChart(
    const QVector<InvestigationRecord> &records
    )
{
    auto showEmptyTimeline =
        [this]() {
            if (timelineScrollBar != nullptr) {
                const QSignalBlocker blocker(
                    timelineScrollBar
                    );

                timelineScrollBar->setRange(
                    0,
                    0
                    );

                timelineScrollBar->setValue(
                    0
                    );

                timelineScrollBar->setVisible(
                    false
                    );
            }

            if (timelineRangeLabel != nullptr) {
                timelineRangeLabel->setText(
                    tr("Visible: —")
                    );
            }

            auto *chart =
                new QChart();

            chart->setMargins(
                QMargins(
                    0,
                    0,
                    0,
                    0
                    )
                );

            chart->setTitle(
                tr("No events to display")
                );

            timelineChartView->setChart(
                chart
                );
        };

    /*
     * A timeline cannot be generated unless the
     * complete investigation has valid temporal
     * boundaries.
     */
    const std::optional<QDateTime>
        effectiveFirstTimestamp =
        effectiveTimelineFirstTimestamp();

    const std::optional<QDateTime>
        effectiveLastTimestamp =
        effectiveTimelineLastTimestamp();

    if (!effectiveFirstTimestamp.has_value()
        || !effectiveLastTimestamp.has_value()
        || effectiveFirstTimestamp.value()
               > effectiveLastTimestamp.value()) {
        showEmptyTimeline();
        return;
    }

    /*
     * Zero represents Auto. Every explicit combo
     * value is stored in milliseconds.
     */
    const qint64 requestedIntervalMilliseconds =
        timelineIntervalCombo != nullptr
            ? timelineIntervalCombo
                  ->currentData()
                  .toLongLong()
            : 0;

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  *effectiveFirstTimestamp,
                  *effectiveLastTimestamp
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        showEmptyTimeline();
        return;
    }

    /*
     * Calculate a stable Y-axis maximum for the
     * complete currently filtered investigation.
     *
     * This result is cached because moving the
     * horizontal timeline window must not require
     * rescanning every visible record.
     */
    if (!timelineScaleValid
        || timelineScaleIntervalMilliseconds
               != intervalMilliseconds) {
        const EventTimelineScale scale =
            timelineAnalyzer
                .scaleForIntervalMilliseconds(
                    records,
                    *effectiveFirstTimestamp,
                    *effectiveLastTimestamp,
                    intervalMilliseconds
                    );

        timelineScaleMaximum =
            std::max(
                1,
                hasSeverityData
                    ? scale.maximumSeriesCount
                    : scale.maximumTotalCount
                );

        timelineScaleIntervalMilliseconds =
            intervalMilliseconds;

        timelineScaleValid =
            true;
    }

    /*
     * Determine how many logical buckets exist
     * across the complete investigation without
     * actually materializing all of them.
     */
    const qint64 totalBucketCount =
        timelineAnalyzer
            .intervalBucketCountMilliseconds(
                *effectiveFirstTimestamp,
                *effectiveLastTimestamp,
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        showEmptyTimeline();
        return;
    }

    QVector<EventCountBucket> buckets;

    if (automaticInterval) {
        /*
         * Auto selects a resolution intended to
         * keep the complete overview compact, so
         * materializing its complete bucket set is
         * safe.
         */
        buckets =
            timelineAnalyzer
                .groupRecordsByIntervalMilliseconds(
                    records,
                    *effectiveFirstTimestamp,
                    *effectiveLastTimestamp,
                    intervalMilliseconds
                    );

        if (timelineScrollBar != nullptr) {
            const QSignalBlocker blocker(
                timelineScrollBar
                );

            timelineScrollBar->setRange(
                0,
                0
                );

            timelineScrollBar->setValue(
                0
                );

            timelineScrollBar->setVisible(
                false
                );
        }
    } else {
        /*
         * Explicit resolutions preserve the
         * selected detail level. Only the current
         * visible window is materialized.
         */
        const qint64 maximumStartBucketIndex =
            std::max<qint64>(
                0,
                totalBucketCount
                    - TimelineVisibleBucketCount
                );

        const int scrollMaximum =
            timelineScrollMaximum(
                totalBucketCount
                );

        int scrollValue = 0;

        if (timelineScrollBar != nullptr) {
            scrollValue =
                std::clamp(
                    timelineScrollBar->value(),
                    0,
                    scrollMaximum
                    );

            int pageStep = 1;

            /*
             * When every scrollbar position maps
             * directly to one logical bucket, make
             * Page Up/Down move approximately one
             * visible window.
             */
            if (maximumStartBucketIndex
                <= std::numeric_limits<int>::max()) {
                pageStep =
                    std::max(
                        1,
                        std::min(
                            TimelineVisibleBucketCount,
                            std::max(
                                1,
                                scrollMaximum
                                )
                            )
                        );
            } else if (scrollMaximum > 0) {
                /*
                 * Extremely large logical ranges
                 * use a scaled integer scrollbar.
                 */
                const long double pageFraction =
                    static_cast<long double>(
                        TimelineVisibleBucketCount
                        )
                    / static_cast<long double>(
                        totalBucketCount
                        );

                pageStep =
                    std::max(
                        1,
                        static_cast<int>(
                            std::llround(
                                static_cast<long double>(
                                    scrollMaximum
                                    )
                                * pageFraction
                                )
                            )
                        );
            }

            {
                const QSignalBlocker blocker(
                    timelineScrollBar
                    );

                timelineScrollBar->setRange(
                    0,
                    scrollMaximum
                    );

                timelineScrollBar->setSingleStep(
                    1
                    );

                timelineScrollBar->setPageStep(
                    pageStep
                    );

                timelineScrollBar->setValue(
                    scrollValue
                    );
            }

            timelineScrollBar->setVisible(
                maximumStartBucketIndex > 0
                );
        }

        const qint64 startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                scrollValue
                );

        /*
         * Only the visible window is constructed.
         * A multi-hour 1-ms investigation can
         * therefore contain millions of logical
         * buckets without millions of chart
         * objects being allocated.
         */
        buckets =
            timelineAnalyzer
                .groupRecordsByIntervalWindowMilliseconds(
                    records,
                    *effectiveFirstTimestamp,
                    *effectiveLastTimestamp,
                    intervalMilliseconds,
                    startBucketIndex,
                    TimelineVisibleBucketCount
                    );
    }

    if (buckets.isEmpty()) {
        showEmptyTimeline();
        return;
    }

    /*
     * When severity is unavailable, render one
     * TOTAL series.
     */
    if (!hasSeverityData) {
        auto *totalSet =
            new QBarSet(
                tr("TOTAL")
                );

        connect(
            totalSet,
            &QBarSet::doubleClicked,
            this,
            [this](
                int index
                ) {
                drillDownTimelineBucket(
                    index,
                    QString()
                    );
            },
            Qt::QueuedConnection
            );

        QStringList categories;

        for (const EventCountBucket &bucket
             : std::as_const(buckets)) {
            categories.append(
                timelineDisplayLabel(
                    bucket.label,
                    intervalMilliseconds
                    )
                );

            *totalSet
                << bucket.totalCount();
        }

        auto *series =
            new QBarSeries();

        series->append(
            totalSet
            );

        auto *chart =
            new QChart();

        chart->setMargins(
            QMargins(
                0,
                0,
                0,
                0
                )
            );

        chart->addSeries(
            series
            );

        chart->setAnimationOptions(
            QChart::NoAnimation
            );

        /*
         * A single TOTAL series needs no legend.
         */
        chart->legend()->setVisible(
            false
            );

        auto *axisX =
            new QBarCategoryAxis();

        axisX->append(
            categories
            );

        axisX->setTruncateLabels(
            false
            );

        chart->addAxis(
            axisX,
            Qt::AlignBottom
            );

        series->attachAxis(
            axisX
            );

        auto *axisY =
            new QValueAxis();

        /*
         * Use the maximum calculated from the
         * complete filtered investigation, not
         * only this horizontal window.
         */
        configureEventCountAxis(
            axisY,
            timelineScaleMaximum
            );

        chart->addAxis(
            axisY,
            Qt::AlignLeft
            );

        series->attachAxis(
            axisY
            );

        timelineChartView->setChart(
            chart
            );

        updateTimelineRangeLabel(
            timelineScrollBar != nullptr
                ? timelineScrollBar->value()
                : 0
            );

        return;
    }

    /*
     * Severity-aware timeline.
     */
    auto *traceSet =
        new QBarSet(
            tr("TRACE")
            );

    auto *debugSet =
        new QBarSet(
            tr("DEBUG")
            );

    auto *infoSet =
        new QBarSet(
            tr("INFO")
            );

    auto *warnSet =
        new QBarSet(
            tr("WARN")
            );

    auto *errorSet =
        new QBarSet(
            tr("ERROR")
            );

    auto *criticalSet =
        new QBarSet(
            tr("CRITICAL")
            );

    auto connectTimelineDrillDown =
        [this](
            QBarSet *barSet,
            const QString &severity
            ) {
            connect(
                barSet,
                &QBarSet::doubleClicked,
                this,
                [
                    this,
                    severity
            ](
                    int index
                    ) {
                    drillDownTimelineBucket(
                        index,
                        severity
                        );
                },
                Qt::QueuedConnection
                );
        };

    connectTimelineDrillDown(
        traceSet,
        QStringLiteral("TRACE")
        );

    connectTimelineDrillDown(
        debugSet,
        QStringLiteral("DEBUG")
        );

    connectTimelineDrillDown(
        infoSet,
        QStringLiteral("INFO")
        );

    connectTimelineDrillDown(
        warnSet,
        QStringLiteral("WARN")
        );

    connectTimelineDrillDown(
        errorSet,
        QStringLiteral("ERROR")
        );

    connectTimelineDrillDown(
        criticalSet,
        QStringLiteral("CRITICAL")
        );

    bool hasUnspecifiedEvents =
        false;

    for (const EventCountBucket &bucket
         : std::as_const(buckets)) {
        if (bucket.unspecifiedCount > 0) {
            hasUnspecifiedEvents =
                true;

            break;
        }
    }

    QBarSet *unspecifiedSet =
        nullptr;

    if (hasUnspecifiedEvents) {
        unspecifiedSet =
            new QBarSet(
                tr("UNSPECIFIED")
                );
    }

    if (unspecifiedSet != nullptr) {
        connectTimelineDrillDown(
            unspecifiedSet,
            QString()
            );
    }

    QStringList categories;

    for (const EventCountBucket &bucket
         : std::as_const(buckets)) {
        categories.append(
            timelineDisplayLabel(
                bucket.label,
                intervalMilliseconds
                )
            );

        *traceSet
            << bucket.traceCount;

        *debugSet
            << bucket.debugCount;

        *infoSet
            << bucket.infoCount;

        *warnSet
            << bucket.warningCount;

        *errorSet
            << bucket.errorCount;

        *criticalSet
            << bucket.criticalCount;

        if (unspecifiedSet != nullptr) {
            *unspecifiedSet
                << bucket.unspecifiedCount;
        }
    }

    auto *series =
        new QBarSeries();

    series->append(
        traceSet
        );

    series->append(
        debugSet
        );

    series->append(
        infoSet
        );

    series->append(
        warnSet
        );

    series->append(
        errorSet
        );

    series->append(
        criticalSet
        );

    if (unspecifiedSet != nullptr) {
        series->append(
            unspecifiedSet
            );
    }

    auto *chart =
        new QChart();

    chart->setMargins(
        QMargins(
            0,
            0,
            0,
            0
            )
        );

    chart->addSeries(
        series
        );

    chart->setAnimationOptions(
        QChart::NoAnimation
        );

    /*
     * Keep the severity key beneath the chart.
     * The timeline panel is too short for all
     * severity values to fit reliably in a
     * right-hand vertical legend.
     */
    QLegend *legend =
        chart->legend();

    legend->setAlignment(
        Qt::AlignBottom
        );

    legend->setContentsMargins(
        0,
        0,
        0,
        0
        );

    if (legend->layout() != nullptr) {
        legend->layout()
        ->setContentsMargins(
            0,
            0,
            0,
            0
            );
    }

    auto *axisX =
        new QBarCategoryAxis();

    axisX->append(
        categories
        );

    axisX->setTruncateLabels(
        false
        );

    chart->addAxis(
        axisX,
        Qt::AlignBottom
        );

    series->attachAxis(
        axisX
        );

    auto *axisY =
        new QValueAxis();

    /*
     * Keep the Y-axis scale fixed while scrolling.
     */
    configureEventCountAxis(
        axisY,
        timelineScaleMaximum
        );

    chart->addAxis(
        axisY,
        Qt::AlignLeft
        );

    series->attachAxis(
        axisY
        );

    timelineChartView->setChart(
        chart
        );

    updateTimelineRangeLabel(
        timelineScrollBar != nullptr
            ? timelineScrollBar->value()
            : 0
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

void MainWindow::updateTimelineRangeLabel(
    int scrollValue
    )
{
    if (timelineRangeLabel == nullptr) {
        return;
    }

    const std::optional<QDateTime>
        effectiveFirstTimestamp =
        effectiveTimelineFirstTimestamp();

    const std::optional<QDateTime>
        effectiveLastTimestamp =
        effectiveTimelineLastTimestamp();

    if (!effectiveFirstTimestamp.has_value()
        || !effectiveLastTimestamp.has_value()
        || effectiveFirstTimestamp.value()
               > effectiveLastTimestamp.value()) {
        timelineRangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    const qint64 requestedIntervalMilliseconds =
        timelineIntervalCombo != nullptr
            ? timelineIntervalCombo
                  ->currentData()
                  .toLongLong()
            : 0;

    const bool automaticInterval =
        requestedIntervalMilliseconds <= 0;

    const qint64 intervalMilliseconds =
        automaticInterval
            ? automaticTimelineIntervalMilliseconds(
                  *effectiveFirstTimestamp,
                  *effectiveLastTimestamp
                  )
            : requestedIntervalMilliseconds;

    if (intervalMilliseconds <= 0) {
        timelineRangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    const qint64 totalBucketCount =
        timelineAnalyzer
            .intervalBucketCountMilliseconds(
                *effectiveFirstTimestamp,
                *effectiveLastTimestamp,
                intervalMilliseconds
                );

    if (totalBucketCount <= 0) {
        timelineRangeLabel->setText(
            tr("Visible: —")
            );

        return;
    }

    qint64 startBucketIndex = 0;
    qint64 visibleBucketCount =
        totalBucketCount;

    if (!automaticInterval) {
        startBucketIndex =
            timelineStartBucketIndex(
                totalBucketCount,
                scrollValue
                );

        visibleBucketCount =
            std::min<qint64>(
                TimelineVisibleBucketCount,
                totalBucketCount
                    - startBucketIndex
                );
    }

    const qint64 firstBucketEpoch =
        normalizedTimelineBucketEpoch(
            *effectiveFirstTimestamp,
            intervalMilliseconds
            );

    qint64 visibleFirstEpoch =
        firstBucketEpoch
        + startBucketIndex
              * intervalMilliseconds;

    visibleFirstEpoch =
        std::max(
            visibleFirstEpoch,
            effectiveFirstTimestamp
                ->toMSecsSinceEpoch()
            );

    qint64 visibleLastEpoch =
        visibleFirstEpoch
        + visibleBucketCount
              * intervalMilliseconds
        - 1;

    visibleLastEpoch =
        std::min(
            visibleLastEpoch,
            effectiveLastTimestamp
                ->toMSecsSinceEpoch()
            );

    const QDateTime visibleFirst =
        QDateTime::fromMSecsSinceEpoch(
            visibleFirstEpoch,
            QTimeZone::UTC
            );

    const QDateTime visibleLast =
        QDateTime::fromMSecsSinceEpoch(
            visibleLastEpoch,
            QTimeZone::UTC
            );

    QString firstText;
    QString lastText;

    if (intervalMilliseconds < 1000) {
        firstText =
            visibleFirst.toString(
                QStringLiteral(
                    "yyyy-MM-dd HH:mm:ss.zzz"
                    )
                );

        if (visibleFirst.date()
            == visibleLast.date()) {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "HH:mm:ss.zzz"
                        )
                    );
        } else {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "yyyy-MM-dd HH:mm:ss.zzz"
                        )
                    );
        }
    } else {
        firstText =
            visibleFirst.toString(
                QStringLiteral(
                    "yyyy-MM-dd HH:mm:ss"
                    )
                );

        if (visibleFirst.date()
            == visibleLast.date()) {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "HH:mm:ss"
                        )
                    );
        } else {
            lastText =
                visibleLast.toString(
                    QStringLiteral(
                        "yyyy-MM-dd HH:mm:ss"
                        )
                    );
        }
    }

    const QString rangeText =
        tr("Visible: %1 – %2 UTC")
            .arg(
                firstText,
                lastText
                );

    timelineRangeLabel->setText(
        rangeText
        );

    if (timelineScrollBar != nullptr) {
        timelineScrollBar->setToolTip(
            rangeText
            );
    }
}

std::optional<QDateTime>
MainWindow::effectiveTimelineFirstTimestamp() const
{
    if (!timelineFirstTimestamp.has_value()) {
        return std::nullopt;
    }

    QDateTime effective =
        timelineFirstTimestamp.value();

    if (investigationController == nullptr) {
        return effective;
    }

    const std::optional<QDateTime> &filterStart =
        investigationController
            ->proxyModel()
            ->timeRangeStart();

    if (filterStart.has_value()
        && filterStart.value() > effective) {
        effective =
            filterStart.value();
    }

    return effective;
}

std::optional<QDateTime>
MainWindow::effectiveTimelineLastTimestamp() const
{
    if (!timelineLastTimestamp.has_value()) {
        return std::nullopt;
    }

    QDateTime effective =
        timelineLastTimestamp.value();

    if (investigationController == nullptr) {
        return effective;
    }

    const std::optional<QDateTime> &filterEnd =
        investigationController
            ->proxyModel()
            ->timeRangeEnd();

    if (filterEnd.has_value()
        && filterEnd.value() < effective) {
        effective =
            filterEnd.value();
    }

    return effective;
}

void MainWindow::updateDataCapabilities()
{
    hasSeverityData = false;
    hasSubsystemData = false;
    hasTimestampData = false;
    hasEventCodeData = false;
    hasEntityData = false;
    hasCustomFieldData = false;

    timelineFirstTimestamp.reset();
    timelineLastTimestamp.reset();

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        levelFilterCombo->setVisible(
            false
            );

        subsystemFilterCombo->setVisible(
            false
            );

        eventCodeFilterWidget->setVisible(
            false
            );

        entityFilterWidget->setVisible(
            false
            );

        timeRangeButton->setVisible(
            false
            );

        customFiltersButton->setVisible(
            false
            );

        previousIssueButton->setVisible(
            false
            );

        nextIssueButton->setVisible(
            false
            );

        return;
    }

    /*
     * Determine which canonical and dynamic
     * investigation capabilities actually exist
     * in the active session.
     */
    hasSeverityData =
        session->hasSeverityData();

    hasSubsystemData =
        session->hasSubsystemData();

    hasEventCodeData =
        session->hasEventCodeData();

    hasEntityData =
        session->hasEntityData();

    hasCustomFieldData =
        session->hasCustomFieldData();

    timelineFirstTimestamp =
        session->firstTimestamp();

    timelineLastTimestamp =
        session->lastTimestamp();

    hasTimestampData =
        timelineFirstTimestamp.has_value()
        && timelineLastTimestamp.has_value();

    /*
     * The custom editor only needs the names of
     * fields available in this investigation.
     * Individual values remain user-supplied and
     * are not enumerated eagerly.
     */
    customFieldFilterEditor
        ->setAvailableFields(
            hasCustomFieldData
                ? session
                      ->availableCustomFields()
                : QStringList()
            );

    /*
     * Clear filter controls that are no longer
     * meaningful for the active investigation.
     *
     * These operations are UI synchronization,
     * not user filter changes, so suppress their
     * signals. bindActiveSession()/applyFilters()
     * will perform one coherent refresh afterward.
     */
    if (!hasSeverityData) {
        const QSignalBlocker blocker(
            levelFilterCombo
            );

        levelFilterCombo
            ->clearSelection();
    }

    if (!hasSubsystemData) {
        const QSignalBlocker blocker(
            subsystemFilterCombo
            );

        subsystemFilterCombo
            ->clearSelection();
    }

    if (!hasEventCodeData) {
        const QSignalBlocker blocker(
            eventCodeFilterCombo
            );

        eventCodeFilterCombo
            ->clearSelection();
    }

    if (!hasEntityData) {
        const QSignalBlocker blocker(
            entityFilterCombo
            );

        entityFilterCombo
            ->clearSelection();
    }

    if (!hasCustomFieldData) {
        const QSignalBlocker blocker(
            customFieldFilterEditor
            );

        customFieldFilterEditor
            ->clearFilters();

        updateCustomFiltersButton();
    }

    /*
     * A time-range filter cannot remain active
     * when the investigation has no usable
     * timestamp domain.
     */
    if (!hasTimestampData) {
        const QSignalBlocker
            startCheckBlocker(
                timeRangeStartCheckBox
                );

        const QSignalBlocker
            startEditBlocker(
                timeRangeStartEdit
                );

        const QSignalBlocker
            endCheckBlocker(
                timeRangeEndCheckBox
                );

        const QSignalBlocker
            endEditBlocker(
                timeRangeEndEdit
                );

        timeRangeStartCheckBox
            ->setChecked(
                false
                );

        timeRangeEndCheckBox
            ->setChecked(
                false
                );

        timeRangeStartEdit
            ->setEnabled(
                false
                );

        timeRangeEndEdit
            ->setEnabled(
                false
                );

        updateTimeRangeButton();
    }

    /*
     * The compact two-row filter bar exposes only
     * controls that are meaningful for the active
     * investigation.
     */
    levelFilterCombo->setVisible(
        hasSeverityData
        );

    subsystemFilterCombo->setVisible(
        hasSubsystemData
        );

    eventCodeFilterWidget->setVisible(
        hasEventCodeData
        );

    entityFilterWidget->setVisible(
        hasEntityData
        );

    timeRangeButton->setVisible(
        hasTimestampData
        );

    customFiltersButton->setVisible(
        hasCustomFieldData
        );

    previousIssueButton->setVisible(
        hasSeverityData
        );

    nextIssueButton->setVisible(
        hasSeverityData
        );

    /*
     * Warning/error grouping requires both
     * severity and subsystem information.
     */
    if (issueSummaryGroup != nullptr) {
        issueSummaryGroup->setVisible(
            hasSeverityData
            && hasSubsystemData
            );
    }
}

void MainWindow::connectEventTableSelectionModel()
{
    QObject::disconnect(
        eventSelectionConnection
        );

    QItemSelectionModel *selectionModel =
        eventTable->selectionModel();

    if (selectionModel == nullptr) {
        eventSelectionConnection =
            QMetaObject::Connection();

        return;
    }

    eventSelectionConnection =
        connect(
            selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            [this](
                const QItemSelection &,
                const QItemSelection &
                ) {
                updateEventDetailFromSelection();

                updateEventNavigationState();
            }
            );
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

        eventTable->setModel(
            nullptr
            );

        connectEventTableSelectionModel();

        clearEventDetail();

        updateEventNavigationState();

        searchDebounceTimer->stop();

        /*
         * Clear all filter controls without
         * triggering individual filter refreshes.
         */
        {
            const QSignalBlocker
                severityBlocker(
                    levelFilterCombo
                    );

            const QSignalBlocker
                subsystemBlocker(
                    subsystemFilterCombo
                    );

            const QSignalBlocker
                searchBlocker(
                    searchInput
                    );

            const QSignalBlocker
                eventCodeBlocker(
                    eventCodeFilterCombo
                    );

            const QSignalBlocker
                entityBlocker(
                    entityFilterCombo
                    );

            const QSignalBlocker
                customFieldBlocker(
                    customFieldFilterEditor
                    );

            const QSignalBlocker
                timeStartCheckBlocker(
                    timeRangeStartCheckBox
                    );

            const QSignalBlocker
                timeStartEditBlocker(
                    timeRangeStartEdit
                    );

            const QSignalBlocker
                timeEndCheckBlocker(
                    timeRangeEndCheckBox
                    );

            const QSignalBlocker
                timeEndEditBlocker(
                    timeRangeEndEdit
                    );

            levelFilterCombo
                ->clearSelection();

            subsystemFilterCombo
                ->clearSelection();

            searchInput->clear();

            eventCodeFilterCombo
                ->clearSelection();

            entityFilterCombo
                ->clearSelection();

            customFieldFilterEditor
                ->clearFilters();

            customFieldFilterEditor
                ->setAvailableFields(
                    QStringList()
                    );

            timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            timeRangeEndCheckBox
                ->setChecked(
                    false
                    );

            timeRangeStartEdit
                ->setEnabled(
                    false
                    );

            timeRangeEndEdit
                ->setEnabled(
                    false
                    );
        }

        updateCustomFiltersButton();
        updateTimeRangeButton();

        resizeCustomFiltersDialogToContents();

        resetFiltersButton->setEnabled(
            false
            );

        hasSeverityData = false;
        hasSubsystemData = false;
        hasTimestampData = false;
        hasEventCodeData = false;
        hasEntityData = false;
        hasCustomFieldData = false;

        timelineFirstTimestamp.reset();
        timelineLastTimestamp.reset();

        /*
         * Hide the complete two-row filter bar
         * controls when nothing is loaded.
         */
        levelFilterCombo->setVisible(
            false
            );

        subsystemFilterCombo->setVisible(
            false
            );

        searchInput->setVisible(
            false
            );

        eventCodeFilterWidget->setVisible(
            false
            );

        entityFilterWidget->setVisible(
            false
            );

        timeRangeButton->setVisible(
            false
            );

        customFiltersButton->setVisible(
            false
            );

        resetFiltersButton->setVisible(
            false
            );

        filterPresetsButton->setVisible(
            false
            );

        previousIssueButton->setVisible(
            false
            );

        nextIssueButton->setVisible(
            false
            );

        issueSummaryTable->setRowCount(
            0
            );

        if (issueSummaryGroup != nullptr) {
            issueSummaryGroup->setVisible(
                false
                );
        }

        timelineScaleValid =
            false;

        updateTimelineChart(
            QVector<InvestigationRecord>()
            );

        summaryLabel->setText(
            tr("No log file loaded.")
            );

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

    eventTable->setModel(
        investigationController
            ->proxyModel()
        );

    connectEventTableSelectionModel();

    InvestigationFilterProxyModel
        *proxyModel =
        investigationController
            ->proxyModel();

    syncBookmarkPresentation();

    /*
     * Capture the active session's filter state
     * before rebuilding/populating any controls.
     */
    const QStringList severityFilters =
        proxyModel->severityFilters();

    const QStringList subsystemFilters =
        proxyModel->subsystemFilters();

    const QStringList eventCodeFilters =
        proxyModel->eventCodeFilters();

    const QStringList entityFilters =
        proxyModel->entityFilters();

    const CustomFieldFilterMap
        customFieldFilters =
        proxyModel->customFieldFilters();

    const QString searchText =
        proxyModel->searchText();

    const std::optional<QDateTime>
        timeRangeStart =
        proxyModel->timeRangeStart();

    const std::optional<QDateTime>
        timeRangeEnd =
        proxyModel->timeRangeEnd();

    /*
     * Search and Reset are meaningful whenever
     * an investigation is active.
     */
    searchInput->setVisible(
        true
        );

    resetFiltersButton->setVisible(
        true
        );

    resetFiltersButton->setEnabled(
        true
        );

    filterPresetsButton->setVisible(
        true
        );

    /*
     * Detect active-session capabilities and
     * repopulate the dynamic categorical controls.
     */
    updateDataCapabilities();

    refreshSubsystemFilterOptions();
    refreshCanonicalFilterOptions();

    /*
     * Restore this investigation's independent
     * filter state without causing a cascade of
     * intermediate applyFilters() calls.
     */
    {
        const QSignalBlocker
            severityBlocker(
                levelFilterCombo
                );

        const QSignalBlocker
            subsystemBlocker(
                subsystemFilterCombo
                );

        const QSignalBlocker
            searchBlocker(
                searchInput
                );

        const QSignalBlocker
            eventCodeBlocker(
                eventCodeFilterCombo
                );

        const QSignalBlocker
            entityBlocker(
                entityFilterCombo
                );

        const QSignalBlocker
            customFieldBlocker(
                customFieldFilterEditor
                );

        const QSignalBlocker
            timeStartCheckBlocker(
                timeRangeStartCheckBox
                );

        const QSignalBlocker
            timeStartEditBlocker(
                timeRangeStartEdit
                );

        const QSignalBlocker
            timeEndCheckBlocker(
                timeRangeEndCheckBox
                );

        const QSignalBlocker
            timeEndEditBlocker(
                timeRangeEndEdit
                );

        levelFilterCombo
            ->setSelectedValues(
                hasSeverityData
                    ? severityFilters
                    : QStringList()
                );

        subsystemFilterCombo
            ->setSelectedValues(
                hasSubsystemData
                    ? subsystemFilters
                    : QStringList()
                );

        eventCodeFilterCombo
            ->setSelectedValues(
                hasEventCodeData
                    ? eventCodeFilters
                    : QStringList()
                );

        entityFilterCombo
            ->setSelectedValues(
                hasEntityData
                    ? entityFilters
                    : QStringList()
                );

        customFieldFilterEditor
            ->setFilters(
                hasCustomFieldData
                    ? customFieldFilters
                    : CustomFieldFilterMap()
                );

        searchInput->setText(
            searchText
            );

        /*
         * Restore the time range against the
         * complete session bounds. Inactive ends
         * display the investigation boundary but
         * remain disabled.
         */
        if (hasTimestampData) {
            timeRangeStartEdit
                ->setDateTime(
                    timeRangeStart.has_value()
                        ? timeRangeStart.value()
                        : timelineFirstTimestamp
                              .value()
                    );

            timeRangeEndEdit
                ->setDateTime(
                    timeRangeEnd.has_value()
                        ? timeRangeEnd.value()
                        : timelineLastTimestamp
                              .value()
                    );

            timeRangeStartCheckBox
                ->setChecked(
                    timeRangeStart.has_value()
                    );

            timeRangeEndCheckBox
                ->setChecked(
                    timeRangeEnd.has_value()
                    );

            timeRangeStartEdit
                ->setEnabled(
                    timeRangeStart.has_value()
                    );

            timeRangeEndEdit
                ->setEnabled(
                    timeRangeEnd.has_value()
                    );
        } else {
            timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            timeRangeEndCheckBox
                ->setChecked(
                    false
                    );

            timeRangeStartEdit
                ->setEnabled(
                    false
                    );

            timeRangeEndEdit
                ->setEnabled(
                    false
                    );
        }
    }

    /*
     * Programmatic restoration does not emit the
     * editor/control signals used to maintain
     * these compact summaries.
     */
    updateCustomFiltersButton();
    updateTimeRangeButton();

    resizeCustomFiltersDialogToContents();

    /*
     * Apply the complete restored state once.
     */
    applyFilters();

    /*
     * ---------------------------------------------------------
     * Restore per-session column widths
     * ---------------------------------------------------------
     */

    const QVector<int> &columnWidths =
        session->columnWidths();

    const int columnCount =
        eventTable
            ->horizontalHeader()
            ->count();

    if (columnWidths.size()
        == columnCount) {
        for (
            int column = 0;
            column < columnCount;
            ++column
            ) {
            eventTable->setColumnWidth(
                column,
                columnWidths[column]
                );
        }
    } else {
        eventTable->resizeColumnsToContents();

        QVector<int> measuredWidths;

        measuredWidths.reserve(
            columnCount
            );

        for (
            int column = 0;
            column < columnCount;
            ++column
            ) {
            measuredWidths.append(
                eventTable->columnWidth(
                    column
                    )
                );
        }

        session->setColumnWidths(
            std::move(
                measuredWidths
                )
            );
    }

    eventTable
        ->horizontalHeader()
        ->setStretchLastSection(
            true
            );

    updateEventNavigationState();
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

void MainWindow::
    resizeCustomFiltersDialogToContents()
{
    if (customFiltersDialog == nullptr) {
        return;
    }

    /*
     * Defer until Qt has processed the editor's
     * visibility/layout changes. This is especially
     * important when the last active criterion is
     * removed and the active-filter container hides.
     */
    QTimer::singleShot(
        0,
        customFiltersDialog,
        [this]() {
            customFiltersDialog->adjustSize();
        }
        );
}