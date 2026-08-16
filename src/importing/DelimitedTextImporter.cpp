#include "DelimitedTextImporter.h"

#include <optional>
#include <utility>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QVariant>

#include "../domain/RecordIdentity.h"
#include "../domain/RecordSeverity.h"
#include "../domain/RecordTimestamp.h"
#include "ImportDiagnostic.h"

namespace
{
struct ParsedDelimitedRecord
{
    QStringList fields;
    bool valid = true;
    QString errorMessage;
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

    result.diagnostics.append(diagnostic);
}

ParsedDelimitedRecord parseDelimitedRecord(
    const QString &source,
    QChar delimiter
    )
{
    ParsedDelimitedRecord result;

    QString currentField;

    bool inQuotedField = false;
    bool quotedFieldClosed = false;

    for (qsizetype index = 0;
         index < source.size();
         ++index) {
        const QChar character =
            source.at(index);

        if (inQuotedField) {
            if (character == QLatin1Char('"')) {
                const bool hasEscapedQuote =
                    index + 1 < source.size()
                    && source.at(index + 1)
                           == QLatin1Char('"');

                if (hasEscapedQuote) {
                    currentField.append(
                        QLatin1Char('"')
                        );

                    ++index;
                } else {
                    inQuotedField = false;
                    quotedFieldClosed = true;
                }

                continue;
            }

            currentField.append(character);
            continue;
        }

        if (quotedFieldClosed) {
            if (character == delimiter) {
                result.fields.append(
                    currentField
                    );

                currentField.clear();
                quotedFieldClosed = false;

                continue;
            }

            result.valid = false;
            result.errorMessage =
                QStringLiteral(
                    "Unexpected content follows a closing quote."
                    );

            return result;
        }

        if (character == delimiter) {
            result.fields.append(
                currentField
                );

            currentField.clear();
            continue;
        }

        if (character == QLatin1Char('"')) {
            if (!currentField.isEmpty()) {
                result.valid = false;
                result.errorMessage =
                    QStringLiteral(
                        "A quoted field must begin with a quote."
                        );

                return result;
            }

            inQuotedField = true;
            continue;
        }

        currentField.append(character);
    }

    if (inQuotedField) {
        result.valid = false;
        result.errorMessage =
            QStringLiteral(
                "The record contains an unterminated quoted field."
                );

        return result;
    }

    result.fields.append(
        currentField
        );

    return result;
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
        values.constFind(normalizedPath);

    if (iterator == values.constEnd()) {
        return std::nullopt;
    }

    const QString text =
        iterator.value().trimmed();

    if (text.isEmpty()) {
        return std::nullopt;
    }

    return text;
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

    for (qsizetype index = 0;
         index < canonicalPaths.size();
         ++index) {
        const QString path =
            canonicalPaths.at(index).trimmed();

        if (!path.isEmpty()) {
            fields.insert(path);
        }
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const QString path =
            mapping.sourcePath.trimmed();

        if (!path.isEmpty()) {
            fields.insert(path);
        }
    }

    return fields;
}

QHash<QString, QVariant> readCustomAttributes(
    const QHash<QString, QString> &values,
    const QStringList &headers,
    const ImportProfile &profile
    )
{
    QHash<QString, QVariant> attributes;

    if (profile.preserveUnmappedFields) {
        const QSet<QString> mappedFields =
            mappedSourceFields(profile);

        for (qsizetype index = 0;
             index < headers.size();
             ++index) {
            const QString &header =
                headers.at(index);

            if (mappedFields.contains(header)) {
                continue;
            }

            attributes.insert(
                header,
                QVariant(values.value(header))
                );
        }
    }

    for (const CustomFieldMapping &mapping
         : profile.customFields) {
        const QString sourcePath =
            mapping.sourcePath.trimmed();

        const auto iterator =
            values.constFind(sourcePath);

        if (iterator == values.constEnd()) {
            continue;
        }

        attributes.insert(
            mapping.name,
            QVariant(iterator.value())
            );
    }

    return attributes;
}

InvestigationRecord createInvestigationRecord(
    const QHash<QString, QString> &values,
    const QStringList &headers,
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
            values,
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
            values,
            profile.canonicalFields.subsystemPath
            );

    record.eventCode =
        readOptionalString(
            values,
            profile.canonicalFields.eventCodePath
            );

    record.entityId =
        readOptionalString(
            values,
            profile.canonicalFields.entityIdPath
            );

    record.message =
        readOptionalString(
            values,
            profile.canonicalFields.messagePath
            );

    record.customAttributes =
        readCustomAttributes(
            values,
            headers,
            profile
            );

    return record;
}

struct ParsedDelimitedHeader
{
    QStringList headers;
    bool valid = false;
};

