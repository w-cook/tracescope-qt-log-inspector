#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

#include "../domain/InvestigationRecordState.h"

class InvestigationStateStore
{
public:
    InvestigationRecordState stateForRecord(
        const QString &recordId
        ) const;

    bool hasStateForRecord(
        const QString &recordId
        ) const;

    void setBookmarked(
        const QString &recordId,
        bool bookmarked
        );

    void setNote(
        const QString &recordId,
        const QString &note
        );

    void setFindingStatus(
        const QString &recordId,
        FindingStatus status
        );

    QStringList statefulRecordIds() const;

    void retainOnly(
        const QSet<QString> &recordIds
        );

private:
    QHash<QString, InvestigationRecordState>
        m_states;

    void updateState(
        const QString &recordId,
        const InvestigationRecordState &state
        );
};