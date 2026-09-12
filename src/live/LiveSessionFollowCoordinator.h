#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "LiveFileFollower.h"
#include "LiveLineImportAdapter.h"
#include "LiveLineSourceContext.h"
#include "LiveStructuredJsonImportAdapter.h"
#include "LiveStructuredJsonSourceContext.h"

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

    inline static constexpr int
        DefaultPollIntervalMilliseconds = 250;

    int pollIntervalMilliseconds() const;

    bool setPollIntervalMilliseconds(
        int intervalMilliseconds
        );

signals:
    void stateChanged();

    void sessionUpdated();

    void sourceGenerationChanged(
        quint64 sourceGeneration
        );

    void pollError(
        const QString &errorMessage
        );

private:
    enum class IngestionKind
    {
        Unsupported,
        Line,
        StructuredJson
    };

    static IngestionKind ingestionKindForProfile(
        const ImportProfile &profile
        );

    void pollFromTimer();

    InvestigationSession *m_session = nullptr;

    LiveFileFollower m_follower;

    LiveLineSourceContext
        m_lineSourceContext;

    LiveLineImportAdapter
        m_lineImportAdapter;

    LiveStructuredJsonSourceContext
        m_structuredJsonSourceContext;

    LiveStructuredJsonImportAdapter
        m_structuredJsonImportAdapter;

    IngestionKind m_ingestionKind =
        IngestionKind::Unsupported;

    QTimer m_pollTimer;
};