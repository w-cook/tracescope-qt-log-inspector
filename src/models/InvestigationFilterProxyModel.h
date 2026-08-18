#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QStringMatcher>
#include <QStringList>

#include "InvestigationTableModel.h"

class InvestigationFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit InvestigationFilterProxyModel(
        QObject *parent = nullptr
        );

    void setSeverityFilters(const QStringList &severities);
    void setSeverityFilter(const QString &severity);
    void setSubsystemFilter(const QString &subsystem);
    void setSearchText(const QString &searchText);

    QStringList severityFilters() const;
    QString severityFilter() const;
    QString subsystemFilter() const;
    QString searchText() const;

protected:
    bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent
        ) const override;

private:
    QStringList m_severityFilters;
    QString m_subsystemFilter;
    QString m_searchText;

    QStringMatcher m_searchMatcher;

    const InvestigationTableModel *
    investigationModel() const;

    bool matchesSearchText(
        const InvestigationRecord &record
        ) const;
};