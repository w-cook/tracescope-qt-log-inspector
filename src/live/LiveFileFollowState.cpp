#include "LiveFileFollowState.h"

#include <algorithm>
#include <utility>

LiveFileFollowStatus
LiveFileFollowState::status() const
{
    return m_status;
}

bool LiveFileFollowState::isStopped() const
{
    return m_status
           == LiveFileFollowStatus::Stopped;
}

bool LiveFileFollowState::isFollowing() const
{
    return m_status
           == LiveFileFollowStatus::Following;
}

bool LiveFileFollowState::isPaused() const
{
    return m_status
           == LiveFileFollowStatus::Paused;
}

qint64 LiveFileFollowState::readOffset() const
{
    return m_readOffset;
}

quint64
LiveFileFollowState::sourceGeneration() const
{
    return m_sourceGeneration;
}

const QByteArray &
LiveFileFollowState::pendingBytes() const
{
    return m_pendingBytes;
}

void LiveFileFollowState::startAtOffset(
    qint64 offset
    )
{
    m_readOffset =
        std::max<qint64>(
            0,
            offset
            );

    m_pendingBytes.clear();

    m_status =
        LiveFileFollowStatus::Following;
}

bool LiveFileFollowState::pause()
{
    if (m_status
        != LiveFileFollowStatus::Following) {
        return false;
    }

    m_status =
        LiveFileFollowStatus::Paused;

    return true;
}

bool LiveFileFollowState::resume()
{
    if (m_status
        != LiveFileFollowStatus::Paused) {
        return false;
    }

    m_status =
        LiveFileFollowStatus::Following;

    return true;
}

void LiveFileFollowState::stop()
{
    m_status =
        LiveFileFollowStatus::Stopped;

    /*
     * Pause retains incomplete source data because
     * following will resume from the same cursor.
     *
     * Stop ends the current following episode, so
     * an incomplete physical record must not leak
     * into a later start operation.
     */
    m_pendingBytes.clear();
}

bool LiveFileFollowState::advanceReadOffset(
    qint64 byteCount
    )
{
    /*
     * A paused follower deliberately does not
     * consume source bytes. This leaves new file
     * content available for catch-up on resume.
     */
    if (m_status
            != LiveFileFollowStatus::Following
        || byteCount <= 0) {
        return false;
    }

    m_readOffset +=
        byteCount;

    return true;
}

void LiveFileFollowState::setPendingBytes(
    QByteArray bytes
    )
{
    m_pendingBytes =
        std::move(bytes);
}

void LiveFileFollowState::
    beginNextSourceGeneration()
{
    ++m_sourceGeneration;

    /*
     * A replacement/truncation begins a new logical
     * source stream. Physical positions therefore
     * restart at the beginning and incomplete bytes
     * from the previous generation are invalid.
     */
    m_readOffset = 0;
    m_pendingBytes.clear();
}