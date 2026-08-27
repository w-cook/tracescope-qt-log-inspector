#include "InvestigationFilterPanel.h"

#include <algorithm>
#include <utility>

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QResizeEvent>

#include "../CustomFieldFilterEditor.h"
#include "../MultiSelectFilterComboBox.h"
#include "../../domain/RecordSeverity.h"
#include "../../preferences/FilterPresetStore.h"
#include "../../workspace/InvestigationSession.h"

namespace
{
constexpr int SearchDebounceIntervalMs =
    600;
}

InvestigationFilterPanel::
    InvestigationFilterPanel(
        FilterPresetStore *presetStore,
        QWidget *parent
        )
    : QWidget(parent),
    m_presetStore(presetStore),
    m_levelFilterCombo(
        new MultiSelectFilterComboBox(this)
        ),
    m_subsystemFilterCombo(
        new MultiSelectFilterComboBox(this)
        ),
    m_eventCodeFilterCombo(
        new MultiSelectFilterComboBox(this)
        ),
    m_entityFilterCombo(
        new MultiSelectFilterComboBox(this)
        ),
    m_findingStatusFilterCombo(
        new MultiSelectFilterComboBox(this)
        ),
    m_searchInput(
        new QLineEdit(this)
        ),
    m_bookmarksOnlyCheckBox(
        new QCheckBox(
            tr("Bookmarks only"),
            this
            )
        ),
    m_resetFiltersButton(
        new QPushButton(
            tr("Reset Filters"),
            this
            )
        ),
    m_filterPresetsButton(
        new QPushButton(
            tr("Presets"),
            this
            )
        ),
    m_filterPresetsMenu(
        new QMenu(this)
        ),
    m_eventCodeFilterWidget(
        new QWidget(this)
        ),
    m_entityFilterWidget(
        new QWidget(this)
        ),
    m_customFiltersButton(
        new QPushButton(
            tr("Custom Filters"),
            this
            )
        ),
    m_customFiltersDialog(
        new QDialog(this)
        ),
    m_customFieldFilterEditor(
        new CustomFieldFilterEditor(
            m_customFiltersDialog
            )
        ),
    m_timeRangeButton(
        new QPushButton(
            tr("Time Range"),
            this
            )
        ),
    m_timeRangeDialog(
        new QDialog(this)
        ),
    m_searchDebounceTimer(
        new QTimer(this)
        )
{
    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->setSpacing(
        4
        );

    /*
     * ---------------------------------------------------------
     * Primary categorical controls
     * ---------------------------------------------------------
     */
    m_levelFilterCombo
        ->setEmptySelectionText(
            tr("All severities")
            );

    for (
        const QString &severity
        : {
            QStringLiteral("TRACE"),
            QStringLiteral("DEBUG"),
            QStringLiteral("INFO"),
            QStringLiteral("WARN"),
            QStringLiteral("ERROR"),
            QStringLiteral("CRITICAL")
        }
        ) {
        m_levelFilterCombo
            ->addFilterItem(
                severity,
                severity
                );
    }

    m_levelFilterCombo->setMinimumWidth(
        150
        );

    m_subsystemFilterCombo
        ->setEmptySelectionText(
            tr("All subsystems")
            );

    m_subsystemFilterCombo->setMinimumWidth(
        190
        );

    m_subsystemFilterCombo
        ->setSizeAdjustPolicy(
            QComboBox::
            AdjustToMinimumContentsLengthWithIcon
            );

    m_subsystemFilterCombo
        ->setMinimumContentsLength(
            18
            );

    /*
     * Event code.
     */
    auto *eventCodeLayout =
        new QHBoxLayout(
            m_eventCodeFilterWidget
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

    m_eventCodeFilterCombo
        ->setEmptySelectionText(
            tr("All event codes")
            );

    m_eventCodeFilterCombo
        ->setMinimumWidth(
            170
            );

    eventCodeLayout->addWidget(
        m_eventCodeFilterCombo
        );

    /*
     * Entity.
     */
    auto *entityLayout =
        new QHBoxLayout(
            m_entityFilterWidget
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

    m_entityFilterCombo
        ->setEmptySelectionText(
            tr("All entities")
            );

    m_entityFilterCombo
        ->setMinimumWidth(
            170
            );

    entityLayout->addWidget(
        m_entityFilterCombo
        );

    auto *primaryFilterLayout =
        new QHBoxLayout();

    primaryFilterLayout
        ->setContentsMargins(
            0,
            0,
            0,
            0
            );

    primaryFilterLayout->setSpacing(
        8
        );

    primaryFilterLayout->addWidget(
        m_levelFilterCombo,
        1
        );

    primaryFilterLayout->addWidget(
        m_subsystemFilterCombo,
        1
        );

    primaryFilterLayout->addWidget(
        m_eventCodeFilterWidget,
        1
        );

    primaryFilterLayout->addWidget(
        m_entityFilterWidget,
        1
        );

    layout->addLayout(
        primaryFilterLayout
        );

    for (
        QWidget *filterWidget
        : {
            static_cast<QWidget *>(
                m_levelFilterCombo
                ),
            static_cast<QWidget *>(
                m_subsystemFilterCombo
                ),
            m_eventCodeFilterWidget,
            m_entityFilterWidget
        }
        ) {
        filterWidget->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Fixed
            );
    }

    /*
     * ---------------------------------------------------------
     * Search
     * ---------------------------------------------------------
     */
    m_searchInput->setPlaceholderText(
        tr(
            "Search canonical fields and "
            "custom attributes..."
            )
        );

    m_searchInput->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );

    m_searchDebounceTimer->setSingleShot(
        true
        );

    m_searchDebounceTimer->setInterval(
        SearchDebounceIntervalMs
        );

    /*
     * ---------------------------------------------------------
     * Finding-status filter
     * ---------------------------------------------------------
     */
    m_findingStatusFilterCombo
        ->setEmptySelectionText(
            tr("All statuses")
            );

    m_findingStatusFilterCombo
        ->addFilterItem(
            tr("Open"),
            QStringLiteral("OPEN")
            );

    m_findingStatusFilterCombo
        ->addFilterItem(
            tr("Resolved"),
            QStringLiteral("RESOLVED")
            );

    m_findingStatusFilterCombo
        ->addFilterItem(
            tr("Dismissed"),
            QStringLiteral("DISMISSED")
            );

    m_findingStatusFilterCombo
        ->setMinimumWidth(
            130
            );

    m_findingStatusFilterCombo
        ->setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed
            );

    m_findingStatusFilterCombo
        ->setToolTip(
            tr(
                "Filter events by investigation "
                "finding status"
                )
            );

    /*
     * ---------------------------------------------------------
     * Bookmark filter
     * ---------------------------------------------------------
     */
    m_bookmarksOnlyCheckBox
        ->setToolTip(
            tr(
                "Show only bookmarked events"
                )
            );

    m_bookmarksOnlyCheckBox
        ->setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed
            );

    /*
     * ---------------------------------------------------------
     * Time-range dialog
     * ---------------------------------------------------------
     */
    m_timeRangeFilterWidget =
        new QWidget(
            m_timeRangeDialog
            );

    auto *timeRangeLayout =
        new QHBoxLayout(
            m_timeRangeFilterWidget
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
            m_timeRangeFilterWidget
            );

    m_timeRangeStartCheckBox =
        new QCheckBox(
            tr("From"),
            m_timeRangeFilterWidget
            );

    m_timeRangeStartEdit =
        new QDateTimeEdit(
            m_timeRangeFilterWidget
            );

    m_timeRangeEndCheckBox =
        new QCheckBox(
            tr("To"),
            m_timeRangeFilterWidget
            );

    m_timeRangeEndEdit =
        new QDateTimeEdit(
            m_timeRangeFilterWidget
            );

    for (
        QDateTimeEdit *edit
        : {
            m_timeRangeStartEdit,
            m_timeRangeEndEdit
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
        m_timeRangeStartCheckBox
        );

    timeRangeLayout->addWidget(
        m_timeRangeStartEdit
        );

    timeRangeLayout->addSpacing(
        12
        );

    timeRangeLayout->addWidget(
        m_timeRangeEndCheckBox
        );

    timeRangeLayout->addWidget(
        m_timeRangeEndEdit
        );

    timeRangeLayout->addStretch();

    m_timeRangeDialog->setWindowTitle(
        tr("Time Range Filter")
        );

    m_timeRangeDialog->setModal(
        false
        );

    m_timeRangeDialog->setWindowModality(
        Qt::NonModal
        );

    auto *timeDialogLayout =
        new QVBoxLayout(
            m_timeRangeDialog
            );

    timeDialogLayout->addWidget(
        m_timeRangeFilterWidget
        );

    auto *timeCloseButton =
        new QPushButton(
            tr("Close"),
            m_timeRangeDialog
            );

    timeDialogLayout->addWidget(
        timeCloseButton,
        0,
        Qt::AlignRight
        );

    /*
     * ---------------------------------------------------------
     * Custom-field dialog
     * ---------------------------------------------------------
     */
    m_customFiltersDialog
        ->setWindowTitle(
            tr("Custom Field Filters")
            );

    m_customFiltersDialog->setModal(
        false
        );

    m_customFiltersDialog
        ->setWindowModality(
            Qt::NonModal
            );

    m_customFiltersDialog
        ->setMinimumWidth(
            600
            );

    auto *customDialogLayout =
        new QVBoxLayout(
            m_customFiltersDialog
            );

    customDialogLayout->addWidget(
        m_customFieldFilterEditor
        );

    auto *customCloseButton =
        new QPushButton(
            tr("Close"),
            m_customFiltersDialog
            );

    customDialogLayout->addWidget(
        customCloseButton,
        0,
        Qt::AlignRight
        );

    /*
 * ---------------------------------------------------------
 * Secondary filter controls
 * ---------------------------------------------------------
 */
    m_secondaryFilterLayout =
        new QGridLayout();

    m_secondaryFilterLayout
        ->setContentsMargins(
            0,
            0,
            0,
            0
            );

    m_secondaryFilterLayout
        ->setHorizontalSpacing(
            8
            );

    m_secondaryFilterLayout
        ->setVerticalSpacing(
            4
            );

    m_filterPresetsButton->setMenu(
        m_filterPresetsMenu
        );

    /*
     * Start in the normal wide presentation.
     *
     * Search | Status | Bookmarks | Time Range |
     * Custom Filters | Presets | Reset Filters
     *
     * updateResponsiveLayout() will move these same
     * controls to two rows when the panel becomes too
     * narrow.
     */
    m_secondaryFilterLayout->addWidget(
        m_searchInput,
        0,
        0
        );

    m_secondaryFilterLayout->addWidget(
        m_findingStatusFilterCombo,
        0,
        1
        );

    m_secondaryFilterLayout->addWidget(
        m_bookmarksOnlyCheckBox,
        0,
        2
        );

    m_secondaryFilterLayout->addWidget(
        m_timeRangeButton,
        0,
        3
        );

    m_secondaryFilterLayout->addWidget(
        m_customFiltersButton,
        0,
        4
        );

    m_secondaryFilterLayout->addWidget(
        m_filterPresetsButton,
        0,
        5
        );

    m_secondaryFilterLayout->addWidget(
        m_resetFiltersButton,
        0,
        6
        );

    /*
     * Search absorbs the additional horizontal space.
     * The remaining controls stay near their preferred
     * widths.
     */
    m_secondaryFilterLayout->setColumnStretch(
        0,
        1
        );

    for (int column = 1;
         column <= 6;
         ++column) {
        m_secondaryFilterLayout->setColumnStretch(
            column,
            0
            );
    }

    layout->addLayout(
        m_secondaryFilterLayout
        );

    /*
     * ---------------------------------------------------------
     * Immediate categorical/state changes
     * ---------------------------------------------------------
     */
    auto requestImmediateChange =
        [this]() {
            m_searchDebounceTimer->stop();

            emit filterChangeRequested();
        };

    connect(
        m_levelFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        requestImmediateChange
        );

    connect(
        m_subsystemFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        requestImmediateChange
        );

    connect(
        m_eventCodeFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        requestImmediateChange
        );

    connect(
        m_entityFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        requestImmediateChange
        );

    connect(
        m_findingStatusFilterCombo,
        &MultiSelectFilterComboBox::
        selectionChanged,
        this,
        requestImmediateChange
        );

    connect(
        m_bookmarksOnlyCheckBox,
        &QCheckBox::toggled,
        this,
        [requestImmediateChange](bool) {
            requestImmediateChange();
        }
        );

    /*
     * ---------------------------------------------------------
     * Search
     * ---------------------------------------------------------
     */
    connect(
        m_searchInput,
        &QLineEdit::textChanged,
        this,
        [this]() {
            m_searchDebounceTimer->start();
        }
        );

    connect(
        m_searchInput,
        &QLineEdit::returnPressed,
        this,
        [this]() {
            m_searchDebounceTimer->stop();

            emit filterChangeRequested();
        }
        );

    connect(
        m_searchDebounceTimer,
        &QTimer::timeout,
        this,
        [this]() {
            emit filterChangeRequested();
        }
        );

    /*
     * ---------------------------------------------------------
     * Reset
     * ---------------------------------------------------------
     */
    connect(
        m_resetFiltersButton,
        &QPushButton::clicked,
        this,
        [this]() {
            resetFilters();

            emit filterChangeRequested();
        }
        );

    /*
     * ---------------------------------------------------------
     * Presets
     * ---------------------------------------------------------
     */
    connect(
        m_filterPresetsMenu,
        &QMenu::aboutToShow,
        this,
        &InvestigationFilterPanel::
        refreshFilterPresetsMenu
        );

    /*
     * ---------------------------------------------------------
     * Modeless dialogs
     * ---------------------------------------------------------
     */
    connect(
        m_timeRangeButton,
        &QPushButton::clicked,
        this,
        [this]() {
            m_timeRangeDialog->show();
            m_timeRangeDialog->raise();
            m_timeRangeDialog
                ->activateWindow();
        }
        );

    connect(
        timeCloseButton,
        &QPushButton::clicked,
        m_timeRangeDialog,
        &QDialog::close
        );

    connect(
        m_customFiltersButton,
        &QPushButton::clicked,
        this,
        [this]() {
            m_customFiltersDialog->show();
            m_customFiltersDialog->raise();
            m_customFiltersDialog
                ->activateWindow();
        }
        );

    connect(
        customCloseButton,
        &QPushButton::clicked,
        m_customFiltersDialog,
        &QDialog::close
        );

    m_customFiltersDialog->adjustSize();

    /*
     * ---------------------------------------------------------
     * Custom fields
     * ---------------------------------------------------------
     */
    connect(
        m_customFieldFilterEditor,
        &CustomFieldFilterEditor::
        filtersChanged,
        this,
        [this]() {
            updateCustomFiltersButton();

            resizeCustomFiltersDialogToContents();

            emit filterChangeRequested();
        }
        );

    /*
     * ---------------------------------------------------------
     * Time-range controls
     * ---------------------------------------------------------
     */
    connect(
        m_timeRangeStartCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            m_timeRangeStartEdit->setEnabled(
                checked
                );

            if (
                checked
                && m_timeRangeEndCheckBox
                       ->isChecked()
                && m_timeRangeStartEdit
                           ->dateTime()
                       > m_timeRangeEndEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    m_timeRangeEndEdit
                    );

                m_timeRangeEndEdit
                    ->setDateTime(
                        m_timeRangeStartEdit
                            ->dateTime()
                        );
            }

            updateTimeRangeButton();

            emit filterChangeRequested();
        }
        );

    connect(
        m_timeRangeEndCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked) {
            m_timeRangeEndEdit->setEnabled(
                checked
                );

            if (
                checked
                && m_timeRangeStartCheckBox
                       ->isChecked()
                && m_timeRangeEndEdit
                           ->dateTime()
                       < m_timeRangeStartEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    m_timeRangeStartEdit
                    );

                m_timeRangeStartEdit
                    ->setDateTime(
                        m_timeRangeEndEdit
                            ->dateTime()
                        );
            }

            updateTimeRangeButton();

            emit filterChangeRequested();
        }
        );

    connect(
        m_timeRangeStartEdit,
        &QDateTimeEdit::dateTimeChanged,
        this,
        [this](
            const QDateTime &dateTime
            ) {
            if (
                !m_timeRangeStartCheckBox
                     ->isChecked()
                ) {
                return;
            }

            if (
                m_timeRangeEndCheckBox
                    ->isChecked()
                && dateTime
                       > m_timeRangeEndEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    m_timeRangeEndEdit
                    );

                m_timeRangeEndEdit
                    ->setDateTime(
                        dateTime
                        );
            }

            updateTimeRangeButton();

            emit filterChangeRequested();
        }
        );

    connect(
        m_timeRangeEndEdit,
        &QDateTimeEdit::dateTimeChanged,
        this,
        [this](
            const QDateTime &dateTime
            ) {
            if (
                !m_timeRangeEndCheckBox
                     ->isChecked()
                ) {
                return;
            }

            if (
                m_timeRangeStartCheckBox
                    ->isChecked()
                && dateTime
                       < m_timeRangeStartEdit
                             ->dateTime()
                ) {
                const QSignalBlocker blocker(
                    m_timeRangeStartEdit
                    );

                m_timeRangeStartEdit
                    ->setDateTime(
                        dateTime
                        );
            }

            updateTimeRangeButton();

            emit filterChangeRequested();
        }
        );

    syncFromSession();
}

