#include "IisW3cImporter.h"

#include <optional>
#include <utility>

#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>

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

    source.sourcePath =
        sourcePath;

    source.recordNumber =
        recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(sourcePath)
                .fileName();
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

QString normalizeLine(
    const QString &line
    )
{
    QString normalized =
        line;

    if (!normalized.isEmpty()
        && normalized.front()
               == QChar(0xfeff)) {
        normalized.remove(
            0,
            1
            );
    }

    return normalized;
}

QStringList splitWhitespace(
    const QString &text
    )
{
    static const QRegularExpression
        whitespacePattern(
            QStringLiteral(
                R"(\s+)"
                )
            );

    return text.split(
        whitespacePattern,
        Qt::SkipEmptyParts
        );
}

bool isFieldsDirective(
    const QString &trimmedLine
    )
{
    return trimmedLine.startsWith(
        QStringLiteral(
            "#Fields:"
            )
        );
}

QStringList parseFieldsDirective(
    const QString &trimmedLine
    )
{
    const QString prefix =
        QStringLiteral(
            "#Fields:"
            );

    const QString fieldText =
        trimmedLine
            .mid(prefix.size())
            .trimmed();

    if (fieldText.isEmpty()) {
        return {};
    }

    return splitWhitespace(
        fieldText
        );
}

bool isDataRecordLine(
    const QString &line
    )
{
    const QString trimmed =
        normalizeLine(line)
            .trimmed();

    return !trimmed.isEmpty()
           && !trimmed.startsWith(
               QLatin1Char('#')
               );
}

void insertSourceFields(
    const QStringList &fieldNames,
    const QStringList &values,
    QJsonObject &object
    )
{
    for (qsizetype index = 0;
         index < fieldNames.size();
         ++index) {
        const QString value =
            values.at(index);

        /*
         * W3C uses '-' when a field does not
         * contain a value. Treat that as
         * absent rather than preserving '-'
         * as real source data.
         */
        if (value
            == QStringLiteral("-")) {
            continue;
        }

        object.insert(
            fieldNames.at(index),
            value
            );
    }
}

void insertGeneratedTimestamp(
    QJsonObject &object
    )
{
    const QString date =
        object.value(
                  QStringLiteral(
                      "date"
                      )
                  )
            .toString();

    const QString time =
        object.value(
                  QStringLiteral(
                      "time"
                      )
                  )
            .toString();

    if (date.isEmpty()
        || time.isEmpty()) {
        return;
    }

    /*
     * IIS W3C date/time values are UTC.
     * Producing ISO 8601 here lets the
     * existing timestamp mapper perform
     * validation and canonical conversion.
     */
    object.insert(
        QStringLiteral(
            "timestamp"
            ),
        QStringLiteral(
            "%1T%2Z"
            )
            .arg(
                date,
                time
                )
        );
}

void insertGeneratedMessage(
    QJsonObject &object
    )
{
    const QString method =
        object.value(
                  QStringLiteral(
                      "cs-method"
                      )
                  )
            .toString();

    const QString uriStem =
        object.value(
                  QStringLiteral(
                      "cs-uri-stem"
                      )
                  )
            .toString();

    const QString uriQuery =
        object.value(
                  QStringLiteral(
                      "cs-uri-query"
                      )
                  )
            .toString();

    QString target =
        uriStem;

    if (!uriQuery.isEmpty()) {
        if (!target.isEmpty()) {
            target +=
                QLatin1Char('?');
        }

        target +=
            uriQuery;
    }

    QStringList messageParts;

    if (!method.isEmpty()) {
        messageParts.append(
            method
            );
    }

    if (!target.isEmpty()) {
        messageParts.append(
            target
            );
    }

    if (messageParts.isEmpty()) {
        return;
    }

    object.insert(
        QStringLiteral(
            "message"
            ),
        messageParts.join(
            QLatin1Char(' ')
            )
        );
}

