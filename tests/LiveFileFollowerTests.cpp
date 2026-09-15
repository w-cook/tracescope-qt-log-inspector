#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "../src/live/LiveFileFollower.h"

class LiveFileFollowerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void normalizesSourcePath();
    void startsAtCurrentFileEnd();
    void startCapturesSourceIdentity();
    void rejectsMissingSource();
    void lifecycleTransitionsEmitStateChanges();
    void startDoesNotReplacePauseOrResume();
    void restartAfterStopPreservesUnreadGrowth();
    void restartAfterStoppedTruncationBeginsNewGeneration();
    void pollingAtFileEndReportsNoChange();
    void pollingReportsGrowthWithoutConsumingIt();
    void pollingWhilePausedPreservesCatchUpRange();
    void pollingDetectsSourceTruncation();
    void missingSourceDoesNotDestroyFollowState();
    void detectsSamePathReplacementWithoutSizeRegression();
    void readsOnlyNewlyAppendedBytes();
    void boundedReadsDrainBacklogIncrementally();
    void pausedFollowerDoesNotConsumeAvailableBytes();
    void sourceResetReadBeginsAtNewGenerationStart();
    void rejectsInvalidReadChunkSize();
    void firstStartCanUseEarlierImportBoundary();
};

void LiveFileFollowerTests::
    normalizesSourcePath()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    LiveFileFollower follower(
        sourcePath
        );

    QCOMPARE(
        follower.sourcePath(),
        QFileInfo(sourcePath)
            .absoluteFilePath()
        );
}

void LiveFileFollowerTests::
    startsAtCurrentFileEnd()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(
            QByteArray("existing\n")
            ),
        qint64(9)
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    /*
     * Grow the file after follower construction.
     * start() must inspect the current file state,
     * not a stale constructor-time snapshot.
     */
    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(
            QByteArray("more\n")
            ),
        qint64(5)
        );

    file.close();

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        follower.state().readOffset(),
        QFileInfo(sourcePath).size()
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(14)
        );
}

void LiveFileFollowerTests::
    startCapturesSourceIdentity()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    const QByteArray content(
        "existing record one\n"
        "existing record two\n"
        );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const SourcePhysicalIdentity
        *identity =
        follower.sourceIdentity();

    QVERIFY(
        identity != nullptr
        );

    if (!identity) {
        return;
    }

    QCOMPARE(
        identity->observedSizeBytes,
        qint64(content.size())
        );

    QCOMPARE(
        identity->fingerprintLength,
        qint64(content.size())
        );

    QVERIFY(
        !identity
             ->prefixFingerprint
             .isEmpty()
        );
}

void LiveFileFollowerTests::
    rejectsMissingSource()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    LiveFileFollower follower(
        directory.filePath(
            QStringLiteral(
                "missing.log"
                )
            )
        );

    QSignalSpy stateSpy(
        &follower,
        &LiveFileFollower::stateChanged
        );

    QVERIFY(
        !follower.start()
        );

    QVERIFY(
        follower.state().isStopped()
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(0)
        );

    QCOMPARE(
        stateSpy.count(),
        0
        );
}

void LiveFileFollowerTests::
    lifecycleTransitionsEmitStateChanges()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QSignalSpy stateSpy(
        &follower,
        &LiveFileFollower::stateChanged
        );

    QVERIFY(
        follower.start()
        );

    QCOMPARE(
        stateSpy.count(),
        1
        );

    QVERIFY(
        follower.pause()
        );

    QVERIFY(
        follower.state().isPaused()
        );

    QCOMPARE(
        stateSpy.count(),
        2
        );

    QVERIFY(
        follower.resume()
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        stateSpy.count(),
        3
        );

    QVERIFY(
        follower.stop()
        );

    QVERIFY(
        follower.state().isStopped()
        );

    QCOMPARE(
        stateSpy.count(),
        4
        );

    /*
     * Invalid/no-op transitions do not produce
     * misleading UI state notifications.
     */
    QVERIFY(
        !follower.stop()
        );

    QCOMPARE(
        stateSpy.count(),
        4
        );
}

void LiveFileFollowerTests::
    startDoesNotReplacePauseOrResume()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 initialOffset =
        follower.state().readOffset();

    QVERIFY(
        !follower.start()
        );

    QCOMPARE(
        follower.state().readOffset(),
        initialOffset
        );

    QVERIFY(
        follower.pause()
        );

    QVERIFY(
        !follower.start()
        );

    QVERIFY(
        follower.state().isPaused()
        );

    QCOMPARE(
        follower.state().readOffset(),
        initialOffset
        );

    QVERIFY(
        follower.resume()
        );
}

