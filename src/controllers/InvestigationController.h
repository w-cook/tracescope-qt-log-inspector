#pragma once

#include <QObject>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "../models/InvestigationFilterProxyModel.h"
#include "../models/InvestigationTableModel.h"

class InvestigationController : public QObject
{
    Q_OBJECT

public:
    explicit InvestigationController(
        QObject *parent = nullptr
        );

    InvestigationTableModel *sourceModel();
    InvestigationFilterProxyModel *proxyModel();

    void setRecords(
        const QVector<InvestigationRecord> &records
        );

    void setFilters(
        const QString &severity,
        const QString &subsystem,
        const QString &searchText
        );

    int totalRecordCount() const;

    QStringList availableSubsystems() const;

    QVector<InvestigationRecord> visibleRecords() const;

    const QVector<InvestigationRecord> &
    allRecords() const;

    const InvestigationRecord *recordForProxyIndex(
        const QModelIndex &proxyIndex
        ) const;

private:
    InvestigationTableModel m_sourceModel;
    InvestigationFilterProxyModel m_proxyModel;
};