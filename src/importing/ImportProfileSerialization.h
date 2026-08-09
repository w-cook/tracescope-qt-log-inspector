#pragma once

#include <optional>

#include <QByteArray>
#include <QString>

#include "ImportProfile.h"

struct ProfileDeserializationResult
{
    std::optional<ImportProfile> profile;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return profile.has_value();
    }
};

class ImportProfileSerializer
{
public:
    QByteArray serialize(
        const ImportProfile &profile
        ) const;

    ProfileDeserializationResult deserialize(
        const QByteArray &json
        ) const;
};