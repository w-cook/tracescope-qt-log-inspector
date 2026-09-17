#pragma once

#include <optional>

#include <QString>

#include "InvestigationSessionSnapshot.h"

struct InvestigationSessionSnapshotSaveResult
{
    bool succeeded = false;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return succeeded;
    }
};

struct InvestigationSessionSnapshotLoadResult
{
    std::optional<InvestigationSessionSnapshot>
        snapshot;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return snapshot.has_value();
    }
};

class InvestigationSessionSnapshotFile
{
public:
    InvestigationSessionSnapshotSaveResult
    save(
        const QString &filePath,
        const InvestigationSessionSnapshot &snapshot
        ) const;

    InvestigationSessionSnapshotLoadResult
    load(
        const QString &filePath
        ) const;
};