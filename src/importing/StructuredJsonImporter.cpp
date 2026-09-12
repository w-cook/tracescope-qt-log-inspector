#include "StructuredJsonImporter.h"

#include <optional>
#include <utility>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>

#include "ImportDiagnostic.h"
#include "JsonObjectRecordMapper.h"
#include "StructuredJsonRecordStreamFramer.h"

namespace
{
RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber
    )
{
    RecordSourceMetadata source;

    source.sourcePath =
        sourcePath;

    source.recordNumber =
        recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(
                sourcePath
                ).fileName();
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

    diagnostic.code =
        code;

    diagnostic.message =
        message;

    diagnostic.severity =
        severity;

    diagnostic.source =
        source;

    result.diagnostics.append(
        diagnostic
        );
}

QJsonValue readJsonPath(
    const QJsonObject &object,
    const QString &path
    )
{
    const QString normalizedPath =
        path.trimmed();

    if (normalizedPath.isEmpty()) {
        return QJsonValue(
            QJsonValue::Undefined
            );
    }

    const QStringList segments =
        normalizedPath.split(
            QLatin1Char('.'),
            Qt::KeepEmptyParts
            );

    QJsonValue currentValue(
        object
        );

    for (const QString &segment
         : segments) {
        if (segment.isEmpty()
            || !currentValue.isObject()) {
            return QJsonValue(
                QJsonValue::Undefined
                );
        }

        currentValue =
            currentValue
                .toObject()
                .value(
                    segment
                    );

        if (currentValue.isUndefined()) {
            return currentValue;
        }
    }

    return currentValue;
}

QString compactJson(
    const QJsonObject &object
    )
{
    return QString::fromUtf8(
        QJsonDocument(
            object
            ).toJson(
                QJsonDocument::Compact
                )
        );
}

void processObject(
    const QJsonObject &object,
    qint64 recordNumber,
    const QString &sourcePath,
    const ImportProfile &profile,
    ImportResult &result
    )
{
    ++result.processedRecordCount;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
            );

    const QString rawSource =
        compactJson(
            object
            );

    result.records.append(
        JsonObjectRecordMapper::mapRecord(
            object,
            rawSource,
            source,
            profile,
            result
            )
        );
}

void processArray(
    const QJsonArray &array,
    const QString &sourcePath,
    const ImportProfile &profile,
    qint64 maxProcessedRecords,
    ImportResult &result
    )
{
    for (qsizetype index = 0;
         index < array.size();
         ++index) {
        if (maxProcessedRecords > 0
            && result.processedRecordCount
                   >= maxProcessedRecords) {
            result.sourceTruncated =
                true;

            break;
        }

        ++result.processedRecordCount;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                index + 1
                );

        const QJsonValue value =
            array.at(
                index
                );

        if (!value.isObject()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "STRUCTURED_JSON_RECORD_NOT_OBJECT"
                    ),
                QStringLiteral(
                    "The selected JSON record is "
                    "not an object."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        const QJsonObject object =
            value.toObject();

        const QString rawSource =
            compactJson(
                object
                );

        result.records.append(
            JsonObjectRecordMapper::mapRecord(
                object,
                rawSource,
                source,
                profile,
                result
                )
            );
    }
}

