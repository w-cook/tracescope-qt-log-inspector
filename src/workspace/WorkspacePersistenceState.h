#pragma once

#include <QVector>
#include <QString>

#include "../importing/ImportProfile.h"

struct PersistedInvestigationSession
{
    QString sessionId;
    QString sourcePath;

    ImportProfile importProfile;
};

struct WorkspacePersistenceState
{
    inline static constexpr int
        CurrentSchemaVersion = 1;

    int schemaVersion =
        CurrentSchemaVersion;

    QVector<PersistedInvestigationSession>
        sessions;
};