void InvestigationFilterPanel::setSession(
    InvestigationSession *session
    )
{
    m_searchDebounceTimer->stop();

    m_session =
        session;

    syncFromSession();
}

InvestigationSession *
InvestigationFilterPanel::session() const
{
    return m_session;
}

void InvestigationFilterPanel::
    applyToSession()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    const std::optional<QDateTime>
        timeRangeStart =
        m_timeRangeStartCheckBox
                ->isChecked()
            ? std::optional<QDateTime>(
                  m_timeRangeStartEdit
                      ->dateTime()
                  )
            : std::nullopt;

    const std::optional<QDateTime>
        timeRangeEnd =
        m_timeRangeEndCheckBox
                ->isChecked()
            ? std::optional<QDateTime>(
                  m_timeRangeEndEdit
                      ->dateTime()
                  )
            : std::nullopt;

    controller->setFilterState(
        m_levelFilterCombo
            ->selectedValues(),
        m_subsystemFilterCombo
            ->selectedValues(),
        m_searchInput->text(),
        m_eventCodeFilterCombo
            ->selectedValues(),
        m_entityFilterCombo
            ->selectedValues(),
        timeRangeStart,
        timeRangeEnd,
        m_customFieldFilterEditor
            ->filters(),
        m_findingStatusFilterCombo
            ->selectedValues(),
        m_bookmarksOnlyCheckBox
            ->isChecked()
        );
}

