#pragma once

#include "../ILogRecordRenderer.h"

class WindowsEventXmlRenderer final
    : public ILogRecordRenderer
{
public:
    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

    QByteArray finalContent() const override;
};