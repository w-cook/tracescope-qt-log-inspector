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
    const QString normalizedSeverity =
        severity.trimmed();

    setFilters(
        normalizedSeverity.isEmpty()
            ? QStringList()
            : QStringList {
                  normalizedSeverity
              },
        subsystem,
        searchText
        );
}

void InvestigationController::setFilters(
    const QStringList &severities,
    const QString &subsystem,
    const QString &searchText
    )
{
    m_proxyModel.setSeverityFilters(
        severities
        );

    m_proxyModel.setSubsystemFilter(
        subsystem
        );

    m_proxyModel.setSearchText(
        searchText
        );
}

void InvestigationController::
    setEventCodeFilters(
        const QStringList &eventCodes
        )
{
    m_proxyModel.setEventCodeFilters(
        eventCodes
        );
}

void InvestigationController::
    setEntityFilters(
        const QStringList &entityIds
        )
{
    m_proxyModel.setEntityFilters(
        entityIds
        );
}

void InvestigationController::
    setTimeRangeFilter(
        const std::optional<QDateTime> &startTime,
        const std::optional<QDateTime> &endTime
        )
{
    m_proxyModel.setTimeRangeFilter(
        startTime,
        endTime
        );
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
             .severityFilters()
             .isEmpty()
        || !m_proxyModel
                .subsystemFilter()
                .isEmpty()
        || !m_proxyModel
                .searchText()
                .isEmpty()
        || !m_proxyModel
                .eventCodeFilters()
                .isEmpty()
        || !m_proxyModel
                .entityFilters()
                .isEmpty()
        || m_proxyModel
               .timeRangeStart()
               .has_value()
        || m_proxyModel
               .timeRangeEnd()
               .has_value();

    if (!filtersActive) {
        return m_sourceModel.records();
    }

    return visibleRecords();
}