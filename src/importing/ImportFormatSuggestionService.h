#pragma once

#include <QString>

#include "ImportFormatSuggestion.h"

class ImportFormatSuggestionService
{
public:
    ImportFormatSuggestion suggestForFile(
        const QString &filePath
        ) const;
};