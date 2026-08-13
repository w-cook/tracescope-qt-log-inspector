#include <QtTest/QtTest>

#include "../src/analysis/TelemetryIssueAnalyzer.h"

class IssueAnalyzerTests : public QObject
{
    Q_OBJECT

private slots:
    void groupWarningsAndErrorsBySubsystemIgnoresInfoEvents();
    void groupWarningsAndErrorsBySubsystemCountsWarningsAndErrors();
    void groupWarningsAndErrorsBySubsystemSortsByTotalDescending();
    void groupWarningsAndErrorsBySubsystemCountsCriticalAsError();
};

static QVector<TelemetryEvent> sampleEvents()
{
    return {
        {
            "2026-07-07T10:14:22.381Z",
            "INFO",
            "Startup",
            "SESSION_START",
            "Telemetry session initialized",
            "SYS-001"
        },
        {
            "2026-07-07T10:14:23.014Z",
            "WARN",
            "Tracking",
            "TRACK_LOST",
            "Track 402 lost for 1200ms",
            "TRK-402"
        },
        {
            "2026-07-07T10:14:24.219Z",
            "ERROR",
            "Comms",
            "PACKET_DROP",
            "Packet loss exceeded threshold",
            "LINK-A"
        },
        {
            "2026-07-07T10:14:29.102Z",
            "WARN",
            "Comms",
            "LATENCY_SPIKE",
            "Link latency exceeded threshold",
            "LINK-A"
        }
    };
}

void IssueAnalyzerTests::groupWarningsAndErrorsBySubsystemIgnoresInfoEvents()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups = analyzer.groupWarningsAndErrorsBySubsystem(sampleEvents());

    QCOMPARE(groups.size(), 2);
}

void IssueAnalyzerTests::groupWarningsAndErrorsBySubsystemCountsWarningsAndErrors()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups = analyzer.groupWarningsAndErrorsBySubsystem(sampleEvents());

    const auto commsGroup = std::find_if(
        groups.begin(),
        groups.end(),
        [](const TelemetryIssueGroup &group) {
            return group.subsystem == "Comms";
        }
        );

    QVERIFY(commsGroup != groups.end());
    QCOMPARE(commsGroup->warningCount, 1);
    QCOMPARE(commsGroup->errorCount, 1);
    QCOMPARE(commsGroup->totalCount(), 2);
}

void IssueAnalyzerTests::groupWarningsAndErrorsBySubsystemSortsByTotalDescending()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups = analyzer.groupWarningsAndErrorsBySubsystem(sampleEvents());

    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups[0].subsystem, QString("Comms"));
    QCOMPARE(groups[0].totalCount(), 2);
    QCOMPARE(groups[1].subsystem, QString("Tracking"));
    QCOMPARE(groups[1].totalCount(), 1);
}

void IssueAnalyzerTests::groupWarningsAndErrorsBySubsystemCountsCriticalAsError()
{
    const QVector<TelemetryEvent> events {
        {
            "2026-08-12T08:24:16.771Z",
            "CRITICAL",
            "TelemetryPipeline",
            "QUEUE_UNAVAILABLE",
            "Telemetry queue is unavailable",
            "telemetry-worker"
        }
    };

    TelemetryIssueAnalyzer analyzer;

    const auto groups =
        analyzer.groupWarningsAndErrorsBySubsystem(
            events
            );

    QCOMPARE(groups.size(), 1);

    QCOMPARE(
        groups.first().subsystem,
        QString("TelemetryPipeline")
        );

    QCOMPARE(
        groups.first().warningCount,
        0
        );

    QCOMPARE(
        groups.first().errorCount,
        1
        );

    QCOMPARE(
        groups.first().totalCount(),
        1
        );
}

QTEST_MAIN(IssueAnalyzerTests)

#include "IssueAnalyzerTests.moc"