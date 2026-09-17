#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QString>

#include "../sources/SourcePhysicalIdentity.h"

#include "LiveFileFollowState.h"

enum class LiveFileObservationKind
{
    Inactive,
    NoChange,
    Appended,
    SourceReset,
    Missing
};

struct LiveFileObservation
{
    LiveFileObservationKind kind =
        LiveFileObservationKind::Inactive;

    qint64 observedSizeBytes = 0;
    qint64 availableByteCount = 0;
};

struct LiveFileReadResult
{
    LiveFileObservation observation;

    QByteArray bytes;

    bool succeeded = true;
    QString errorMessage;
};

class LiveFileFollower : public QObject
{
    Q_OBJECT

public:
    explicit LiveFileFollower(
        QString sourcePath,
        QObject *parent = nullptr
        );

    LiveFileFollower(
        QString sourcePath,
        quint64 initialSourceGeneration,
        QObject *parent
        );

    const QString &sourcePath() const;

    const SourcePhysicalIdentity *
    sourceIdentity() const;

    const LiveFileFollowState &state() const;

    bool start(
        qint64 initialReadOffset = -1
        );
    bool pause();
    bool resume();
    bool stop();

    LiveFileObservation poll();

    inline static constexpr qint64
        DefaultReadChunkSizeBytes =
        256 * 1024;

    LiveFileReadResult readAvailableBytes(
        qint64 maxByteCount =
        DefaultReadChunkSizeBytes
        );

signals:
    void stateChanged();

private:
    QString m_sourcePath;

    bool m_hasSourceIdentity = false;

    SourcePhysicalIdentity
        m_sourceIdentity;

    LiveFileFollowState m_state;
};