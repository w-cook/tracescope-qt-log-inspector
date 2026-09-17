#pragma once

#include "../ILogRecordRenderer.h"

enum class SyslogFormat
{
    Rfc5424,
    Rfc3164
};

class SyslogRenderer final
    : public ILogRecordRenderer
{
public:
    explicit SyslogRenderer(
        SyslogFormat format
        );

    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

    static bool isValidRfc5424AppName(
        const QString &value
        );

    static bool isValidRfc5424MessageId(
        const QString &value
        );

    static bool isValidRfc5424StructuredDataName(
        const QString &value
        );

    static bool isValidRfc3164Tag(
        const QString &value
        );

private:
    QByteArray renderRfc5424(
        const LiveLogRecord &record,
        const QDateTime &timestamp
        ) const;

    QByteArray renderRfc3164(
        const LiveLogRecord &record,
        const QDateTime &timestamp
        ) const;

    SyslogFormat m_format;
};