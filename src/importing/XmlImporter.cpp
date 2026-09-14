#include "XmlImporter.h"

#include <optional>
#include <utility>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>

#include "../io/SharedReadFile.h"

#include "ImportDiagnostic.h"
#include "JsonObjectRecordMapper.h"
#include "StructuredXmlRecordStreamParser.h"

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
    const std::optional<RecordSourceMetadata>
        &source = std::nullopt
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

constexpr qint64 parserReadChunkSize =
    64 * 1024;

constexpr qint64 progressReportByteInterval =
    256 * 1024;

void reportProgressIfNeeded(
    QIODevice &device,
    qint64 totalBytes,
    qint64 processedRecordCount,
    qint64 &lastReportedBytes,
    const ImportExecutionContext &executionContext
    )
{
    const qint64 bytesProcessed =
        device.pos();

    if (bytesProcessed
            - lastReportedBytes
        < progressReportByteInterval) {
        return;
    }

    executionContext.report({
        bytesProcessed,
        totalBytes,
        processedRecordCount
    });

    lastReportedBytes =
        bytesProcessed;
}

void processRecord(
    const StructuredXmlRecord &record,
    const QString &sourcePath,
    const ImportProfile &profile,
    ImportResult &result
    )
{
    ++result.processedRecordCount;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            result.processedRecordCount
            );

    result.records.append(
        JsonObjectRecordMapper::mapRecord(
            record.object,
            record.rawSource,
            source,
            profile,
            result
            )
        );
}
}

XmlImporter::XmlImporter(
    ImportProfile profile
    )
    : profile(
          std::move(
              profile
              )
          )
{
    this->profile.importerId =
        QStringLiteral("xml");
}

QString XmlImporter::id() const
{
    return QStringLiteral("xml");
}

QString XmlImporter::displayName() const
{
    return QStringLiteral(
        "Structured XML"
        );
}

ImportResult XmlImporter::importContent(
    const QByteArray &xml,
    const QString &sourcePath,
    qint64 maxProcessedRecords
    ) const
{
    QBuffer buffer;

    buffer.setData(
        xml
        );

    buffer.open(
        QIODevice::ReadOnly
        );

    return importDevice(
        buffer,
        sourcePath,
        maxProcessedRecords,
        {}
        );
}

ImportResult XmlImporter::importFile(
    const QString &filePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    QFile file;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            file,
            filePath
            );

    if (!openResult.succeeded) {
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
                    openResult.errorMessage
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                filePath,
                0
                )
            );

        return result;
    }

    return importDevice(
        file,
        filePath,
        maxProcessedRecords,
        executionContext
        );
}

