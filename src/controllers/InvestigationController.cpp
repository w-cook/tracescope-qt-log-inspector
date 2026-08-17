#include "InvestigationController.h"

#include <QSet>

#include <algorithm>

InvestigationController::InvestigationController(
    QObject *parent
    )
    : QObject(parent)
{
    m_proxyModel.setSourceModel(
        &m_sourceModel
        );
}

InvestigationTableModel *
InvestigationController::sourceModel()
{
    return &m_sourceModel;
}

InvestigationFilterProxyModel *
InvestigationController::proxyModel()
{
    return &m_proxyModel;
}

void InvestigationController::setRecords(
    const QVector<InvestigationRecord> &records
    )
{
    m_sourceModel.setRecords(records);
}

void InvestigationController::setFilters(
    const QString &severity,
    const QString &subsystem,
    const QString &searchText
    )
{
    m_proxyModel.setSeverityFilter(severity);
    m_proxyModel.setSubsystemFilter(subsystem);
    m_proxyModel.setSearchText(searchText);
}

int InvestigationController::totalRecordCount() const
{
    return m_sourceModel.rowCount();
}

QStringList InvestigationController::
    availableSubsystems() const
{
    QSet<QString> subsystems;

    const QVector<InvestigationRecord> &records =
        m_sourceModel.records();

    for (const InvestigationRecord &record : records) {
        if (record.subsystem.has_value()
            && !record.subsystem->isEmpty()) {
            subsystems.insert(
                record.subsystem.value()
                );
        }
    }

    QStringList sortedSubsystems =
        subsystems.values();

    std::sort(
        sortedSubsystems.begin(),
        sortedSubsystems.end(),
        [](const QString &left, const QString &right) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );

    return sortedSubsystems;
}

QVector<InvestigationRecord>
InvestigationController::visibleRecords() const
{
    QVector<InvestigationRecord> records;

    records.reserve(
        m_proxyModel.rowCount()
        );

    for (
        int proxyRow = 0;
        proxyRow < m_proxyModel.rowCount();
        ++proxyRow
        ) {
        const QModelIndex proxyIndex =
            m_proxyModel.index(
                proxyRow,
                0
                );

        const InvestigationRecord *record =
            recordForProxyIndex(proxyIndex);

        if (record != nullptr) {
            records.append(*record);
        }
    }

    return records;
}

const QVector<InvestigationRecord> &
InvestigationController::allRecords() const
{
    return m_sourceModel.records();
}

const InvestigationRecord *
InvestigationController::recordForProxyIndex(
    const QModelIndex &proxyIndex
    ) const
{
    if (!proxyIndex.isValid()) {
        return nullptr;
    }

    const QModelIndex sourceIndex =
        m_proxyModel.mapToSource(
            proxyIndex
            );

    if (!sourceIndex.isValid()) {
        return nullptr;
    }

    return m_sourceModel.recordAt(
        sourceIndex.row()
        );
}

QVector<InvestigationRecord>
InvestigationController::recordsForAnalysis() const
{
    const bool filtersActive =
        !m_proxyModel
             .severityFilter()
             .isEmpty()
        || !m_proxyModel
                .subsystemFilter()
                .isEmpty()
        || !m_proxyModel
                .searchText()
                .isEmpty();

    if (!filtersActive) {
        return m_sourceModel.records();
    }

    return visibleRecords();
}