void LiveFileFollowerTests::
    restartAfterStopPreservesUnreadGrowth()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    const QByteArray existingBytes(
        "existing\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(
            existingBytes
            ),
        qint64(
            existingBytes.size()
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 stoppedOffset =
        follower.state().readOffset();

    const quint64 stoppedGeneration =
        follower.state().sourceGeneration();

    QVERIFY(
        follower.stop()
        );

    const QByteArray stoppedGrowth(
        "arrived while stopped\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(
            stoppedGrowth
            ),
        qint64(
            stoppedGrowth.size()
            )
        );

    file.close();

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        follower.state().readOffset(),
        stoppedOffset
        );

    QCOMPARE(
        follower.state().sourceGeneration(),
        stoppedGeneration
        );

    const LiveFileReadResult readResult =
        follower.readAvailableBytes();

    QVERIFY2(
        readResult.succeeded,
        qPrintable(
            readResult.errorMessage
            )
        );

    QCOMPARE(
        readResult.observation.kind,
        LiveFileObservationKind::Appended
        );

    QCOMPARE(
        readResult.bytes,
        stoppedGrowth
        );

    QCOMPARE(
        follower.state().readOffset(),
        stoppedOffset
            + stoppedGrowth.size()
        );
}

void LiveFileFollowerTests::
    restartAfterStoppedTruncationBeginsNewGeneration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    const QByteArray originalBytes(
        "original-record-one\n"
        "original-record-two\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(
            originalBytes
            ),
        qint64(
            originalBytes.size()
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        follower.stop()
        );

    const QByteArray replacementBytes(
        "new\n"
        );

    QVERIFY(
        replacementBytes.size()
        < originalBytes.size()
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(
            replacementBytes
            ),
        qint64(
            replacementBytes.size()
            )
        );

    file.close();

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        follower.state().sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(0)
        );

    /*
     * The generation transition was detected during
     * Start. The new source contents are therefore
     * ordinary unread bytes when polling begins.
     */
    const LiveFileReadResult readResult =
        follower.readAvailableBytes();

    QVERIFY2(
        readResult.succeeded,
        qPrintable(
            readResult.errorMessage
            )
        );

    QCOMPARE(
        readResult.observation.kind,
        LiveFileObservationKind::Appended
        );

    QCOMPARE(
        readResult.bytes,
        replacementBytes
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(
            replacementBytes.size()
            )
        );
}

void LiveFileFollowerTests::
    pollingAtFileEndReportsNoChange()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::NoChange
        );

    QCOMPARE(
        observation.observedSizeBytes,
        startingOffset
        );

    QCOMPARE(
        observation.availableByteCount,
        qint64(0)
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );
}

void LiveFileFollowerTests::
    pollingReportsGrowthWithoutConsumingIt()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    const QByteArray appendedBytes(
        "first live record\n"
        "second live record\n"
        );

    QCOMPARE(
        file.write(
            appendedBytes
            ),
        qint64(
            appendedBytes.size()
            )
        );

    file.close();

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::Appended
        );

    QCOMPARE(
        observation.availableByteCount,
        qint64(
            appendedBytes.size()
            )
        );

    QCOMPARE(
        observation.observedSizeBytes,
        startingOffset
            + appendedBytes.size()
        );

    /*
     * Merely observing growth must never acknowledge
     * those bytes as successfully consumed.
     */
    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );

    /*
     * Until a future read/framing step consumes the
     * bytes, another poll must still expose them.
     */
    const LiveFileObservation repeated =
        follower.poll();

    QVERIFY(
        repeated.kind
        == LiveFileObservationKind::Appended
        );

    QCOMPARE(
        repeated.availableByteCount,
        qint64(
            appendedBytes.size()
            )
        );
}

void LiveFileFollowerTests::
    pollingWhilePausedPreservesCatchUpRange()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    QVERIFY(
        follower.pause()
        );

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    const QByteArray appendedBytes(
        "arrived while paused\n"
        );

    file.write(
        appendedBytes
        );

    file.close();

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::Appended
        );

    QCOMPARE(
        observation.availableByteCount,
        qint64(
            appendedBytes.size()
            )
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );

    QVERIFY(
        follower.state().isPaused()
        );
}

