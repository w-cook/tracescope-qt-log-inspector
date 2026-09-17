#pragma once

#include <QHash>
#include <QString>
#include <QVariant>

struct LiveLogRecord
{
    qint64 timestampOffsetMs = 0;

    QString severity;
    QString subsystem;
    QString eventCode;
    QString entityId;
    QString message;

    QHash<QString, QVariant> attributes;
};