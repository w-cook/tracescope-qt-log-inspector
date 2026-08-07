#include "JsonLineLogParser.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>
#include <QTextStream>

#include "../domain/RecordIdentity.h"
#include "../domain/RecordSeverity.h"
#include "../domain/RecordTimestamp.h"
#include "../importing/ImportDiagnostic.h"

namespace
{
const QSet<QString> canonicalFieldNames {
    QStringLiteral("timestamp"),
    QStringLiteral("level"),
    QStringLiteral("subsystem"),
    QStringLiteral("eventCode"),
    QStringLiteral("message"),
    QStringLiteral("entityId")
};

RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber
    )
{
    RecordSourceMetadata source;
    source.sourcePath = sourcePath;
    source.recordNumber = recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName = QFileInfo(sourcePath).fileName();
    }

    return source;
}

std::optional<QString> readOptionalString(
    const QJsonObject &object,
    const QString &fieldName
    )
{
    const QJsonValue value = object.value(fieldName);

    if (!value.isString()) {
        return std::nullopt;
    }

    const QString text = value.toString().trimmed();

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
    const std::optional<RecordSourceMetadata> &source = std::nullopt
    )
{
    ImportDiagnostic diagnostic;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.severity = severity;
    diagnostic.source = source;

    result.diagnostics.append(diagnostic);
}

QHash<QString, QVariant> readCustomAttributes(
    const QJsonObject &object
    )
{
    QHash<QString, QVariant> attributes;

    for (auto iterator = object.constBegin();
         iterator != object.constEnd();
         ++iterator) {
        if (canonicalFieldNames.contains(iterator.key())) {
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
    ImportResult &result
    )
{
    InvestigationRecord record;

    record.rawSource = rawSource;
    record.source = source;
    record.recordId = createStableRecordIdentity(
        source,
        rawSource
        );

    const auto timestampText = readOptionalString(
        object,
        QStringLiteral("timestamp")
        );

    if (timestampText.has_value()) {
        record.timestamp = parseRecordTimestamp(*timestampText);

        if (!record.timestamp.has_value()) {
            appendDiagnostic(
                result,
                QStringLiteral("INVALID_TIMESTAMP"),
                QStringLiteral(
                    "The timestamp value could not be parsed."
                    ),
                ImportDiagnosticSeverity::Warning,
                source
                );
        }
    }

    const auto severityText = readOptionalString(
        object,
        QStringLiteral("level")
        );

    if (severityText.has_value()) {
        record.severity = parseRecordSeverity(*severityText);

        if (!record.severity.has_value()) {
            appendDiagnostic(
                result,
                QStringLiteral("UNMAPPED_SEVERITY"),
                QStringLiteral(
                    "The severity value could not be mapped."
                    ),
                ImportDiagnosticSeverity::Warning,
                source
                );
        }
    }

    record.subsystem = readOptionalString(
        object,
        QStringLiteral("subsystem")
        );

    record.eventCode = readOptionalString(
        object,
        QStringLiteral("eventCode")
        );

    record.message = readOptionalString(
        object,
        QStringLiteral("message")
        );

    record.entityId = readOptionalString(
        object,
        QStringLiteral("entityId")
        );

    record.customAttributes = readCustomAttributes(object);

    return record;
}

TelemetryEvent createLegacyTelemetryEvent(
    const QJsonObject &object
    )
{
    TelemetryEvent event;

    event.timestamp = object.value(
                                QStringLiteral("timestamp")
                                ).toString();

    event.level = object.value(
                            QStringLiteral("level")
                            ).toString();

    event.subsystem = object.value(
                                QStringLiteral("subsystem")
                                ).toString();

    event.eventCode = object.value(
                                QStringLiteral("eventCode")
                                ).toString();

    event.message = object.value(
                              QStringLiteral("message")
                              ).toString();

    event.entityId = object.value(
                               QStringLiteral("entityId")
                               ).toString();

    return event;
}

struct JsonLineParseOutput
{
    ImportResult importResult;
    QVector<TelemetryEvent> legacyEvents;
};

JsonLineParseOutput parseLinesInternal(
    const QStringList &lines,
    const QString &sourcePath
    )
{
    JsonLineParseOutput output;

    for (qsizetype index = 0; index < lines.size(); ++index) {
        const QString rawSource = lines.at(index);
        const QString trimmed = rawSource.trimmed();

        if (trimmed.isEmpty()) {
            continue;
        }

        ++output.importResult.processedRecordCount;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                index + 1
                );

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(
            trimmed.toUtf8(),
            &parseError
            );

        if (parseError.error != QJsonParseError::NoError) {
            appendDiagnostic(
                output.importResult,
                QStringLiteral("MALFORMED_JSON"),
                QStringLiteral(
                    "The source record is not valid JSON: %1"
                    ).arg(parseError.errorString()),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        if (!document.isObject()) {
            appendDiagnostic(
                output.importResult,
                QStringLiteral("JSON_VALUE_NOT_OBJECT"),
                QStringLiteral(
                    "The JSON source record must be an object."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        const QJsonObject object = document.object();

        output.importResult.records.append(
            createInvestigationRecord(
                object,
                rawSource,
                source,
                output.importResult
                )
            );

        output.legacyEvents.append(
            createLegacyTelemetryEvent(object)
            );
    }

    return output;
}

JsonLineParseOutput parseFileInternal(
    const QString &filePath
    )
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        JsonLineParseOutput output;

        const RecordSourceMetadata source =
            createSourceMetadata(filePath, 0);

        appendDiagnostic(
            output.importResult,
            QStringLiteral("FILE_OPEN_FAILED"),
            QStringLiteral(
                "The source file could not be opened: %1"
                ).arg(file.errorString()),
            ImportDiagnosticSeverity::Error,
            source
            );

        return output;
    }

    QTextStream stream(&file);
    QStringList lines;

    while (!stream.atEnd()) {
        lines.append(stream.readLine());
    }

    return parseLinesInternal(lines, filePath);
}
}

ImportResult JsonLineLogParser::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    JsonLineParseOutput output = parseLinesInternal(
        lines,
        sourcePath
        );

    return output.importResult;
}

ImportResult JsonLineLogParser::importFile(
    const QString &filePath
    ) const
{
    JsonLineParseOutput output = parseFileInternal(filePath);

    return output.importResult;
}

QVector<TelemetryEvent> JsonLineLogParser::parseLines(
    const QStringList &lines
    ) const
{
    JsonLineParseOutput output = parseLinesInternal(
        lines,
        {}
        );

    return output.legacyEvents;
}

QVector<TelemetryEvent> JsonLineLogParser::parseFile(
    const QString &filePath
    ) const
{
    JsonLineParseOutput output = parseFileInternal(filePath);

    return output.legacyEvents;
}