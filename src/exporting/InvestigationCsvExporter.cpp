#include "InvestigationCsvExporter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QTextStream>

#include <algorithm>
#include <utility>

#include "../domain/RecordSeverity.h"

namespace
{
bool isCanonicalKey(
    const QString &key
    )
{
    return key == QStringLiteral("timestamp")
    || key == QStringLiteral("level")
        || key == QStringLiteral("severity")
        || key == QStringLiteral("subsystem")
        || key == QStringLiteral("eventCode")
        || key == QStringLiteral("entityId")
        || key == QStringLiteral("message");
}
}

bool InvestigationCsvExporter::exportToFile(
    const QVector<InvestigationRecord> &records,
    const QString &filePath
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Text
            )) {
        return false;
    }

    QTextStream stream(&file);

    const QStringList customKeys =
        customAttributeKeys(records);

    QStringList headers {
        QStringLiteral("timestamp"),
        QStringLiteral("level"),
        QStringLiteral("subsystem"),
        QStringLiteral("eventCode"),
        QStringLiteral("entityId"),
        QStringLiteral("message")
    };

    headers.append(customKeys);

    QStringList escapedHeaders;
    escapedHeaders.reserve(headers.size());

    for (
        const QString &header :
        std::as_const(headers)
        ) {
        escapedHeaders.append(
            escapeCsvField(header)
            );
    }

    stream
        << escapedHeaders.join(
               QLatin1Char(',')
               )
        << QLatin1Char('\n');

    for (
        const InvestigationRecord &record :
        records
        ) {
        QStringList fields;

        fields.reserve(
            headers.size()
            );

        fields.append(
            record.timestamp.has_value()
                ? record.timestamp->toString(
                      Qt::ISODateWithMs
                      )
                : QString()
            );

        fields.append(
            record.severity.has_value()
                ? recordSeverityToString(
                      record.severity.value()
                      )
                : QString()
            );

        fields.append(
            record.subsystem.value_or(
                QString()
                )
            );

        fields.append(
            record.eventCode.value_or(
                QString()
                )
            );

        fields.append(
            record.entityId.value_or(
                QString()
                )
            );

        fields.append(
            record.message.value_or(
                QString()
                )
            );

        for (
            const QString &key :
            customKeys
            ) {
            fields.append(
                variantToString(
                    record.customAttributes.value(
                        key
                        )
                    )
                );
        }

        QStringList escapedFields;
        escapedFields.reserve(
            fields.size()
            );

        for (
            const QString &field :
            std::as_const(fields)
            ) {
            escapedFields.append(
                escapeCsvField(field)
                );
        }

        stream
            << escapedFields.join(
                   QLatin1Char(',')
                   )
            << QLatin1Char('\n');
    }

    return true;
}

QStringList InvestigationCsvExporter::
    customAttributeKeys(
        const QVector<InvestigationRecord> &records
        ) const
{
    QSet<QString> keys;

    for (
        const InvestigationRecord &record :
        records
        ) {
        for (
            auto iterator =
            record.customAttributes.constBegin();
            iterator !=
            record.customAttributes.constEnd();
            ++iterator
            ) {
            if (!isCanonicalKey(
                    iterator.key()
                    )) {
                keys.insert(
                    iterator.key()
                    );
            }
        }
    }

    QStringList sortedKeys =
        keys.values();

    std::sort(
        sortedKeys.begin(),
        sortedKeys.end(),
        [](const QString &left, const QString &right) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );

    return sortedKeys;
}

QString InvestigationCsvExporter::
    variantToString(
        const QVariant &value
        ) const
{
    if (!value.isValid()
        || value.isNull()) {
        return {};
    }

    const QJsonValue jsonValue =
        QJsonValue::fromVariant(
            value
            );

    if (jsonValue.isObject()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toObject()
                ).toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    if (jsonValue.isArray()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toArray()
                ).toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    return value.toString();
}

QString InvestigationCsvExporter::
    escapeCsvField(
        const QString &value
        ) const
{
    QString escaped = value;

    escaped.replace(
        "\"",
        "\"\""
        );

    if (escaped.contains(",")
        || escaped.contains("\"")
        || escaped.contains("\n")
        || escaped.contains("\r")) {
        return "\""
               + escaped
               + "\"";
    }

    return escaped;
}