#pragma once

#include <QString>

enum class FindingStatus
{
    None,
    Open,
    Resolved,
    Dismissed
};

struct InvestigationRecordState
{
    bool bookmarked = false;
    QString note;
    FindingStatus findingStatus =
        FindingStatus::None;

    bool isEmpty() const
    {
        return !bookmarked
               && note.trimmed().isEmpty()
               && findingStatus
                      == FindingStatus::None;
    }
};