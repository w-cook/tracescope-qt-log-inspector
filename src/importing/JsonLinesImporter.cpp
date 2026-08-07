#include "JsonLinesImporter.h"

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
#include "ImportDiagnostic.h"

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
        record.timestamp = parseRecordTimestamp(
            *timestampText
            );

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
        record.severity = parseRecordSeverity(
            *severityText
            );

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

    record.customAttributes =
        readCustomAttributes(object);

    return record;
}
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
        const QString rawSource = lines.at(index);
        const QString trimmed = rawSource.trimmed();

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
                result,
                QStringLiteral("JSON_VALUE_NOT_OBJECT"),
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
            QStringLiteral("FILE_OPEN_FAILED"),
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