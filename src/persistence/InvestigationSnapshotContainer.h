#pragma once

#include <optional>

#include <QByteArray>
#include <QString>

struct InvestigationSnapshotContainerDecodeResult
{
    std::optional<QByteArray> payload;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return payload.has_value();
    }
};

class InvestigationSnapshotContainer
{
public:
    inline static constexpr quint16
        CurrentContainerVersion = 1;

    QByteArray encode(
        const QByteArray &payload
        ) const;

    InvestigationSnapshotContainerDecodeResult
    decode(
        const QByteArray &container
        ) const;
};