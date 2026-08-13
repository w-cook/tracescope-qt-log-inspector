#pragma once

#include <QString>

struct ImportFormatSuggestion
{
    QString importerId;
    QString displayName;
    QString reason;
    QString profilePresetId;

    bool hasSuggestion() const
    {
        return !importerId.isEmpty();
    }
};