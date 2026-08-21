#include "InvestigationFilterProxyModel.h"

#include "../domain/RecordSeverity.h"

namespace
{
QStringList normalizedExactFilterValues(
    const QStringList &values
    )
{
    QStringList normalized;

    normalized.reserve(
        values.size()
        );

    for (const QString &value : values) {
        const QString candidate =
            value.trimmed();

        if (candidate.isEmpty()
            || normalized.contains(candidate)) {
            continue;
        }

        normalized.append(
            candidate
            );
    }

    return normalized;
}

QStringList normalizedSeverityFilterValues(
    const QStringList &values
    )
{
    QStringList normalized;

    normalized.reserve(
        values.size()
        );

    for (const QString &value : values) {
        const QString candidate =
            value.trimmed().toUpper();

        if (candidate.isEmpty()
            || normalized.contains(candidate)) {
            continue;
        }

        normalized.append(
            candidate
            );
    }

    return normalized;
}

CustomFieldFilterMap normalizedCustomFieldFilters(
    const CustomFieldFilterMap &filters
    )
{
    CustomFieldFilterMap normalized;

    for (
        auto filterIterator =
        filters.constBegin();
        filterIterator != filters.constEnd();
        ++filterIterator
        ) {
        if (filterIterator
                .key()
                .isEmpty()
            || filterIterator
                   .value()
                   .isEmpty()) {
            continue;
        }

        QStringList values;

        for (const QString &value
             : filterIterator.value()) {
            if (!values.contains(value)) {
                values.append(value);
            }
        }

        if (!values.isEmpty()) {
            normalized.insert(
                filterIterator.key(),
                values
                );
        }
    }

    return normalized;
}
}

InvestigationFilterProxyModel::
    InvestigationFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(InvestigationTableModel::SortRole);

    m_searchMatcher.setCaseSensitivity(
        Qt::CaseInsensitive
        );
}

void InvestigationFilterProxyModel::
    setFilterState(
        const QStringList &severities,
        const QStringList &subsystems,
        const QString &searchText,
        const QStringList &eventCodes,
        const QStringList &entityIds,
        const std::optional<QDateTime> &startTime,
        const std::optional<QDateTime> &endTime,
        const CustomFieldFilterMap
            &customFieldFilters
        )
{
    const QStringList normalizedSeverities =
        normalizedSeverityFilterValues(
            severities
            );

    const QStringList normalizedSubsystems =
        normalizedExactFilterValues(
            subsystems
            );

    const QString normalizedSearchText =
        searchText.trimmed();

    const QStringList normalizedEventCodes =
        normalizedExactFilterValues(
            eventCodes
            );

    const QStringList normalizedEntityIds =
        normalizedExactFilterValues(
            entityIds
            );

    const std::optional<QDateTime>
        normalizedStart =
        startTime.has_value()
                && startTime->isValid()
            ? startTime
            : std::nullopt;

    const std::optional<QDateTime>
        normalizedEnd =
        endTime.has_value()
                && endTime->isValid()
            ? endTime
            : std::nullopt;

    const CustomFieldFilterMap
        normalizedCustomFilters =
        normalizedCustomFieldFilters(
            customFieldFilters
            );

    /*
     * Avoid resetting the proxy when the complete
     * requested state already matches the active
     * state.
     */
    if (m_severityFilters
            == normalizedSeverities
        && m_subsystemFilters
               == normalizedSubsystems
        && m_searchText
               == normalizedSearchText
        && m_eventCodeFilters
               == normalizedEventCodes
        && m_entityFilters
               == normalizedEntityIds
        && m_timeRangeStart
               == normalizedStart
        && m_timeRangeEnd
               == normalizedEnd
        && m_customFieldFilters
               == normalizedCustomFilters) {
        return;
    }

    /*
     * A complete filter state represents one
     * logical investigation change. Update every
     * criterion under one model reset so large
     * investigations are evaluated once instead
     * of repeatedly for each category.
     *
     * Selection is intentionally cleared by the
     * investigation UI whenever filters change,
     * so reset semantics are appropriate here.
     */
    beginResetModel();

    m_severityFilters =
        normalizedSeverities;

    m_subsystemFilters =
        normalizedSubsystems;

    m_searchText =
        normalizedSearchText;

    m_eventCodeFilters =
        normalizedEventCodes;

    m_entityFilters =
        normalizedEntityIds;

    m_timeRangeStart =
        normalizedStart;

    m_timeRangeEnd =
        normalizedEnd;

    m_customFieldFilters =
        normalizedCustomFilters;

    m_searchMatcher.setPattern(
        m_searchText
        );

    endResetModel();
}