void InvestigationFilterPanel::
    syncFromSession()
{
    m_searchDebounceTimer->stop();

    if (m_session == nullptr) {
        {
            const QSignalBlocker severityBlocker(
                m_levelFilterCombo
                );

            const QSignalBlocker subsystemBlocker(
                m_subsystemFilterCombo
                );

            const QSignalBlocker eventCodeBlocker(
                m_eventCodeFilterCombo
                );

            const QSignalBlocker entityBlocker(
                m_entityFilterCombo
                );

            const QSignalBlocker searchBlocker(
                m_searchInput
                );

            const QSignalBlocker findingBlocker(
                m_findingStatusFilterCombo
                );

            const QSignalBlocker bookmarkBlocker(
                m_bookmarksOnlyCheckBox
                );

            const QSignalBlocker customBlocker(
                m_customFieldFilterEditor
                );

            const QSignalBlocker startCheckBlocker(
                m_timeRangeStartCheckBox
                );

            const QSignalBlocker startEditBlocker(
                m_timeRangeStartEdit
                );

            const QSignalBlocker endCheckBlocker(
                m_timeRangeEndCheckBox
                );

            const QSignalBlocker endEditBlocker(
                m_timeRangeEndEdit
                );

            m_levelFilterCombo
                ->clearSelection();

            m_subsystemFilterCombo->clear();
            m_subsystemFilterCombo
                ->clearSelection();

            m_eventCodeFilterCombo->clear();
            m_eventCodeFilterCombo
                ->clearSelection();

            m_entityFilterCombo->clear();
            m_entityFilterCombo
                ->clearSelection();

            m_searchInput->clear();

            m_findingStatusFilterCombo
                ->clearSelection();

            m_bookmarksOnlyCheckBox
                ->setChecked(
                    false
                    );

            m_customFieldFilterEditor
                ->clearFilters();

            m_customFieldFilterEditor
                ->setAvailableFields(
                    QStringList()
                    );

            m_timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeStartEdit
                ->setEnabled(
                    false
                    );

            m_timeRangeEndEdit
                ->setEnabled(
                    false
                    );
        }

        m_levelFilterCombo->setVisible(
            false
            );

        m_subsystemFilterCombo->setVisible(
            false
            );

        m_eventCodeFilterWidget->setVisible(
            false
            );

        m_entityFilterWidget->setVisible(
            false
            );

        m_searchInput->setVisible(
            false
            );

        m_findingStatusFilterCombo
            ->setVisible(
                false
                );

        m_bookmarksOnlyCheckBox
            ->setVisible(
                false
                );

        m_timeRangeButton->setVisible(
            false
            );

        m_customFiltersButton->setVisible(
            false
            );

        m_filterPresetsButton->setVisible(
            false
            );

        m_resetFiltersButton->setVisible(
            false
            );

        m_resetFiltersButton->setEnabled(
            false
            );

        updateCustomFiltersButton();
        updateTimeRangeButton();

        resizeCustomFiltersDialogToContents();

        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        return;
    }

    InvestigationFilterProxyModel *proxyModel =
        controller->proxyModel();

    if (proxyModel == nullptr) {
        return;
    }

    /*
     * Dynamic option collections must be rebuilt
     * before restoring their selected values.
     */
    refreshSubsystemOptions();
    refreshCanonicalOptions();

    {
        const QSignalBlocker blocker(
            m_customFieldFilterEditor
            );

        m_customFieldFilterEditor
            ->setAvailableFields(
                m_session
                        ->hasCustomFieldData()
                    ? m_session
                          ->availableCustomFields()
                    : QStringList()
                );
    }

    const bool hasTimestampData =
        m_session
            ->firstTimestamp()
            .has_value()
        && m_session
               ->lastTimestamp()
               .has_value();

    /*
     * Restore the independent filter state already
     * stored by this session's proxy model.
     */
    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            m_subsystemFilterCombo
            );

        const QSignalBlocker eventCodeBlocker(
            m_eventCodeFilterCombo
            );

        const QSignalBlocker entityBlocker(
            m_entityFilterCombo
            );

        const QSignalBlocker searchBlocker(
            m_searchInput
            );

        const QSignalBlocker findingBlocker(
            m_findingStatusFilterCombo
            );

        const QSignalBlocker bookmarkBlocker(
            m_bookmarksOnlyCheckBox
            );

        const QSignalBlocker customBlocker(
            m_customFieldFilterEditor
            );

        const QSignalBlocker startCheckBlocker(
            m_timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            m_timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            m_timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            m_timeRangeEndEdit
            );

        m_levelFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasSeverityData()
                    ? proxyModel
                          ->severityFilters()
                    : QStringList()
                );

        m_subsystemFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasSubsystemData()
                    ? proxyModel
                          ->subsystemFilters()
                    : QStringList()
                );

        m_eventCodeFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasEventCodeData()
                    ? proxyModel
                          ->eventCodeFilters()
                    : QStringList()
                );

        m_entityFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasEntityData()
                    ? proxyModel
                          ->entityFilters()
                    : QStringList()
                );

        m_customFieldFilterEditor
            ->setFilters(
                m_session
                        ->hasCustomFieldData()
                    ? proxyModel
                          ->customFieldFilters()
                    : CustomFieldFilterMap()
                );

        m_searchInput->setText(
            proxyModel->searchText()
            );

        m_findingStatusFilterCombo
            ->setSelectedValues(
                proxyModel
                    ->findingStatusFilters()
                );

        m_bookmarksOnlyCheckBox
            ->setChecked(
                proxyModel
                    ->bookmarkedOnly()
                );

        if (hasTimestampData) {
            const std::optional<QDateTime>
                &start =
                proxyModel
                    ->timeRangeStart();

            const std::optional<QDateTime>
                &end =
                proxyModel
                    ->timeRangeEnd();

            m_timeRangeStartEdit
                ->setDateTime(
                    start.has_value()
                        ? start.value()
                        : m_session
                              ->firstTimestamp()
                              .value()
                    );

            m_timeRangeEndEdit
                ->setDateTime(
                    end.has_value()
                        ? end.value()
                        : m_session
                              ->lastTimestamp()
                              .value()
                    );

            m_timeRangeStartCheckBox
                ->setChecked(
                    start.has_value()
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    end.has_value()
                    );

            m_timeRangeStartEdit
                ->setEnabled(
                    start.has_value()
                    );

            m_timeRangeEndEdit
                ->setEnabled(
                    end.has_value()
                    );
        } else {
            m_timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeStartEdit
                ->setEnabled(
                    false
                    );

            m_timeRangeEndEdit
                ->setEnabled(
                    false
                    );
        }
    }

    /*
     * Capability-driven presentation.
     */
    m_levelFilterCombo->setVisible(
        m_session->hasSeverityData()
        );

    m_subsystemFilterCombo->setVisible(
        m_session->hasSubsystemData()
        );

    m_eventCodeFilterWidget->setVisible(
        m_session->hasEventCodeData()
        );

    m_entityFilterWidget->setVisible(
        m_session->hasEntityData()
        );

    m_timeRangeButton->setVisible(
        hasTimestampData
        );

    m_customFiltersButton->setVisible(
        m_session->hasCustomFieldData()
        );

    /*
     * These controls are meaningful for every
     * active investigation.
     */
    m_searchInput->setVisible(
        true
        );

    m_findingStatusFilterCombo->setVisible(
        true
        );

    m_bookmarksOnlyCheckBox->setVisible(
        true
        );

    m_filterPresetsButton->setVisible(
        true
        );

    m_resetFiltersButton->setVisible(
        true
        );

    m_resetFiltersButton->setEnabled(
        true
        );

    updateCustomFiltersButton();
    updateTimeRangeButton();

    resizeCustomFiltersDialogToContents();
}

