#pragma once

#include <QList>
#include <QString>

enum class ProfileValidationSeverity
{
    Warning,
    Error
};

struct ProfileValidationIssue
{
    QString code;
    QString message;

    ProfileValidationSeverity severity =
        ProfileValidationSeverity::Error;
};

struct ProfileValidationResult
{
    QList<ProfileValidationIssue> issues;

    bool isValid() const;
    bool hasWarnings() const;
};