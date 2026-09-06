#pragma once

#include "ILogRecordRenderer.h"
#include "LiveLogScenario.h"

#include <functional>

#include <QDateTime>
#include <QString>

struct LiveLogScenarioPlayerOptions
{
    QString outputPath;

    double speed = 1.0;

    QDateTime scenarioStart;
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

    explicit LiveLogScenarioPlayer(
        SleepFunction sleepFunction = {}
        );

    LiveLogScenarioPlayResult play(
        const LiveLogScenario &scenario,
        const ILogRecordRenderer &renderer,
        const LiveLogScenarioPlayerOptions &options
        ) const;

private:
    SleepFunction sleepFunction;
};