void InvestigationFilterPanel::
    refreshSubsystemOptions()
{
    if (m_session == nullptr) {
        return;
    }

    InvestigationFilterProxyModel *proxyModel =
        m_session
            ->investigationController()
            ->proxyModel();

    if (proxyModel == nullptr) {
        return;
    }

    const QStringList selected =
        proxyModel->subsystemFilters();

    const QSignalBlocker blocker(
        m_subsystemFilterCombo
        );

    m_subsystemFilterCombo->clear();

    int widestTextWidth =
        m_subsystemFilterCombo
            ->fontMetrics()
            .horizontalAdvance(
                QStringLiteral(
                    "All subsystems"
                    )
                );

    for (
        const QString &subsystem
        : m_session
              ->availableSubsystems()
        ) {
        m_subsystemFilterCombo
            ->addFilterItem(
                subsystem,
                subsystem
                );

        widestTextWidth =
            std::max(
                widestTextWidth,
                m_subsystemFilterCombo
                    ->fontMetrics()
                    .horizontalAdvance(
                        subsystem
                        )
                );
    }

    m_subsystemFilterCombo
        ->view()
        ->setMinimumWidth(
            std::clamp(
                widestTextWidth + 40,
                240,
                650
                )
            );

    m_subsystemFilterCombo
        ->setSelectedValues(
            selected
            );
}

void InvestigationFilterPanel::
    refreshCanonicalOptions()
{
    if (m_session == nullptr) {
        return;
    }

    auto populate =
        [](
            MultiSelectFilterComboBox *combo,
            const QStringList &values,
            const QStringList &selected
            ) {
            const QSignalBlocker blocker(
                combo
                );

            combo->clear();

            int widestTextWidth =
                0;

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

            combo
                ->view()
                ->setMinimumWidth(
                    std::clamp(
                        widestTextWidth + 50,
                        220,
                        700
                        )
                    );

            combo->setSelectedValues(
                selected
                );
        };

    InvestigationFilterProxyModel *proxyModel =
        m_session
            ->investigationController()
            ->proxyModel();

    if (proxyModel == nullptr) {
        return;
    }

    populate(
        m_eventCodeFilterCombo,
        m_session
            ->availableEventCodes(),
        proxyModel
            ->eventCodeFilters()
        );

    populate(
        m_entityFilterCombo,
        m_session
            ->availableEntities(),
        proxyModel
            ->entityFilters()
        );
}

