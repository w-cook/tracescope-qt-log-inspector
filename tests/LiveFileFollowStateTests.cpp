#include <QtTest>

#include "../src/live/LiveFileFollowState.h"

class LiveFileFollowStateTests
    : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToStoppedInitialGeneration();
    void startsAtExistingFileEnd();
    void pauseAndResumePreserveCatchUpState();
    void stopEndsCurrentFollowEpisode();
    void nextGenerationResetsPhysicalSourceState();
};

void LiveFileFollowStateTests::
    defaultsToStoppedInitialGeneration()
{
    LiveFileFollowState state;

    QVERIFY(
        state.isStopped()
        );

    QVERIFY(
        !state.isFollowing()
        );

    QVERIFY(
        !state.isPaused()
        );

    QCOMPARE(
        state.status(),
        LiveFileFollowStatus::Stopped
        );

    QCOMPARE(
        state.readOffset(),
        qint64(0)
        );

    QCOMPARE(
        state.sourceGeneration(),
        quint64(0)
        );
}

void LiveFileFollowStateTests::
    startsAtExistingFileEnd()
{
    LiveFileFollowState state;

    state.startAtOffset(
        4096
        );

    QVERIFY(
        state.isFollowing()
        );

    QCOMPARE(
        state.readOffset(),
        qint64(4096)
        );

    QCOMPARE(
        state.sourceGeneration(),
        quint64(0)
        );

    QVERIFY(
        state.advanceReadOffset(
            128
            )
        );

    QCOMPARE(
        state.readOffset(),
        qint64(4224)
        );
}

void LiveFileFollowStateTests::
    pauseAndResumePreserveCatchUpState()
{
    LiveFileFollowState state;

    state.startAtOffset(
        100
        );

    QVERIFY(
        state.advanceReadOffset(
            25
            )
        );

    QVERIFY(
        state.pause()
        );

    QVERIFY(
        state.isPaused()
        );

    QCOMPARE(
        state.readOffset(),
        qint64(125)
        );

    /*
     * Physical growth while paused must not move
     * TraceScope's consumed-source cursor.
     */
    QVERIFY(
        !state.advanceReadOffset(
            50
            )
        );

    QCOMPARE(
        state.readOffset(),
        qint64(125)
        );

    QVERIFY(
        state.resume()
        );

    QVERIFY(
        state.isFollowing()
        );

    /*
     * Resume continues from the same consumed-source
     * position so accumulated source growth can be
     * caught up rather than skipped.
     */
    QCOMPARE(
        state.readOffset(),
        qint64(125)
        );

    QVERIFY(
        state.advanceReadOffset(
            50
            )
        );

    QCOMPARE(
        state.readOffset(),
        qint64(175)
        );
}

void LiveFileFollowStateTests::
    stopEndsCurrentFollowEpisode()
{
    LiveFileFollowState state;

    state.startAtOffset(
        250
        );

    state.beginNextSourceGeneration();

    QCOMPARE(
        state.sourceGeneration(),
        quint64(1)
        );

    QVERIFY(
        state.advanceReadOffset(
            30
            )
        );

    state.stop();

    QVERIFY(
        state.isStopped()
        );

    QCOMPARE(
        state.readOffset(),
        qint64(30)
        );

    QCOMPARE(
        state.sourceGeneration(),
        quint64(1)
        );

    /*
     * Stopped state must not consume additional
     * physical source bytes.
     */
    QVERIFY(
        !state.advanceReadOffset(
            10
            )
        );

    QCOMPARE(
        state.readOffset(),
        qint64(30)
        );
}

void LiveFileFollowStateTests::
    nextGenerationResetsPhysicalSourceState()
{
    LiveFileFollowState state;

    state.startAtOffset(
        800
        );

    QVERIFY(
        state.advanceReadOffset(
            50
            )
        );

    state.beginNextSourceGeneration();

    /*
     * A physical source reset changes generation
     * and restarts its byte cursor, but does not
     * override the user's active-follow intent.
     */
    QVERIFY(
        state.isFollowing()
        );

    QCOMPARE(
        state.sourceGeneration(),
        quint64(1)
        );

    QCOMPARE(
        state.readOffset(),
        qint64(0)
        );

    state.beginNextSourceGeneration();

    QCOMPARE(
        state.sourceGeneration(),
        quint64(2)
        );

    QCOMPARE(
        state.readOffset(),
        qint64(0)
        );

    QVERIFY(
        state.isFollowing()
        );
}

QTEST_MAIN(LiveFileFollowStateTests)

#include "LiveFileFollowStateTests.moc"