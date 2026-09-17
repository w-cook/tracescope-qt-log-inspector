#pragma once

#include "LiveLogRecord.h"

#include <QtGlobal>

#include <variant>

enum class LiveLogWriteMode
{
    Normal,
    Partial
};

struct LiveLogWriteOptions
{
    LiveLogWriteMode mode = LiveLogWriteMode::Normal;
    double splitFraction = 0.5;
    qint64 holdMs = 0;
};

struct LiveLogRecordStep
{
    LiveLogRecord record;
    LiveLogWriteOptions write;
};

struct LiveLogWaitStep
{
    qint64 durationMs = 0;
};

struct LiveLogTruncateStep
{
};

struct LiveLogReplaceStep
{
};

struct LiveLogRotateStep
{
};

using LiveLogScenarioStep = std::variant<
    LiveLogRecordStep,
    LiveLogWaitStep,
    LiveLogTruncateStep,
    LiveLogReplaceStep,
    LiveLogRotateStep>;