void InvestigationFilterPanel::
    resetFilters()
{
    if (m_session == nullptr) {
        return;
    }

    m_searchDebounceTimer->stop();

    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            m_subsystemFilterCombo
            );

        const QSignalBlocker eventCodeBlocker(
            m_eventCodeFilterCombo
            );

        const QSignalBlocker entityBlocker(
            m_entityFilterCombo
            );

        const QSignalBlocker searchBlocker(
            m_searchInput
            );

        const QSignalBlocker findingBlocker(
            m_findingStatusFilterCombo
            );

        const QSignalBlocker bookmarkBlocker(
            m_bookmarksOnlyCheckBox
            );

        const QSignalBlocker customBlocker(
            m_customFieldFilterEditor
            );

        const QSignalBlocker startCheckBlocker(
            m_timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            m_timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            m_timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            m_timeRangeEndEdit
            );

        m_levelFilterCombo
            ->clearSelection();

        m_subsystemFilterCombo
            ->clearSelection();

        m_eventCodeFilterCombo
            ->clearSelection();

        m_entityFilterCombo
            ->clearSelection();

        m_searchInput->clear();

        m_findingStatusFilterCombo
            ->clearSelection();

        m_bookmarksOnlyCheckBox
            ->setChecked(
                false
                );

        m_customFieldFilterEditor
            ->clearFilters();

        m_timeRangeStartCheckBox
            ->setChecked(
                false
                );

        m_timeRangeEndCheckBox
            ->setChecked(
                false
                );

        m_timeRangeStartEdit
            ->setEnabled(
                false
                );

        m_timeRangeEndEdit
            ->setEnabled(
                false
                );

        if (
            m_session
                ->firstTimestamp()
                .has_value()
            ) {
            m_timeRangeStartEdit
                ->setDateTime(
                    m_session
                        ->firstTimestamp()
                        .value()
                    );
        }

        if (
            m_session
                ->lastTimestamp()
                .has_value()
            ) {
            m_timeRangeEndEdit
                ->setDateTime(
                    m_session
                        ->lastTimestamp()
                        .value()
                    );
        }
    }

    updateTimeRangeButton();
    updateCustomFiltersButton();

    resizeCustomFiltersDialogToContents();
}

InvestigationFilterPreset
    InvestigationFilterPanel::
    currentFilterPreset(
        const QString &name
        ) const
{
    InvestigationFilterPreset preset;

    preset.name =
        name.trimmed();

    if (m_session == nullptr) {
        return preset;
    }

    InvestigationFilterProxyModel *proxyModel =
        m_session
            ->investigationController()
            ->proxyModel();

    if (proxyModel == nullptr) {
        return preset;
    }

    preset.severities =
        proxyModel->severityFilters();

    preset.subsystems =
        proxyModel->subsystemFilters();

    preset.searchText =
        proxyModel->searchText();

    preset.findingStatuses =
        proxyModel
            ->findingStatusFilters();

    preset.bookmarkedOnly =
        proxyModel->bookmarkedOnly();

    preset.eventCodes =
        proxyModel->eventCodeFilters();

    preset.entityIds =
        proxyModel->entityFilters();

    preset.timeRangeStart =
        proxyModel->timeRangeStart();

    preset.timeRangeEnd =
        proxyModel->timeRangeEnd();

    preset.customFieldFilters =
        proxyModel
            ->customFieldFilters();

    return preset;
}

void InvestigationFilterPanel::
    applyFilterPreset(
        const InvestigationFilterPreset &preset
        )
{
    if (m_session == nullptr) {
        return;
    }

    m_searchDebounceTimer->stop();

    const bool hasTimestampData =
        m_session
            ->firstTimestamp()
            .has_value()
        && m_session
               ->lastTimestamp()
               .has_value();

    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            m_subsystemFilterCombo
            );

        const QSignalBlocker eventCodeBlocker(
            m_eventCodeFilterCombo
            );

        const QSignalBlocker entityBlocker(
            m_entityFilterCombo
            );

        const QSignalBlocker searchBlocker(
            m_searchInput
            );

        const QSignalBlocker findingBlocker(
            m_findingStatusFilterCombo
            );

        const QSignalBlocker bookmarkBlocker(
            m_bookmarksOnlyCheckBox
            );

        const QSignalBlocker customBlocker(
            m_customFieldFilterEditor
            );

        const QSignalBlocker startCheckBlocker(
            m_timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            m_timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            m_timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            m_timeRangeEndEdit
            );

        m_levelFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasSeverityData()
                    ? preset.severities
                    : QStringList()
                );

        m_subsystemFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasSubsystemData()
                    ? preset.subsystems
                    : QStringList()
                );

        m_eventCodeFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasEventCodeData()
                    ? preset.eventCodes
                    : QStringList()
                );

        m_entityFilterCombo
            ->setSelectedValues(
                m_session
                        ->hasEntityData()
                    ? preset.entityIds
                    : QStringList()
                );

        m_customFieldFilterEditor
            ->setFilters(
                m_session
                        ->hasCustomFieldData()
                    ? preset
                          .customFieldFilters
                    : CustomFieldFilterMap()
                );

        m_searchInput->setText(
            preset.searchText
            );

        m_findingStatusFilterCombo
            ->setSelectedValues(
                preset.findingStatuses
                );

        m_bookmarksOnlyCheckBox
            ->setChecked(
                preset.bookmarkedOnly
                );

        if (hasTimestampData) {
            const bool hasStart =
                preset
                    .timeRangeStart
                    .has_value();

            const bool hasEnd =
                preset
                    .timeRangeEnd
                    .has_value();

            m_timeRangeStartCheckBox
                ->setChecked(
                    hasStart
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    hasEnd
                    );

            m_timeRangeStartEdit
                ->setEnabled(
                    hasStart
                    );

            m_timeRangeEndEdit
                ->setEnabled(
                    hasEnd
                    );

            m_timeRangeStartEdit
                ->setDateTime(
                    hasStart
                        ? preset
                              .timeRangeStart
                              .value()
                        : m_session
                              ->firstTimestamp()
                              .value()
                    );

            m_timeRangeEndEdit
                ->setDateTime(
                    hasEnd
                        ? preset
                              .timeRangeEnd
                              .value()
                        : m_session
                              ->lastTimestamp()
                              .value()
                    );
        } else {
            m_timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeStartEdit
                ->setEnabled(
                    false
                    );

            m_timeRangeEndEdit
                ->setEnabled(
                    false
                    );
        }
    }

    updateTimeRangeButton();
    updateCustomFiltersButton();

    resizeCustomFiltersDialogToContents();

    emit filterChangeRequested();
}

