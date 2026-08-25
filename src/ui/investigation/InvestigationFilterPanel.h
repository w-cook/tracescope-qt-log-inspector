#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "../../domain/InvestigationRecord.h"
#include "../../domain/InvestigationRecordState.h"
#include "../../filtering/InvestigationFilterPreset.h"
#include "../../models/InvestigationFilterProxyModel.h"

class CustomFieldFilterEditor;
class FilterPresetStore;
class InvestigationSession;
class MultiSelectFilterComboBox;
class QCheckBox;
class QDateTimeEdit;
class QDialog;
class QLineEdit;
class QMenu;
class QPushButton;
class QTimer;

class InvestigationFilterPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationFilterPanel(
        FilterPresetStore *presetStore,
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    /*
     * Apply the currently displayed filter-control
     * state to the bound session's controller.
     *
     * MainWindow remains responsible for refreshing
     * the other investigation components after the
     * controller state changes.
     */
    void applyToSession();

    /*
     * Cross-component filter mutations.
     *
     * These methods update the filter controls as
     * one signal-blocked transaction but do not emit
     * filterChangeRequested(). The caller performs
     * one coordinated apply afterward.
     */
    bool addCustomFieldFilter(
        const QString &fieldName,
        const QString &value
        );

    bool configureIssueDrillDown(
        const QString &subsystem,
        QStringList targetSeverities
        );

    bool configureTimelineDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
        const QString &severity,
        const QString &subsystem
        );

    bool configureBurstDrillDown(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        );

    bool revealRecord(
        const InvestigationRecord &record,
        const InvestigationFilterMatch &match,
        const InvestigationRecordState &state
        );

    bool hasFindingStatusFilter() const;

    bool bookmarksOnly() const;

signals:
    /*
     * User interaction inside the filter surface
     * requests one coordinated investigation
     * refresh from MainWindow.
     */
    void filterChangeRequested();

private:
    void syncFromSession();

    void refreshSubsystemOptions();

    void refreshCanonicalOptions();

    void resetFilters();

    InvestigationFilterPreset currentFilterPreset(
        const QString &name
        ) const;

    void applyFilterPreset(
        const InvestigationFilterPreset &preset
        );

    void refreshFilterPresetsMenu();

    void updateCustomFiltersButton();

    void updateTimeRangeButton();

    void resizeCustomFiltersDialogToContents();

    FilterPresetStore *m_presetStore =
        nullptr;

    InvestigationSession *m_session =
        nullptr;

    MultiSelectFilterComboBox
        *m_levelFilterCombo = nullptr;

    MultiSelectFilterComboBox
        *m_subsystemFilterCombo = nullptr;

    MultiSelectFilterComboBox
        *m_eventCodeFilterCombo = nullptr;

    MultiSelectFilterComboBox
        *m_entityFilterCombo = nullptr;

    MultiSelectFilterComboBox
        *m_findingStatusFilterCombo = nullptr;

    QLineEdit *m_searchInput =
        nullptr;

    QCheckBox *m_bookmarksOnlyCheckBox =
        nullptr;

    QPushButton *m_resetFiltersButton =
        nullptr;

    QPushButton *m_filterPresetsButton =
        nullptr;

    QMenu *m_filterPresetsMenu =
        nullptr;

    QWidget *m_eventCodeFilterWidget =
        nullptr;

    QWidget *m_entityFilterWidget =
        nullptr;

    QPushButton *m_customFiltersButton =
        nullptr;

    QDialog *m_customFiltersDialog =
        nullptr;

    CustomFieldFilterEditor
        *m_customFieldFilterEditor = nullptr;

    QPushButton *m_timeRangeButton =
        nullptr;

    QDialog *m_timeRangeDialog =
        nullptr;

    QWidget *m_timeRangeFilterWidget =
        nullptr;

    QCheckBox *m_timeRangeStartCheckBox =
        nullptr;

    QDateTimeEdit *m_timeRangeStartEdit =
        nullptr;

    QCheckBox *m_timeRangeEndCheckBox =
        nullptr;

    QDateTimeEdit *m_timeRangeEndEdit =
        nullptr;

    QTimer *m_searchDebounceTimer =
        nullptr;
};