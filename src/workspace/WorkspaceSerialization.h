#pragma once

#include <optional>

#include <QByteArray>
#include <QString>

#include "WorkspacePersistenceState.h"

struct WorkspaceDeserializationResult
{
    std::optional<WorkspacePersistenceState>
        workspace;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return workspace.has_value();
    }
};

class WorkspaceSerializer
{
public:
    QByteArray serialize(
        const WorkspacePersistenceState &workspace
        ) const;

    WorkspaceDeserializationResult deserialize(
        const QByteArray &json
        ) const;
};