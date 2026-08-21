#include "InvestigationStateStore.h"

InvestigationRecordState
InvestigationStateStore::stateForRecord(
    const QString &recordId
    ) const
{
    return m_states.value(recordId);
}

bool InvestigationStateStore::hasStateForRecord(
    const QString &recordId
    ) const
{
    return m_states.contains(recordId);
}

void InvestigationStateStore::setBookmarked(
    const QString &recordId,
    bool bookmarked
    )
{
    InvestigationRecordState state =
        stateForRecord(recordId);

    state.bookmarked = bookmarked;

    updateState(recordId, state);
}

void InvestigationStateStore::setNote(
    const QString &recordId,
    const QString &note
    )
{
    InvestigationRecordState state =
        stateForRecord(recordId);

    state.note = note;

    updateState(recordId, state);
}

void InvestigationStateStore::setFindingStatus(
    const QString &recordId,
    FindingStatus status
    )
{
    InvestigationRecordState state =
        stateForRecord(recordId);

    state.findingStatus = status;

    updateState(recordId, state);
}

QStringList
InvestigationStateStore::statefulRecordIds() const
{
    return m_states.keys();
}

void InvestigationStateStore::retainOnly(
    const QSet<QString> &recordIds
    )
{
    for (auto iterator = m_states.begin();
         iterator != m_states.end();) {
        if (!recordIds.contains(iterator.key())) {
            iterator = m_states.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

QSet<QString>
InvestigationStateStore::bookmarkedRecordIds() const
{
    QSet<QString> recordIds;

    for (
        auto iterator = m_states.constBegin();
        iterator != m_states.constEnd();
        ++iterator
        ) {
        if (iterator.value().bookmarked) {
            recordIds.insert(
                iterator.key()
                );
        }
    }

    return recordIds;
}

void InvestigationStateStore::updateState(
    const QString &recordId,
    const InvestigationRecordState &state
    )
{
    if (recordId.isEmpty()) {
        return;
    }

    if (state.isEmpty()) {
        m_states.remove(recordId);
        return;
    }

    m_states.insert(recordId, state);
}