void InvestigationFilterProxyModel::setSeverityFilters(
    const QStringList &severities
    )
{
    QStringList normalized;

    normalized.reserve(
        severities.size()
        );

    for (const QString &severity : severities) {
        const QString value =
            severity.trimmed().toUpper();

        if (value.isEmpty()
            || normalized.contains(value)) {
            continue;
        }

        normalized.append(value);
    }

    if (m_severityFilters == normalized) {
        return;
    }

    beginFilterChange();

    m_severityFilters = normalized;

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
        );
}

void InvestigationFilterProxyModel::setSeverityFilter(
    const QString &severity
    )
{
    const QString normalized =
        severity.trimmed();

    if (normalized.isEmpty()) {
        setSeverityFilters(
            QStringList()
            );

        return;
    }

    setSeverityFilters(
        QStringList { normalized }
        );
}

void InvestigationFilterProxyModel::
    setSubsystemFilters(
        const QStringList &subsystems
        )
{
    QStringList normalized;

    normalized.reserve(
        subsystems.size()
        );

    for (const QString &subsystem
         : subsystems) {
        const QString candidate =
            subsystem.trimmed();

        if (candidate.isEmpty()
            || normalized.contains(
                candidate
                )) {
            continue;
        }

        normalized.append(
            candidate
            );
    }

    if (m_subsystemFilters
        == normalized) {
        return;
    }

    /*
     * Subsystem values may be heavily interleaved
     * throughout large investigations. Incremental
     * proxy row reconciliation can become much
     * more expensive than rebuilding the proxy.
     *
     * Investigation selection is intentionally
     * cleared whenever filters change, so model
     * reset semantics are appropriate here.
     */
    beginResetModel();

    m_subsystemFilters =
        normalized;

    endResetModel();
}

void InvestigationFilterProxyModel::
    setSubsystemFilter(
        const QString &subsystem
        )
{
    const QString normalized =
        subsystem.trimmed();

    setSubsystemFilters(
        normalized.isEmpty()
            ? QStringList()
            : QStringList {
                  normalized
              }
        );
}

void InvestigationFilterProxyModel::setSearchText(
    const QString &searchText
    )
{
    const QString normalized = searchText.trimmed();

    if (m_searchText == normalized) {
        return;
    }

    beginFilterChange();

    m_searchText = normalized;

    m_searchMatcher.setPattern(
        m_searchText
        );

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
        );
}

void InvestigationFilterProxyModel::
    setTimeRangeFilter(
        const std::optional<QDateTime> &startTime,
        const std::optional<QDateTime> &endTime
        )
{
    const std::optional<QDateTime>
        normalizedStart =
        startTime.has_value()
                && startTime->isValid()
            ? startTime
            : std::nullopt;

    const std::optional<QDateTime>
        normalizedEnd =
        endTime.has_value()
                && endTime->isValid()
            ? endTime
            : std::nullopt;

    if (m_timeRangeStart == normalizedStart
        && m_timeRangeEnd == normalizedEnd) {
        return;
    }

    beginFilterChange();

    m_timeRangeStart =
        normalizedStart;

    m_timeRangeEnd =
        normalizedEnd;

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
        );
}

void InvestigationFilterProxyModel::
    setEventCodeFilters(
        const QStringList &eventCodes
        )
{
    const QStringList normalized =
        normalizedExactFilterValues(
            eventCodes
            );

    if (m_eventCodeFilters == normalized) {
        return;
    }

    /*
     * Event-code values can be distributed
     * throughout very large investigations.
     * Incremental row reconciliation can become
     * substantially more expensive than
     * repopulating the proxy in that case.
     *
     * Investigation selection is intentionally
     * cleared whenever filters change, so the
     * selection-invalidating semantics of a model
     * reset are appropriate here.
     */
    beginResetModel();

    m_eventCodeFilters =
        normalized;

    endResetModel();
}

