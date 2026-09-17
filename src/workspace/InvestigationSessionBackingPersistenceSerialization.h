#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "InvestigationSessionBackingPersistence.h"

struct InvestigationSessionBackingPersistenceDeserializationResult
{
    std::optional<
        PersistedInvestigationSessionBacking>
        backing;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return backing.has_value();
    }
};

class InvestigationSessionBackingPersistenceSerializer
{
public:
    QJsonObject serialize(
        const PersistedInvestigationSessionBacking
            &backing
        ) const;

    InvestigationSessionBackingPersistenceDeserializationResult
    deserialize(
        const QJsonObject &object
        ) const;
};