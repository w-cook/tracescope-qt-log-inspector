#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "InvestigationPresentationState.h"

struct PresentationStateDeserializationResult
{
    std::optional<
        InvestigationSessionPresentationState>
        state;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return state.has_value();
    }
};

class InvestigationPresentationStateSerializer
{
public:
    QJsonObject serialize(
        const InvestigationSessionPresentationState
            &state
        ) const;

    PresentationStateDeserializationResult
    deserialize(
        const QJsonObject &object
        ) const;
};