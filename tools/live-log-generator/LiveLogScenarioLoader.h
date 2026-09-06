#pragma once

#include "LiveLogScenario.h"

#include <optional>

#include <QByteArray>
#include <QString>

struct LiveLogScenarioLoadResult
{
    std::optional<LiveLogScenario> scenario;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return scenario.has_value();
    }
};

class LiveLogScenarioLoader
{
public:
    LiveLogScenarioLoadResult loadFile(
        const QString &filePath
        ) const;

    LiveLogScenarioLoadResult deserialize(
        const QByteArray &json
        ) const;
};