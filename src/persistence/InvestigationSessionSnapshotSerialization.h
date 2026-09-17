#pragma once

#include <optional>

#include <QByteArray>
#include <QString>

#include "InvestigationSessionSnapshot.h"

struct InvestigationSessionSnapshotDeserializationResult
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

class InvestigationSessionSnapshotSerializer
{
public:
    QByteArray serialize(
        const InvestigationSessionSnapshot &snapshot
        ) const;

    InvestigationSessionSnapshotDeserializationResult
    deserialize(
        const QByteArray &json
        ) const;
};