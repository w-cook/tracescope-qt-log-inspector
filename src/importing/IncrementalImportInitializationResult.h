#pragma once

#include <QString>

struct IncrementalImportInitializationResult
{
    bool succeeded = true;

    QString errorMessage;
};