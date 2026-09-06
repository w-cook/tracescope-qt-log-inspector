#include "JsonLinesRenderer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QByteArray JsonLinesRenderer::initialContent() const
{
    return {};
}

QByteArray JsonLinesRenderer::renderRecord(
    const LiveLogRecord &record,
    const QDateTime &scenarioStart
    ) const
{
    QJsonObject object;

    /*
     * Write custom attributes first so canonical
     * generator fields always retain their required
     * meaning if a scenario attribute happens to use
     * the same name.
     */
    for (auto iterator =
         record.attributes.constBegin();
         iterator != record.attributes.constEnd();
         ++iterator) {
        object.insert(
            iterator.key(),
            QJsonValue::fromVariant(
                iterator.value()
                )
            );
    }

    const QDateTime timestamp =
        scenarioStart
            .addMSecs(
                record.timestampOffsetMs
                )
            .toUTC();

    object.insert(
        QStringLiteral("timestamp"),
        timestamp.toString(
            Qt::ISODateWithMs
            )
        );

    object.insert(
        QStringLiteral("level"),
        record.severity
        );

    object.insert(
        QStringLiteral("subsystem"),
        record.subsystem
        );

    object.insert(
        QStringLiteral("eventCode"),
        record.eventCode
        );

    object.insert(
        QStringLiteral("entityId"),
        record.entityId
        );

    object.insert(
        QStringLiteral("message"),
        record.message
        );

    QByteArray rendered =
        QJsonDocument(object).toJson(
            QJsonDocument::Compact
            );

    rendered.append('\n');

    return rendered;
}