void InvestigationFilterPanel::
    refreshFilterPresetsMenu()
{
    m_filterPresetsMenu->clear();

    QAction *saveAction =
        m_filterPresetsMenu
            ->addAction(
                tr(
                    "Save Current Filters..."
                    )
                );

    saveAction->setEnabled(
        m_session != nullptr
        && m_presetStore != nullptr
        );

    connect(
        saveAction,
        &QAction::triggered,
        this,
        [this]() {
            if (m_session == nullptr
                || m_presetStore == nullptr) {
                return;
            }

            bool accepted =
                false;

            const QString name =
                QInputDialog::getText(
                    this,
                    tr(
                        "Save Filter Preset"
                        ),
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
                existing =
                m_presetStore->presets();

            bool existingName =
                false;

            for (
                const InvestigationFilterPreset
                    &preset
                : existing
                ) {
                if (
                    preset.name.compare(
                        name,
                        Qt::CaseInsensitive
                        )
                    == 0
                    ) {
                    existingName =
                        true;

                    break;
                }
            }

            if (existingName) {
                const auto overwrite =
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

            m_presetStore->savePreset(
                currentFilterPreset(
                    name
                    )
                );
        }
        );

    m_filterPresetsMenu
        ->addSeparator();

    if (m_presetStore == nullptr) {
        return;
    }

    const QVector<
        InvestigationFilterPreset>
        presets =
        m_presetStore->presets();

    if (presets.isEmpty()) {
        QAction *emptyAction =
            m_filterPresetsMenu
                ->addAction(
                    tr("No Saved Presets")
                    );

        emptyAction->setEnabled(
            false
            );

        return;
    }

    for (
        const InvestigationFilterPreset
            &preset
        : presets
        ) {
        QAction *presetAction =
            m_filterPresetsMenu
                ->addAction(
                    preset.name
                    );

        presetAction->setEnabled(
            m_session != nullptr
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

    m_filterPresetsMenu
        ->addSeparator();

    QMenu *deleteMenu =
        m_filterPresetsMenu
            ->addMenu(
                tr("Delete Preset")
                );

    for (
        const InvestigationFilterPreset
            &preset
        : presets
        ) {
        QAction *deleteAction =
            deleteMenu->addAction(
                preset.name
                );

        connect(
            deleteAction,
            &QAction::triggered,
            this,
            [this, preset]() {
                if (m_presetStore == nullptr) {
                    return;
                }

                const auto confirmation =
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

                m_presetStore
                    ->removePreset(
                        preset.name
                        );
            }
            );
    }
}

void InvestigationFilterPanel::
    updateCustomFiltersButton()
{
    const CustomFieldFilterMap &filters =
        m_customFieldFilterEditor
            ->filters();

    int criterionCount =
        0;

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
        m_customFiltersButton
            ->setText(
                tr("Custom Filters")
                );

        m_customFiltersButton
            ->setToolTip(
                tr(
                    "No custom-field filters active"
                    )
                );

        return;
    }

    m_customFiltersButton
        ->setText(
            tr("Custom Filters (%1)")
                .arg(
                    criterionCount
                    )
            );

    QStringList descriptions;

    for (
        auto iterator =
        filters.constBegin();
        iterator != filters.constEnd();
        ++iterator
        ) {
        for (
            const QString &value
            : iterator.value()
            ) {
            descriptions.append(
                QStringLiteral(
                    "%1 = %2"
                    )
                    .arg(
                        iterator.key(),
                        value
                        )
                );
        }
    }

    m_customFiltersButton
        ->setToolTip(
            descriptions.join(
                QStringLiteral("\n")
                )
            );
}

void InvestigationFilterPanel::
    updateTimeRangeButton()
{
    const bool hasStart =
        m_timeRangeStartCheckBox
            ->isChecked();

    const bool hasEnd =
        m_timeRangeEndCheckBox
            ->isChecked();

    if (!hasStart && !hasEnd) {
        m_timeRangeButton->setText(
            tr("Time Range")
            );

        m_timeRangeButton->setToolTip(
            tr(
                "No time-range filter active"
                )
            );

        return;
    }

    QStringList parts;

    if (hasStart) {
        parts.append(
            tr("From %1")
                .arg(
                    m_timeRangeStartEdit
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
                    m_timeRangeEndEdit
                        ->dateTime()
                        .toString(
                            QStringLiteral(
                                "yyyy-MM-dd HH:mm:ss.zzz"
                                )
                            )
                    )
            );
    }

    m_timeRangeButton->setText(
        tr("Time Range (Active)")
        );

    m_timeRangeButton->setToolTip(
        parts.join(
            QStringLiteral("\n")
            )
        );
}

void InvestigationFilterPanel::
    resizeCustomFiltersDialogToContents()
{
    QTimer::singleShot(
        0,
        m_customFiltersDialog,
        [this]() {
            m_customFiltersDialog
                ->adjustSize();
        }
        );
}

bool InvestigationFilterPanel::
    addCustomFieldFilter(
        const QString &fieldName,
        const QString &value
        )
{
    if (m_session == nullptr
        || fieldName.isEmpty()
        || value.isEmpty()) {
        return false;
    }

    const CustomFieldFilterMap before =
        m_customFieldFilterEditor
            ->filters();

    {
        const QSignalBlocker blocker(
            m_customFieldFilterEditor
            );

        m_customFieldFilterEditor
            ->addFilter(
                fieldName,
                value
                );
    }

    const bool changed =
        before
        != m_customFieldFilterEditor
               ->filters();

    if (changed) {
        updateCustomFiltersButton();

        resizeCustomFiltersDialogToContents();
    }

    return changed;
}

bool InvestigationFilterPanel::
    configureIssueDrillDown(
        const QString &subsystem,
        QStringList targetSeverities
        )
{
    if (m_session == nullptr
        || subsystem.isEmpty()
        || targetSeverities.isEmpty()
        || !m_session->hasSeverityData()
        || !m_session->hasSubsystemData()) {
        return false;
    }

    const QStringList currentSeverities =
        m_levelFilterCombo
            ->selectedValues();

    if (!currentSeverities.isEmpty()) {
        QStringList intersection;

        for (
            const QString &severity
            : targetSeverities
            ) {
            if (
                currentSeverities.contains(
                    severity
                    )
                ) {
                intersection.append(
                    severity
                    );
            }
        }

        if (intersection.isEmpty()) {
            return false;
        }

        targetSeverities =
            std::move(
                intersection
                );
    }

    m_searchDebounceTimer->stop();

    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            m_subsystemFilterCombo
            );

        m_levelFilterCombo
            ->setSelectedValues(
                targetSeverities
                );

        m_subsystemFilterCombo
            ->setSelectedValues(
                QStringList {
                    subsystem
                }
                );
    }

    return true;
}

bool InvestigationFilterPanel::
    configureTimelineDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
        const QString &severity,
        const QString &subsystem
        )
{
    if (m_session == nullptr
        || !startTimestamp.isValid()
        || !endTimestamp.isValid()
        || startTimestamp
               > endTimestamp) {
        return false;
    }

    m_searchDebounceTimer->stop();

    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker subsystemBlocker(
            m_subsystemFilterCombo
            );

        const QSignalBlocker startCheckBlocker(
            m_timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            m_timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            m_timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            m_timeRangeEndEdit
            );

        if (!severity.isEmpty()) {
            m_levelFilterCombo
                ->setSelectedValues(
                    QStringList {
                        severity
                    }
                    );
        }

        if (!subsystem.isEmpty()) {
            m_subsystemFilterCombo
                ->setSelectedValues(
                    QStringList {
                        subsystem
                    }
                    );
        }

        m_timeRangeStartEdit
            ->setDateTime(
                startTimestamp
                );

        m_timeRangeEndEdit
            ->setDateTime(
                endTimestamp
                );

        m_timeRangeStartCheckBox
            ->setChecked(
                true
                );

        m_timeRangeEndCheckBox
            ->setChecked(
                true
                );

        m_timeRangeStartEdit
            ->setEnabled(
                true
                );

        m_timeRangeEndEdit
            ->setEnabled(
                true
                );
    }

    updateTimeRangeButton();

    return true;
}

