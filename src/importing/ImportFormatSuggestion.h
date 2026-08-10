#pragma once

#include <QString>

struct ImportFormatSuggestion
{
    QString importerId;
    QString displayName;
    QString reason;

    bool hasSuggestion() const
    {
        return !importerId.isEmpty();
    }
};