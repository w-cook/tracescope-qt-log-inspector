#pragma once

#include "ImportProfile.h"
#include "ImportProfileValidation.h"

class ImportProfileValidator
{
public:
    ProfileValidationResult validate(
        const ImportProfile &profile
        ) const;
};