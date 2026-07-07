#pragma once

#include <QString>

struct TelemetryEvent
{
    QString timestamp;
    QString level;
    QString subsystem;
    QString eventCode;
    QString message;
    QString entityId;
};