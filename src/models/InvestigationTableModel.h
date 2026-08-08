#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <QVector>

#include "../domain/InvestigationRecord.h"

class InvestigationTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum DataRole
    {
        SortRole = Qt::UserRole + 1
    };

    explicit InvestigationTableModel(QObject *parent = nullptr);

    int rowCount(
        const QModelIndex &parent = QModelIndex()
        ) const override;

    int columnCount(
        const QModelIndex &parent = QModelIndex()
        ) const override;

    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole
        ) const override;

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
        ) const override;

    void setRecords(const QVector<InvestigationRecord> &records);

    const QVector<InvestigationRecord> &records() const;

    const InvestigationRecord *recordAt(int row) const;

private:
    struct ColumnDefinition
    {
        QString key;
        QString header;
        bool custom = false;
    };

    QVector<InvestigationRecord> m_records;
    QVector<ColumnDefinition> m_columns;

    void rebuildColumns();

    QVariant displayValue(
        const InvestigationRecord &record,
        const ColumnDefinition &column
        ) const;

    QVariant sortValue(
        const InvestigationRecord &record,
        const ColumnDefinition &column
        ) const;
};