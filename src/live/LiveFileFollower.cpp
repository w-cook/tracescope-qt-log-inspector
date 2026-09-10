#include "LiveFileFollower.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace
{
constexpr qint64 SourceFingerprintByteCount =
    4096;
}

LiveFileFollower::LiveFileFollower(
    QString sourcePath,
    QObject *parent
    )
    : QObject(parent),
    m_sourcePath(
        QFileInfo(
            std::move(sourcePath)
            ).absoluteFilePath()
        )
{
}

const QString &
LiveFileFollower::sourcePath() const
{
    return m_sourcePath;
}

const LiveFileFollowState &
LiveFileFollower::state() const
{
    return m_state;
}

bool LiveFileFollower::start()
{
    /*
     * Start and Resume have intentionally different
     * meanings. A paused follower must use resume()
     * so that its unread catch-up range is preserved.
     */
    if (!m_state.isStopped()) {
        return false;
    }

    const QFileInfo fileInfo(
        m_sourcePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return false;
    }

    if (m_hasSourceIdentity
        && (
            fileInfo.size()
                < m_state.readOffset()
            || sourceIdentityChanged(
                fileInfo
                )
            )) {
        m_state.beginNextSourceGeneration();
    }

    /*
     * The investigation has already imported the
     * current source contents. Begin following at
     * the current EOF so that only future physical
     * growth is consumed.
     */
    m_state.startAtOffset(
        fileInfo.size()
        );

    captureSourceIdentity(
        fileInfo
        );

    emit stateChanged();

    return true;
}

bool LiveFileFollower::pause()
{
    if (!m_state.pause()) {
        return false;
    }

    emit stateChanged();

    return true;
}

bool LiveFileFollower::resume()
{
    if (!m_state.resume()) {
        return false;
    }

    emit stateChanged();

    return true;
}

bool LiveFileFollower::stop()
{
    if (m_state.isStopped()) {
        return false;
    }

    m_state.stop();

    emit stateChanged();

    return true;
}

LiveFileObservation LiveFileFollower::poll()
{
    if (m_state.isStopped()) {
        return {
            LiveFileObservationKind::Inactive,
            0,
            0
        };
    }

    const QFileInfo fileInfo(
        m_sourcePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return {
            LiveFileObservationKind::Missing,
            0,
            0
        };
    }

    const qint64 observedSize =
        fileInfo.size();

    if (sourceIdentityChanged(
            fileInfo
            )) {
        m_state.beginNextSourceGeneration();

        captureSourceIdentity(
            fileInfo
            );

        emit stateChanged();

        return {
            LiveFileObservationKind::SourceReset,
            observedSize,
            observedSize
        };
    }

    /*
     * The physical source is now shorter than the
     * position TraceScope has already consumed.
     *
     * The old investigation records remain historical
     * evidence, but new physical records belong to a
     * new source generation beginning at byte zero.
     */
    if (observedSize
        < m_state.readOffset()) {
        m_state.beginNextSourceGeneration();

        captureSourceIdentity(
            fileInfo
            );

        emit stateChanged();

        return {
            LiveFileObservationKind::SourceReset,
            observedSize,
            observedSize
        };
    }

    const qint64 availableByteCount =
        observedSize
        - m_state.readOffset();

    if (availableByteCount > 0) {
        return {
            LiveFileObservationKind::Appended,
            observedSize,
            availableByteCount
        };
    }

    return {
        LiveFileObservationKind::NoChange,
        observedSize,
        0
    };
}

