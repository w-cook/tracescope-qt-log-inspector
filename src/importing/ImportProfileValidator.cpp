#include "ImportProfileValidator.h"

#include <QSet>
#include <QStringList>

namespace
{
void appendIssue(
    ProfileValidationResult &result,
    const QString &code,
    const QString &message,
    ProfileValidationSeverity severity =
    ProfileValidationSeverity::Error
    )
{
    result.issues.append({
        code,
        message,
        severity
    });
}

bool isValidSourcePath(
    const QString &path,
    bool allowEmpty = true
    )
{
    if (path.isEmpty()) {
        return allowEmpty;
    }

    if (path != path.trimmed()) {
        return false;
    }

    const QStringList segments =
        path.split(
            QLatin1Char('.'),
            Qt::KeepEmptyParts
            );

    for (const QString &segment : segments) {
        if (segment.trimmed().isEmpty()) {
            return false;
        }
    }

    return true;
}

bool isKnownTimestampRuleType(
    TimestampRuleType type
    )
{
    switch (type) {
    case TimestampRuleType::Iso8601:
    case TimestampRuleType::QtFormat:
        return true;
    }

    return false;
}

bool isKnownSeverity(
    RecordSeverity severity
    )
{
    switch (severity) {
    case RecordSeverity::Trace:
    case RecordSeverity::Debug:
    case RecordSeverity::Info:
    case RecordSeverity::Warning:
    case RecordSeverity::Error:
    case RecordSeverity::Critical:
        return true;
    }

    return false;
}

void validateCanonicalPath(
    ProfileValidationResult &result,
    const QString &path,
    const QString &fieldName
    )
{
    if (!isValidSourcePath(path)) {
        appendIssue(
            result,
            QStringLiteral(
                "INVALID_CANONICAL_FIELD_PATH"
                ),
            QStringLiteral(
                "The source path for canonical field '%1' is invalid."
                ).arg(fieldName)
            );
    }
}
}

