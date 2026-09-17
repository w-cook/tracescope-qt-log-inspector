#pragma once

#include "ILogRecordRenderer.h"
#include "LiveLogScenario.h"

#include <functional>

#include <QDateTime>
#include <QString>

enum class LiveLogLoopBehavior
{
    Append,
    Restart
};

struct LiveLogScenarioPlayerOptions
{
    QString outputPath;
    double speed = 1.0;
    QDateTime scenarioStart;

    bool loop = false;

    LiveLogLoopBehavior loopBehavior =
        LiveLogLoopBehavior::Append;
};

struct LiveLogScenarioPlayResult
{
    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return errorCode.isEmpty();
    }
};

class LiveLogScenarioPlayer
{
public:
    using SleepFunction =
        std::function<void(qint64)>;

    using ContinueLoopFunction =
        std::function<bool()>;

    explicit LiveLogScenarioPlayer(
        SleepFunction sleepFunction = {},
        ContinueLoopFunction continueLoopFunction = {}
        );

    LiveLogScenarioPlayResult play(
        const LiveLogScenario &scenario,
        const ILogRecordRenderer &renderer,
        const LiveLogScenarioPlayerOptions &options
        ) const;

private:
    SleepFunction sleepFunction;
    ContinueLoopFunction continueLoopFunction;
};