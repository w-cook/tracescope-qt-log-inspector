#pragma once

#include "../ILogRecordRenderer.h"
#include "../LiveLogScenario.h"

#include <QChar>
#include <QStringList>

class DelimitedTextRenderer final
    : public ILogRecordRenderer
{
public:
    DelimitedTextRenderer(
        QChar delimiter,
        QStringList attributeKeys
        );

    static QStringList attributeKeysForScenario(
        const LiveLogScenario &scenario
        );

    QByteArray initialContent() const override;

    QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const override;

private:
    QString renderField(
        const QString &value
        ) const;

    QByteArray renderFields(
        const QStringList &fields
        ) const;

    QString attributeValue(
        const QVariant &value
        ) const;

    QVariant readAttribute(
        const LiveLogRecord &record,
        const QString &key
        ) const;

    QChar delimiter;
    QStringList attributeKeys;
};