#include <QtTest/QtTest>

#include "../src/analysis/EventTimelineAnalyzer.h"

class EventTimelineAnalyzerTests : public QObject
{
    Q_OBJECT

private slots:
    void groupEventsByMinuteCountsEventsBySeverity();
    void groupEventsByMinuteSkipsInvalidTimestamps();
    void groupEventsByMinuteFillsMissingMinutes();
    void filteredEventsPreserveFullTimelineRange();
    void groupEventsByMinuteHandlesMidnightBoundary();
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

void EventTimelineAnalyzerTests::groupEventsByMinuteFillsMissingMinutes()
{
    const QVector<TelemetryEvent> events = {
        {
            "2026-07-07T11:02:15.000Z",
            "INFO",
            "Startup",
            "START",
            "Started",
            "SYS-1"
        },
        {
            "2026-07-07T11:05:20.000Z",
            "WARN",
            "Tracking",
            "LATE_EVENT",
            "Late event",
            "TRK-1"
        }
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupEventsByMinute(
            events
            );

    QCOMPARE(buckets.size(), 4);

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral("11:02")
        );
    QCOMPARE(
        buckets.at(1).label,
        QStringLiteral("11:03")
        );
    QCOMPARE(
        buckets.at(2).label,
        QStringLiteral("11:04")
        );
    QCOMPARE(
        buckets.at(3).label,
        QStringLiteral("11:05")
        );

    QCOMPARE(
        buckets.at(1).totalCount(),
        0
        );

    QCOMPARE(
        buckets.at(2).totalCount(),
        0
        );
}

void EventTimelineAnalyzerTests::filteredEventsPreserveFullTimelineRange()
{
    const QVector<TelemetryEvent>
        allEvents = {
            {
                "2026-07-07T11:02:15.000Z",
                "INFO",
                "Startup",
                "START",
                "Started",
                "SYS-1"
            },
            {
                "2026-07-07T11:03:25.000Z",
                "WARN",
                "Tracking",
                "WARNING",
                "Warning",
                "TRK-1"
            },
            {
                "2026-07-07T11:05:20.000Z",
                "INFO",
                "Shutdown",
                "STOP",
                "Stopped",
                "SYS-1"
            }
        };

    const QVector<TelemetryEvent>
        filteredEvents = {
            allEvents.at(1)
};

EventTimelineAnalyzer analyzer;

const auto buckets =
    analyzer.groupEventsByMinute(
        filteredEvents,
        allEvents
        );

QCOMPARE(buckets.size(), 4);

QCOMPARE(
    buckets.at(0).totalCount(),
    0
    );

QCOMPARE(
    buckets.at(1).warningCount,
    1
    );

QCOMPARE(
    buckets.at(2).totalCount(),
    0
    );

QCOMPARE(
    buckets.at(3).totalCount(),
    0
    );
}

void EventTimelineAnalyzerTests::groupEventsByMinuteHandlesMidnightBoundary()
{
    const QVector<TelemetryEvent> events = {
        {
            "2026-07-07T23:59:30.000Z",
            "INFO",
            "System",
            "BEFORE_MIDNIGHT",
            "Before midnight",
            "SYS-1"
        },
        {
            "2026-07-08T00:01:10.000Z",
            "ERROR",
            "System",
            "AFTER_MIDNIGHT",
            "After midnight",
            "SYS-1"
        }
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupEventsByMinute(
            events
            );

    QCOMPARE(buckets.size(), 3);

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral(
            "2026-07-07 23:59"
            )
        );

    QCOMPARE(
        buckets.at(1).label,
        QStringLiteral(
            "2026-07-08 00:00"
            )
        );

    QCOMPARE(
        buckets.at(2).label,
        QStringLiteral(
            "2026-07-08 00:01"
            )
        );

    QCOMPARE(
        buckets.at(1).totalCount(),
        0
        );
}

QTEST_MAIN(EventTimelineAnalyzerTests)

#include "EventTimelineAnalyzerTests.moc"