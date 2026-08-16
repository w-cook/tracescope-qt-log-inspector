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
#include <QVariant>
#include <QByteArray>

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
        processLine(
            lines.at(index),
            sourcePath,
            index + 1,
            result
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

    if (!file.open(QIODevice::ReadOnly)) {
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

    constexpr qint64 progressReportByteInterval =
        256 * 1024;

    ImportResult result;

    const qint64 totalBytes =
        file.size();

    qint64 physicalLineNumber = 0;
    qint64 lastReportedBytes = 0;

    executionContext.report({
        0,
        totalBytes,
        0
    });

    while (!file.atEnd()) {
        if (executionContext
                .cancellationRequested()) {
            result.cancelled = true;
            break;
        }

        QByteArray lineBytes =
            file.readLine();

        ++physicalLineNumber;

        QString rawSource =
            QString::fromUtf8(lineBytes);

        if (rawSource.endsWith('\n')) {
            rawSource.chop(1);
        }

        if (rawSource.endsWith('\r')) {
            rawSource.chop(1);
        }

        const bool hasRecord =
            !rawSource.trimmed().isEmpty();

        if (maxProcessedRecords > 0
            && result.processedRecordCount >=
                   maxProcessedRecords) {
            if (hasRecord) {
                result.sourceTruncated = true;
                break;
            }
        } else {
            processLine(
                rawSource,
                filePath,
                physicalLineNumber,
                result
                );
        }

        const qint64 bytesProcessed =
            file.pos();

        if (bytesProcessed -
                lastReportedBytes >=
            progressReportByteInterval) {
            executionContext.report({
                bytesProcessed,
                totalBytes,
                result.processedRecordCount
            });

            lastReportedBytes =
                bytesProcessed;
        }
    }

    const qint64 finalBytesProcessed =
        file.pos();

    executionContext.report({
        finalBytesProcessed,
        totalBytes,
        result.processedRecordCount
    });

    return result;
}

void JsonLinesImporter::processLine(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    ImportResult &result
    ) const
{
    const QString trimmed =
        rawSource.trimmed();

    if (trimmed.isEmpty()) {
        return;
    }

    ++result.processedRecordCount;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
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

        return;
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

        return;
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