ProfileValidationResult
ImportProfileValidator::validate(
    const ImportProfile &profile
    ) const
{
    ProfileValidationResult result;

    if (profile.schemaVersion !=
        ImportProfile::CurrentSchemaVersion) {
        appendIssue(
            result,
            QStringLiteral(
                "UNSUPPORTED_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "Import profile schema version %1 is not supported."
                ).arg(profile.schemaVersion)
            );
    }

    if (profile.name.trimmed().isEmpty()) {
        appendIssue(
            result,
            QStringLiteral(
                "PROFILE_NAME_REQUIRED"
                ),
            QStringLiteral(
                "The import profile must have a name."
                )
            );
    }

    if (profile.importerId.trimmed().isEmpty()) {
        appendIssue(
            result,
            QStringLiteral(
                "IMPORTER_ID_REQUIRED"
                ),
            QStringLiteral(
                "The import profile must identify an importer."
                )
            );
    }

    if (!isValidSourcePath(
            profile.recordPath
            )) {
        appendIssue(
            result,
            QStringLiteral(
                "INVALID_RECORD_PATH"
                ),
            QStringLiteral(
                "The import profile record path is invalid."
                )
            );
    }

    validateCanonicalPath(
        result,
        profile.canonicalFields.timestampPath,
        QStringLiteral("timestamp")
        );

    validateCanonicalPath(
        result,
        profile.canonicalFields.severityPath,
        QStringLiteral("severity")
        );

    validateCanonicalPath(
        result,
        profile.canonicalFields.subsystemPath,
        QStringLiteral("subsystem")
        );

    validateCanonicalPath(
        result,
        profile.canonicalFields.eventCodePath,
        QStringLiteral("eventCode")
        );

    validateCanonicalPath(
        result,
        profile.canonicalFields.entityIdPath,
        QStringLiteral("entityId")
        );

    validateCanonicalPath(
        result,
        profile.canonicalFields.messagePath,
        QStringLiteral("message")
        );

    QSet<QString> customFieldNames;

    const QSet<QString> reservedNames {
        QStringLiteral("timestamp"),
        QStringLiteral("severity"),
        QStringLiteral("subsystem"),
        QStringLiteral("eventcode"),
        QStringLiteral("entityid"),
        QStringLiteral("message")
    };

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const QString name =
            mapping.name.trimmed();

        const QString normalizedName =
            name.toCaseFolded();

        if (name.isEmpty()) {
            appendIssue(
                result,
                QStringLiteral(
                    "CUSTOM_FIELD_NAME_REQUIRED"
                    ),
                QStringLiteral(
                    "Each custom field mapping must have a name."
                    )
                );
        } else {
            if (reservedNames.contains(
                    normalizedName
                    )) {
                appendIssue(
                    result,
                    QStringLiteral(
                        "CUSTOM_FIELD_NAME_RESERVED"
                        ),
                    QStringLiteral(
                        "Custom field name '%1' conflicts with a canonical field."
                        ).arg(name)
                    );
            }

            if (customFieldNames.contains(
                    normalizedName
                    )) {
                appendIssue(
                    result,
                    QStringLiteral(
                        "DUPLICATE_CUSTOM_FIELD_NAME"
                        ),
                    QStringLiteral(
                        "Custom field name '%1' is duplicated."
                        ).arg(name)
                    );
            } else {
                customFieldNames.insert(
                    normalizedName
                    );
            }
        }

        if (!isValidSourcePath(
                mapping.sourcePath,
                false
                )) {
            appendIssue(
                result,
                QStringLiteral(
                    "INVALID_CUSTOM_FIELD_PATH"
                    ),
                QStringLiteral(
                    "Custom field '%1' must have a valid source path."
                    ).arg(name)
                );
        }
    }

    QSet<QString> normalizedAliases;

    for (auto iterator =
         profile.severityAliases.constBegin();
         iterator !=
         profile.severityAliases.constEnd();
         ++iterator) {
        const QString alias =
            iterator.key().trimmed();

        const QString normalizedAlias =
            alias.toCaseFolded();

        if (alias.isEmpty()) {
            appendIssue(
                result,
                QStringLiteral(
                    "SEVERITY_ALIAS_REQUIRED"
                    ),
                QStringLiteral(
                    "Severity aliases cannot be empty."
                    )
                );

            continue;
        }

        if (normalizedAliases.contains(
                normalizedAlias
                )) {
            appendIssue(
                result,
                QStringLiteral(
                    "DUPLICATE_SEVERITY_ALIAS"
                    ),
                QStringLiteral(
                    "Severity alias '%1' is duplicated."
                    ).arg(alias)
                );
        } else {
            normalizedAliases.insert(
                normalizedAlias
                );
        }

        if (!isKnownSeverity(
                iterator.value()
                )) {
            appendIssue(
                result,
                QStringLiteral(
                    "INVALID_SEVERITY_TARGET"
                    ),
                QStringLiteral(
                    "Severity alias '%1' has an invalid target severity."
                    ).arg(alias)
                );
        }
    }

    if (!profile.canonicalFields
             .timestampPath
             .trimmed()
             .isEmpty()
        && profile.timestampRules.isEmpty()) {
        appendIssue(
            result,
            QStringLiteral(
                "TIMESTAMP_RULE_REQUIRED"
                ),
            QStringLiteral(
                "A mapped timestamp field requires at least one timestamp rule."
                )
            );
    }

    for (const TimestampRule &rule
         : profile.timestampRules) {
        if (!isKnownTimestampRuleType(
                rule.type
                )) {
            appendIssue(
                result,
                QStringLiteral(
                    "INVALID_TIMESTAMP_RULE_TYPE"
                    ),
                QStringLiteral(
                    "The profile contains an unsupported timestamp rule type."
                    )
                );

            continue;
        }

        if (rule.type ==
                TimestampRuleType::QtFormat
            && rule.format.trimmed().isEmpty()) {
            appendIssue(
                result,
                QStringLiteral(
                    "TIMESTAMP_FORMAT_REQUIRED"
                    ),
                QStringLiteral(
                    "Qt timestamp rules require a format string."
                    )
                );
        }

        if (rule.type ==
                TimestampRuleType::Iso8601
            && !rule.format.isEmpty()) {
            appendIssue(
                result,
                QStringLiteral(
                    "ISO_TIMESTAMP_FORMAT_IGNORED"
                    ),
                QStringLiteral(
                    "ISO 8601 timestamp rules do not use a custom format string."
                    ),
                ProfileValidationSeverity::Warning
                );
        }
    }

    return result;
}