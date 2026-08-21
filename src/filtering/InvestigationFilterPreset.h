#pragma once

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>

struct InvestigationFilterPreset
{
    QString name;

    QStringList severities;
    QStringList subsystems;

    QString searchText;

    QStringList findingStatuses;

    bool bookmarkedOnly = false;

    QStringList eventCodes;
    QStringList entityIds;

    std::optional<QDateTime> timeRangeStart;
    std::optional<QDateTime> timeRangeEnd;

    QMap<QString, QStringList>
        customFieldFilters;
};