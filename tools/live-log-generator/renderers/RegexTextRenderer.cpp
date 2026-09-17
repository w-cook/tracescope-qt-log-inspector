#include "RegexTextRenderer.h"

#include <utility>

#include <QMetaType>

RegexTextRenderer::RegexTextRenderer(
    QStringList attributeKeys
    )
    : attributeKeys(
          std::move(attributeKeys)
          )
{
}

QByteArray
RegexTextRenderer::initialContent() const
{
    return {};
}

QByteArray
RegexTextRenderer::renderRecord(
    const LiveLogRecord &record,
    const QDateTime &scenarioStart
    ) const
{
    const QDateTime timestamp =
        scenarioStart
            .addMSecs(
                record.timestampOffsetMs
                )
            .toUTC();

    QStringList fields {
        timestamp.toString(
            Qt::ISODateWithMs
            ),
        QStringLiteral("[%1]")
            .arg(record.severity),
        QStringLiteral("[%1]")
            .arg(record.subsystem),
        QStringLiteral("[%1]")
            .arg(record.eventCode),
        QStringLiteral("[%1]")
            .arg(record.entityId)
    };

    for (const QString &key
         : attributeKeys) {
        fields.append(
            QStringLiteral("[%1=%2]")
                .arg(
                    key,
                    attributeValue(
                        readAttribute(
                            record,
                            key
                            )
                        )
                    )
            );
    }

    fields.append(record.message);

    QString line =
        fields.join(
            QLatin1Char(' ')
            );

    line.append(
        QLatin1Char('\n')
        );

    return line.toUtf8();
}

QString
RegexTextRenderer::attributeValue(
    const QVariant &value
    ) const
{
    if (!value.isValid()
        || value.isNull()) {
        return {};
    }

    if (value.metaType().id()
        == QMetaType::Bool) {
        return value.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}

QVariant
RegexTextRenderer::readAttribute(
    const LiveLogRecord &record,
    const QString &key
    ) const
{
    const auto exact =
        record.attributes.constFind(key);

    if (exact
        != record.attributes.constEnd()) {
        return exact.value();
    }

    const QString normalized =
        key.toCaseFolded();

    QStringList matches;

    for (auto iterator =
         record.attributes.constBegin();
         iterator
         != record.attributes.constEnd();
         ++iterator) {
        if (iterator.key()
                .toCaseFolded()
            == normalized) {
            matches.append(
                iterator.key()
                );
        }
    }

    if (matches.isEmpty()) {
        return {};
    }

    matches.sort(Qt::CaseSensitive);

    return record.attributes.value(
        matches.first()
        );
}