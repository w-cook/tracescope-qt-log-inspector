#pragma once

#include <optional>

#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringMatcher>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QHash>

#include "../domain/InvestigationRecordState.h"

#include "InvestigationTableModel.h"

using CustomFieldFilterMap =
    QMap<QString, QStringList>;

class InvestigationFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit InvestigationFilterProxyModel(
        QObject *parent = nullptr
        );

    void setFilterState(
        const QStringList &severities,
        const QStringList &subsystems,
        const QString &searchText,
        const QStringList &eventCodes,
        const QStringList &entityIds,
        const std::optional<QDateTime> &startTime,
        const std::optional<QDateTime> &endTime,
        const CustomFieldFilterMap &customFieldFilters,
        const QStringList &findingStatuses,
        bool bookmarkedOnly
        );

    void setBookmarkedOnly(
        bool bookmarkedOnly
        );

    bool bookmarkedOnly() const;

    void setSeverityFilters(const QStringList &severities);
    void setSeverityFilter(const QString &severity);
    void setSubsystemFilters(
        const QStringList &subsystems
        );

    void setSubsystemFilter(
        const QString &subsystem
        );
    void setSearchText(const QString &searchText);

    void setTimeRangeFilter(
        const std::optional<QDateTime> &startTime,
        const std::optional<QDateTime> &endTime
        );

    void setEventCodeFilters(
        const QStringList &eventCodes
        );

    void setEntityFilters(
        const QStringList &entityIds
        );

    void setCustomFieldFilters(
        const CustomFieldFilterMap &filters
        );

    void setFindingStatusFilters(
        const QStringList &statuses
        );

    QStringList findingStatusFilters() const;

    void setBookmarkedRecordIds(
        const QSet<QString> &recordIds
        );

    void setInvestigationStateIndicators(
        const QSet<QString> &bookmarkedRecordIds,
        const QSet<QString> &notedRecordIds,
        const QHash<QString, FindingStatus>
            &findingStatuses
        );

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
        ) const override;

    QStringList eventCodeFilters() const;
    QStringList entityFilters() const;

    const std::optional<QDateTime> &
    timeRangeStart() const;

    const std::optional<QDateTime> &
    timeRangeEnd() const;

    QStringList severityFilters() const;
    QString severityFilter() const;
    QStringList subsystemFilters() const;
    QString subsystemFilter() const;
    QString searchText() const;

    const CustomFieldFilterMap &
    customFieldFilters() const;

protected:
    bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent
        ) const override;

private:
    bool m_bookmarkedOnly = false;

    QStringList m_severityFilters;
    QStringList m_subsystemFilters;
    QString m_searchText;
    QStringList m_eventCodeFilters;
    QStringList m_entityFilters;

    CustomFieldFilterMap m_customFieldFilters;

    QStringList m_findingStatusFilters;

    std::optional<QDateTime> m_timeRangeStart;
    std::optional<QDateTime> m_timeRangeEnd;

    QStringMatcher m_searchMatcher;

    QSet<QString> m_bookmarkedRecordIds;

    QSet<QString> m_notedRecordIds;

    QHash<QString, FindingStatus>
        m_findingStatuses;

    const InvestigationTableModel *
    investigationModel() const;

    bool matchesSearchText(
        const InvestigationRecord &record
        ) const;
};