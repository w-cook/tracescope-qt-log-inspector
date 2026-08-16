#include "InvestigationFilterProxyModel.h"

#include "../domain/RecordSeverity.h"

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

void InvestigationFilterProxyModel::setSeverityFilter(
    const QString &severity
    )
{
    const QString normalized =
        severity.trimmed().toUpper();

    if (m_severityFilter == normalized) {
        return;
    }

    beginFilterChange();

    m_severityFilter = normalized;

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
        );
}

void InvestigationFilterProxyModel::setSubsystemFilter(
    const QString &subsystem
    )
{
    const QString normalized = subsystem.trimmed();

    if (m_subsystemFilter == normalized) {
        return;
    }

    beginFilterChange();

    m_subsystemFilter = normalized;

    endFilterChange(
        QSortFilterProxyModel::Direction::Rows
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

QString InvestigationFilterProxyModel::severityFilter() const
{
    return m_severityFilter;
}

QString InvestigationFilterProxyModel::subsystemFilter() const
{
    return m_subsystemFilter;
}

QString InvestigationFilterProxyModel::searchText() const
{
    return m_searchText;
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

    if (!m_severityFilter.isEmpty()) {
        if (!record->severity.has_value()
            || recordSeverityToString(
                   record->severity.value()
                   ) != m_severityFilter) {
            return false;
        }
    }

    if (!m_subsystemFilter.isEmpty()) {
        if (!record->subsystem.has_value()
            || record->subsystem.value()
                   != m_subsystemFilter) {
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