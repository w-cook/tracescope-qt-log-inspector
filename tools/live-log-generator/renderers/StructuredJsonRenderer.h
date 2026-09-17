#pragma once

#include "../ILogRecordRenderer.h"

class StructuredJsonRenderer final
    : public ILogRecordRenderer
{
public:
    QByteArray initialContent() const override;

    QByteArray recordSeparator() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

    QByteArray finalContent() const override;
};