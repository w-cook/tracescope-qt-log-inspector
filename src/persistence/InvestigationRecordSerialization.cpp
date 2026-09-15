#include "InvestigationRecordSerialization.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QJsonValue>
#include <QStringList>

#include "../domain/RecordSeverity.h"

namespace
{
InvestigationRecordDeserializationResult failure(
    const QString &code,
    const QString &message
    )
{
    InvestigationRecordDeserializationResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

QStringList sortedCustomAttributeKeys(
    const InvestigationRecord &record
    )
{
    QStringList keys =
        record.customAttributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](
            const QString &left,
            const QString &right
            ) {
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

void insertOptionalString(
    QJsonObject &object,
    const QString &key,
    const std::optional<QString> &value
    )
{
    if (!value.has_value()) {
        return;
    }

    object.insert(
        key,
        *value
        );
}

bool readOptionalString(
    const QJsonObject &object,
    const QString &key,
    std::optional<QString> &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (value.isUndefined()
        || value.isNull()) {
        result.reset();

        return true;
    }

    if (!value.isString()) {
        return false;
    }

    result =
        value.toString();

    return true;
}

bool readSignedInteger(
    const QJsonValue &value,
    qint64 &result
    )
{
    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number
               < static_cast<double>(
                   std::numeric_limits<qint64>::min()
                   )
        || number
               > static_cast<double>(
                   std::numeric_limits<qint64>::max()
                   )) {
        return false;
    }

    result =
        static_cast<qint64>(number);

    return true;
}

bool readUnsignedInteger(
    const QJsonValue &value,
    quint64 &result
    )
{
    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number < 0.0
        || number
               > static_cast<double>(
                   std::numeric_limits<quint64>::max()
                   )) {
        return false;
    }

    result =
        static_cast<quint64>(number);

    return true;
}
}

QJsonObject InvestigationRecordSerializer::serialize(
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
                *record.severity
                )
            );
    }

    insertOptionalString(
        object,
        QStringLiteral("subsystem"),
        record.subsystem
        );

    insertOptionalString(
        object,
        QStringLiteral("eventCode"),
        record.eventCode
        );

    insertOptionalString(
        object,
        QStringLiteral("entityId"),
        record.entityId
        );

    insertOptionalString(
        object,
        QStringLiteral("message"),
        record.message
        );

    QJsonObject customAttributes;

    const QStringList customAttributeKeys =
        sortedCustomAttributeKeys(record);

    for (const QString &key
         : customAttributeKeys) {
        customAttributes.insert(
            key,
            QJsonValue::fromVariant(
                record.customAttributes.value(
                    key
                    )
                )
            );
    }

    object.insert(
        QStringLiteral("customAttributes"),
        customAttributes
        );

    object.insert(
        QStringLiteral("rawSource"),
        record.rawSource
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

    source.insert(
        QStringLiteral("sourceGeneration"),
        static_cast<qint64>(
            record.source.sourceGeneration
            )
        );

    object.insert(
        QStringLiteral("source"),
        source
        );

    return object;
}

