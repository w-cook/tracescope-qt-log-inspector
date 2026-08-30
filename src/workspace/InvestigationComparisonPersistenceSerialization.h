#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "WorkspacePersistenceState.h"

struct ComparisonPersistenceDeserializationResult
{
    std::optional<PersistedInvestigationComparison>
        comparison;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return comparison.has_value();
    }
};

class InvestigationComparisonPersistenceSerializer
{
public:
    QJsonObject serialize(
        const PersistedInvestigationComparison
            &comparison
        ) const;

    ComparisonPersistenceDeserializationResult
    deserialize(
        const QJsonObject &object
        ) const;
};