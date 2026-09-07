#pragma once

#include "LiveLogRecord.h"

#include <QByteArray>
#include <QDateTime>

class ILogRecordRenderer
{
public:
    virtual ~ILogRecordRenderer() = default;

    virtual QByteArray initialContent() const = 0;

    virtual QByteArray recordSeparator() const
    {
        return {};
    }

    virtual QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const = 0;

    virtual QByteArray finalContent() const
    {
        return {};
    }
};