#include "InvestigationTableModel.h"

#include <QSet>

#include <algorithm>
#include <utility>

#include "../domain/RecordSeverity.h"

namespace
{
const QString TimestampKey = QStringLiteral("timestamp");
const QString SeverityKey = QStringLiteral("severity");
const QString SubsystemKey = QStringLiteral("subsystem");
const QString EventCodeKey = QStringLiteral("eventCode");
const QString EntityIdKey = QStringLiteral("entityId");
const QString MessageKey = QStringLiteral("message");

bool isCanonicalKey(const QString &key)
{
    return key == TimestampKey
           || key == SeverityKey
           || key == SubsystemKey
           || key == EventCodeKey
           || key == EntityIdKey
           || key == MessageKey;
}
}

InvestigationTableModel::InvestigationTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    rebuildColumns();
}

int InvestigationTableModel::rowCount(
    const QModelIndex &parent
    ) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_records.size();
}

int InvestigationTableModel::columnCount(
    const QModelIndex &parent
    ) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_columns.size();
}

QVariant InvestigationTableModel::data(
    const QModelIndex &index,
    int role
    ) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_records.size()
        || index.column() < 0
        || index.column() >= m_columns.size()) {
        return {};
    }

    const InvestigationRecord &record =
        m_records[index.row()];

    const ColumnDefinition &column =
        m_columns[index.column()];

    if (role == Qt::DisplayRole) {
        return displayValue(record, column);
    }

    if (role == SortRole) {
        return sortValue(record, column);
    }

    return {};
}

QVariant InvestigationTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
    ) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return section + 1;
    }

    if (section < 0 || section >= m_columns.size()) {
        return {};
    }

    return m_columns[section].header;
}

void InvestigationTableModel::setRecords(
    const QVector<InvestigationRecord> &records
    )
{
    beginResetModel();

    m_records = records;
    rebuildColumns();

    endResetModel();
}

const QVector<InvestigationRecord> &
InvestigationTableModel::records() const
{
    return m_records;
}

const InvestigationRecord *
InvestigationTableModel::recordAt(int row) const
{
    if (row < 0 || row >= m_records.size()) {
        return nullptr;
    }

    return &m_records[row];
}

void InvestigationTableModel::rebuildColumns()
{
    m_columns = {
        {
            TimestampKey,
            QStringLiteral("Timestamp"),
            false
        },
        {
            SeverityKey,
            QStringLiteral("Level"),
            false
        },
        {
            SubsystemKey,
            QStringLiteral("Subsystem"),
            false
        },
        {
            EventCodeKey,
            QStringLiteral("Event Code"),
            false
        },
        {
            EntityIdKey,
            QStringLiteral("Entity ID"),
            false
        },
        {
            MessageKey,
            QStringLiteral("Message"),
            false
        }
    };

    QSet<QString> customKeys;

    for (const InvestigationRecord &record : std::as_const(m_records)) {
        for (
            auto iterator = record.customAttributes.constBegin();
            iterator != record.customAttributes.constEnd();
            ++iterator
            ) {
            if (!isCanonicalKey(iterator.key())) {
                customKeys.insert(iterator.key());
            }
        }
    }

    QList<QString> sortedKeys = customKeys.values();

    std::sort(
        sortedKeys.begin(),
        sortedKeys.end(),
        [](const QString &left, const QString &right) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );

    for (const QString &key : std::as_const(sortedKeys)) {
        m_columns.append(
            {
                key,
                key,
                true
            }
            );
    }
}

QVariant InvestigationTableModel::displayValue(
    const InvestigationRecord &record,
    const ColumnDefinition &column
    ) const
{
    if (column.custom) {
        return record.customAttributes.value(column.key);
    }

    if (column.key == TimestampKey) {
        if (!record.timestamp.has_value()) {
            return {};
        }

        return record.timestamp->toString(Qt::ISODateWithMs);
    }

    if (column.key == SeverityKey) {
        if (!record.severity.has_value()) {
            return {};
        }

        return recordSeverityToString(
            record.severity.value()
            );
    }

    if (column.key == SubsystemKey) {
        return record.subsystem.has_value()
        ? QVariant(record.subsystem.value())
        : QVariant();
    }

    if (column.key == EventCodeKey) {
        return record.eventCode.has_value()
        ? QVariant(record.eventCode.value())
        : QVariant();
    }

    if (column.key == EntityIdKey) {
        return record.entityId.has_value()
        ? QVariant(record.entityId.value())
        : QVariant();
    }

    if (column.key == MessageKey) {
        return record.message.has_value()
        ? QVariant(record.message.value())
        : QVariant();
    }

    return {};
}

QVariant InvestigationTableModel::sortValue(
    const InvestigationRecord &record,
    const ColumnDefinition &column
    ) const
{
    if (column.custom) {
        return record.customAttributes.value(column.key);
    }

    if (column.key == TimestampKey) {
        return record.timestamp.has_value()
        ? QVariant(record.timestamp.value())
        : QVariant();
    }

    if (column.key == SeverityKey) {
        return record.severity.has_value()
        ? QVariant::fromValue(
              static_cast<int>(
                  record.severity.value()
                  )
              )
        : QVariant();
    }

    return displayValue(record, column);
}