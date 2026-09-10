#include "LiveLineSourceContext.h"

#include <algorithm>
#include <utility>

#include <QFile>
#include <QFileDevice>

namespace
{
constexpr qint64 SourceScanChunkSizeBytes =
    256 * 1024;
}

LiveLineSourceInitializationResult
    LiveLineSourceContext::
    initializeFromExistingFile(
        const QString &sourcePath,
        qint64 existingByteCount
        )
{
    LiveLineSourceInitializationResult result;

    if (existingByteCount < 0) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing live source byte count "
                "cannot be negative."
                );

        return result;
    }

    QFile file(
        sourcePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing live source could not "
                "be opened while initializing line "
                "position: %1"
                )
                .arg(
                    file.errorString()
                    );

        return result;
    }

    qint64 bytesScanned = 0;
    qint64 newlineCount = 0;

    bool sawBytes = false;
    char lastByte = '\0';

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

        const QByteArray chunk =
            file.read(
                requested
                );

        if (chunk.isEmpty()) {
            break;
        }

        sawBytes = true;

        bytesScanned +=
            chunk.size();

        newlineCount +=
            chunk.count(
                '\n'
                );

        lastByte =
            chunk.back();
    }

    if (file.error()
        != QFileDevice::NoError) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The existing live source could not "
                "be completely scanned while "
                "initializing line position: %1"
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
                "The existing live source became "
                "shorter than the captured baseline "
                "while initializing line position."
                );

        return result;
    }

    result.endsAtLineBoundary =
        !sawBytes
        || lastByte == '\n';

    result.existingPhysicalLineCount =
        newlineCount;

    if (sawBytes
        && lastByte != '\n') {
        ++result.existingPhysicalLineCount;
    }

    m_nextPhysicalLineNumber =
        result.existingPhysicalLineCount
        + 1;

    m_sourceGeneration = 0;

    m_framer.reset();

    return result;
}

LiveFramedLineBatch
LiveLineSourceContext::appendBytes(
    QByteArray bytes
    )
{
    LiveFramedLineBatch result;

    result.firstPhysicalLineNumber =
        m_nextPhysicalLineNumber;

    result.sourceGeneration =
        m_sourceGeneration;

    result.lines =
        m_framer.appendBytes(
            std::move(bytes)
            );

    m_nextPhysicalLineNumber +=
        result.lines.size();

    return result;
}

void LiveLineSourceContext::
    beginSourceGeneration(
        quint64 sourceGeneration
        )
{
    m_sourceGeneration =
        sourceGeneration;

    /*
     * A replacement/truncation creates a fresh
     * physical source. Its first physical line is
     * therefore line one regardless of the previous
     * generation's position.
     */
    m_nextPhysicalLineNumber = 1;

    /*
     * Never allow an incomplete physical record from
     * the previous generation to join the first bytes
     * of the new generation.
     */
    m_framer.reset();
}

qint64
    LiveLineSourceContext::
    nextPhysicalLineNumber() const
{
    return m_nextPhysicalLineNumber;
}

quint64
    LiveLineSourceContext::
    sourceGeneration() const
{
    return m_sourceGeneration;
}

const LiveLineRecordFramer &
LiveLineSourceContext::framer() const
{
    return m_framer;
}