LiveFileReadResult
LiveFileFollower::readAvailableBytes(
    qint64 maxByteCount
    )
{
    LiveFileReadResult result;

    result.observation =
        poll();

    /*
     * poll() remains useful while paused because it
     * can observe accumulated growth or a source
     * generation change. Actual byte consumption,
     * however, occurs only while actively following.
     */
    if (!m_state.isFollowing()) {
        return result;
    }

    if (result.observation.kind
            != LiveFileObservationKind::Appended
        && result.observation.kind
               != LiveFileObservationKind::SourceReset) {
        return result;
    }

    if (result.observation.availableByteCount
        <= 0) {
        return result;
    }

    if (maxByteCount <= 0) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live read chunk size must "
                "be greater than zero."
                );

        return result;
    }

    const qint64 byteCount =
        std::min(
            maxByteCount,
            result.observation
                .availableByteCount
            );

    const qint64 startingOffset =
        m_state.readOffset();

    QFile file(
        m_sourcePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live source could not be "
                "opened for reading: %1"
                )
                .arg(
                    file.errorString()
                    );

        return result;
    }

    if (!file.seek(
            startingOffset
            )) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live source could not be "
                "positioned at byte %1: %2"
                )
                .arg(
                    startingOffset
                    )
                .arg(
                    file.errorString()
                    );

        return result;
    }

    const QByteArray bytes =
        file.read(
            byteCount
            );

    /*
     * Accept only the complete range we intended to
     * consume. A short read can mean that the source
     * changed between observation and physical read.
     *
     * In that case leave the cursor untouched so the
     * next poll can reassess the source safely.
     */
    if (bytes.size()
        != byteCount) {
        result.succeeded = false;

        result.errorMessage =
            file.error()
                    != QFileDevice::NoError
                ? QStringLiteral(
                      "The live source could not "
                      "be read: %1"
                      )
                      .arg(
                          file.errorString()
                          )
                : QStringLiteral(
                      "The live source changed "
                      "before the observed byte "
                      "range could be read."
                      );

        return result;
    }

    if (!m_state.advanceReadOffset(
            bytes.size()
            )) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live source cursor could "
                "not be advanced."
                );

        return result;
    }

    result.bytes =
        bytes;

    /*
     * An initially empty generation has no prefix
     * available to fingerprint. Once real bytes have
     * arrived, establish that bounded identity so
     * later same-path replacement detection retains
     * its portable fallback.
     */
    if (m_sourceFingerprintLength == 0) {
        const QFileInfo fileInfo(
            m_sourcePath
            );

        if (fileInfo.exists()
            && fileInfo.isFile()) {
            captureSourceIdentity(
                fileInfo
                );
        }
    }

    return result;
}

QByteArray
LiveFileFollower::sourcePrefixFingerprint(
    qint64 byteCount
    ) const
{
    if (byteCount <= 0) {
        return QByteArray();
    }

    QFile file(
        m_sourcePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        return QByteArray();
    }

    const QByteArray bytes =
        file.read(
            byteCount
            );

    if (bytes.size()
        != byteCount) {
        return QByteArray();
    }

    return QCryptographicHash::hash(
        bytes,
        QCryptographicHash::Sha256
        );
}

void LiveFileFollower::captureSourceIdentity(
    const QFileInfo &fileInfo
    )
{
    m_hasSourceIdentity = true;

    m_sourceBirthTime =
        fileInfo.birthTime();

    m_sourceFingerprintLength =
        std::min<qint64>(
            fileInfo.size(),
            SourceFingerprintByteCount
            );

    m_sourcePrefixFingerprint =
        sourcePrefixFingerprint(
            m_sourceFingerprintLength
            );
}

bool LiveFileFollower::sourceIdentityChanged(
    const QFileInfo &fileInfo
    ) const
{
    if (!m_hasSourceIdentity) {
        return false;
    }

    const QDateTime currentBirthTime =
        fileInfo.birthTime();

    /*
     * Creation time is a strong replacement signal
     * when the underlying filesystem exposes it.
     */
    if (m_sourceBirthTime.isValid()
        && currentBirthTime.isValid()
        && currentBirthTime
               != m_sourceBirthTime) {
        return true;
    }

    /*
     * Fall back to a bounded fingerprint of bytes
     * that belonged to the original generation.
     *
     * Ordinary appends cannot alter this prefix.
     */
    if (m_sourceFingerprintLength <= 0
        || m_sourcePrefixFingerprint.isEmpty()
        || fileInfo.size()
               < m_sourceFingerprintLength) {
        return false;
    }

    const QByteArray currentFingerprint =
        sourcePrefixFingerprint(
            m_sourceFingerprintLength
            );

    if (currentFingerprint.isEmpty()) {
        return false;
    }

    return currentFingerprint
           != m_sourcePrefixFingerprint;
}