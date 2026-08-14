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
#include "JsonObjectRecordMapper.h"

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

    result.diagnostics.append(diagnostic);
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
            JsonObjectRecordMapper::mapRecord(
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
    const QString &filePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
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