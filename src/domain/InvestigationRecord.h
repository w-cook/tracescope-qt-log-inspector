#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVariant>

#include <optional>

#include "RecordSeverity.h"
#include "RecordSourceMetadata.h"

struct InvestigationRecord
{
    QString recordId;

    std::optional<QDateTime> timestamp;
    std::optional<RecordSeverity> severity;
    std::optional<QString> subsystem;
    std::optional<QString> eventCode;
    std::optional<QString> entityId;
    std::optional<QString> message;

    QHash<QString, QVariant> customAttributes;

    QString rawSource;
    RecordSourceMetadata source;
};