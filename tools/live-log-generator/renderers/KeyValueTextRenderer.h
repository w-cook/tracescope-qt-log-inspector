#pragma once

#include "../ILogRecordRenderer.h"

class KeyValueTextRenderer final
    : public ILogRecordRenderer
{
public:
    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

    static bool isValidAttributeKey(
        const QString &key
        );

private:
    static bool isCanonicalKey(
        const QString &key
        );

    static QString renderValue(
        const QVariant &value
        );

    static QString renderStringValue(
        const QString &value
        );

    static QStringList customKeysForRecord(
        const LiveLogRecord &record
        );
};