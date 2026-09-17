#include "IisW3cRenderer.h"

#include <QStringList>

namespace
{
QString attributeValue(
    const LiveLogRecord &record,
    const QString &key
    )
{
    const auto iterator =
        record.attributes.constFind(
            key
            );

    if (iterator
            == record.attributes.constEnd()
        || !iterator.value().isValid()
        || iterator.value().isNull()) {
        return QStringLiteral("-");
    }

    if (iterator.value().metaType().id()
        == QMetaType::Bool) {
        return iterator.value().toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    const QString value =
        iterator.value().toString();

    if (value.isEmpty()) {
        return QStringLiteral("-");
    }

    return value;
}
}

QByteArray IisW3cRenderer::
    initialContent() const
{
    return QByteArray(
        "#Software: Microsoft Internet Information Services 10.0\n"
        "#Version: 1.0\n"
        "#Fields: date time s-ip cs-method "
        "cs-uri-stem cs-uri-query s-port "
        "cs-username c-ip cs(User-Agent) "
        "cs(Referer) sc-status sc-substatus "
        "sc-win32-status time-taken\n"
        );
}

QByteArray IisW3cRenderer::
    renderRecord(
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

    const QStringList values({
        timestamp.toString(
            QStringLiteral("yyyy-MM-dd")
            ),
        timestamp.toString(
            QStringLiteral("HH:mm:ss")
            ),
        attributeValue(
            record,
            QStringLiteral("serverIp")
            ),
        attributeValue(
            record,
            QStringLiteral("method")
            ),
        attributeValue(
            record,
            QStringLiteral("path")
            ),
        attributeValue(
            record,
            QStringLiteral("query")
            ),
        attributeValue(
            record,
            QStringLiteral("serverPort")
            ),
        attributeValue(
            record,
            QStringLiteral("username")
            ),
        attributeValue(
            record,
            QStringLiteral("clientIp")
            ),
        attributeValue(
            record,
            QStringLiteral("userAgent")
            ),
        attributeValue(
            record,
            QStringLiteral("referer")
            ),
        attributeValue(
            record,
            QStringLiteral("status")
            ),
        attributeValue(
            record,
            QStringLiteral("substatus")
            ),
        attributeValue(
            record,
            QStringLiteral("win32Status")
            ),
        attributeValue(
            record,
            QStringLiteral("timeTakenMs")
            )
    });

    return values
               .join(QLatin1Char(' '))
               .toUtf8()
           + '\n';
}