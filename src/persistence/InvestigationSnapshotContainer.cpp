#include "InvestigationSnapshotContainer.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>

namespace
{
const QByteArray ContainerMagic =
    QByteArrayLiteral("TSINV");

constexpr quint8 CompressionMethodZlib = 1;

constexpr int ChecksumSizeBytes = 32;

constexpr int CompressionLevel = 9;

InvestigationSnapshotContainerDecodeResult
failure(
    const QString &code,
    const QString &message
    )
{
    InvestigationSnapshotContainerDecodeResult
        result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}
}

QByteArray InvestigationSnapshotContainer::encode(
    const QByteArray &payload
    ) const
{
    const QByteArray compressedPayload =
        qCompress(
            payload,
            CompressionLevel
            );

    const QByteArray checksum =
        QCryptographicHash::hash(
            payload,
            QCryptographicHash::Sha256
            );

    QByteArray container;

    QDataStream stream(
        &container,
        QIODevice::WriteOnly
        );

    stream.setByteOrder(
        QDataStream::BigEndian
        );

    stream.writeRawData(
        ContainerMagic.constData(),
        ContainerMagic.size()
        );

    stream
        << CurrentContainerVersion
        << CompressionMethodZlib
        << static_cast<quint64>(
               payload.size()
               );

    stream.writeRawData(
        checksum.constData(),
        checksum.size()
        );

    container.append(
        compressedPayload
        );

    return container;
}

InvestigationSnapshotContainerDecodeResult
InvestigationSnapshotContainer::decode(
    const QByteArray &container
    ) const
{
    const qsizetype minimumHeaderSize =
        ContainerMagic.size()
        + static_cast<qsizetype>(
            sizeof(quint16)
            )
        + static_cast<qsizetype>(
            sizeof(quint8)
            )
        + static_cast<qsizetype>(
            sizeof(quint64)
            )
        + ChecksumSizeBytes;

    if (container.size()
        < minimumHeaderSize) {
        return failure(
            QStringLiteral(
                "TRUNCATED_CONTAINER"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "container is truncated."
                )
            );
    }

    QBuffer buffer;

    buffer.setData(
        container
        );

    if (!buffer.open(
            QIODevice::ReadOnly
            )) {
        return failure(
            QStringLiteral(
                "CONTAINER_READ_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "container could not be read."
                )
            );
    }

    QDataStream stream(
        &buffer
        );

    stream.setByteOrder(
        QDataStream::BigEndian
        );

    QByteArray magic(
        ContainerMagic.size(),
        '\0'
        );

    if (stream.readRawData(
            magic.data(),
            magic.size()
            )
        != magic.size()) {
        return failure(
            QStringLiteral(
                "TRUNCATED_CONTAINER"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "container header is truncated."
                )
            );
    }

    if (magic != ContainerMagic) {
        return failure(
            QStringLiteral(
                "INVALID_CONTAINER_MAGIC"
                ),
            QStringLiteral(
                "The file is not a TraceScope "
                "investigation snapshot."
                )
            );
    }

    quint16 containerVersion = 0;
    quint8 compressionMethod = 0;
    quint64 expectedPayloadSize = 0;

    stream
        >> containerVersion
        >> compressionMethod
        >> expectedPayloadSize;

    if (stream.status()
        != QDataStream::Ok) {
        return failure(
            QStringLiteral(
                "TRUNCATED_CONTAINER"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "container header is incomplete."
                )
            );
    }

    if (containerVersion
        != CurrentContainerVersion) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_CONTAINER_VERSION"
                ),
            QStringLiteral(
                "Investigation snapshot container "
                "version %1 is not supported."
                )
                .arg(
                    containerVersion
                    )
            );
    }

    if (compressionMethod
        != CompressionMethodZlib) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_COMPRESSION"
                ),
            QStringLiteral(
                "The investigation snapshot uses "
                "an unsupported compression method."
                )
            );
    }

    QByteArray expectedChecksum(
        ChecksumSizeBytes,
        '\0'
        );

    if (stream.readRawData(
            expectedChecksum.data(),
            expectedChecksum.size()
            )
        != expectedChecksum.size()) {
        return failure(
            QStringLiteral(
                "TRUNCATED_CONTAINER"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "checksum is incomplete."
                )
            );
    }

    const qint64 payloadOffset =
        buffer.pos();

    const QByteArray compressedPayload =
        container.mid(
            payloadOffset
            );

    if (compressedPayload.isEmpty()) {
        return failure(
            QStringLiteral(
                "TRUNCATED_CONTAINER"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "contains no compressed payload."
                )
            );
    }

    const QByteArray payload =
        qUncompress(
            compressedPayload
            );

    /*
     * A valid snapshot JSON payload is never empty.
     * qUncompress() also returns an empty QByteArray
     * when decompression fails.
     */
    if (payload.isEmpty()) {
        return failure(
            QStringLiteral(
                "DECOMPRESSION_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "payload could not be decompressed."
                )
            );
    }

    if (static_cast<quint64>(
            payload.size()
            )
        != expectedPayloadSize) {
        return failure(
            QStringLiteral(
                "PAYLOAD_SIZE_MISMATCH"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "payload size does not match its "
                "container metadata."
                )
            );
    }

    const QByteArray actualChecksum =
        QCryptographicHash::hash(
            payload,
            QCryptographicHash::Sha256
            );

    if (actualChecksum
        != expectedChecksum) {
        return failure(
            QStringLiteral(
                "CHECKSUM_MISMATCH"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "failed its integrity check."
                )
            );
    }

    InvestigationSnapshotContainerDecodeResult
        result;

    result.payload =
        payload;

    return result;
}