void LiveFileFollowerTests::
    pollingDetectsSourceTruncation()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray(
            "existing-record-one\n"
            "existing-record-two\n"
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        follower.state().readOffset()
        > 5
        );

    QSignalSpy stateSpy(
        &follower,
        &LiveFileFollower::stateChanged
        );

    /*
     * Replace the physical contents with a shorter
     * source at the same path.
     */
    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(
            QByteArray("new\n")
            ),
        qint64(4)
        );

    file.close();

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::SourceReset
        );

    QCOMPARE(
        observation.observedSizeBytes,
        qint64(4)
        );

    QCOMPARE(
        observation.availableByteCount,
        qint64(4)
        );

    QCOMPARE(
        follower.state().sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(0)
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        stateSpy.count(),
        1
        );
}

void LiveFileFollowerTests::
    missingSourceDoesNotDestroyFollowState()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    QVERIFY(
        QFile::remove(
            sourcePath
            )
        );

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::Missing
        );

    /*
     * Rotation/replacement may temporarily remove
     * the watched path. Do not silently convert that
     * filesystem condition into a user-requested Stop.
     */
    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );

    QCOMPARE(
        follower.state().sourceGeneration(),
        quint64(0)
        );
}

void LiveFileFollowerTests::
    detectsSamePathReplacementWithoutSizeRegression()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    const QByteArray originalBytes(
        "AAAAAAAAAAAAAAAA\n"
        "BBBBBBBBBBBBBBBB\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(
            originalBytes
            ),
        qint64(
            originalBytes.size()
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 originalOffset =
        follower.state().readOffset();

    /*
     * Remove and recreate the same path with content
     * that is deliberately larger than the old
     * consumed offset. Size regression therefore
     * cannot be the reason replacement is detected.
     */
    QVERIFY(
        QFile::remove(
            sourcePath
            )
        );

    const QByteArray replacementBytes(
        "REPLACEMENT-GENERATION\n"
        "with entirely different source bytes\n"
        "and enough content to exceed the prior size\n"
        );

    QVERIFY(
        replacementBytes.size()
        > originalOffset
        );

    file.setFileName(
        sourcePath
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(
            replacementBytes
            ),
        qint64(
            replacementBytes.size()
            )
        );

    file.close();

    QSignalSpy stateSpy(
        &follower,
        &LiveFileFollower::stateChanged
        );

    const LiveFileObservation observation =
        follower.poll();

    QVERIFY(
        observation.kind
        == LiveFileObservationKind::SourceReset
        );

    QCOMPARE(
        observation.observedSizeBytes,
        qint64(
            replacementBytes.size()
            )
        );

    QCOMPARE(
        observation.availableByteCount,
        qint64(
            replacementBytes.size()
            )
        );

    QCOMPARE(
        follower.state().sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(0)
        );

    QVERIFY(
        follower.state().isFollowing()
        );

    QCOMPARE(
        stateSpy.count(),
        1
        );
}

void LiveFileFollowerTests::
    readsOnlyNewlyAppendedBytes()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    const QByteArray appendedBytes(
        "first-live\n"
        "second-live\n"
        );

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(
            appendedBytes
            ),
        qint64(
            appendedBytes.size()
            )
        );

    file.close();

    const LiveFileReadResult result =
        follower.readAvailableBytes();

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.bytes,
        appendedBytes
        );

    QCOMPARE(
        result.observation.kind,
        LiveFileObservationKind::Appended
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
            + appendedBytes.size()
        );

    const LiveFileReadResult secondRead =
        follower.readAvailableBytes();

    QVERIFY(
        secondRead.succeeded
        );

    QVERIFY(
        secondRead.bytes.isEmpty()
        );

    QCOMPARE(
        secondRead.observation.kind,
        LiveFileObservationKind::NoChange
        );
}