InvestigationRecordDeserializationResult
InvestigationRecordSerializer::deserialize(
    const QJsonObject &object
    ) const
{
    InvestigationRecord record;

    const QJsonValue recordIdValue =
        object.value(
            QStringLiteral("recordId")
            );

    if (!recordIdValue.isString()) {
        return failure(
            QStringLiteral(
                "INVALID_RECORD_ID"
                ),
            QStringLiteral(
                "The investigation record "
                "recordId must be a string."
                )
            );
    }

    record.recordId =
        recordIdValue.toString();

    const QJsonValue timestampValue =
        object.value(
            QStringLiteral("timestamp")
            );

    if (!timestampValue.isUndefined()
        && !timestampValue.isNull()) {
        if (!timestampValue.isString()) {
            return failure(
                QStringLiteral(
                    "INVALID_TIMESTAMP"
                    ),
                QStringLiteral(
                    "The investigation record "
                    "timestamp must be a string."
                    )
                );
        }

        const QDateTime timestamp =
            QDateTime::fromString(
                timestampValue.toString(),
                Qt::ISODateWithMs
                );

        if (!timestamp.isValid()) {
            return failure(
                QStringLiteral(
                    "INVALID_TIMESTAMP"
                    ),
                QStringLiteral(
                    "The investigation record "
                    "timestamp is invalid."
                    )
                );
        }

        record.timestamp =
            timestamp;
    }

    const QJsonValue severityValue =
        object.value(
            QStringLiteral("severity")
            );

    if (!severityValue.isUndefined()
        && !severityValue.isNull()) {
        if (!severityValue.isString()) {
            return failure(
                QStringLiteral(
                    "INVALID_SEVERITY"
                    ),
                QStringLiteral(
                    "The investigation record "
                    "severity must be a string."
                    )
                );
        }

        const std::optional<RecordSeverity>
            severity =
            parseRecordSeverity(
                severityValue.toString()
                );

        if (!severity.has_value()) {
            return failure(
                QStringLiteral(
                    "INVALID_SEVERITY"
                    ),
                QStringLiteral(
                    "The investigation record "
                    "severity is not recognized."
                    )
                );
        }

        record.severity =
            *severity;
    }

    if (!readOptionalString(
            object,
            QStringLiteral("subsystem"),
            record.subsystem
            )
        || !readOptionalString(
            object,
            QStringLiteral("eventCode"),
            record.eventCode
            )
        || !readOptionalString(
            object,
            QStringLiteral("entityId"),
            record.entityId
            )
        || !readOptionalString(
            object,
            QStringLiteral("message"),
            record.message
            )) {
        return failure(
            QStringLiteral(
                "INVALID_CANONICAL_FIELD"
                ),
            QStringLiteral(
                "An optional canonical investigation "
                "record field has an invalid value."
                )
            );
    }

    const QJsonValue customAttributesValue =
        object.value(
            QStringLiteral("customAttributes")
            );

    if (!customAttributesValue.isObject()) {
        return failure(
            QStringLiteral(
                "INVALID_CUSTOM_ATTRIBUTES"
                ),
            QStringLiteral(
                "The investigation record "
                "customAttributes value must be "
                "an object."
                )
            );
    }

    const QJsonObject customAttributes =
        customAttributesValue.toObject();

    for (
        auto iterator =
        customAttributes.constBegin();
        iterator != customAttributes.constEnd();
        ++iterator
        ) {
        record.customAttributes.insert(
            iterator.key(),
            iterator.value().toVariant()
            );
    }

    const QJsonValue rawSourceValue =
        object.value(
            QStringLiteral("rawSource")
            );

    if (!rawSourceValue.isString()) {
        return failure(
            QStringLiteral(
                "INVALID_RAW_SOURCE"
                ),
            QStringLiteral(
                "The investigation record "
                "rawSource must be a string."
                )
            );
    }

    record.rawSource =
        rawSourceValue.toString();

    const QJsonValue sourceValue =
        object.value(
            QStringLiteral("source")
            );

    if (!sourceValue.isObject()) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE"
                ),
            QStringLiteral(
                "The investigation record source "
                "must be an object."
                )
            );
    }

    const QJsonObject source =
        sourceValue.toObject();

    const QJsonValue sourcePathValue =
        source.value(
            QStringLiteral("sourcePath")
            );

    const QJsonValue sourceNameValue =
        source.value(
            QStringLiteral("sourceName")
            );

    if (!sourcePathValue.isString()
        || !sourceNameValue.isString()) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_IDENTITY"
                ),
            QStringLiteral(
                "The investigation record source "
                "path and name must be strings."
                )
            );
    }

    record.source.sourcePath =
        sourcePathValue.toString();

    record.source.sourceName =
        sourceNameValue.toString();

    if (!readSignedInteger(
            source.value(
                QStringLiteral("recordNumber")
                ),
            record.source.recordNumber
            )) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_RECORD_NUMBER"
                ),
            QStringLiteral(
                "The investigation record source "
                "record number must be an integer."
                )
            );
    }

    if (!readUnsignedInteger(
            source.value(
                QStringLiteral("sourceGeneration")
                ),
            record.source.sourceGeneration
            )) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_GENERATION"
                ),
            QStringLiteral(
                "The investigation record source "
                "generation must be a non-negative "
                "integer."
                )
            );
    }

    InvestigationRecordDeserializationResult result;

    result.record =
        std::move(record);

    return result;
}