#include <QtTest/QtTest>

#include "../src/analysis/EventTimelineAnalyzer.h"

class EventTimelineAnalyzerTests : public QObject
{
    Q_OBJECT

private slots:
    void groupEventsByMinuteCountsEventsBySeverity();
    void groupEventsByMinuteSkipsInvalidTimestamps();
};

void EventTimelineAnalyzerTests::groupEventsByMinuteCountsEventsBySeverity()
{
    QVector<TelemetryEvent> events = {
        {
            "2026-07-07T11:02:01.104Z",
            "INFO",
            "Startup",
            "SESSION_START",
            "Session started",
            "SYS-204"
        },
        {
            "2026-07-07T11:02:10.447Z",
            "WARN",
            "Comms",
            "LATENCY_SPIKE",
            "Link latency exceeded threshold",
            "LINK-A"
        },
        {
            "2026-07-07T11:02:11.203Z",
            "ERROR",
            "Comms",
            "PACKET_DROP",
            "Packet loss exceeded threshold",
            "LINK-A"
        },
        {
            "2026-07-07T11:03:01.000Z",
            "WARN",
            "Tracking",
            "TRACK_JITTER",
            "Track jitter detected",
            "TRK-118"
        }
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets = analyzer.groupEventsByMinute(events);

    QCOMPARE(buckets.size(), 2);

    QCOMPARE(buckets[0].label, QString("11:02"));
    QCOMPARE(buckets[0].infoCount, 1);
    QCOMPARE(buckets[0].warningCount, 1);
    QCOMPARE(buckets[0].errorCount, 1);
    QCOMPARE(buckets[0].totalCount(), 3);

    QCOMPARE(buckets[1].label, QString("11:03"));
    QCOMPARE(buckets[1].infoCount, 0);
    QCOMPARE(buckets[1].warningCount, 1);
    QCOMPARE(buckets[1].errorCount, 0);
    QCOMPARE(buckets[1].totalCount(), 1);
}

void EventTimelineAnalyzerTests::groupEventsByMinuteSkipsInvalidTimestamps()
{
    QVector<TelemetryEvent> events = {
        {
            "not-a-timestamp",
            "WARN",
            "Tracking",
            "TRACK_JITTER",
            "Track jitter detected",
            "TRK-118"
        },
        {
            "2026-07-07T11:04:01.000Z",
            "ERROR",
            "Comms",
            "PACKET_DROP",
            "Packet loss exceeded threshold",
            "LINK-A"
        }
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets = analyzer.groupEventsByMinute(events);

    QCOMPARE(buckets.size(), 1);
    QCOMPARE(buckets[0].label, QString("11:04"));
    QCOMPARE(buckets[0].errorCount, 1);
}

QTEST_MAIN(EventTimelineAnalyzerTests)

#include "EventTimelineAnalyzerTests.moc"