void InvestigationFilterProxyModel::
    setEntityFilters(
        const QStringList &entityIds
        )
{
    const QStringList normalized =
        normalizedExactFilterValues(
            entityIds
            );

    if (m_entityFilters == normalized) {
        return;
    }

    beginFilterChange();

    m_entityFilters =
        normalized;

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
        );
}

void InvestigationFilterProxyModel::
    setCustomFieldFilters(
        const CustomFieldFilterMap &filters
        )
{
    CustomFieldFilterMap normalized;

    for (
        auto filterIterator =
        filters.constBegin();
        filterIterator !=
        filters.constEnd();
        ++filterIterator
        ) {
        if (filterIterator
                .key()
                .isEmpty()
            || filterIterator
                   .value()
                   .isEmpty()) {
            continue;
        }

        QStringList values;

        for (const QString &value
             : filterIterator.value()) {
            if (!values.contains(value)) {
                values.append(value);
            }
        }

        if (!values.isEmpty()) {
            normalized.insert(
                filterIterator.key(),
                values
                );
        }
    }

    if (m_customFieldFilters
        == normalized) {
        return;
    }

    /*
     * Custom values may be heavily interleaved
     * throughout large investigations. Rebuilding
     * the proxy avoids the expensive incremental
     * row reconciliation already observed with
     * other categorical filters.
     */
    beginResetModel();

    m_customFieldFilters =
        normalized;

    endResetModel();
}

void InvestigationFilterProxyModel::
    setBookmarkedRecordIds(
        const QSet<QString> &recordIds
        )
{
    if (m_bookmarkedRecordIds
        == recordIds) {
        return;
    }

    m_bookmarkedRecordIds =
        recordIds;

    if (rowCount() > 0) {
        emit headerDataChanged(
            Qt::Vertical,
            0,
            rowCount() - 1
            );
    }
}

QVariant InvestigationFilterProxyModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
    ) const
{
    const QVariant defaultValue =
        QSortFilterProxyModel::headerData(
            section,
            orientation,
            role
            );

    if (orientation != Qt::Vertical
        || role != Qt::DisplayRole
        || section < 0
        || section >= rowCount()) {
        return defaultValue;
    }

    const InvestigationTableModel *model =
        investigationModel();

    if (model == nullptr) {
        return defaultValue;
    }

    const QModelIndex proxyIndex =
        index(
            section,
            0
            );

    if (!proxyIndex.isValid()) {
        return defaultValue;
    }

    const QModelIndex sourceIndex =
        mapToSource(
            proxyIndex
            );

    const InvestigationRecord *record =
        model->recordAt(
            sourceIndex.row()
            );

    if (record == nullptr
        || !m_bookmarkedRecordIds.contains(
            record->recordId
            )) {
        return defaultValue;
    }

    return QStringLiteral("%1 ★")
        .arg(
            defaultValue.toString()
            );
}

QStringList
InvestigationFilterProxyModel::severityFilters() const
{
    return m_severityFilters;
}

QString
InvestigationFilterProxyModel::severityFilter() const
{
    if (m_severityFilters.size() != 1) {
        return QString();
    }

    return m_severityFilters.at(0);
}

QStringList InvestigationFilterProxyModel::
    subsystemFilters() const
{
    return m_subsystemFilters;
}

QString InvestigationFilterProxyModel::
    subsystemFilter() const
{
    return m_subsystemFilters.size() == 1
               ? m_subsystemFilters.first()
               : QString();
}

QString InvestigationFilterProxyModel::searchText() const
{
    return m_searchText;
}

const CustomFieldFilterMap &
    InvestigationFilterProxyModel::
    customFieldFilters() const
{
    return m_customFieldFilters;
}

QStringList
    InvestigationFilterProxyModel::
    eventCodeFilters() const
{
    return m_eventCodeFilters;
}

QStringList
    InvestigationFilterProxyModel::
    entityFilters() const
{
    return m_entityFilters;
}

const std::optional<QDateTime> &
    InvestigationFilterProxyModel::
    timeRangeStart() const
{
    return m_timeRangeStart;
}

