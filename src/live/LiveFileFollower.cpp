#include "LiveFileFollower.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <utility>

#include "../io/SharedReadFile.h"

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

const SourcePhysicalIdentity *
LiveFileFollower::sourceIdentity() const
{
    if (!m_hasSourceIdentity) {
        return nullptr;
    }

    return &m_sourceIdentity;
}

const LiveFileFollowState &
LiveFileFollower::state() const
{
    return m_state;
}

bool LiveFileFollower::start(
    qint64 initialReadOffset
    )
{
    /*
     * Start and Resume have intentionally different
     * meanings. A paused follower must use resume()
     * so that its unread catch-up range and active
     * parser state are preserved.
     *
     * A stopped follower may be either:
     *
     * - starting for the first time after the
     *   investigation imported its existing source
     * - restarting after an earlier Stop
     *
     * For the first Start, callers may provide a
     * conservative byte boundary captured before the
     * initial import began. This deliberately permits
     * overlap between static import and live ingestion
     * so records written while the import was running,
     * or before the user presses Start, cannot fall
     * through an unobserved gap.
     *
     * Replayed generation-zero records retain stable
     * identities and are deduplicated by the session.
     *
     * A later Start always resumes from the retained
     * cursor unless the source changed generation
     * while following was stopped.
     */
    if (!m_state.isStopped()) {
        return false;
    }

    if (initialReadOffset < -1) {
        return false;
    }

    const QFileInfo fileInfo(
        m_sourcePath
        );

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return false;
    }

    const bool isRestart =
        m_hasSourceIdentity;

    if (isRestart
        && (
            fileInfo.size()
                < m_state.readOffset()
            || sourcePhysicalIdentityChanged(
                m_sourcePath,
                m_sourceIdentity
                )
            )) {
        /*
         * Truncation/replacement occurred while
         * following was stopped. Unread bytes from
         * the old generation may no longer be
         * recoverable, so begin the new physical
         * generation at byte zero.
         */
        m_state.beginNextSourceGeneration();
    }

    qint64 startOffset = 0;

    if (isRestart) {
        /*
         * Once a follower has established an identity,
         * any caller-supplied initial boundary no
         * longer applies. Stop -> Start continuity is
         * governed by the retained cursor.
         */
        startOffset =
            m_state.readOffset();
    } else if (initialReadOffset >= 0) {
        /*
         * The source became shorter than the
         * conservative pre-import boundary before
         * live following could begin. Treat that as
         * evidence that the original physical
         * generation is no longer available and begin
         * the current generation from byte zero.
         */
        if (fileInfo.size()
            < initialReadOffset) {
            m_state.beginNextSourceGeneration();

            startOffset = 0;
        } else {
            startOffset =
                initialReadOffset;
        }
    } else {
        /*
         * Legacy/default first-start behavior for
         * callers that do not have an explicit import
         * handoff boundary.
         */
        startOffset =
            fileInfo.size();
    }

    m_state.startAtOffset(
        startOffset
        );

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            m_sourcePath
            );

    if (identityResult.succeeded) {
        m_sourceIdentity =
            identityResult.identity;

        m_hasSourceIdentity = true;
    }

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

    if (sourcePhysicalIdentityChanged(
            m_sourcePath,
            m_sourceIdentity
            )) {
        m_state.beginNextSourceGeneration();

        const SourcePhysicalIdentityCaptureResult
            identityResult =
            captureSourcePhysicalIdentity(
                m_sourcePath
                );

        if (identityResult.succeeded) {
            m_sourceIdentity =
                identityResult.identity;

            m_hasSourceIdentity = true;
        }

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

        const SourcePhysicalIdentityCaptureResult
            identityResult =
            captureSourcePhysicalIdentity(
                m_sourcePath
                );

        if (identityResult.succeeded) {
            m_sourceIdentity =
                identityResult.identity;

            m_hasSourceIdentity = true;
        }

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

    QFile file;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            file,
            m_sourcePath
            );

    if (!openResult.succeeded) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The live source could not be "
                "opened for reading: %1"
                )
                .arg(
                    openResult.errorMessage
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
    if (m_sourceIdentity.fingerprintLength == 0) {
        const SourcePhysicalIdentityCaptureResult
            identityResult =
            captureSourcePhysicalIdentity(
                m_sourcePath
                );

        if (identityResult.succeeded) {
            m_sourceIdentity =
                identityResult.identity;

            m_hasSourceIdentity = true;
        }
    }

    return result;
}