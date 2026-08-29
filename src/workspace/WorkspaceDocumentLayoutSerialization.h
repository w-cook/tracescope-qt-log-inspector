#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "WorkspaceDocumentLayoutState.h"

struct WorkspaceDocumentLayoutDeserializationResult
{
    std::optional<WorkspaceDocumentLayoutState>
        layout;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return layout.has_value();
    }
};

class WorkspaceDocumentLayoutSerializer
{
public:
    QJsonObject serialize(
        const WorkspaceDocumentLayoutState &state
        ) const;

    WorkspaceDocumentLayoutDeserializationResult
    deserialize(
        const QJsonObject &object
        ) const;
};