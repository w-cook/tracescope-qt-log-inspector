#include "SourcePhysicalIdentity.h"

#include <algorithm>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

#include "../io/SharedReadFile.h"

namespace
{
constexpr qint64 FingerprintByteCount =
    4096;

QByteArray sourcePrefixFingerprint(
    const QString &sourcePath,
    qint64 byteCount
    )
{
    if (byteCount <= 0) {
        return {};
    }

    QFile file;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            file,
            sourcePath
            );

    if (!openResult.succeeded) {
        return {};
    }

    const QByteArray bytes =
        file.read(byteCount);

    if (bytes.size() != byteCount) {
        return {};
    }

    return QCryptographicHash::hash(
        bytes,
        QCryptographicHash::Sha256
        );
}
}

SourcePhysicalIdentityCaptureResult
captureSourcePhysicalIdentity(
    const QString &sourcePath
    )
{
    SourcePhysicalIdentityCaptureResult result;

    const QFileInfo fileInfo(
        sourcePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        result.errorMessage =
            QStringLiteral(
                "The source file does not exist."
                );

        return result;
    }

    SourcePhysicalIdentity identity;

    identity.birthTime =
        fileInfo.birthTime();

    identity.observedSizeBytes =
        std::max<qint64>(
            0,
            fileInfo.size()
            );

    identity.fingerprintLength =
        std::min<qint64>(
            identity.observedSizeBytes,
            FingerprintByteCount
            );

    identity.prefixFingerprint =
        sourcePrefixFingerprint(
            fileInfo.absoluteFilePath(),
            identity.fingerprintLength
            );

    /*
     * An empty source has no bytes to fingerprint.
     * Its filesystem birth time may still provide
     * identity. If the filesystem exposes neither,
     * capture still succeeds but the identity remains
     * weak; normal size/generation handling remains
     * conservative.
     */
    result.identity =
        std::move(identity);

    result.succeeded = true;

    return result;
}

bool sourcePhysicalIdentityChanged(
    const QString &sourcePath,
    const SourcePhysicalIdentity &identity
    )
{
    const QFileInfo fileInfo(
        sourcePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return false;
    }

    const QDateTime currentBirthTime =
        fileInfo.birthTime();

    if (identity.birthTime.isValid()
        && currentBirthTime.isValid()
        && currentBirthTime
               != identity.birthTime) {
        return true;
    }

    /*
     * A source shorter than the size captured with the
     * persisted identity is a conservative indication
     * that the old generation was truncated/reset.
     *
     * LiveFileFollower separately uses its consumed
     * read offset for this while running. This field is
     * primarily useful after process/workspace reopen.
     */
    if (fileInfo.size()
        < identity.observedSizeBytes) {
        return true;
    }

    if (identity.fingerprintLength <= 0
        || identity.prefixFingerprint.isEmpty()
        || fileInfo.size()
               < identity.fingerprintLength) {
        return false;
    }

    const QByteArray currentFingerprint =
        sourcePrefixFingerprint(
            fileInfo.absoluteFilePath(),
            identity.fingerprintLength
            );

    if (currentFingerprint.isEmpty()) {
        return false;
    }

    return currentFingerprint
           != identity.prefixFingerprint;
}