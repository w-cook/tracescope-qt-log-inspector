#pragma once

#include <optional>

#include <QVector>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QStringList>

#include "../domain/InvestigationRecordState.h"
#include "../importing/ImportProfile.h"

struct PersistedInvestigationRecordState
{
    QString recordId;

    bool bookmarked = false;
    QString note;

    FindingStatus findingStatus =
        FindingStatus::None;
};

struct PersistedInvestigationFilterState
{
    QStringList severities;
    QStringList subsystems;
    QString searchText;

    QStringList eventCodes;
    QStringList entityIds;

    std::optional<QDateTime> startTime;
    std::optional<QDateTime> endTime;

    QMap<QString, QStringList>
        customFieldFilters;

    QStringList findingStatuses;

    bool bookmarkedOnly = false;
};

struct PersistedInvestigationSession
{
    QString sessionId;
    QString sourcePath;

    ImportProfile importProfile;

    QVector<PersistedInvestigationRecordState>
        recordStates;

    PersistedInvestigationFilterState
        filterState;
};

struct WorkspacePersistenceState
{
    inline static constexpr int
        CurrentSchemaVersion = 1;

    int schemaVersion =
        CurrentSchemaVersion;

    QVector<PersistedInvestigationSession>
        sessions;
};