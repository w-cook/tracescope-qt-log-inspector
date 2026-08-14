#include "RegexTextImporter.h"

#include <optional>
#include <utility>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
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

    result.diagnostics.append(
        diagnostic
        );
}

std::optional<QString> readOptionalString(
    const QHash<QString, QString> &values,
    const QString &path
    )
{
    const QString normalizedPath =
        path.trimmed();

    if (normalizedPath.isEmpty()) {
        return std::nullopt;
    }

    const auto iterator =
        values.constFind(
            normalizedPath
            );

    if (iterator == values.constEnd()) {
        return std::nullopt;
    }

    const QString value =
        iterator.value().trimmed();

    if (value.isEmpty()) {
        return std::nullopt;
    }

    return value;
}

std::optional<RecordSeverity>
parseProfileSeverity(
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

    return parseRecordSeverity(
        value
        );
}

std::optional<QDateTime>
parseProfileTimestamp(
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

QSet<QString> mappedSourceFields(
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

    for (const QString &path
         : canonicalPaths) {
        const QString normalized =
            path.trimmed();

        if (!normalized.isEmpty()) {
            fields.insert(normalized);
        }
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const QString normalized =
            mapping.sourcePath.trimmed();

        if (!normalized.isEmpty()) {
            fields.insert(normalized);
        }
    }

    return fields;
}

QHash<QString, QVariant> readCustomAttributes(
    const QHash<QString, QString> &values,
    const ImportProfile &profile
    )
{
    QHash<QString, QVariant> attributes;

    if (profile.preserveUnmappedFields) {
        const QSet<QString> mappedFields =
            mappedSourceFields(
                profile
                );

        for (auto iterator =
             values.constBegin();
             iterator != values.constEnd();
             ++iterator) {
            if (mappedFields.contains(
                    iterator.key()
                    )) {
                continue;
            }

            attributes.insert(
                iterator.key(),
                iterator.value()
                );
        }
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const auto iterator =
            values.constFind(
                mapping.sourcePath.trimmed()
                );

        if (iterator == values.constEnd()) {
            continue;
        }

        attributes.insert(
            mapping.name,
            iterator.value()
            );
    }

    return attributes;
}

InvestigationRecord createRecord(
    const QHash<QString, QString> &values,
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
            values,
            profile.canonicalFields
                .timestampPath
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
            values,
            profile.canonicalFields
                .severityPath
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
            values,
            profile.canonicalFields
                .subsystemPath
            );

    record.eventCode =
        readOptionalString(
            values,
            profile.canonicalFields
                .eventCodePath
            );

    record.entityId =
        readOptionalString(
            values,
            profile.canonicalFields
                .entityIdPath
            );

    record.message =
        readOptionalString(
            values,
            profile.canonicalFields
                .messagePath
            );

    record.customAttributes =
        readCustomAttributes(
            values,
            profile
            );

    return record;
}
}

RegexTextImporter::RegexTextImporter(
    ImportProfile profile
    )
    : profile(std::move(profile))
{
    this->profile.importerId =
        QStringLiteral("regex-text");
}

QString RegexTextImporter::id() const
{
    return QStringLiteral(
        "regex-text"
        );
}

QString RegexTextImporter::displayName() const
{
    return QStringLiteral(
        "Regex Plain Text"
        );
}

ImportResult RegexTextImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    const QRegularExpression expression(
        profile.regexPattern
        );

    if (!expression.isValid()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "INVALID_REGEX_PATTERN"
                ),
            QStringLiteral(
                "The configured regular expression "
                "is invalid: %1"
                )
                .arg(
                    expression.errorString()
                    ),
            ImportDiagnosticSeverity::Error
            );

        return result;
    }

    const QStringList captureNames =
        expression.namedCaptureGroups();

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        const QString rawSource =
            lines.at(index);

        if (rawSource.trimmed().isEmpty()) {
            continue;
        }

        ++result.processedRecordCount;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                index + 1
                );

        const QRegularExpressionMatch match =
            expression.match(
                rawSource
                );

        const bool completeMatch =
            match.hasMatch()
            && match.capturedStart(0) == 0
            && match.capturedLength(0)
                   == rawSource.size();

        if (!completeMatch) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "REGEX_RECORD_NO_MATCH"
                    ),
                QStringLiteral(
                    "The source record does not match "
                    "the configured regular expression."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        QHash<QString, QString> values;

        for (qsizetype captureIndex = 1;
             captureIndex <
             captureNames.size();
             ++captureIndex) {
            const QString captureName =
                captureNames.at(
                    captureIndex
                    );

            if (captureName.isEmpty()) {
                continue;
            }

            /*
             * An optional capture group that did not
             * participate in this match is absent from
             * the source-field set. A capture that
             * participated but captured an empty string
             * remains a real field.
             */
            if (match.capturedStart(
                    captureIndex
                    ) < 0) {
                continue;
            }

            values.insert(
                captureName,
                match.captured(
                    captureIndex
                    )
                );
        }

        result.records.append(
            createRecord(
                values,
                rawSource,
                source,
                profile,
                result
                )
            );
    }

    return result;
}

ImportResult RegexTextImporter::importFile(
    const QString &filePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )) {
        ImportResult result;

        appendDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The source file could not be opened: %1"
                )
                .arg(
                    file.errorString()
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                filePath,
                0
                )
            );

        return result;
    }

    QTextStream stream(&file);

    QStringList lines;

    qint64 processedRecords = 0;
    bool sourceTruncated = false;

    while (!stream.atEnd()) {
        const QString line =
            stream.readLine();

        if (maxProcessedRecords > 0
            && processedRecords >=
                   maxProcessedRecords) {
            if (!line.trimmed().isEmpty()) {
                sourceTruncated = true;
                break;
            }

            continue;
        }

        lines.append(line);

        if (!line.trimmed().isEmpty()) {
            ++processedRecords;
        }
    }

    ImportResult result =
        importLines(
            lines,
            filePath
            );

    result.sourceTruncated =
        sourceTruncated;

    return result;
}