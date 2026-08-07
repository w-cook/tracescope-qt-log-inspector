#include "JsonLinesImporter.h"

#include <optional>
#include <utility>

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
    const JsonLinesImportConfig &config
    )
{
    QSet<QString> fields;

    const QStringList paths {
        config.timestampPath,
        config.severityPath,
        config.subsystemPath,
        config.eventCodePath,
        config.entityIdPath,
        config.messagePath
    };

    for (const QString &path : paths) {
        const QString trimmedPath = path.trimmed();

        if (trimmedPath.isEmpty()) {
            continue;
        }

        if (!trimmedPath.contains(
                QLatin1Char('.')
                )) {
            fields.insert(trimmedPath);
        }
    }

    return fields;
}

QHash<QString, QVariant> readCustomAttributes(
    const QJsonObject &object,
    const JsonLinesImportConfig &config
    )
{
    QHash<QString, QVariant> attributes;

    const QSet<QString> mappedFields =
        mappedTopLevelFields(config);

    for (auto iterator = object.constBegin();
         iterator != object.constEnd();
         ++iterator) {
        if (mappedFields.contains(iterator.key())) {
            continue;
        }

        attributes.insert(
            iterator.key(),
            iterator.value().toVariant()
            );
    }

    return attributes;
}

InvestigationRecord createInvestigationRecord(
    const QJsonObject &object,
    const QString &rawSource,
    const RecordSourceMetadata &source,
    const JsonLinesImportConfig &config,
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
            config.timestampPath
            );

    if (timestampText.has_value()) {
        record.timestamp =
            parseRecordTimestamp(
                *timestampText
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
            config.severityPath
            );

    if (severityText.has_value()) {
        record.severity =
            parseRecordSeverity(
                *severityText
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
            config.subsystemPath
            );

    record.eventCode =
        readOptionalString(
            object,
            config.eventCodePath
            );

    record.entityId =
        readOptionalString(
            object,
            config.entityIdPath
            );

    record.message =
        readOptionalString(
            object,
            config.messagePath
            );

    record.customAttributes =
        readCustomAttributes(
            object,
            config
            );

    return record;
}
}

JsonLinesImporter::JsonLinesImporter(
    JsonLinesImportConfig config
    )
    : config(std::move(config))
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
                config,
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