void LiveFileFollowerTests::
    boundedReadsDrainBacklogIncrementally()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const QByteArray appendedBytes(
        "ABCDEFGHIJKL"
        );

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    file.write(
        appendedBytes
        );

    file.close();

    const LiveFileReadResult first =
        follower.readAvailableBytes(
            5
            );

    QVERIFY(first.succeeded);

    QCOMPARE(
        first.bytes,
        QByteArray("ABCDE")
        );

    QCOMPARE(
        first.observation
            .availableByteCount,
        qint64(12)
        );

    const LiveFileReadResult second =
        follower.readAvailableBytes(
            5
            );

    QVERIFY(second.succeeded);

    QCOMPARE(
        second.bytes,
        QByteArray("FGHIJ")
        );

    QCOMPARE(
        second.observation
            .availableByteCount,
        qint64(7)
        );

    const LiveFileReadResult third =
        follower.readAvailableBytes(
            5
            );

    QVERIFY(third.succeeded);

    QCOMPARE(
        third.bytes,
        QByteArray("KL")
        );

    QCOMPARE(
        third.observation
            .availableByteCount,
        qint64(2)
        );

    const LiveFileReadResult complete =
        follower.readAvailableBytes(
            5
            );

    QVERIFY(complete.succeeded);
    QVERIFY(complete.bytes.isEmpty());

    QCOMPARE(
        complete.observation.kind,
        LiveFileObservationKind::NoChange
        );
}

void LiveFileFollowerTests::
    pausedFollowerDoesNotConsumeAvailableBytes()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const qint64 startingOffset =
        follower.state().readOffset();

    QVERIFY(
        follower.pause()
        );

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    const QByteArray appendedBytes(
        "while-paused\n"
        );

    file.write(
        appendedBytes
        );

    file.close();

    const LiveFileReadResult pausedRead =
        follower.readAvailableBytes();

    QVERIFY(
        pausedRead.succeeded
        );

    QVERIFY(
        pausedRead.bytes.isEmpty()
        );

    QCOMPARE(
        pausedRead.observation.kind,
        LiveFileObservationKind::Appended
        );

    QCOMPARE(
        pausedRead.observation
            .availableByteCount,
        qint64(
            appendedBytes.size()
            )
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );

    QVERIFY(
        follower.resume()
        );

    const LiveFileReadResult resumedRead =
        follower.readAvailableBytes();

    QVERIFY(
        resumedRead.succeeded
        );

    QCOMPARE(
        resumedRead.bytes,
        appendedBytes
        );
}

void LiveFileFollowerTests::
    sourceResetReadBeginsAtNewGenerationStart()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray(
            "old-record-one\n"
            "old-record-two\n"
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    const QByteArray replacement(
        "new\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    file.write(
        replacement
        );

    file.close();

    const LiveFileReadResult result =
        follower.readAvailableBytes();

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.observation.kind,
        LiveFileObservationKind::SourceReset
        );

    QCOMPARE(
        follower.state()
            .sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        result.bytes,
        replacement
        );

    QCOMPARE(
        follower.state().readOffset(),
        qint64(
            replacement.size()
            )
        );
}

void LiveFileFollowerTests::
    rejectsInvalidReadChunkSize()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("live.log")
            );

    QFile file(sourcePath);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    file.write(
        QByteArray("existing\n")
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start()
        );

    QVERIFY(
        file.open(
            QIODevice::Append
            )
        );

    file.write(
        QByteArray("new\n")
        );

    file.close();

    const qint64 startingOffset =
        follower.state().readOffset();

    const LiveFileReadResult result =
        follower.readAvailableBytes(
            0
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        result.bytes.isEmpty()
        );

    QCOMPARE(
        follower.state().readOffset(),
        startingOffset
        );
}

void LiveFileFollowerTests::
    firstStartCanUseEarlierImportBoundary()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    QFile file(sourcePath);

    const QByteArray importedBytes(
        "first\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    QCOMPARE(
        file.write(
            importedBytes
            ),
        qint64(
            importedBytes.size()
            )
        );

    file.close();

    const qint64 importBoundary =
        importedBytes.size();

    const QByteArray growthBeforeStart(
        "second\n"
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(
            growthBeforeStart
            ),
        qint64(
            growthBeforeStart.size()
            )
        );

    file.close();

    LiveFileFollower follower(
        sourcePath
        );

    QVERIFY(
        follower.start(
            importBoundary
            )
        );

    QCOMPARE(
        follower.state().readOffset(),
        importBoundary
        );

    const LiveFileReadResult readResult =
        follower.readAvailableBytes();

    QVERIFY2(
        readResult.succeeded,
        qPrintable(
            readResult.errorMessage
            )
        );

    QCOMPARE(
        readResult.bytes,
        growthBeforeStart
        );
}

QTEST_MAIN(LiveFileFollowerTests)

#include "LiveFileFollowerTests.moc"