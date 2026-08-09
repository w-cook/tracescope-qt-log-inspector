#pragma once

#include <QString>

struct CanonicalFieldMappings
{
    QString timestampPath =
        QStringLiteral("timestamp");

    QString severityPath =
        QStringLiteral("level");

    QString subsystemPath =
        QStringLiteral("subsystem");

    QString eventCodePath =
        QStringLiteral("eventCode");

    QString entityIdPath =
        QStringLiteral("entityId");

    QString messagePath =
        QStringLiteral("message");
};