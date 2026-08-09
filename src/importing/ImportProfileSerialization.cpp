#include "ImportProfileSerialization.h"

#include <cmath>
#include <limits>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "../domain/RecordSeverity.h"

namespace
{
ProfileDeserializationResult failure(
    const QString &code,
    const QString &message
    )
{
    ProfileDeserializationResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

QString timestampRuleTypeToString(
    TimestampRuleType type
    )
{
    switch (type) {
    case TimestampRuleType::Iso8601:
        return QStringLiteral("iso8601");

    case TimestampRuleType::QtFormat:
        return QStringLiteral("qt-format");
    }

    return {};
}

std::optional<TimestampRuleType>
timestampRuleTypeFromString(
    const QString &value
    )
{
    if (value == QStringLiteral("iso8601")) {
        return TimestampRuleType::Iso8601;
    }

    if (value == QStringLiteral("qt-format")) {
        return TimestampRuleType::QtFormat;
    }

    return std::nullopt;
}

bool readRequiredString(
    const QJsonObject &object,
    const QString &key,
    QString &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isString()) {
        return false;
    }

    result = value.toString();

    return true;
}

bool readCanonicalFields(
    const QJsonObject &root,
    CanonicalFieldMappings &mappings
    )
{
    const QJsonValue fieldsValue =
        root.value(
            QStringLiteral("canonicalFields")
            );

    if (!fieldsValue.isObject()) {
        return false;
    }

    const QJsonObject fields =
        fieldsValue.toObject();

    return
        readRequiredString(
            fields,
            QStringLiteral("timestamp"),
            mappings.timestampPath
            )
        &&
        readRequiredString(
            fields,
            QStringLiteral("severity"),
            mappings.severityPath
            )
        &&
        readRequiredString(
            fields,
            QStringLiteral("subsystem"),
            mappings.subsystemPath
            )
        &&
        readRequiredString(
            fields,
            QStringLiteral("eventCode"),
            mappings.eventCodePath
            )
        &&
        readRequiredString(
            fields,
            QStringLiteral("entityId"),
            mappings.entityIdPath
            )
        &&
        readRequiredString(
            fields,
            QStringLiteral("message"),
            mappings.messagePath
            );
}

QJsonObject serializeCanonicalFields(
    const CanonicalFieldMappings &mappings
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("timestamp"),
        mappings.timestampPath
        );

    object.insert(
        QStringLiteral("severity"),
        mappings.severityPath
        );

    object.insert(
        QStringLiteral("subsystem"),
        mappings.subsystemPath
        );

    object.insert(
        QStringLiteral("eventCode"),
        mappings.eventCodePath
        );

    object.insert(
        QStringLiteral("entityId"),
        mappings.entityIdPath
        );

    object.insert(
        QStringLiteral("message"),
        mappings.messagePath
        );

    return object;
}
}

QByteArray ImportProfileSerializer::serialize(
    const ImportProfile &profile
    ) const
{
    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        profile.schemaVersion
        );

    root.insert(
        QStringLiteral("name"),
        profile.name
        );

    root.insert(
        QStringLiteral("importerId"),
        profile.importerId
        );

    root.insert(
        QStringLiteral("canonicalFields"),
        serializeCanonicalFields(
            profile.canonicalFields
            )
        );

    QJsonArray customFields;

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        QJsonObject object;

        object.insert(
            QStringLiteral("name"),
            mapping.name
            );

        object.insert(
            QStringLiteral("sourcePath"),
            mapping.sourcePath
            );

        customFields.append(object);
    }

    root.insert(
        QStringLiteral("customFields"),
        customFields
        );

    QJsonObject severityAliases;

    for (auto iterator =
         profile.severityAliases.constBegin();
         iterator !=
         profile.severityAliases.constEnd();
         ++iterator) {
        severityAliases.insert(
            iterator.key(),
            recordSeverityToString(
                iterator.value()
                )
            );
    }

    root.insert(
        QStringLiteral("severityAliases"),
        severityAliases
        );

    QJsonArray timestampRules;

    for (const TimestampRule &rule
         : profile.timestampRules) {
        QJsonObject object;

        object.insert(
            QStringLiteral("type"),
            timestampRuleTypeToString(
                rule.type
                )
            );

        if (!rule.format.isEmpty()) {
            object.insert(
                QStringLiteral("format"),
                rule.format
                );
        }

        timestampRules.append(object);
    }

    root.insert(
        QStringLiteral("timestampRules"),
        timestampRules
        );

    root.insert(
        QStringLiteral(
            "preserveUnmappedFields"
            ),
        profile.preserveUnmappedFields
        );

    return QJsonDocument(root).toJson(
        QJsonDocument::Indented
        );
}

