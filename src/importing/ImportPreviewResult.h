#pragma once

#include <QString>

#include "ImportProfileValidation.h"
#include "ImportResult.h"

struct ImportPreviewResult
{
    ProfileValidationResult profileValidation;
    ImportResult importResult;

    QString errorCode;
    QString errorMessage;

    bool sourceTruncated = false;

    bool canDisplayPreview() const
    {
        return profileValidation.isValid()
        && errorCode.isEmpty();
    }
};