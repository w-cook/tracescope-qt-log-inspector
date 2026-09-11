#pragma once

#include <QObject>
#include <QString>

#include "LiveFileFollower.h"
#include "LiveLineImportAdapter.h"
#include "LiveLineSourceContext.h"

class InvestigationSession;

struct LiveSessionFollowStartResult
{
    bool succeeded = true;
    QString errorMessage;

    qint64 baselineByteCount = 0;
};

struct LiveSessionFollowPollResult
{
    bool succeeded = true;
    QString errorMessage;

    LiveFileObservation observation;

    qint64 processedRecordCount = 0;
    qint64 importedRecordCount = 0;

    bool sourceGenerationChanged = false;
};

class LiveSessionFollowCoordinator
    : public QObject
{
    Q_OBJECT

public:
    explicit LiveSessionFollowCoordinator(
        InvestigationSession &session,
        QObject *parent = nullptr
        );

    static bool supportsProfile(
        const ImportProfile &profile
        );

    bool isSupported() const;

    const LiveFileFollowState &state() const;

    LiveSessionFollowStartResult start();

    bool pause();
    bool resume();
    bool stop();

    LiveSessionFollowPollResult pollOnce(
        qint64 maxByteCount =
        LiveFileFollower::
        DefaultReadChunkSizeBytes
        );

private:
    InvestigationSession *m_session = nullptr;

    LiveFileFollower m_follower;
    LiveLineSourceContext m_sourceContext;
    LiveLineImportAdapter m_importAdapter;
};