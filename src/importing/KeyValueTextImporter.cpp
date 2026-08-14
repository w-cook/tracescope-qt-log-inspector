#include "KeyValueTextImporter.h"

#include <optional>
#include <utility>

#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QStringList>
#include <QTextStream>

#include "ImportDiagnostic.h"
#include "JsonObjectRecordMapper.h"

namespace
{
struct ParsedKeyValueLine
{
    bool success = false;

    QJsonObject values;

    QString errorMessage;

    QStringList duplicateKeys;
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

bool isWhitespace(
    QChar character
    )
{
    return character.isSpace();
}

ParsedKeyValueLine parseLine(
    const QString &line
    )
{
    ParsedKeyValueLine result;

    qsizetype position = 0;

    const auto skipWhitespace =
        [&line, &position]()
    {
        while (position < line.size()
               && isWhitespace(
                   line.at(position)
                   )) {
            ++position;
        }
    };

    skipWhitespace();

    while (position < line.size()) {
        const qsizetype keyStart =
            position;

        while (position < line.size()
               && line.at(position)
                      != QLatin1Char('=')
               && !isWhitespace(
                   line.at(position)
                   )) {
            ++position;
        }

        const QString key =
            line.mid(
                keyStart,
                position - keyStart
                );

        if (key.isEmpty()) {
            result.errorMessage =
                QStringLiteral(
                    "Expected a key before '='."
                    );

            return result;
        }

        if (position >= line.size()
            || line.at(position)
                   != QLatin1Char('=')) {
            result.errorMessage =
                QStringLiteral(
                    "Expected '=' after key '%1'."
                    )
                    .arg(key);

            return result;
        }

        ++position;

        QString value;

        if (position < line.size()
            && line.at(position)
                   == QLatin1Char('"')) {
            ++position;

            bool closingQuoteFound = false;

            while (position < line.size()) {
                const QChar character =
                    line.at(position);

                if (character
                    == QLatin1Char('"')) {
                    ++position;

                    closingQuoteFound = true;
                    break;
                }

                if (character
                    == QLatin1Char('\\')) {
                    ++position;

                    if (position >=
                        line.size()) {
                        result.errorMessage =
                            QStringLiteral(
                                "Quoted value for key '%1' "
                                "ends with an incomplete escape sequence."
                                )
                                .arg(key);

                        return result;
                    }

                    const QChar escaped =
                        line.at(position);

                    switch (escaped.unicode()) {
                    case '"':
                        value.append(
                            QLatin1Char('"')
                            );
                        break;

                    case '\\':
                        value.append(
                            QLatin1Char('\\')
                            );
                        break;

                    case 'n':
                        value.append(
                            QLatin1Char('\n')
                            );
                        break;

                    case 'r':
                        value.append(
                            QLatin1Char('\r')
                            );
                        break;

                    case 't':
                        value.append(
                            QLatin1Char('\t')
                            );
                        break;

                    default:
                        /*
                         * Preserve unknown escape
                         * sequences literally instead
                         * of silently discarding the
                         * escape marker.
                         */
                        value.append(
                            QLatin1Char('\\')
                            );

                        value.append(
                            escaped
                            );
                        break;
                    }

                    ++position;
                    continue;
                }

                value.append(
                    character
                    );

                ++position;
            }

            if (!closingQuoteFound) {
                result.errorMessage =
                    QStringLiteral(
                        "Quoted value for key '%1' "
                        "is not terminated."
                        )
                        .arg(key);

                return result;
            }

            if (position < line.size()
                && !isWhitespace(
                    line.at(position)
                    )) {
                result.errorMessage =
                    QStringLiteral(
                        "Unexpected characters follow "
                        "the quoted value for key '%1'."
                        )
                        .arg(key);

                return result;
            }
        } else {
            const qsizetype valueStart =
                position;

            while (position < line.size()
                   && !isWhitespace(
                       line.at(position)
                       )) {
                ++position;
            }

            value =
                line.mid(
                    valueStart,
                    position - valueStart
                    );
        }

        if (result.values.contains(key)) {
            if (!result.duplicateKeys
                     .contains(key)) {
                result.duplicateKeys.append(
                    key
                    );
            }
        }

        /*
         * Last value wins for duplicate keys.
         * A warning is emitted by the importer,
         * but the record remains usable.
         */
        result.values.insert(
            key,
            value
            );

        skipWhitespace();
    }

    if (result.values.isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The source record does not contain "
                "any key-value assignments."
                );

        return result;
    }

    result.success = true;

    return result;
}

void processKeyValueRecord(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    const ImportProfile &profile,
    ImportResult &result
    )
{
    if (rawSource.trimmed().isEmpty()) {
        return;
    }

    ++result.processedRecordCount;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
            );

    const ParsedKeyValueLine parsed =
        parseLine(
            rawSource
            );

    if (!parsed.success) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "KEY_VALUE_RECORD_MALFORMED"
                ),
            parsed.errorMessage,
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    for (const QString &duplicateKey
         : parsed.duplicateKeys) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "KEY_VALUE_DUPLICATE_KEY"
                ),
            QStringLiteral(
                "Key '%1' occurs more than once; "
                "the last value was used."
                )
                .arg(duplicateKey),
            ImportDiagnosticSeverity::Warning,
            source
            );
    }

    result.records.append(
        JsonObjectRecordMapper::mapRecord(
            parsed.values,
            rawSource,
            source,
            profile,
            result
            )
        );
}
}

KeyValueTextImporter::KeyValueTextImporter(
    ImportProfile profile
    )
    : profile(std::move(profile))
{
    this->profile.importerId =
        QStringLiteral(
            "key-value"
            );
}

QString KeyValueTextImporter::id() const
{
    return QStringLiteral(
        "key-value"
        );
}

QString KeyValueTextImporter::displayName() const
{
    return QStringLiteral(
        "Key-Value / logfmt"
        );
}

ImportResult KeyValueTextImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        processKeyValueRecord(
            lines.at(index),
            sourcePath,
            index + 1,
            profile,
            result
            );
    }

    return result;
}

ImportResult KeyValueTextImporter::importFile(
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

        if (!rawSource.trimmed().isEmpty()) {
            if (maxProcessedRecords > 0
                && result.processedRecordCount >=
                       maxProcessedRecords) {
                result.sourceTruncated = true;
                break;
            }

            processKeyValueRecord(
                rawSource,
                filePath,
                physicalLineNumber,
                profile,
                result
                );
        }

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