void processIisLine(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    QStringList &activeFields,
    const ImportProfile &profile,
    ImportResult &result
    )
{
    const QString normalized =
        normalizeLine(
            rawSource
            );

    const QString trimmed =
        normalized.trimmed();

    if (trimmed.isEmpty()) {
        return;
    }

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
            );

    /*
     * W3C directives and comments do not
     * represent imported records.
     */
    if (trimmed.startsWith(
            QLatin1Char('#')
            )) {
        if (!isFieldsDirective(
                trimmed
                )) {
            return;
        }

        QStringList fields =
            parseFieldsDirective(
                trimmed
                );

        if (fields.isEmpty()) {
            activeFields.clear();

            appendDiagnostic(
                result,
                QStringLiteral(
                    "IIS_W3C_FIELDS_EMPTY"
                    ),
                QStringLiteral(
                    "The #Fields directive does not "
                    "define any source fields."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            return;
        }

        activeFields =
            std::move(fields);

        return;
    }

    ++result.processedRecordCount;

    if (activeFields.isEmpty()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "IIS_W3C_FIELDS_REQUIRED"
                ),
            QStringLiteral(
                "The record appears before a valid "
                "#Fields directive."
                ),
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    const QStringList values =
        splitWhitespace(
            trimmed
            );

    if (values.size()
        != activeFields.size()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "IIS_W3C_FIELD_COUNT_MISMATCH"
                ),
            QStringLiteral(
                "The record contains %1 field value(s), "
                "but the active #Fields directive defines "
                "%2 field(s)."
                )
                .arg(
                    values.size()
                    )
                .arg(
                    activeFields.size()
                    ),
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    QJsonObject sourceValues;

    insertSourceFields(
        activeFields,
        values,
        sourceValues
        );

    insertGeneratedTimestamp(
        sourceValues
        );

    insertGeneratedMessage(
        sourceValues
        );

    /*
     * date and time have been consumed into the
     * generated canonical timestamp. The raw source
     * still preserves the original values, so keeping
     * them as custom attributes only creates duplicate
     * investigation columns.
     */
    sourceValues.remove(
        QStringLiteral("date")
        );

    sourceValues.remove(
        QStringLiteral("time")
        );

    result.records.append(
        JsonObjectRecordMapper::mapRecord(
            sourceValues,
            rawSource,
            source,
            profile,
            result
            )
        );
}
}

IisW3cImporter::IisW3cImporter(
    ImportProfile profile
    )
    : profile(std::move(profile))
{
    this->profile.importerId =
        QStringLiteral(
            "iis-w3c"
            );
}

QString IisW3cImporter::id() const
{
    return QStringLiteral(
        "iis-w3c"
        );
}

QString IisW3cImporter::displayName() const
{
    return QStringLiteral(
        "IIS W3C Extended Log"
        );
}

ImportResult IisW3cImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    QStringList activeFields;

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        processIisLine(
            lines.at(index),
            sourcePath,
            index + 1,
            activeFields,
            profile,
            result
            );
    }

    return result;
}

ImportResult IisW3cImporter::importFile(
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

    constexpr qint64 progressReportByteInterval =
        256 * 1024;

    ImportResult result;

    QStringList activeFields;

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
            QString::fromUtf8(
                lineBytes
                );

        if (rawSource.endsWith('\n')) {
            rawSource.chop(1);
        }

        if (rawSource.endsWith('\r')) {
            rawSource.chop(1);
        }

        const bool dataRecord =
            isDataRecordLine(
                rawSource
                );

        if (maxProcessedRecords > 0
            && dataRecord
            && result.processedRecordCount
                   >= maxProcessedRecords) {
            result.sourceTruncated = true;
            break;
        }

        processIisLine(
            rawSource,
            filePath,
            physicalLineNumber,
            activeFields,
            profile,
            result
            );

        const qint64 bytesProcessed =
            file.pos();

        if (bytesProcessed
                - lastReportedBytes
            >= progressReportByteInterval) {
            executionContext.report({
                bytesProcessed,
                totalBytes,
                result.processedRecordCount
            });

            lastReportedBytes =
                bytesProcessed;
        }
    }

    executionContext.report({
        file.pos(),
        totalBytes,
        result.processedRecordCount
    });

    return result;
}