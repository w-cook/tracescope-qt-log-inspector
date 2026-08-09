#include "JsonLinesImporter.h"

#include <optional>
#include <utility>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

#include "../domain/RecordIdentity.h"
#include "../domain/RecordSeverity.h"
#include "../domain/RecordTimestamp.h"
#include "ImportDiagnostic.h"

namespace
{
RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber
    )
{
    RecordSourceMetadata source;
    source.sourcePath = sourcePath;
    source.recordNumber = recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(sourcePath).fileName();
    }

    return source;
}

QJsonValue readJsonPath(
    const QJsonObject &object,
    const QString &path
    )
{
    const QString trimmedPath = path.trimmed();

    if (trimmedPath.isEmpty()) {
        return QJsonValue(QJsonValue::Undefined);
    }

    const QStringList segments =
        trimmedPath.split(QLatin1Char('.'));

    QJsonValue currentValue(object);

    for (const QString &segment : segments) {
        if (segment.isEmpty() ||
            !currentValue.isObject()) {
            return QJsonValue(
                QJsonValue::Undefined
                );
        }

        currentValue =
            currentValue.toObject().value(segment);

        if (currentValue.isUndefined()) {
            return currentValue;
        }
    }

    return currentValue;
}

std::optional<QString> readOptionalString(
    const QJsonObject &object,
    const QString &path
    )
{
    const QJsonValue value =
        readJsonPath(object, path);

    if (!value.isString()) {
        return std::nullopt;
    }

    const QString text =
        value.toString().trimmed();

    if (text.isEmpty()) {
        return std::nullopt;
    }

    return text;
}

void appendDiagnostic(
    ImportResult &result,
    const QString &code,
    const QString &message,
    ImportDiagnosticSeverity severity,
    const std::optional<RecordSourceMetadata> &source =
    std::nullopt
    )
{
    ImportDiagnostic diagnostic;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.severity = severity;
    diagnostic.source = source;

    result.diagnostics.append(diagnostic);
}

QSet<QString> mappedTopLevelFields(
    const ImportProfile &profile
    )
{
    QSet<QString> fields;

    const QStringList canonicalPaths {
        profile.canonicalFields.timestampPath,
        profile.canonicalFields.severityPath,
        profile.canonicalFields.subsystemPath,
        profile.canonicalFields.eventCodePath,
        profile.canonicalFields.entityIdPath,
        profile.canonicalFields.messagePath
    };

    auto addTopLevelPath =
        [&fields](const QString &path) {
            const QString trimmedPath =
                path.trimmed();

            if (trimmedPath.isEmpty()) {
                return;
            }

            if (!trimmedPath.contains(
                    QLatin1Char('.')
                    )) {
                fields.insert(trimmedPath);
            }
        };

    for (const QString &path
         : canonicalPaths) {
        addTopLevelPath(path);
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        addTopLevelPath(
            mapping.sourcePath
            );
    }

    return fields;
}

std::optional<RecordSeverity> parseProfileSeverity(
    const QString &value,
    const ImportProfile &profile
    )
{
    const QString normalized =
        value.trimmed().toCaseFolded();

    for (auto iterator =
         profile.severityAliases.constBegin();
         iterator !=
         profile.severityAliases.constEnd();
         ++iterator) {
        if (iterator.key()
                .trimmed()
                .toCaseFolded()
            == normalized) {
            return iterator.value();
        }
    }

    return parseRecordSeverity(value);
}

std::optional<QDateTime> parseProfileTimestamp(
    const QString &value,
    const ImportProfile &profile
    )
{
    const QString normalized =
        value.trimmed();

    if (normalized.isEmpty()) {
        return std::nullopt;
    }

    for (const TimestampRule &rule
         : profile.timestampRules) {
        switch (rule.type) {
        case TimestampRuleType::Iso8601: {
            const auto timestamp =
                parseRecordTimestamp(
                    normalized
                    );

            if (timestamp.has_value()) {
                return timestamp;
            }

            break;
        }

        case TimestampRuleType::QtFormat: {
            const QDateTime timestamp =
                QDateTime::fromString(
                    normalized,
                    rule.format
                    );

            if (timestamp.isValid()) {
                return timestamp;
            }

            break;
        }
        }
    }

    return std::nullopt;
}