ParsedDelimitedHeader parseDelimitedHeader(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    QChar delimiter,
    ImportResult &result
    )
{
    ParsedDelimitedHeader header;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
            );

    const ParsedDelimitedRecord parsed =
        parseDelimitedRecord(
            rawSource,
            delimiter
            );

    if (!parsed.valid) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "MALFORMED_DELIMITED_HEADER"
                ),
            QStringLiteral(
                "The delimited header could not be parsed: %1"
                ).arg(parsed.errorMessage),
            ImportDiagnosticSeverity::Error,
            source
            );

        return header;
    }

    header.headers = parsed.fields;

    if (!header.headers.isEmpty()
        && !header.headers.first().isEmpty()
        && header.headers.first()
                   .at(0)
                   .unicode()
               == 0xFEFF) {
        header.headers[0].remove(0, 1);
    }

    QSet<QString> normalizedHeaders;
    bool headerIsValid = true;

    for (qsizetype index = 0;
         index < header.headers.size();
         ++index) {
        header.headers[index] =
            header.headers.at(index).trimmed();

        const QString &name =
            header.headers.at(index);

        if (name.isEmpty()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "EMPTY_DELIMITED_HEADER"
                    ),
                QStringLiteral(
                    "Delimited source headers cannot be empty."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            headerIsValid = false;
            continue;
        }

        const QString normalized =
            name.toCaseFolded();

        if (normalizedHeaders.contains(
                normalized
                )) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "DUPLICATE_DELIMITED_HEADER"
                    ),
                QStringLiteral(
                    "Delimited source header '%1' is duplicated."
                    ).arg(name),
                ImportDiagnosticSeverity::Error,
                source
                );

            headerIsValid = false;
            continue;
        }

        normalizedHeaders.insert(
            normalized
            );
    }

    header.valid = headerIsValid;

    return header;
}

void processDelimitedRecord(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    QChar delimiter,
    const QStringList &headers,
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

    const ParsedDelimitedRecord parsed =
        parseDelimitedRecord(
            rawSource,
            delimiter
            );

    if (!parsed.valid) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "MALFORMED_DELIMITED_RECORD"
                ),
            QStringLiteral(
                "The delimited source record could not be parsed: %1"
                ).arg(parsed.errorMessage),
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    if (parsed.fields.size()
        != headers.size()) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "DELIMITED_COLUMN_COUNT_MISMATCH"
                ),
            QStringLiteral(
                "The source record contains %1 columns, "
                "but the header defines %2."
                )
                .arg(parsed.fields.size())
                .arg(headers.size()),
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    QHash<QString, QString> values;

    for (qsizetype fieldIndex = 0;
         fieldIndex < headers.size();
         ++fieldIndex) {
        values.insert(
            headers.at(fieldIndex),
            parsed.fields.at(fieldIndex)
            );
    }

    result.records.append(
        createInvestigationRecord(
            values,
            headers,
            rawSource,
            source,
            profile,
            result
            )
        );
}
}

DelimitedTextImporter::DelimitedTextImporter(
    QString importerId,
    QString importerDisplayName,
    QChar delimiter,
    ImportProfile profile
    )
    : importerId(std::move(importerId)),
    importerDisplayName(
        std::move(importerDisplayName)
        ),
    delimiter(delimiter),
    profile(std::move(profile))
{
    this->profile.importerId =
        this->importerId;
}

QString DelimitedTextImporter::id() const
{
    return importerId;
}

QString DelimitedTextImporter::displayName() const
{
    return importerDisplayName;
}

ImportResult DelimitedTextImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    qsizetype headerIndex = -1;

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        if (!lines.at(index)
                 .trimmed()
                 .isEmpty()) {
            headerIndex = index;
            break;
        }
    }

    if (headerIndex < 0) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "DELIMITED_HEADER_REQUIRED"
                ),
            QStringLiteral(
                "The delimited source does not contain "
                "a header record."
                ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    const ParsedDelimitedHeader parsedHeader =
        parseDelimitedHeader(
            lines.at(headerIndex),
            sourcePath,
            headerIndex + 1,
            delimiter,
            result
            );

    if (!parsedHeader.valid) {
        return result;
    }

    const QStringList &headers =
        parsedHeader.headers;

    for (qsizetype index =
         headerIndex + 1;
         index < lines.size();
         ++index) {
        processDelimitedRecord(
            lines.at(index),
            sourcePath,
            index + 1,
            delimiter,
            headers,
            profile,
            result
            );
    }

    return result;
}

ImportResult DelimitedTextImporter::importFile(
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
                ).arg(
                    file.errorString()
                    ),
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

    bool headerFound = false;
    QStringList headers;

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

        if (!headerFound) {
            if (!rawSource.trimmed().isEmpty()) {
                const ParsedDelimitedHeader
                    parsedHeader =
                    parseDelimitedHeader(
                        rawSource,
                        filePath,
                        physicalLineNumber,
                        delimiter,
                        result
                        );

                if (!parsedHeader.valid) {
                    executionContext.report({
                        file.pos(),
                        totalBytes,
                        result.processedRecordCount
                    });

                    return result;
                }

                headers =
                    parsedHeader.headers;

                headerFound = true;
            }
        } else if (
            !rawSource.trimmed().isEmpty()
            ) {
            if (maxProcessedRecords > 0
                && result.processedRecordCount >=
                       maxProcessedRecords) {
                result.sourceTruncated = true;
                break;
            }

            processDelimitedRecord(
                rawSource,
                filePath,
                physicalLineNumber,
                delimiter,
                headers,
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

    if (!headerFound
        && !result.cancelled) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "DELIMITED_HEADER_REQUIRED"
                ),
            QStringLiteral(
                "The delimited source does not contain a header record."
                ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                filePath,
                0
                )
            );
    }

    executionContext.report({
        file.pos(),
        totalBytes,
        result.processedRecordCount
    });

    return result;
}