bool InvestigationFilterPanel::
    configureBurstDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        )
{
    if (m_session == nullptr
        || !startTimestamp.isValid()
        || !endTimestamp.isValid()) {
        return false;
    }

    const QStringList currentSeverities =
        m_levelFilterCombo
            ->selectedValues();

    const QStringList elevatedSeverities = {
        QStringLiteral("WARN"),
        QStringLiteral("ERROR"),
        QStringLiteral("CRITICAL")
    };

    QStringList selectedSeverities;

    if (currentSeverities.isEmpty()) {
        selectedSeverities =
            elevatedSeverities;
    } else {
        for (
            const QString &severity
            : currentSeverities
            ) {
            if (
                elevatedSeverities.contains(
                    severity
                    )
                ) {
                selectedSeverities.append(
                    severity
                    );
            }
        }
    }

    if (selectedSeverities.isEmpty()) {
        return false;
    }

    QDateTime start =
        startTimestamp;

    QDateTime end =
        endTimestamp;

    /*
     * Burst drill-down narrows an existing time
     * window but never widens it.
     */
    if (
        m_timeRangeStartCheckBox
            ->isChecked()
        && m_timeRangeStartEdit
                   ->dateTime()
               > start
        ) {
        start =
            m_timeRangeStartEdit
                ->dateTime();
    }

    if (
        m_timeRangeEndCheckBox
            ->isChecked()
        && m_timeRangeEndEdit
                   ->dateTime()
               < end
        ) {
        end =
            m_timeRangeEndEdit
                ->dateTime();
    }

    if (start > end) {
        return false;
    }

    m_searchDebounceTimer->stop();

    {
        const QSignalBlocker severityBlocker(
            m_levelFilterCombo
            );

        const QSignalBlocker startCheckBlocker(
            m_timeRangeStartCheckBox
            );

        const QSignalBlocker startEditBlocker(
            m_timeRangeStartEdit
            );

        const QSignalBlocker endCheckBlocker(
            m_timeRangeEndCheckBox
            );

        const QSignalBlocker endEditBlocker(
            m_timeRangeEndEdit
            );

        m_levelFilterCombo
            ->setSelectedValues(
                selectedSeverities
                );

        m_timeRangeStartCheckBox
            ->setChecked(
                true
                );

        m_timeRangeEndCheckBox
            ->setChecked(
                true
                );

        m_timeRangeStartEdit
            ->setEnabled(
                true
                );

        m_timeRangeEndEdit
            ->setEnabled(
                true
                );

        m_timeRangeStartEdit
            ->setDateTime(
                start
                );

        m_timeRangeEndEdit
            ->setDateTime(
                end
                );
    }

    updateTimeRangeButton();

    return true;
}

bool InvestigationFilterPanel::
    revealRecord(
        const InvestigationRecord &record,
        const InvestigationFilterMatch &match,
        const InvestigationRecordState &state
        )
{
    if (m_session == nullptr
        || match.allMatch()) {
        return false;
    }

    m_searchDebounceTimer->stop();

    const QSignalBlocker severityBlocker(
        m_levelFilterCombo
        );

    const QSignalBlocker subsystemBlocker(
        m_subsystemFilterCombo
        );

    const QSignalBlocker eventCodeBlocker(
        m_eventCodeFilterCombo
        );

    const QSignalBlocker entityBlocker(
        m_entityFilterCombo
        );

    const QSignalBlocker searchBlocker(
        m_searchInput
        );

    const QSignalBlocker startCheckBlocker(
        m_timeRangeStartCheckBox
        );

    const QSignalBlocker startEditBlocker(
        m_timeRangeStartEdit
        );

    const QSignalBlocker endCheckBlocker(
        m_timeRangeEndCheckBox
        );

    const QSignalBlocker endEditBlocker(
        m_timeRangeEndEdit
        );

    const QSignalBlocker customBlocker(
        m_customFieldFilterEditor
        );

    const QSignalBlocker findingBlocker(
        m_findingStatusFilterCombo
        );

    const QSignalBlocker bookmarkBlocker(
        m_bookmarksOnlyCheckBox
        );

    /*
     * Severity
     */
    if (!match.severity) {
        if (record.severity.has_value()) {
            QStringList selected =
                m_levelFilterCombo
                    ->selectedValues();

            const QString value =
                recordSeverityToString(
                    record
                        .severity
                        .value()
                    );

            if (!selected.contains(
                    value
                    )) {
                selected.append(
                    value
                    );
            }

            m_levelFilterCombo
                ->setSelectedValues(
                    selected
                    );
        } else {
            m_levelFilterCombo
                ->clearSelection();
        }
    }

    /*
     * Subsystem
     */
    if (!match.subsystem) {
        if (record.subsystem.has_value()) {
            QStringList selected =
                m_subsystemFilterCombo
                    ->selectedValues();

            const QString value =
                record
                    .subsystem
                    .value();

            if (!selected.contains(
                    value
                    )) {
                selected.append(
                    value
                    );
            }

            m_subsystemFilterCombo
                ->setSelectedValues(
                    selected
                    );
        } else {
            m_subsystemFilterCombo
                ->clearSelection();
        }
    }

    /*
     * Event code
     */
    if (!match.eventCode) {
        if (record.eventCode.has_value()) {
            QStringList selected =
                m_eventCodeFilterCombo
                    ->selectedValues();

            const QString value =
                record
                    .eventCode
                    .value();

            if (!selected.contains(
                    value
                    )) {
                selected.append(
                    value
                    );
            }

            m_eventCodeFilterCombo
                ->setSelectedValues(
                    selected
                    );
        } else {
            m_eventCodeFilterCombo
                ->clearSelection();
        }
    }

    /*
     * Entity
     */
    if (!match.entity) {
        if (record.entityId.has_value()) {
            QStringList selected =
                m_entityFilterCombo
                    ->selectedValues();

            const QString value =
                record
                    .entityId
                    .value();

            if (!selected.contains(
                    value
                    )) {
                selected.append(
                    value
                    );
            }

            m_entityFilterCombo
                ->setSelectedValues(
                    selected
                    );
        } else {
            m_entityFilterCombo
                ->clearSelection();
        }
    }

    /*
     * Free-text search cannot be widened
     * deterministically, so clear it only when it
     * excludes the target.
     */
    if (!match.search) {
        m_searchInput->clear();
    }

    /*
     * Time range.
     */
    if (!match.timeRange) {
        if (!record.timestamp.has_value()) {
            m_timeRangeStartCheckBox
                ->setChecked(
                    false
                    );

            m_timeRangeEndCheckBox
                ->setChecked(
                    false
                    );
        } else {
            const QDateTime timestamp =
                record
                    .timestamp
                    ->toTimeZone(
                        QTimeZone::UTC
                        );

            if (
                m_timeRangeStartCheckBox
                    ->isChecked()
                && timestamp
                       < m_timeRangeStartEdit
                             ->dateTime()
                ) {
                m_timeRangeStartEdit
                    ->setDateTime(
                        timestamp
                        );
            }

            if (
                m_timeRangeEndCheckBox
                    ->isChecked()
                && timestamp
                       > m_timeRangeEndEdit
                             ->dateTime()
                ) {
                m_timeRangeEndEdit
                    ->setDateTime(
                        timestamp
                        );
            }
        }
    }

    /*
     * Custom-field criteria.
     */
    if (!match.customFields) {
        CustomFieldFilterMap filters =
            m_customFieldFilterEditor
                ->filters();

        for (
            auto iterator =
            filters.begin();
            iterator != filters.end();
            ) {
            const auto attributeIterator =
                record
                    .customAttributes
                    .constFind(
                        iterator.key()
                        );

            if (
                attributeIterator
                == record
                       .customAttributes
                       .constEnd()
                ) {
                iterator =
                    filters.erase(
                        iterator
                        );

                continue;
            }

            const QString value =
                attributeIterator
                    .value()
                    .toString();

            if (
                !iterator
                     .value()
                     .contains(
                         value
                         )
                ) {
                iterator
                    .value()
                    .append(
                        value
                        );
            }

            ++iterator;
        }

        m_customFieldFilterEditor
            ->setFilters(
                filters
                );
    }

    /*
     * Finding status.
     */
    if (!match.findingStatus) {
        QString statusValue;

        switch (state.findingStatus) {
        case FindingStatus::Open:
            statusValue =
                QStringLiteral("OPEN");
            break;

        case FindingStatus::Resolved:
            statusValue =
                QStringLiteral(
                    "RESOLVED"
                    );
            break;

        case FindingStatus::Dismissed:
            statusValue =
                QStringLiteral(
                    "DISMISSED"
                    );
            break;

        case FindingStatus::None:
            break;
        }

        if (!statusValue.isEmpty()) {
            QStringList selected =
                m_findingStatusFilterCombo
                    ->selectedValues();

            if (!selected.contains(
                    statusValue
                    )) {
                selected.append(
                    statusValue
                    );
            }

            m_findingStatusFilterCombo
                ->setSelectedValues(
                    selected
                    );
        } else {
            m_findingStatusFilterCombo
                ->clearSelection();
        }
    }

    if (!match.bookmark) {
        m_bookmarksOnlyCheckBox
            ->setChecked(
                false
                );
    }

    updateTimeRangeButton();
    updateCustomFiltersButton();

    return true;
}

