#pragma once

#include "../ILogRecordRenderer.h"

#include <QStringList>

class RegexTextRenderer final
    : public ILogRecordRenderer
{
public:
    explicit RegexTextRenderer(
        QStringList attributeKeys
        );

    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

private:
    QString attributeValue(
        const QVariant &value
        ) const;

    QVariant readAttribute(
        const LiveLogRecord &record,
        const QString &key
        ) const;

    QStringList attributeKeys;
};