ProfileDeserializationResult
ImportProfileSerializer::deserialize(
    const QByteArray &json
    ) const
{
    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json,
            &parseError
            );

    if (parseError.error !=
        QJsonParseError::NoError) {
        return failure(
            QStringLiteral("INVALID_JSON"),
            QStringLiteral(
                "The import profile is not valid JSON: %1"
                ).arg(
                    parseError.errorString()
                    )
            );
    }

    if (!document.isObject()) {
        return failure(
            QStringLiteral(
                "PROFILE_ROOT_NOT_OBJECT"
                ),
            QStringLiteral(
                "The import profile root must be a JSON object."
                )
            );
    }

    const QJsonObject root =
        document.object();

    ImportProfile profile;

    const QJsonValue schemaVersionValue =
        root.value(
            QStringLiteral("schemaVersion")
            );

    if (!schemaVersionValue.isDouble()) {
        return failure(
            QStringLiteral(
                "INVALID_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "The import profile schemaVersion must be an integer."
                )
            );
    }

    const double schemaVersionNumber =
        schemaVersionValue.toDouble();

    if (schemaVersionNumber <
            std::numeric_limits<int>::min()
        ||
        schemaVersionNumber >
            std::numeric_limits<int>::max()
        ||
        std::floor(schemaVersionNumber) !=
            schemaVersionNumber) {
        return failure(
            QStringLiteral(
                "INVALID_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "The import profile schemaVersion must be an integer."
                )
            );
    }

    profile.schemaVersion =
        static_cast<int>(
            schemaVersionNumber
            );

    if (!readRequiredString(
            root,
            QStringLiteral("name"),
            profile.name
            )) {
        return failure(
            QStringLiteral(
                "INVALID_PROFILE_NAME"
                ),
            QStringLiteral(
                "The import profile name must be a string."
                )
            );
    }

    if (!readRequiredString(
            root,
            QStringLiteral("importerId"),
            profile.importerId
            )) {
        return failure(
            QStringLiteral(
                "INVALID_IMPORTER_ID"
                ),
            QStringLiteral(
                "The import profile importerId must be a string."
                )
            );
    }

    if (!readCanonicalFields(
            root,
            profile.canonicalFields
            )) {
        return failure(
            QStringLiteral(
                "INVALID_CANONICAL_FIELDS"
                ),
            QStringLiteral(
                "The import profile must contain all canonical field mappings as strings."
                )
            );
    }

    const QJsonValue customFieldsValue =
        root.value(
            QStringLiteral("customFields")
            );

    if (!customFieldsValue.isArray()) {
        return failure(
            QStringLiteral(
                "INVALID_CUSTOM_FIELDS"
                ),
            QStringLiteral(
                "The import profile customFields value must be an array."
                )
            );
    }

    for (const QJsonValue &value
         : customFieldsValue.toArray()) {
        if (!value.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_CUSTOM_FIELD"
                    ),
                QStringLiteral(
                    "Each custom field mapping must be an object."
                    )
                );
        }

        const QJsonObject object =
            value.toObject();

        CustomFieldMapping mapping;

        if (!readRequiredString(
                object,
                QStringLiteral("name"),
                mapping.name
                )
            ||
            !readRequiredString(
                object,
                QStringLiteral("sourcePath"),
                mapping.sourcePath
                )) {
            return failure(
                QStringLiteral(
                    "INVALID_CUSTOM_FIELD"
                    ),
                QStringLiteral(
                    "Each custom field mapping requires string name and sourcePath values."
                    )
                );
        }

        profile.customFields.append(
            mapping
            );
    }

    const QJsonValue aliasesValue =
        root.value(
            QStringLiteral("severityAliases")
            );

    if (!aliasesValue.isObject()) {
        return failure(
            QStringLiteral(
                "INVALID_SEVERITY_ALIASES"
                ),
            QStringLiteral(
                "The import profile severityAliases value must be an object."
                )
            );
    }

    const QJsonObject aliases =
        aliasesValue.toObject();

    for (auto iterator =
         aliases.constBegin();
         iterator != aliases.constEnd();
         ++iterator) {
        if (!iterator.value().isString()) {
            return failure(
                QStringLiteral(
                    "INVALID_SEVERITY_ALIAS_TARGET"
                    ),
                QStringLiteral(
                    "Severity alias '%1' must map to a severity name."
                    ).arg(iterator.key())
                );
        }

        const auto severity =
            parseRecordSeverity(
                iterator.value().toString()
                );

        if (!severity.has_value()) {
            return failure(
                QStringLiteral(
                    "INVALID_SEVERITY_ALIAS_TARGET"
                    ),
                QStringLiteral(
                    "Severity alias '%1' maps to an unknown severity."
                    ).arg(iterator.key())
                );
        }

        profile.severityAliases.insert(
            iterator.key(),
            *severity
            );
    }

    const QJsonValue rulesValue =
        root.value(
            QStringLiteral("timestampRules")
            );

    if (!rulesValue.isArray()) {
        return failure(
            QStringLiteral(
                "INVALID_TIMESTAMP_RULES"
                ),
            QStringLiteral(
                "The import profile timestampRules value must be an array."
                )
            );
    }

    profile.timestampRules.clear();

    for (const QJsonValue &value
         : rulesValue.toArray()) {
        if (!value.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_TIMESTAMP_RULE"
                    ),
                QStringLiteral(
                    "Each timestamp rule must be an object."
                    )
                );
        }

        const QJsonObject object =
            value.toObject();

        QString typeText;

        if (!readRequiredString(
                object,
                QStringLiteral("type"),
                typeText
                )) {
            return failure(
                QStringLiteral(
                    "INVALID_TIMESTAMP_RULE"
                    ),
                QStringLiteral(
                    "Each timestamp rule requires a string type."
                    )
                );
        }

        const auto type =
            timestampRuleTypeFromString(
                typeText
                );

        if (!type.has_value()) {
            return failure(
                QStringLiteral(
                    "UNKNOWN_TIMESTAMP_RULE_TYPE"
                    ),
                QStringLiteral(
                    "Timestamp rule type '%1' is not supported."
                    ).arg(typeText)
                );
        }

        TimestampRule rule;

        rule.type = *type;

        if (object.contains(
                QStringLiteral("format")
                )) {
            const QJsonValue formatValue =
                object.value(
                    QStringLiteral("format")
                    );

            if (!formatValue.isString()) {
                return failure(
                    QStringLiteral(
                        "INVALID_TIMESTAMP_FORMAT"
                        ),
                    QStringLiteral(
                        "Timestamp rule format must be a string."
                        )
                    );
            }

            rule.format =
                formatValue.toString();
        }

        profile.timestampRules.append(
            rule
            );
    }

    const QJsonValue preserveValue =
        root.value(
            QStringLiteral(
                "preserveUnmappedFields"
                )
            );

    if (!preserveValue.isBool()) {
        return failure(
            QStringLiteral(
                "INVALID_PRESERVE_UNMAPPED_FIELDS"
                ),
            QStringLiteral(
                "The import profile preserveUnmappedFields value must be a boolean."
                )
            );
    }

    profile.preserveUnmappedFields =
        preserveValue.toBool();

    ProfileDeserializationResult result;

    result.profile = profile;

    return result;
}