bool InvestigationFilterPanel::
    hasFindingStatusFilter() const
{
    return !m_findingStatusFilterCombo
                ->selectedValues()
                .isEmpty();
}

bool InvestigationFilterPanel::
    bookmarksOnly() const
{
    return m_bookmarksOnlyCheckBox
        ->isChecked();
}

int InvestigationFilterPanel::
    secondaryWideLayoutMinimumWidth() const
{
    const auto controlWidth =
        [](const QWidget *widget) {
            if (widget == nullptr) {
                return 0;
            }

            return std::max(
                widget->minimumWidth(),
                widget->minimumSizeHint()
                    .width()
                );
        };

    /*
     * Give search a useful minimum amount of room
     * in the full-width presentation instead of
     * switching layouts only once it has collapsed
     * to an impractically tiny field.
     */
    constexpr int PreferredSearchWidth =
        260;

    const int spacing =
        m_secondaryFilterLayout != nullptr
            ? std::max(
                  0,
                  m_secondaryFilterLayout
                      ->horizontalSpacing()
                  )
            : 8;

    return PreferredSearchWidth
           + controlWidth(
               m_findingStatusFilterCombo
               )
           + controlWidth(
               m_bookmarksOnlyCheckBox
               )
           + controlWidth(
               m_timeRangeButton
               )
           + controlWidth(
               m_customFiltersButton
               )
           + controlWidth(
               m_filterPresetsButton
               )
           + controlWidth(
               m_resetFiltersButton
               )
           + spacing * 6;
}

void InvestigationFilterPanel::
    updateResponsiveLayout()
{
    if (m_secondaryFilterLayout
        == nullptr) {
        return;
    }

    const bool compact =
        contentsRect().width()
        < secondaryWideLayoutMinimumWidth();

    if (compact
        == m_secondaryLayoutCompact) {
        return;
    }

    m_secondaryLayoutCompact =
        compact;

    const QList<QWidget *> controls = {
        m_searchInput,
        m_findingStatusFilterCombo,
        m_bookmarksOnlyCheckBox,
        m_timeRangeButton,
        m_customFiltersButton,
        m_filterPresetsButton,
        m_resetFiltersButton
    };

    for (QWidget *control
         : controls) {
        m_secondaryFilterLayout
            ->removeWidget(
                control
                );
    }

    if (!compact) {
        /*
         * Normal desktop layout:
         *
         * Search | Status | Bookmarks | actions...
         */
        m_secondaryFilterLayout->addWidget(
            m_searchInput,
            0,
            0
            );

        m_secondaryFilterLayout->addWidget(
            m_findingStatusFilterCombo,
            0,
            1
            );

        m_secondaryFilterLayout->addWidget(
            m_bookmarksOnlyCheckBox,
            0,
            2
            );

        m_secondaryFilterLayout->addWidget(
            m_timeRangeButton,
            0,
            3
            );

        m_secondaryFilterLayout->addWidget(
            m_customFiltersButton,
            0,
            4
            );

        m_secondaryFilterLayout->addWidget(
            m_filterPresetsButton,
            0,
            5
            );

        m_secondaryFilterLayout->addWidget(
            m_resetFiltersButton,
            0,
            6
            );
    } else {
        /*
         * Narrow detached/split-screen layout:
         *
         * Search | Status | Bookmarks
         * Time | Custom | Presets | Reset
         */
        m_secondaryFilterLayout->addWidget(
            m_searchInput,
            0,
            0,
            1,
            2
            );

        m_secondaryFilterLayout->addWidget(
            m_findingStatusFilterCombo,
            0,
            2
            );

        m_secondaryFilterLayout->addWidget(
            m_bookmarksOnlyCheckBox,
            0,
            3
            );

        m_secondaryFilterLayout->addWidget(
            m_timeRangeButton,
            1,
            0
            );

        m_secondaryFilterLayout->addWidget(
            m_customFiltersButton,
            1,
            1
            );

        m_secondaryFilterLayout->addWidget(
            m_filterPresetsButton,
            1,
            2
            );

        m_secondaryFilterLayout->addWidget(
            m_resetFiltersButton,
            1,
            3
            );
    }

    m_secondaryFilterLayout->setColumnStretch(
        0,
        1
        );

    m_secondaryFilterLayout->invalidate();
}

void InvestigationFilterPanel::
    resizeEvent(
        QResizeEvent *event
        )
{
    QWidget::resizeEvent(
        event
        );

    updateResponsiveLayout();
}