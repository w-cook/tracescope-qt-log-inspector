#include "ImportProfileValidation.h"

bool ProfileValidationResult::isValid() const
{
    for (const ProfileValidationIssue &issue
         : issues) {
        if (issue.severity ==
            ProfileValidationSeverity::Error) {
            return false;
        }
    }

    return true;
}

bool ProfileValidationResult::hasWarnings() const
{
    for (const ProfileValidationIssue &issue
         : issues) {
        if (issue.severity ==
            ProfileValidationSeverity::Warning) {
            return true;
        }
    }

    return false;
}