#pragma once

#include "LiveLogScenarioStep.h"

#include <QList>
#include <QString>

struct LiveLogScenario
{
    int schemaVersion = 1;

    QString name;
    QString description;

    QList<LiveLogScenarioStep> steps;
};