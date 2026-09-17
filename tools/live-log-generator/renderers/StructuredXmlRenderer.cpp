#include "StructuredXmlRenderer.h"

#include <algorithm>

#include <QBuffer>
#include <QStringList>
#include <QVariant>
#include <QXmlStreamWriter>

namespace
{
QStringList sortedAttributeKeys(
    const QHash<QString, QVariant> &attributes
    )
{
    QStringList keys =
        attributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            const int insensitive =
                QString::compare(
                    left,
                    right,
                    Qt::CaseInsensitive
                    );

            if (insensitive != 0) {
                return insensitive < 0;
            }

            return left < right;
        }
        );

    return keys;
}

QString attributeValue(
    const QVariant &value
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return QStringLiteral("null");
    }

    if (value.metaType().id()
        == QMetaType::Bool) {
        return value.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}
}

QByteArray
StructuredXmlRenderer::initialContent() const
{
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<session>\n"
        "    <events>\n"
        );
}

QByteArray
StructuredXmlRenderer::renderRecord(
    const LiveLogRecord &record,
    const QDateTime &scenarioStart
    ) const
{
    QByteArray rendered;

    QBuffer buffer(
        &rendered
        );

    buffer.open(
        QIODevice::WriteOnly
        );

    QXmlStreamWriter writer(
        &buffer
        );

    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);

    writer.writeStartElement(
        QStringLiteral("event")
        );

    writer.writeStartElement(
        QStringLiteral("metadata")
        );

    const QDateTime timestamp =
        scenarioStart
            .addMSecs(
                record.timestampOffsetMs
                )
            .toUTC();

    writer.writeTextElement(
        QStringLiteral("timestamp"),
        timestamp.toString(
            Qt::ISODateWithMs
            )
        );

    writer.writeTextElement(
        QStringLiteral("level"),
        record.severity
        );

    writer.writeTextElement(
        QStringLiteral("component"),
        record.subsystem
        );

    writer.writeTextElement(
        QStringLiteral("code"),
        record.eventCode
        );

    writer.writeEndElement();

    writer.writeStartElement(
        QStringLiteral("context")
        );

    writer.writeTextElement(
        QStringLiteral("deviceId"),
        record.entityId
        );

    writer.writeEndElement();

    writer.writeStartElement(
        QStringLiteral("details")
        );

    writer.writeTextElement(
        QStringLiteral("message"),
        record.message
        );

    writer.writeEndElement();

    const QStringList keys =
        sortedAttributeKeys(
            record.attributes
            );

    for (const QString &key : keys) {
        writer.writeStartElement(
            QStringLiteral("Data")
            );

        writer.writeAttribute(
            QStringLiteral("Name"),
            key
            );

        writer.writeCharacters(
            attributeValue(
                record.attributes.value(
                    key
                    )
                )
            );

        writer.writeEndElement();
    }

    writer.writeEndElement();

    buffer.close();

    /*
     * The writer creates an XML fragment rather than
     * the outer document. Indent the fragment so the
     * completed generated file remains readable.
     */
    rendered.replace(
        "\n",
        "\n        "
        );

    rendered.prepend(
        "        "
        );

    rendered.append(
        '\n'
        );

    return rendered;
}

QByteArray
StructuredXmlRenderer::finalContent() const
{
    return QByteArray(
        "    </events>\n"
        "</session>\n"
        );
}