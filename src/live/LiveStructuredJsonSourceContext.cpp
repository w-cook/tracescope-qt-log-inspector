#include "LiveStructuredJsonSourceContext.h"

#include <algorithm>
#include <utility>

#include <QFile>
#include <QFileDevice>

namespace
{
constexpr qint64 SourceScanChunkSizeBytes =
    256 * 1024;
}

LiveStructuredJsonSourceContext::
    LiveStructuredJsonSourceContext(
        QString recordPath
        )
    : m_framer(
          std::move(recordPath)
          )
{
}

LiveStructuredJsonSourceInitializationResult
    LiveStructuredJsonSourceContext::
    initializeFromExistingFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        quint64 sourceGeneration
        )
{
    LiveStructuredJsonSourceInitializationResult
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
     * Initialization always rebuilds framing state
     * from the exact captured physical baseline.
     */
    m_framer.reset();

    m_nextRecordNumber = 1;

    m_sourceGeneration =
        sourceGeneration;

    result.existingRecordBatch
        .firstRecordNumber = 1;

    result.existingRecordBatch
        .sourceGeneration =
        sourceGeneration;

    QFile file(
        sourcePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing structured JSON live "
                "source could not be opened while "
                "initializing record framing: %1"
                )
                .arg(
                    file.errorString()
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

        StructuredJsonRecordFrameResult
            frameResult =
            m_framer.appendBytes(
                std::move(chunk)
                );

        if (!frameResult.succeeded) {
            result.succeeded = false;

            result.errorMessage =
                frameResult.errorMessage;

            return result;
        }

        for (QByteArray &record
             : frameResult.records) {
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
                "The existing structured JSON live "
                "source could not be completely "
                "scanned while initializing record "
                "framing: %1"
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
                "The existing structured JSON live "
                "source became shorter than the "
                "captured baseline while "
                "initializing record framing."
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

LiveStructuredJsonSourceAppendResult
    LiveStructuredJsonSourceContext::
    appendBytes(
        QByteArray bytes
        )
{
    LiveStructuredJsonSourceAppendResult
        result;

    result.batch.firstRecordNumber =
        m_nextRecordNumber;

    result.batch.sourceGeneration =
        m_sourceGeneration;

    StructuredJsonRecordFrameResult
        frameResult =
        m_framer.appendBytes(
            std::move(bytes)
            );

    if (!frameResult.succeeded) {
        result.succeeded = false;

        result.errorMessage =
            frameResult.errorMessage;

        return result;
    }

    result.batch.records =
        std::move(
            frameResult.records
            );

    m_nextRecordNumber +=
        result.batch.records.size();

    return result;
}

void LiveStructuredJsonSourceContext::
    beginSourceGeneration(
        quint64 sourceGeneration
        )
{
    m_sourceGeneration =
        sourceGeneration;

    /*
     * A truncation or replacement represents a
     * fresh structured source. Logical record
     * numbering begins again at one.
     */
    m_nextRecordNumber = 1;

    /*
     * Never allow an incomplete JSON object or
     * container prefix from the previous physical
     * generation to join bytes from the new one.
     */
    m_framer.reset();
}

qint64
    LiveStructuredJsonSourceContext::
    nextRecordNumber() const
{
    return m_nextRecordNumber;
}

quint64
    LiveStructuredJsonSourceContext::
    sourceGeneration() const
{
    return m_sourceGeneration;
}