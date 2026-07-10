#include <QtTest/QtTest>

#include "../src/filtering/TelemetryEventFilter.h"

class FilterTests : public QObject
{
    Q_OBJECT

private slots:
    void applyReturnsAllEventsWhenCriteriaIsEmpty();
    void applyFiltersByLevel();
    void applyFiltersBySubsystem();
    void applySearchesAcrossEventFields();
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
        }
    };
}

void FilterTests::applyReturnsAllEventsWhenCriteriaIsEmpty()
{
    TelemetryEventFilter filter;
    TelemetryFilterCriteria criteria;

    const auto filtered = filter.apply(sampleEvents(), criteria);

    QCOMPARE(filtered.size(), 3);
}

void FilterTests::applyFiltersByLevel()
{
    TelemetryEventFilter filter;

    TelemetryFilterCriteria criteria;
    criteria.level = "WARN";

    const auto filtered = filter.apply(sampleEvents(), criteria);

    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered[0].eventCode, QString("TRACK_LOST"));
}

void FilterTests::applyFiltersBySubsystem()
{
    TelemetryEventFilter filter;

    TelemetryFilterCriteria criteria;
    criteria.subsystem = "Comms";

    const auto filtered = filter.apply(sampleEvents(), criteria);

    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered[0].level, QString("ERROR"));
}

void FilterTests::applySearchesAcrossEventFields()
{
    TelemetryEventFilter filter;

    TelemetryFilterCriteria criteria;
    criteria.searchText = "402";

    const auto filtered = filter.apply(sampleEvents(), criteria);

    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered[0].entityId, QString("TRK-402"));
}

QTEST_MAIN(FilterTests)

#include "FilterTests.moc"