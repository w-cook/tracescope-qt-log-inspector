#pragma once

#include <optional>

#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringMatcher>
#include <QStringList>
#include <QMap>

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
    QStringList m_severityFilters;
    QStringList m_subsystemFilters;
    QString m_searchText;
    QStringList m_eventCodeFilters;
    QStringList m_entityFilters;

    CustomFieldFilterMap m_customFieldFilters;

    std::optional<QDateTime> m_timeRangeStart;
    std::optional<QDateTime> m_timeRangeEnd;

    QStringMatcher m_searchMatcher;

    const InvestigationTableModel *
    investigationModel() const;

    bool matchesSearchText(
        const InvestigationRecord &record
        ) const;
};