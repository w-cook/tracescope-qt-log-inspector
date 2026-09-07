#include "StructuredJsonRenderer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QByteArray
StructuredJsonRenderer::initialContent() const
{
    return QByteArray(
        "{\n"
        "  \"data\": {\n"
        "    \"records\": [\n"
        );
}

QByteArray
StructuredJsonRenderer::recordSeparator() const
{
    return QByteArray(",\n");
}

QByteArray
StructuredJsonRenderer::renderRecord(
    const LiveLogRecord &record,
    const QDateTime &scenarioStart
    ) const
{
    QJsonObject object;

    /*
     * Custom attributes are written first so
     * canonical generator fields retain their
     * required meaning when names collide.
     */
    for (auto iterator =
         record.attributes.constBegin();
         iterator !=
         record.attributes.constEnd();
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

    return QByteArray("      ")
           + QJsonDocument(object).toJson(
               QJsonDocument::Compact
               );
}

QByteArray
StructuredJsonRenderer::finalContent() const
{
    return QByteArray(
        "\n"
        "    ]\n"
        "  }\n"
        "}\n"
        );
}