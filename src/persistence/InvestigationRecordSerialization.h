#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "../domain/InvestigationRecord.h"

struct InvestigationRecordDeserializationResult
{
    std::optional<InvestigationRecord> record;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return record.has_value();
    }
};

class InvestigationRecordSerializer
{
public:
    QJsonObject serialize(
        const InvestigationRecord &record
        ) const;

    InvestigationRecordDeserializationResult
    deserialize(
        const QJsonObject &object
        ) const;
};