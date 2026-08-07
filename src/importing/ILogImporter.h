#pragma once

#include <QString>

#include "ImportResult.h"

class ILogImporter
{
public:
    virtual ~ILogImporter() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    virtual ImportResult importFile(
        const QString &filePath
        ) const = 0;
};