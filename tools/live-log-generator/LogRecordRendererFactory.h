#pragma once

#include "ILogRecordRenderer.h"
#include "LiveLogScenario.h"

#include <memory>

#include <QString>
#include <QStringList>

struct LogRecordRendererCreateResult
{
    std::unique_ptr<ILogRecordRenderer> renderer;

    QString errorCode;
    QString errorMessage;

    bool isSuccess() const
    {
        return renderer != nullptr;
    }
};

class LogRecordRendererFactory
{
public:
    static QStringList supportedFormats();

    static LogRecordRendererCreateResult create(
        const QString &formatId,
        const LiveLogScenario &scenario
        );
};