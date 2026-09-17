#include "AccessLogRenderer.h"

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
        return {};
    }

    return iterator.value().toString();
}

QString optionalValue(
    const LiveLogRecord &record,
    const QString &key
    )
{
    const QString value =
        attributeValue(
            record,
            key
            );

    return value.isEmpty()
               ? QStringLiteral("-")
               : value;
}

bool isCombined(
    AccessLogFormat format
    )
{
    return format
               == AccessLogFormat::ApacheCombined
           || format
                  == AccessLogFormat::NginxCombined;
}
}

AccessLogRenderer::AccessLogRenderer(
    AccessLogFormat format
    )
    : format(format)
{
}

QByteArray AccessLogRenderer::
    initialContent() const
{
    return {};
}

QByteArray AccessLogRenderer::
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

    const QString timestampText =
        timestamp.toString(
            QStringLiteral(
                "dd/MMM/yyyy:HH:mm:ss"
                )
            )
        + QStringLiteral(" +0000");

    QString target =
        optionalValue(
            record,
            QStringLiteral("path")
            );

    const QString query =
        attributeValue(
            record,
            QStringLiteral("query")
            );

    if (!query.isEmpty()) {
        target +=
            QLatin1Char('?');

        target +=
            query;
    }

    const QString request =
        QStringLiteral(
            "%1 %2 HTTP/1.1"
            )
            .arg(
                optionalValue(
                    record,
                    QStringLiteral("method")
                    ),
                target
                );

    QString rendered =
        QStringLiteral(
            "%1 - %2 [%3] \"%4\" %5 %6"
            )
            .arg(
                optionalValue(
                    record,
                    QStringLiteral("clientIp")
                    ),
                optionalValue(
                    record,
                    QStringLiteral("username")
                    ),
                timestampText,
                request,
                optionalValue(
                    record,
                    QStringLiteral("status")
                    ),
                optionalValue(
                    record,
                    QStringLiteral("responseBytes")
                    )
                );

    if (isCombined(format)) {
        rendered +=
            QStringLiteral(
                " \"%1\" \"%2\""
                )
                .arg(
                    optionalValue(
                        record,
                        QStringLiteral("referer")
                        ),
                    optionalValue(
                        record,
                        QStringLiteral("userAgent")
                        )
                    );
    }

    return rendered.toUtf8()
           + '\n';
}