const std::optional<QDateTime> &
    InvestigationFilterProxyModel::
    timeRangeEnd() const
{
    return m_timeRangeEnd;
}

bool InvestigationFilterProxyModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex &sourceParent
    ) const
{
    Q_UNUSED(sourceParent);

    const InvestigationTableModel *model =
        investigationModel();

    if (model == nullptr) {
        return true;
    }

    const InvestigationRecord *record =
        model->recordAt(sourceRow);

    if (record == nullptr) {
        return false;
    }

    if (!m_severityFilters.isEmpty()) {
        if (!record->severity.has_value()
            || !m_severityFilters.contains(
                recordSeverityToString(
                    record->severity.value()
                    )
                )) {
            return false;
        }
    }

    if (!m_subsystemFilters.isEmpty()) {
        if (!record->subsystem.has_value()
            || !m_subsystemFilters.contains(
                record->subsystem.value()
                )) {
            return false;
        }
    }

    if (!m_eventCodeFilters.isEmpty()) {
        if (!record->eventCode.has_value()
            || !m_eventCodeFilters.contains(
                record->eventCode.value()
                )) {
            return false;
        }
    }

    if (!m_entityFilters.isEmpty()) {
        if (!record->entityId.has_value()
            || !m_entityFilters.contains(
                record->entityId.value()
                )) {
            return false;
        }
    }

    if (m_timeRangeStart.has_value()
        || m_timeRangeEnd.has_value()) {
        if (!record->timestamp.has_value()) {
            return false;
        }

        if (m_timeRangeStart.has_value()
            && record->timestamp.value()
                   < m_timeRangeStart.value()) {
            return false;
        }

        if (m_timeRangeEnd.has_value()
            && record->timestamp.value()
                   > m_timeRangeEnd.value()) {
            return false;
        }
    }

    for (
        auto filterIterator =
        m_customFieldFilters.constBegin();
        filterIterator !=
        m_customFieldFilters.constEnd();
        ++filterIterator
        ) {
        const auto attributeIterator =
            record->customAttributes.constFind(
                filterIterator.key()
                );

        if (attributeIterator
            == record
                   ->customAttributes
                   .constEnd()) {
            return false;
        }

        const QString attributeValue =
            attributeIterator
                .value()
                .toString();

        if (!filterIterator
                 .value()
                 .contains(
                     attributeValue
                     )) {
            return false;
        }
    }

    if (!m_searchText.isEmpty()
        && !matchesSearchText(*record)) {
        return false;
    }

    return true;
}

const InvestigationTableModel *
InvestigationFilterProxyModel::investigationModel() const
{
    return qobject_cast<
        const InvestigationTableModel *
        >(sourceModel());
}

bool InvestigationFilterProxyModel::matchesSearchText(
    const InvestigationRecord &record
    ) const
{
    const auto matches =
        [this](const QString &value) {
            return m_searchMatcher.indexIn(
                       value
                       ) >= 0;
        };

    /*
     * The preserved source representation is an
     * efficient fast path for typical searches.
     * Decoded fields are still checked afterward
     * so escaped or normalized values remain
     * searchable.
     */
    if (!record.rawSource.isEmpty()
        && matches(record.rawSource)) {
        return true;
    }

    if (record.message.has_value()
        && matches(record.message.value())) {
        return true;
    }

    if (record.subsystem.has_value()
        && matches(record.subsystem.value())) {
        return true;
    }

    if (record.eventCode.has_value()
        && matches(record.eventCode.value())) {
        return true;
    }

    if (record.entityId.has_value()
        && matches(record.entityId.value())) {
        return true;
    }

    if (record.severity.has_value()
        && matches(
            recordSeverityToString(
                record.severity.value()
                )
            )) {
        return true;
    }

    if (record.timestamp.has_value()
        && matches(
            record.timestamp->toString(
                Qt::ISODateWithMs
                )
            )) {
        return true;
    }

    for (
        auto iterator =
        record.customAttributes.constBegin();
        iterator !=
        record.customAttributes.constEnd();
        ++iterator
        ) {
        if (matches(
                iterator.value().toString()
                )) {
            return true;
        }
    }

    return false;
}