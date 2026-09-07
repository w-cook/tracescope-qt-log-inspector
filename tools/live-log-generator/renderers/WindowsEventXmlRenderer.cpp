#include "WindowsEventXmlRenderer.h"

#include <algorithm>

#include <QBuffer>
#include <QMetaType>
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

QString scalarText(
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

    if (value.metaType().id()
            == QMetaType::Double
        || value.metaType().id()
               == QMetaType::Float) {
        return QString::number(
            value.toDouble(),
            'g',
            15
            );
    }

    return value.toString();
}

int windowsLevel(
    const QString &severity
    )
{
    const QString normalized =
        severity.trimmed().toUpper();

    if (normalized
        == QStringLiteral("CRITICAL")) {
        return 1;
    }

    if (normalized
        == QStringLiteral("ERROR")) {
        return 2;
    }

    if (normalized
            == QStringLiteral("WARN")
        || normalized
               == QStringLiteral("WARNING")) {
        return 3;
    }

    if (normalized
            == QStringLiteral("DEBUG")
        || normalized
               == QStringLiteral("TRACE")) {
        return 5;
    }

    return 4;
}

QString renderedLevel(
    const QString &severity
    )
{
    switch (windowsLevel(severity)) {
    case 1:
        return QStringLiteral("Critical");

    case 2:
        return QStringLiteral("Error");

    case 3:
        return QStringLiteral("Warning");

    case 5:
        return QStringLiteral("Verbose");

    default:
        return QStringLiteral("Information");
    }
}

quint32 stableEventId(
    const QString &eventCode
    )
{
    /*
     * Use a fixed FNV-1a calculation rather than
     * qHash() so generated Event IDs remain stable
     * across runs and processes.
     */
    quint32 hash = 2166136261u;

    const QByteArray bytes =
        eventCode.toUtf8();

    for (const char character : bytes) {
        hash ^=
            static_cast<unsigned char>(
                character
                );

        hash *= 16777619u;
    }

    /*
     * Keep the generated ID in a compact positive
     * range while reserving low values.
     */
    return 1000u
           + (hash % 59000u);
}

bool isReservedNamedDataKey(
    const QString &key
    )
{
    return key.compare(
               QStringLiteral("EventCode"),
               Qt::CaseInsensitive
               ) == 0
           || key.compare(
                  QStringLiteral("DeviceId"),
                  Qt::CaseInsensitive
                  ) == 0;
}

void writeNamedData(
    QXmlStreamWriter &writer,
    const QString &name,
    const QString &value
    )
{
    writer.writeStartElement(
        QStringLiteral("Data")
        );

    writer.writeAttribute(
        QStringLiteral("Name"),
        name
        );

    writer.writeCharacters(
        value
        );

    writer.writeEndElement();
}
}

QByteArray
WindowsEventXmlRenderer::initialContent() const
{
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<Events>\n"
        );
}

QByteArray
WindowsEventXmlRenderer::renderRecord(
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
    writer.setAutoFormattingIndent(2);

    writer.writeStartElement(
        QStringLiteral("Event")
        );

    writer.writeDefaultNamespace(
        QStringLiteral(
            "http://schemas.microsoft.com/"
            "win/2004/08/events/event"
            )
        );

    writer.writeStartElement(
        QStringLiteral("System")
        );

    writer.writeStartElement(
        QStringLiteral("Provider")
        );

    writer.writeAttribute(
        QStringLiteral("Name"),
        record.subsystem.isEmpty()
            ? QStringLiteral(
                  "TraceScope-Live-Generator"
                  )
            : record.subsystem
        );

    writer.writeEndElement();

    writer.writeTextElement(
        QStringLiteral("EventID"),
        QString::number(
            stableEventId(
                record.eventCode
                )
            )
        );

    writer.writeTextElement(
        QStringLiteral("Version"),
        QStringLiteral("1")
        );

    writer.writeTextElement(
        QStringLiteral("Level"),
        QString::number(
            windowsLevel(
                record.severity
                )
            )
        );

    writer.writeTextElement(
        QStringLiteral("Task"),
        QStringLiteral("0")
        );

    writer.writeTextElement(
        QStringLiteral("Opcode"),
        QStringLiteral("0")
        );

    writer.writeTextElement(
        QStringLiteral("Keywords"),
        QStringLiteral(
            "0x0000000000000000"
            )
        );

    const QDateTime timestamp =
        scenarioStart
            .addMSecs(
                record.timestampOffsetMs
                )
            .toUTC();

    writer.writeStartElement(
        QStringLiteral("TimeCreated")
        );

    writer.writeAttribute(
        QStringLiteral("SystemTime"),
        timestamp.toString(
            Qt::ISODateWithMs
            )
        );

    writer.writeEndElement();

    writer.writeTextElement(
        QStringLiteral("EventRecordID"),
        QString::number(
            100000
            + record.timestampOffsetMs
            )
        );

    writer.writeTextElement(
        QStringLiteral("Channel"),
        QStringLiteral("Application")
        );

    writer.writeTextElement(
        QStringLiteral("Computer"),
        QStringLiteral(
            "TraceScope-Live-Generator"
            )
        );

    writer.writeEndElement();

    writer.writeStartElement(
        QStringLiteral("EventData")
        );

    /*
     * Keep the actual semantic scenario event code
     * separately from the numeric Windows EventID.
     * The generator-specific profile maps this
     * NamedData value to TraceScope eventCode.
     */
    writeNamedData(
        writer,
        QStringLiteral("EventCode"),
        record.eventCode
        );

    writeNamedData(
        writer,
        QStringLiteral("DeviceId"),
        record.entityId
        );

    const QStringList keys =
        sortedAttributeKeys(
            record.attributes
            );

    for (const QString &key : keys) {
        if (isReservedNamedDataKey(
                key
                )) {
            continue;
        }

        writeNamedData(
            writer,
            key,
            scalarText(
                record.attributes.value(
                    key
                    )
                )
            );
    }

    writer.writeEndElement();

    writer.writeStartElement(
        QStringLiteral("RenderingInfo")
        );

    writer.writeAttribute(
        QStringLiteral("Culture"),
        QStringLiteral("en-US")
        );

    writer.writeTextElement(
        QStringLiteral("Message"),
        record.message
        );

    writer.writeTextElement(
        QStringLiteral("Level"),
        renderedLevel(
            record.severity
            )
        );

    writer.writeEndElement();

    writer.writeEndElement();

    buffer.close();

    rendered.replace(
        "\n",
        "\n  "
        );

    rendered.prepend(
        "  "
        );

    rendered.append(
        '\n'
        );

    return rendered;
}

QByteArray
WindowsEventXmlRenderer::finalContent() const
{
    return QByteArray(
        "</Events>\n"
        );
}