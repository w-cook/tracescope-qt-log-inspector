#pragma once

#include <QByteArray>
#include <QtTypes>

enum class LiveFileFollowStatus
{
    Stopped,
    Following,
    Paused
};

class LiveFileFollowState
{
public:
    LiveFileFollowStatus status() const;

    bool isStopped() const;
    bool isFollowing() const;
    bool isPaused() const;

    qint64 readOffset() const;
    quint64 sourceGeneration() const;

    const QByteArray &pendingBytes() const;

    void startAtOffset(
        qint64 offset
        );

    bool pause();
    bool resume();

    void stop();

    bool advanceReadOffset(
        qint64 byteCount
        );

    void setPendingBytes(
        QByteArray bytes
        );

    void beginNextSourceGeneration();

private:
    LiveFileFollowStatus m_status =
        LiveFileFollowStatus::Stopped;

    qint64 m_readOffset = 0;

    quint64 m_sourceGeneration = 0;

    QByteArray m_pendingBytes;
};