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

SourceRelocationVerificationResult
verifySourceRelocationCandidate(
    const QString &candidatePath,
    const SourcePhysicalIdentity &expectedIdentity
    )
{
    SourceRelocationVerificationResult result;

    const QFileInfo candidateInfo(
        candidatePath
        );

    if (!candidateInfo.exists()
        || !candidateInfo.isFile()) {
        result.errorMessage =
            QStringLiteral(
                "The candidate source file does not exist."
                );

        return result;
    }

    if (expectedIdentity.observedSizeBytes < 0
        || expectedIdentity.fingerprintLength < 0
        || expectedIdentity.fingerprintLength
               > expectedIdentity.observedSizeBytes) {
        result.errorMessage =
            QStringLiteral(
                "The saved source identity is invalid."
                );

        return result;
    }

    const bool hasFingerprintEvidence =
        expectedIdentity.fingerprintLength > 0
        && !expectedIdentity
                .prefixFingerprint
                .isEmpty();

    const bool hasBirthTimeEvidence =
        expectedIdentity.birthTime.isValid();

    /*
     * A persisted fingerprint is meaningful only when
     * it is a complete SHA-256 digest of exactly the
     * historical prefix length.
     */
    if (expectedIdentity.fingerprintLength > 0
        && expectedIdentity
                   .prefixFingerprint
                   .size()
               != QCryptographicHash::hashLength(
                   QCryptographicHash::Sha256
                   )) {
        result.errorMessage =
            QStringLiteral(
                "The saved source fingerprint is invalid."
                );

        return result;
    }

    if (expectedIdentity.fingerprintLength == 0
        && !expectedIdentity
                .prefixFingerprint
                .isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The saved source fingerprint is invalid."
                );

        return result;
    }

    if (!hasFingerprintEvidence
        && !hasBirthTimeEvidence) {
        result.errorMessage =
            QStringLiteral(
                "The saved source identity does not contain "
                "enough evidence to verify relocation."
                );

        return result;
    }

    /*
     * A candidate shorter than the source size already
     * observed cannot contain the complete historical
     * source state represented by this identity.
     */
    if (candidateInfo.size()
        < expectedIdentity.observedSizeBytes) {
        result.errorMessage =
            QStringLiteral(
                "The candidate source is shorter than the "
                "previously observed source."
                );

        return result;
    }

    if (hasFingerprintEvidence) {
        if (candidateInfo.size()
            < expectedIdentity.fingerprintLength) {
            result.errorMessage =
                QStringLiteral(
                    "The candidate source is too short to "
                    "verify the saved source fingerprint."
                    );

            return result;
        }

        const QByteArray candidateFingerprint =
            sourcePrefixFingerprint(
                candidateInfo.absoluteFilePath(),
                expectedIdentity.fingerprintLength
                );

        if (candidateFingerprint.isEmpty()) {
            result.errorMessage =
                QStringLiteral(
                    "The candidate source fingerprint could "
                    "not be read."
                    );

            return result;
        }

        if (candidateFingerprint
            != expectedIdentity.prefixFingerprint) {
            result.errorMessage =
                QStringLiteral(
                    "The candidate source does not match the "
                    "saved source fingerprint."
                    );

            return result;
        }
    } else {
        /*
         * Birth time is a weaker fallback used only when
         * no byte fingerprint was available, such as an
         * empty source captured on a filesystem that
         * exposes creation time.
         */
        const QDateTime candidateBirthTime =
            candidateInfo.birthTime();

        if (!candidateBirthTime.isValid()
            || candidateBirthTime
                   != expectedIdentity.birthTime) {
            result.errorMessage =
                QStringLiteral(
                    "The candidate source does not match the "
                    "saved source birth time."
                    );

            return result;
        }
    }

    /*
     * Verification is based on the historical evidence,
     * but once proven, capture a fresh physical identity
     * for future same-path replacement/truncation checks.
     */
    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            candidateInfo.absoluteFilePath()
            );

    if (!captureResult.succeeded) {
        result.errorMessage =
            captureResult.errorMessage.isEmpty()
                ? QStringLiteral(
                      "The candidate source identity could "
                      "not be captured."
                      )
                : captureResult.errorMessage;

        return result;
    }

    result.candidateIdentity =
        captureResult.identity;

    result.verified = true;

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