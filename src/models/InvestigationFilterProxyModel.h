#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QStringMatcher>

#include "InvestigationTableModel.h"

class InvestigationFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit InvestigationFilterProxyModel(
        QObject *parent = nullptr
        );

    void setSeverityFilter(const QString &severity);
    void setSubsystemFilter(const QString &subsystem);
    void setSearchText(const QString &searchText);

    QString severityFilter() const;
    QString subsystemFilter() const;
    QString searchText() const;

protected:
    bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent
        ) const override;

private:
    QString m_severityFilter;
    QString m_subsystemFilter;
    QString m_searchText;

    QStringMatcher m_searchMatcher;

    const InvestigationTableModel *
    investigationModel() const;

    bool matchesSearchText(
        const InvestigationRecord &record
        ) const;
};