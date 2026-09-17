#pragma once

#include "../ILogRecordRenderer.h"

class IisW3cRenderer final
    : public ILogRecordRenderer
{
public:
    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;
};