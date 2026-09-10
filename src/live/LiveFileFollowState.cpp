#include "LiveFileFollowState.h"

#include <algorithm>

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

void LiveFileFollowState::startAtOffset(
    qint64 offset
    )
{
    m_readOffset =
        std::max<qint64>(
            0,
            offset
            );

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
}