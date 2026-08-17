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
    sessionTabBar(new QTabBar(this)),
    summaryLabel(new QLabel("No log file loaded.")),
    eventTable(new QTableView(this)),
    eventDetailText(new QPlainTextEdit(this)),
    issueSummaryTable(new QTableWidget(0, 4)),
    issueSummaryGroup(nullptr),
    workspace(new InvestigationWorkspace(this)),
    investigationController(nullptr),
    timelineChartView(new QChartView(this)),
    levelFilterCombo(new QComboBox(this)),
    subsystemFilterCombo(new QComboBox(this)),
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

void MainWindow::buildFilterControls(QVBoxLayout *layout)
{
    levelFilterCombo->addItem("All levels", "");
    levelFilterCombo->addItem("TRACE", "TRACE");
    levelFilterCombo->addItem("DEBUG", "DEBUG");
    levelFilterCombo->addItem("INFO", "INFO");
    levelFilterCombo->addItem("WARN", "WARN");
    levelFilterCombo->addItem("ERROR", "ERROR");
    levelFilterCombo->addItem("CRITICAL", "CRITICAL");

    subsystemFilterCombo->addItem("All subsystems", "");

    subsystemFilterCombo->setMinimumWidth(240);

    subsystemFilterCombo->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon
        );

    subsystemFilterCombo->setMinimumContentsLength(24);

    searchInput->setPlaceholderText(
        "Search canonical fields and custom attributes..."
        );

    searchDebounceTimer->setSingleShot(true);

    searchDebounceTimer->setInterval(
        SearchDebounceIntervalMs
        );

    auto *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(levelFilterCombo);
    filterLayout->addWidget(subsystemFilterCombo);
    filterLayout->addWidget(searchInput);

    layout->addLayout(filterLayout);

    connect(
        levelFilterCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            searchDebounceTimer->stop();
            applyFilters();
        }
        );

    connect(
        subsystemFilterCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            subsystemFilterCombo->setToolTip(
                subsystemFilterCombo
                    ->currentData()
                    .toString()
                );
            searchDebounceTimer->stop();
            applyFilters();
        }
        );

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
}

