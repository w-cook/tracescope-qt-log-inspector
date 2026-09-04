#include "InvestigationRecordExportFormatter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

#include <algorithm>

#include "../domain/RecordSeverity.h"

namespace
{
QStringList sortedCustomAttributeKeys(
    const InvestigationRecord &record
    )
{
    QStringList keys =
        record.customAttributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](const QString &left, const QString &right) {
            const int insensitiveComparison =
                left.compare(
                    right,
                    Qt::CaseInsensitive
                    );

            if (insensitiveComparison != 0) {
                return insensitiveComparison < 0;
            }

            return left.compare(
                       right,
                       Qt::CaseSensitive
                       ) < 0;
        }
        );

    return keys;
}

QJsonObject customAttributesToJson(
    const InvestigationRecord &record
    )
{
    QJsonObject attributes;

    const QStringList keys =
        sortedCustomAttributeKeys(record);

    for (const QString &key : keys) {
        attributes.insert(
            key,
            QJsonValue::fromVariant(
                record.customAttributes.value(
                    key
                    )
                )
            );
    }

    return attributes;
}

QString customAttributeText(
    const QVariant &value
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return QStringLiteral("null");
    }

    const QJsonValue jsonValue =
        QJsonValue::fromVariant(value);

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

    if (jsonValue.isBool()) {
        return jsonValue.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}
}

QString InvestigationRecordExportFormatter::
    toStructuredJson(
        const InvestigationRecord &record
        ) const
{
    QJsonObject object;

    object.insert(
        QStringLiteral("recordId"),
        record.recordId
        );

    if (record.timestamp.has_value()) {
        object.insert(
            QStringLiteral("timestamp"),
            record.timestamp->toString(
                Qt::ISODateWithMs
                )
            );
    }

    if (record.severity.has_value()) {
        object.insert(
            QStringLiteral("severity"),
            recordSeverityToString(
                record.severity.value()
                )
            );
    }

    if (record.subsystem.has_value()) {
        object.insert(
            QStringLiteral("subsystem"),
            record.subsystem.value()
            );
    }

    if (record.eventCode.has_value()) {
        object.insert(
            QStringLiteral("eventCode"),
            record.eventCode.value()
            );
    }

    if (record.entityId.has_value()) {
        object.insert(
            QStringLiteral("entityId"),
            record.entityId.value()
            );
    }

    if (record.message.has_value()) {
        object.insert(
            QStringLiteral("message"),
            record.message.value()
            );
    }

    object.insert(
        QStringLiteral("customAttributes"),
        customAttributesToJson(record)
        );

    QJsonObject source;

    source.insert(
        QStringLiteral("sourcePath"),
        record.source.sourcePath
        );

    source.insert(
        QStringLiteral("sourceName"),
        record.source.sourceName
        );

    source.insert(
        QStringLiteral("recordNumber"),
        record.source.recordNumber
        );

    object.insert(
        QStringLiteral("source"),
        source
        );

    object.insert(
        QStringLiteral("rawSource"),
        record.rawSource
        );

    return QString::fromUtf8(
        QJsonDocument(object).toJson(
            QJsonDocument::Indented
            )
        );
}

QString InvestigationRecordExportFormatter::
    toFormattedText(
        const InvestigationRecord &record
        ) const
{
    QStringList lines;

    lines.append(
        QStringLiteral("Record ID: %1")
            .arg(record.recordId)
        );

    if (record.timestamp.has_value()) {
        lines.append(
            QStringLiteral("Timestamp: %1")
                .arg(
                    record.timestamp->toString(
                        Qt::ISODateWithMs
                        )
                    )
            );
    }

    if (record.severity.has_value()) {
        lines.append(
            QStringLiteral("Severity: %1")
                .arg(
                    recordSeverityToString(
                        record.severity.value()
                        )
                    )
            );
    }

    if (record.subsystem.has_value()) {
        lines.append(
            QStringLiteral("Subsystem: %1")
                .arg(record.subsystem.value())
            );
    }

    if (record.eventCode.has_value()) {
        lines.append(
            QStringLiteral("Event Code: %1")
                .arg(record.eventCode.value())
            );
    }

    if (record.entityId.has_value()) {
        lines.append(
            QStringLiteral("Entity ID: %1")
                .arg(record.entityId.value())
            );
    }

    if (record.message.has_value()) {
        lines.append(
            QStringLiteral("Message: %1")
                .arg(record.message.value())
            );
    }

    const QStringList customKeys =
        sortedCustomAttributeKeys(record);

    if (!customKeys.isEmpty()) {
        lines.append(QString());
        lines.append(
            QStringLiteral("Custom Attributes:")
            );

        for (const QString &key : customKeys) {
            lines.append(
                QStringLiteral("  %1: %2")
                    .arg(
                        key,
                        customAttributeText(
                            record.customAttributes.value(
                                key
                                )
                            )
                        )
                );
        }
    }

    const bool hasSourceContext =
        !record.source.sourcePath.isEmpty()
        || !record.source.sourceName.isEmpty()
        || record.source.recordNumber > 0;

    if (hasSourceContext) {
        lines.append(QString());
        lines.append(
            QStringLiteral("Source:")
            );

        if (!record.source.sourceName.isEmpty()) {
            lines.append(
                QStringLiteral("  Name: %1")
                    .arg(
                        record.source.sourceName
                        )
                );
        }

        if (!record.source.sourcePath.isEmpty()) {
            lines.append(
                QStringLiteral("  Path: %1")
                    .arg(
                        record.source.sourcePath
                        )
                );
        }

        if (record.source.recordNumber > 0) {
            lines.append(
                QStringLiteral("  Record: %1")
                    .arg(
                        record.source.recordNumber
                        )
                );
        }
    }

    if (!record.rawSource.isEmpty()) {
        lines.append(QString());
        lines.append(
            QStringLiteral("Raw Source:")
            );
        lines.append(
            record.rawSource
            );
    }

    return lines.join(
        QLatin1Char('\n')
        );
}