ImportResult XmlImporter::importDevice(
    QIODevice &device,
    const QString &sourcePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    ImportResult result;

    const qint64 totalBytes =
        device.size();

    qint64 lastReportedBytes = 0;

    executionContext.report({
        0,
        totalBytes,
        0
    });

    StructuredXmlRecordStreamParser parser(
        profile.recordPath
        );

    bool parseFailed = false;
    QString parseErrorMessage;

    while (!device.atEnd()) {
        if (executionContext
                .cancellationRequested()) {
            result.cancelled = true;
            break;
        }

        QByteArray chunk =
            device.read(
                parserReadChunkSize
                );

        if (chunk.isEmpty()) {
            break;
        }

        const StructuredXmlRecordStreamParseResult
            parseResult =
            parser.appendBytes(
                std::move(
                    chunk
                    )
                );

        /*
         * A parser call may emit multiple complete
         * records before reaching the current physical
         * end of the file or encountering a later
         * malformed record.
         *
         * Process those completed records first.
         */
        for (const StructuredXmlRecord &record
             : parseResult.records) {
            if (executionContext
                    .cancellationRequested()) {
                result.cancelled = true;
                break;
            }

            if (maxProcessedRecords > 0
                && result.processedRecordCount
                       >= maxProcessedRecords) {
                result.sourceTruncated = true;
                break;
            }

            processRecord(
                record,
                sourcePath,
                profile,
                result
                );
        }

        reportProgressIfNeeded(
            device,
            totalBytes,
            result.processedRecordCount,
            lastReportedBytes,
            executionContext
            );

        if (result.cancelled
            || result.sourceTruncated) {
            break;
        }

        if (!parseResult.succeeded) {
            /*
             * Preserve XmlImporter's historical
             * accounting behavior:
             *
             * a malformed selected record counts as
             * a processed/skipped source record even
             * though no InvestigationRecord is
             * manufactured from its incomplete
             * contents.
             */
            if (parser.hasIncompleteRecord()) {
                ++result.processedRecordCount;
            }

            parseFailed = true;

            parseErrorMessage =
                parseResult.errorMessage;

            break;
        }
    }

    if (parseFailed
        && !result.sourceTruncated
        && !result.cancelled) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "XML_PARSE_ERROR"
                ),
            parseErrorMessage,
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        executionContext.report({
            device.pos(),
            totalBytes,
            result.processedRecordCount
        });

        return result;
    }

    /*
     * Only classify the source as physically open
     * after reaching the actual current end of the
     * device.
     *
     * If import stopped because of a record limit or
     * cancellation, the parser may simply not have
     * reached a perfectly valid closing container yet.
     */
    const bool reachedCurrentSourceEnd =
        !result.cancelled
        && !result.sourceTruncated
        && device.atEnd();

    const bool hasConfiguredRecordPath =
        !profile.recordPath
             .trimmed()
             .isEmpty();

    const bool openRecordContainer =
        reachedCurrentSourceEnd
        && hasConfiguredRecordPath
        && parser.recordContainerLocated()
        && !parser.recordContainerClosed()
        && !parser.documentClosed();

    if (openRecordContainer) {
        /*
         * This is the expected shape of a structured
         * XML source being actively written by another
         * process:
         *
         * <session>
         *   <events>
         *     <event>...</event>
         *     <event>...</event>
         *     ...
         *
         * The outer record container intentionally
         * remains open so future complete record
         * elements can be appended.
         *
         * Any incomplete trailing record remains only
         * in parser state and is not counted or
         * imported by this initial static import.
         */
        appendDiagnostic(
            result,
            QStringLiteral(
                "XML_OPEN_CONTAINER"
                ),
            QStringLiteral(
                "The structured XML record container "
                "is still open. Complete records "
                "available at the current end of the "
                "source were imported."
                ),
            ImportDiagnosticSeverity::Information,
            createSourceMetadata(
                sourcePath,
                0
                )
            );
    } else if (reachedCurrentSourceEnd
               && !parser.documentClosed()) {
        /*
         * Premature EOF is recoverable inside the
         * shared incremental parser because live
         * ingestion may later provide more bytes.
         *
         * For ordinary static import, however, it is
         * only valid when the configured repeating
         * record container itself is still open.
         *
         * Examples rejected here include:
         *
         * - an incomplete document-root record
         * - an incomplete prefix before the configured
         *   record container was reached
         * - a record container that already closed,
         *   followed by a missing outer closing element
         */
        if (parser.hasIncompleteRecord()) {
            ++result.processedRecordCount;
        }

        appendDiagnostic(
            result,
            QStringLiteral(
                "XML_PARSE_ERROR"
                ),
            QStringLiteral(
                "The XML document ended before it "
                "was complete."
                ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        executionContext.report({
            device.pos(),
            totalBytes,
            result.processedRecordCount
        });

        return result;
    }

    if (!result.cancelled
        && !result.sourceTruncated
        && !openRecordContainer
        && result.processedRecordCount == 0) {
        if (profile.recordPath
                .trimmed()
                .isEmpty()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "XML_DOCUMENT_EMPTY"
                    ),
                QStringLiteral(
                    "The XML document does not contain "
                    "a root record element."
                    ),
                ImportDiagnosticSeverity::Error,
                createSourceMetadata(
                    sourcePath,
                    0
                    )
                );
        } else {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "XML_RECORD_PATH_NOT_FOUND"
                    ),
                QStringLiteral(
                    "No XML records were found at "
                    "record path '%1'."
                    )
                    .arg(
                        profile.recordPath
                        ),
                ImportDiagnosticSeverity::Error,
                createSourceMetadata(
                    sourcePath,
                    0
                    )
                );
        }
    }

    executionContext.report({
        device.pos(),
        totalBytes,
        result.processedRecordCount
    });

    return result;
}