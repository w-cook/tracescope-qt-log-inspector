#include "LiveStructuredXmlSourceContext.h"

#include <algorithm>
#include <utility>

#include <QFile>
#include <QFileDevice>

#include "../io/SharedReadFile.h"

namespace
{
constexpr qint64 SourceScanChunkSizeBytes =
    256 * 1024;
}

LiveStructuredXmlSourceContext::
    LiveStructuredXmlSourceContext(
        QString recordPath
        )
    : m_parser(
          std::move(recordPath)
          )
{
}

LiveStructuredXmlSourceInitializationResult
    LiveStructuredXmlSourceContext::
    initializeFromExistingFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        quint64 sourceGeneration
        )
{
    LiveStructuredXmlSourceInitializationResult
        result;

    if (existingByteCount < 0) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing live source byte count "
                "cannot be negative."
                );

        return result;
    }

    /*
     * Reconstruct parser state from exactly the
     * physical baseline captured by LiveFileFollower.
     *
     * Complete baseline records are returned only so
     * their count can establish subsequent logical
     * numbering. They must not be appended to the
     * session again.
     */
    m_parser.reset();

    m_nextRecordNumber = 1;

    m_sourceGeneration =
        sourceGeneration;

    result.existingRecordBatch
        .firstRecordNumber = 1;

    result.existingRecordBatch
        .sourceGeneration =
        sourceGeneration;

    QFile file;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            file,
            sourcePath
            );

    if (!openResult.succeeded) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing structured XML live "
                "source could not be opened while "
                "initializing record parsing: %1"
                )
                .arg(
                    openResult.errorMessage
                    );

        return result;
    }

    qint64 bytesScanned = 0;

    while (bytesScanned
           < existingByteCount) {
        const qint64 remaining =
            existingByteCount
            - bytesScanned;

        const qint64 requested =
            std::min(
                SourceScanChunkSizeBytes,
                remaining
                );

        QByteArray chunk =
            file.read(
                requested
                );

        if (chunk.isEmpty()) {
            break;
        }

        bytesScanned +=
            chunk.size();

        StructuredXmlRecordStreamParseResult
            parseResult =
            m_parser.appendBytes(
                std::move(chunk)
                );

        if (!parseResult.succeeded) {
            result.succeeded = false;

            result.errorMessage =
                parseResult.errorMessage;

            return result;
        }

        for (StructuredXmlRecord &record
             : parseResult.records) {
            result.existingRecordBatch
                .records
                .append(
                    std::move(record)
                    );
        }
    }

    if (file.error()
        != QFileDevice::NoError) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing structured XML live "
                "source could not be completely "
                "scanned while initializing record "
                "parsing: %1"
                )
                .arg(
                    file.errorString()
                    );

        return result;
    }

    if (bytesScanned
        != existingByteCount) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing structured XML live "
                "source became shorter than the "
                "captured baseline while "
                "initializing record parsing."
                );

        return result;
    }

    /*
     * A live XML source must still have its selected
     * repeating-record container open.
     *
     * A complete/closed XML document cannot legally
     * accept another sibling record afterward.
     */
    if (!m_parser.recordContainerLocated()) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "Live following cannot begin because "
                "the configured XML record container "
                "was not found in the captured "
                "source baseline."
                );

        return result;
    }

    if (m_parser.recordContainerClosed()
        || m_parser.documentClosed()) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "Live following cannot begin because "
                "the configured XML record container "
                "is already closed."
                );

        return result;
    }

    m_nextRecordNumber =
        result.existingRecordBatch
            .records
            .size()
        + 1;

    return result;
}

LiveStructuredXmlSourceAppendResult
    LiveStructuredXmlSourceContext::
    appendBytes(
        QByteArray bytes
        )
{
    LiveStructuredXmlSourceAppendResult
        result;

    result.batch.firstRecordNumber =
        m_nextRecordNumber;

    result.batch.sourceGeneration =
        m_sourceGeneration;

    StructuredXmlRecordStreamParseResult
        parseResult =
        m_parser.appendBytes(
            std::move(bytes)
            );

    if (!parseResult.succeeded) {
        result.succeeded = false;

        result.errorMessage =
            parseResult.errorMessage;

        return result;
    }

    result.batch.records =
        std::move(
            parseResult.records
            );

    m_nextRecordNumber +=
        result.batch.records.size();

    return result;
}

void LiveStructuredXmlSourceContext::
    beginSourceGeneration(
        quint64 sourceGeneration
        )
{
    m_sourceGeneration =
        sourceGeneration;

    m_nextRecordNumber = 1;

    /*
     * Never allow open elements or an incomplete
     * record from an earlier physical generation to
     * combine with bytes from a replacement source.
     */
    m_parser.reset();
}

qint64
    LiveStructuredXmlSourceContext::
    nextRecordNumber() const
{
    return m_nextRecordNumber;
}

quint64
    LiveStructuredXmlSourceContext::
    sourceGeneration() const
{
    return m_sourceGeneration;
}