QHash<QString, QVariant> readCustomAttributes(
    const QJsonObject &object,
    const ImportProfile &profile
    )
{
    QHash<QString, QVariant> attributes;

    if (profile.preserveUnmappedFields) {
        const QSet<QString> mappedFields =
            mappedTopLevelFields(profile);

        for (auto iterator =
             object.constBegin();
             iterator != object.constEnd();
             ++iterator) {
            if (mappedFields.contains(
                    iterator.key()
                    )) {
                continue;
            }

            attributes.insert(
                iterator.key(),
                iterator.value().toVariant()
                );
        }
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const QJsonValue value =
            readJsonPath(
                object,
                mapping.sourcePath
                );

        if (value.isUndefined()) {
            continue;
        }

        attributes.insert(
            mapping.name,
            value.toVariant()
            );
    }

    return attributes;
}

InvestigationRecord createInvestigationRecord(
    const QJsonObject &object,
    const QString &rawSource,
    const RecordSourceMetadata &source,
    const ImportProfile &profile,
    ImportResult &result
    )
{
    InvestigationRecord record;

    record.rawSource = rawSource;
    record.source = source;

    record.recordId =
        createStableRecordIdentity(
            source,
            rawSource
            );

    const auto timestampText =
        readOptionalString(
            object,
            profile.canonicalFields.timestampPath
            );

    if (timestampText.has_value()) {
        record.timestamp =
            parseProfileTimestamp(
                *timestampText,
                profile
                );

        if (!record.timestamp.has_value()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "INVALID_TIMESTAMP"
                    ),
                QStringLiteral(
                    "The timestamp value could not be parsed."
                    ),
                ImportDiagnosticSeverity::Warning,
                source
                );
        }
    }

    const auto severityText =
        readOptionalString(
            object,
            profile.canonicalFields.severityPath
            );

    if (severityText.has_value()) {
        record.severity =
            parseProfileSeverity(
                *severityText,
                profile
                );

        if (!record.severity.has_value()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "UNMAPPED_SEVERITY"
                    ),
                QStringLiteral(
                    "The severity value could not be mapped."
                    ),
                ImportDiagnosticSeverity::Warning,
                source
                );
        }
    }

    record.subsystem =
        readOptionalString(
            object,
            profile.canonicalFields.subsystemPath
            );

    record.eventCode =
        readOptionalString(
            object,
            profile.canonicalFields.eventCodePath
            );

    record.entityId =
        readOptionalString(
            object,
            profile.canonicalFields.entityIdPath
            );

    record.message =
        readOptionalString(
            object,
            profile.canonicalFields.messagePath
            );

    record.customAttributes =
        readCustomAttributes(
            object,
            profile
            );

    return record;
}
}

JsonLinesImporter::JsonLinesImporter(
    JsonLinesImportConfig config
    )
{
    profile.canonicalFields =
        std::move(config);
}

JsonLinesImporter::JsonLinesImporter(
    ImportProfile profile
    )
    : profile(std::move(profile))
{
}

QString JsonLinesImporter::id() const
{
    return QStringLiteral("json-lines");
}

QString JsonLinesImporter::displayName() const
{
    return QStringLiteral("JSON Lines");
}

ImportResult JsonLinesImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        const QString rawSource =
            lines.at(index);

        const QString trimmed =
            rawSource.trimmed();

        if (trimmed.isEmpty()) {
            continue;
        }

        ++result.processedRecordCount;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                index + 1
                );

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                trimmed.toUtf8(),
                &parseError
                );

        if (parseError.error !=
            QJsonParseError::NoError) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "MALFORMED_JSON"
                    ),
                QStringLiteral(
                    "The source record is not valid JSON: %1"
                    ).arg(
                        parseError.errorString()
                        ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        if (!document.isObject()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "JSON_VALUE_NOT_OBJECT"
                    ),
                QStringLiteral(
                    "The JSON source record must be an object."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        result.records.append(
            createInvestigationRecord(
                document.object(),
                rawSource,
                source,
                profile,
                result
                )
            );
    }

    return result;
}

ImportResult JsonLinesImporter::importFile(
    const QString &filePath
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text
            )) {
        ImportResult result;

        const RecordSourceMetadata source =
            createSourceMetadata(
                filePath,
                0
                );

        appendDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The source file could not be opened: %1"
                ).arg(file.errorString()),
            ImportDiagnosticSeverity::Error,
            source
            );

        return result;
    }

    QTextStream stream(&file);
    QStringList lines;

    while (!stream.atEnd()) {
        lines.append(stream.readLine());
    }

    return importLines(
        lines,
        filePath
        );
}