bool tryImportOpenRecordArray(
    const QByteArray &json,
    const QString &recordPath,
    const QString &sourcePath,
    const ImportProfile &profile,
    qint64 maxProcessedRecords,
    ImportResult &result
    )
{
    /*
     * A structured source that is actively being
     * written may deliberately have an unclosed
     * outer document and record array.
     *
     * Use the same incremental framing logic as
     * live following so initial import and later
     * live ingestion agree about which complete
     * records are currently available.
     */
    StructuredJsonRecordStreamFramer framer(
        recordPath
        );

    StructuredJsonRecordFrameResult
        frameResult =
        framer.appendBytes(
            json
            );

    /*
     * This fallback applies only to a successfully
     * located record array that is genuinely still
     * open.
     *
     * If the record array has already closed, then
     * a malformed outer document remains an ordinary
     * malformed JSON document and follows the normal
     * error path.
     */
    if (!frameResult.succeeded
        || !framer.recordArrayLocated()
        || framer.recordArrayClosed()) {
        return false;
    }

    qint64 recordNumber = 1;

    for (const QByteArray &recordBytes
         : std::as_const(frameResult.records)) {
        if (maxProcessedRecords > 0
            && result.processedRecordCount
                   >= maxProcessedRecords) {
            result.sourceTruncated =
                true;

            break;
        }

        QJsonParseError recordParseError;

        const QJsonDocument recordDocument =
            QJsonDocument::fromJson(
                recordBytes,
                &recordParseError
                );

        /*
         * The stream framer establishes physical
         * object boundaries but deliberately does
         * not act as the JSON semantic parser.
         *
         * Count a framed-but-malformed object as a
         * processed source record and continue with
         * later complete records.
         */
        if (recordParseError.error
                != QJsonParseError::NoError
            || !recordDocument.isObject()) {
            ++result.processedRecordCount;

            const RecordSourceMetadata source =
                createSourceMetadata(
                    sourcePath,
                    recordNumber
                    );

            appendDiagnostic(
                result,
                QStringLiteral(
                    "MALFORMED_STRUCTURED_JSON_RECORD"
                    ),
                QStringLiteral(
                    "A complete record in the open "
                    "structured JSON stream could "
                    "not be parsed."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            ++recordNumber;

            continue;
        }

        processObject(
            recordDocument.object(),
            recordNumber,
            sourcePath,
            profile,
            result
            );

        ++recordNumber;
    }

    /*
     * This is not an import failure. The outer
     * container is expected to remain open while
     * another application is actively producing
     * records.
     *
     * Preserve that fact as informational context
     * while still returning every complete record
     * currently available.
     */
    appendDiagnostic(
        result,
        QStringLiteral(
            "STRUCTURED_JSON_OPEN_CONTAINER"
            ),
        QStringLiteral(
            "The structured JSON record array is "
            "still open. Complete records available "
            "at the current end of the source were "
            "imported."
            ),
        ImportDiagnosticSeverity::Information,
        createSourceMetadata(
            sourcePath,
            0
            )
        );

    return true;
}
}

StructuredJsonImporter::
    StructuredJsonImporter(
        StructuredJsonImportConfig config,
        ImportProfile profile
        )
    : config(
          std::move(config)
          ),
    profile(
        std::move(profile)
        )
{
    this->profile.importerId =
        QStringLiteral(
            "structured-json"
            );
}

QString StructuredJsonImporter::
    id() const
{
    return QStringLiteral(
        "structured-json"
        );
}

QString StructuredJsonImporter::
    displayName() const
{
    return QStringLiteral(
        "Structured JSON"
        );
}

ImportResult StructuredJsonImporter::
    importContent(
        const QByteArray &json,
        const QString &sourcePath,
        qint64 maxProcessedRecords
        ) const
{
    ImportResult result;

    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json,
            &parseError
            );

    /*
     * Before Phase 15, any incomplete outer JSON
     * document was treated as unusable.
     *
     * Structured live sources intentionally remain
     * open while records are being produced, so try
     * the shared record-stream framing path before
     * reporting an ordinary malformed-document
     * error.
     */
    if (parseError.error
        != QJsonParseError::NoError) {
        if (tryImportOpenRecordArray(
                json,
                config.recordPath.trimmed(),
                sourcePath,
                profile,
                maxProcessedRecords,
                result
                )) {
            return result;
        }

        appendDiagnostic(
            result,
            QStringLiteral(
                "MALFORMED_JSON_DOCUMENT"
                ),
            QStringLiteral(
                "The JSON document could not be "
                "parsed: %1"
                )
                .arg(
                    parseError.errorString()
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    const QString recordPath =
        config.recordPath.trimmed();

    if (recordPath.isEmpty()) {
        if (document.isArray()) {
            processArray(
                document.array(),
                sourcePath,
                profile,
                maxProcessedRecords,
                result
                );

            return result;
        }

        if (document.isObject()) {
            if (maxProcessedRecords > 0
                && maxProcessedRecords < 1) {
                result.sourceTruncated =
                    true;

                return result;
            }

            processObject(
                document.object(),
                1,
                sourcePath,
                profile,
                result
                );

            return result;
        }

        appendDiagnostic(
            result,
            QStringLiteral(
                "STRUCTURED_JSON_ROOT_UNSUPPORTED"
                ),
            QStringLiteral(
                "The JSON document root must be "
                "an object or array."
                ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    if (!document.isObject()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "JSON_RECORD_PATH_REQUIRES_OBJECT_ROOT"
                ),
            QStringLiteral(
                "A configured JSON record path "
                "requires an object document root."
                ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    const QJsonValue selectedValue =
        readJsonPath(
            document.object(),
            recordPath
            );

    if (selectedValue.isUndefined()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "JSON_RECORD_PATH_NOT_FOUND"
                ),
            QStringLiteral(
                "The configured JSON record path "
                "'%1' was not found."
                )
                .arg(
                    recordPath
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    if (selectedValue.isArray()) {
        processArray(
            selectedValue.toArray(),
            sourcePath,
            profile,
            maxProcessedRecords,
            result
            );

        return result;
    }

    if (selectedValue.isObject()) {
        processObject(
            selectedValue.toObject(),
            1,
            sourcePath,
            profile,
            result
            );

        return result;
    }

    appendDiagnostic(
        result,
        QStringLiteral(
            "JSON_RECORD_PATH_NOT_CONTAINER"
            ),
        QStringLiteral(
            "The configured JSON record path '%1' "
            "must resolve to an object or array."
            )
            .arg(
                recordPath
                ),
        ImportDiagnosticSeverity::Error,
        createSourceMetadata(
            sourcePath,
            0
            )
        );

    return result;
}

ImportResult StructuredJsonImporter::
    importFile(
        const QString &filePath,
        qint64 maxProcessedRecords,
        const ImportExecutionContext &executionContext
        ) const
{
    Q_UNUSED(
        executionContext
        );

    QFile file(
        filePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        ImportResult result;

        appendDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The source file could not be "
                "opened: %1"
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

    return importContent(
        file.readAll(),
        filePath,
        maxProcessedRecords
        );
}