#pragma once

#include "../ILogRecordRenderer.h"

enum class AccessLogFormat
{
    ApacheCommon,
    ApacheCombined,
    NginxCombined
};

class AccessLogRenderer final
    : public ILogRecordRenderer
{
public:
    explicit AccessLogRenderer(
        AccessLogFormat format
        );

    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

private:
    AccessLogFormat format;
};