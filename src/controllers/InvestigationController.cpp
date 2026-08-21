#include "InvestigationController.h"

#include <QSet>

#include <algorithm>

namespace
{
bool isNavigableIssue(
    const InvestigationRecord &record
    )
{
    if (!record.severity.has_value()) {
        return false;
    }

    switch (record.severity.value()) {
    case RecordSeverity::Warning:
    case RecordSeverity::Error:
    case RecordSeverity::Critical:
        return true;

    default:
        return false;
    }
}
}

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

void InvestigationController::setFilterState(
    const QStringList &severities,
    const QStringList &subsystems,
    const QString &searchText,
    const QStringList &eventCodes,
    const QStringList &entityIds,
    const std::optional<QDateTime> &startTime,
    const std::optional<QDateTime> &endTime,
    const CustomFieldFilterMap &customFieldFilters,
    bool bookmarkedOnly
    )
{
    m_proxyModel.setFilterState(
        severities,
        subsystems,
        searchText,
        eventCodes,
        entityIds,
        startTime,
        endTime,
        customFieldFilters,
        bookmarkedOnly
        );
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
    setFilters(
        severities,
        subsystem.trimmed().isEmpty()
            ? QStringList()
            : QStringList {
                  subsystem.trimmed()
              },
        searchText
        );
}

void InvestigationController::setFilters(
    const QStringList &severities,
    const QStringList &subsystems,
    const QString &searchText
    )
{
    m_proxyModel.setSeverityFilters(
        severities
        );

    m_proxyModel.setSubsystemFilters(
        subsystems
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

void InvestigationController::
    setCustomFieldFilters(
        const CustomFieldFilterMap &filters
        )
{
    m_proxyModel
        .setCustomFieldFilters(
            filters
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

int InvestigationController::
    adjacentIssueProxyRow(
        int currentProxyRow,
        int direction
        ) const
{
    const int rowCount =
        m_proxyModel.rowCount();

    if (rowCount <= 0
        || direction == 0) {
        return -1;
    }

    const int step =
        direction > 0
            ? 1
            : -1;

    int row = 0;

    /*
     * With no current selection, Next begins at
     * the first visible row while Previous begins
     * at the last visible row.
     */
    if (currentProxyRow < 0
        || currentProxyRow >= rowCount) {
        row =
            step > 0
                ? 0
                : rowCount - 1;
    } else {
        row =
            (
                currentProxyRow
                + step
                + rowCount
                )
            % rowCount;
    }

    /*
     * Scan the proxy rather than the source model
     * so navigation respects both active filters
     * and the table's current sort order.
     *
     * The modular row calculation intentionally
     * wraps at either end of the investigation.
     */
    for (int inspected = 0;
         inspected < rowCount;
         ++inspected) {
        const QModelIndex proxyIndex =
            m_proxyModel.index(
                row,
                0
                );

        const InvestigationRecord *record =
            recordForProxyIndex(
                proxyIndex
                );

        if (record != nullptr
            && isNavigableIssue(
                *record
                )) {
            return row;
        }

        row =
            (
                row
                + step
                + rowCount
                )
            % rowCount;
    }

    return -1;
}

int InvestigationController::
    adjacentVisibleProxyRow(
        int currentProxyRow,
        int direction
        ) const
{
    const int rowCount =
        m_proxyModel.rowCount();

    if (rowCount <= 0
        || direction == 0) {
        return -1;
    }

    if (currentProxyRow < 0
        || currentProxyRow >= rowCount) {
        return direction > 0
                   ? 0
                   : rowCount - 1;
    }

    const int targetRow =
        currentProxyRow
        + (direction > 0 ? 1 : -1);

    if (targetRow < 0
        || targetRow >= rowCount) {
        return -1;
    }

    return targetRow;
}

QVector<InvestigationRecord>
InvestigationController::recordsForAnalysis() const
{
    const bool filtersActive =
        !m_proxyModel
            .severityFilters()
            .isEmpty()
        || !m_proxyModel
            .subsystemFilters()
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
            .has_value()
        || !m_proxyModel
            .customFieldFilters()
            .isEmpty();

    if (!filtersActive) {
        return m_sourceModel.records();
    }

    return visibleRecords();
}