void MainWindow::applyFilters()
{
    if (investigationController == nullptr) {
        return;
    }

    timelineScaleValid =
        false;

    investigationController->setFilters(
        levelFilterCombo->currentData().toString(),
        subsystemFilterCombo->currentData().toString(),
        searchInput->text()
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
}

void MainWindow::refreshSubsystemFilterOptions()
{
    if (investigationController == nullptr) {
        return;
    }

    const QString selectedSubsystem =
        subsystemFilterCombo
            ->currentData()
            .toString();

    subsystemFilterCombo->blockSignals(true);
    subsystemFilterCombo->clear();

    subsystemFilterCombo->addItem(
        "All subsystems",
        ""
        );

    subsystemFilterCombo->setItemData(
        0,
        QStringLiteral("All subsystems"),
        Qt::ToolTipRole
        );

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

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
        subsystemFilterCombo->addItem(
            subsystem,
            subsystem
            );

        const int itemIndex =
            subsystemFilterCombo->count() - 1;

        subsystemFilterCombo->setItemData(
            itemIndex,
            subsystem,
            Qt::ToolTipRole
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

    /*
     * Keep the filter control itself compact,
     * but let its popup expand enough to show
     * long subsystem/logger names clearly.
     */
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

    const int previousIndex =
        subsystemFilterCombo->findData(
            selectedSubsystem
            );

    if (previousIndex >= 0) {
        subsystemFilterCombo->setCurrentIndex(
            previousIndex
            );
    }

    subsystemFilterCombo->setToolTip(
        subsystemFilterCombo
            ->currentData()
            .toString()
        );

    subsystemFilterCombo->blockSignals(false);
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
    detailLayout->addWidget(eventDetailText);

    return detailGroup;
}

void MainWindow::updateEventDetailFromSelection()
{
    if (investigationController == nullptr
        || eventTable->selectionModel() == nullptr) {
        clearEventDetail();
        return;
    }

    const QModelIndexList selectedRows =
        eventTable->selectionModel()
            ->selectedRows();

    if (selectedRows.isEmpty()) {
        clearEventDetail();
        return;
    }

    const InvestigationRecord *record =
        investigationController
            ->recordForProxyIndex(
                selectedRows.first()
                );

    if (record == nullptr) {
        clearEventDetail();
        return;
    }

    displayEventDetail(*record);
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

    auto *issueLayout = new QVBoxLayout(issueSummaryGroup);
    issueLayout->addWidget(issueSummaryTable);

    return issueSummaryGroup;
}

void MainWindow::updateIssueSummary(const QVector<InvestigationRecord> &records)
{
    if (!hasSeverityData
        || !hasSubsystemData) {
        issueSummaryTable->setRowCount(0);
        return;
    }

    const auto groups = issueAnalyzer.groupWarningsAndErrorsBySubsystem(records);

    issueSummaryTable->setRowCount(groups.size());

    for (int row = 0; row < groups.size(); ++row) {
        const TelemetryIssueGroup &group = groups[row];

        issueSummaryTable->setItem(row, 0, new QTableWidgetItem(group.subsystem));
        issueSummaryTable->setItem(
            row,
            1,
            new QTableWidgetItem(QString::number(group.warningCount))
            );
        issueSummaryTable->setItem(
            row,
            2,
            new QTableWidgetItem(QString::number(group.errorCount))
            );
        issueSummaryTable->setItem(
            row,
            3,
            new QTableWidgetItem(QString::number(group.totalCount()))
            );
    }

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
    if (!timelineFirstTimestamp.has_value()
        || !timelineLastTimestamp.has_value()) {
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
                  *timelineFirstTimestamp,
                  *timelineLastTimestamp
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
                    *timelineFirstTimestamp,
                    *timelineLastTimestamp,
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
                *timelineFirstTimestamp,
                *timelineLastTimestamp,
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
                    *timelineFirstTimestamp,
                    *timelineLastTimestamp,
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
                    *timelineFirstTimestamp,
                    *timelineLastTimestamp,
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

    if (!timelineFirstTimestamp.has_value()
        || !timelineLastTimestamp.has_value()) {
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
                  *timelineFirstTimestamp,
                  *timelineLastTimestamp
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
                *timelineFirstTimestamp,
                *timelineLastTimestamp,
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
            *timelineFirstTimestamp,
            intervalMilliseconds
            );

    const qint64 visibleFirstEpoch =
        firstBucketEpoch
        + startBucketIndex
              * intervalMilliseconds;

    qint64 visibleLastEpoch =
        visibleFirstEpoch
        + visibleBucketCount
              * intervalMilliseconds
        - 1;

    visibleLastEpoch =
        std::min(
            visibleLastEpoch,
            timelineLastTimestamp
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

void MainWindow::updateDataCapabilities()
{
    hasSeverityData = false;
    hasSubsystemData = false;

    timelineFirstTimestamp.reset();
    timelineLastTimestamp.reset();

    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        return;
    }

    hasSeverityData =
        session->hasSeverityData();

    hasSubsystemData =
        session->hasSubsystemData();

    timelineFirstTimestamp =
        session->firstTimestamp();

    timelineLastTimestamp =
        session->lastTimestamp();

    /*
     * Clear filters that no longer apply before
     * hiding their controls.
     */
    if (!hasSeverityData) {
        levelFilterCombo->blockSignals(true);
        levelFilterCombo->setCurrentIndex(0);
        levelFilterCombo->blockSignals(false);
    }

    if (!hasSubsystemData) {
        subsystemFilterCombo->blockSignals(true);
        subsystemFilterCombo->setCurrentIndex(0);
        subsystemFilterCombo->blockSignals(false);
    }

    levelFilterCombo->setVisible(
        hasSeverityData
        );

    subsystemFilterCombo->setVisible(
        hasSubsystemData
        );

    /*
     * Warning/error grouping only has useful
     * meaning when both concepts are available.
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
            }
            );
}

void MainWindow::bindActiveSession()
{
    InvestigationSession *session =
        workspace->activeSession();

    if (session == nullptr) {
        if (reloadAction != nullptr) {
            reloadAction->setEnabled(false);
        }

        investigationController =
            nullptr;

        currentFilePath.clear();

        eventTable->setModel(nullptr);

        connectEventTableSelectionModel();

        clearEventDetail();

        levelFilterCombo->blockSignals(true);
        subsystemFilterCombo->blockSignals(true);
        searchInput->blockSignals(true);

        levelFilterCombo->setCurrentIndex(0);
        subsystemFilterCombo->setCurrentIndex(0);
        searchInput->clear();

        levelFilterCombo->blockSignals(false);
        subsystemFilterCombo->blockSignals(false);
        searchInput->blockSignals(false);

        hasSeverityData = false;
        hasSubsystemData = false;

        levelFilterCombo->setVisible(false);
        subsystemFilterCombo->setVisible(false);
        searchInput->setVisible(false);

        issueSummaryTable->setRowCount(0);

        if (issueSummaryGroup != nullptr) {
            issueSummaryGroup->setVisible(false);
        }

        timelineFirstTimestamp.reset();
        timelineLastTimestamp.reset();

        timelineScaleValid = false;

        updateTimelineChart(
            QVector<InvestigationRecord>()
            );

        summaryLabel->setText(
            tr("No log file loaded.")
            );

        return;
    }

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

    InvestigationFilterProxyModel *proxyModel =
        investigationController
            ->proxyModel();

    const QString severityFilter =
        proxyModel->severityFilter();

    const QString subsystemFilter =
        proxyModel->subsystemFilter();

    const QString searchText =
        proxyModel->searchText();

    searchInput->setVisible(true);

    updateDataCapabilities();

    refreshSubsystemFilterOptions();

    levelFilterCombo->blockSignals(true);
    subsystemFilterCombo->blockSignals(true);
    searchInput->blockSignals(true);

    int severityIndex =
        hasSeverityData
            ? levelFilterCombo->findData(
                  severityFilter
                  )
            : 0;

    if (severityIndex < 0) {
        severityIndex = 0;
    }

    levelFilterCombo->setCurrentIndex(
        severityIndex
        );

    int subsystemIndex =
        hasSubsystemData
            ? subsystemFilterCombo->findData(
                  subsystemFilter
                  )
            : 0;

    if (subsystemIndex < 0) {
        subsystemIndex = 0;
    }

    subsystemFilterCombo->setCurrentIndex(
        subsystemIndex
        );

    searchInput->setText(
        searchText
        );

    levelFilterCombo->blockSignals(false);
    subsystemFilterCombo->blockSignals(false);
    searchInput->blockSignals(false);

    applyFilters();

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
            std::move(measuredWidths)
            );
    }

    eventTable
        ->horizontalHeader()
        ->setStretchLastSection(true);
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