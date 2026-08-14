#include <QtTest/QtTest>

#include <optional>

#include <QDateTime>

#include "../src/analysis/EventTimelineAnalyzer.h"
#include "../src/domain/RecordSeverity.h"

class EventTimelineAnalyzerTests : public QObject
{
    Q_OBJECT

private slots:
    void groupRecordsByMinuteCountsRecordsBySeverity();
    void groupRecordsByMinuteSkipsInvalidTimestamps();
    void groupRecordsByMinuteFillsMissingMinutes();
    void filteredRecordsPreserveFullTimelineRange();
    void groupRecordsByMinuteHandlesMidnightBoundary();
    void groupRecordsByMinuteCountsUnspecifiedSeverity();
};

static InvestigationRecord makeRecord(
    const QString &timestamp,
    std::optional<RecordSeverity> severity =
        std::nullopt
    )
{
    InvestigationRecord record;

    record.timestamp =
        QDateTime::fromString(
            timestamp,
            Qt::ISODateWithMs
            );

    record.severity = severity;

    return record;
}

void EventTimelineAnalyzerTests::
    groupRecordsByMinuteCountsRecordsBySeverity()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:00.100Z"
                ),
            RecordSeverity::Trace
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:00.500Z"
                ),
            RecordSeverity::Debug
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:01.104Z"
                ),
            RecordSeverity::Info
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:10.447Z"
                ),
            RecordSeverity::Warning
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:11.203Z"
                ),
            RecordSeverity::Error
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:12.000Z"
                ),
            RecordSeverity::Critical
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:03:01.000Z"
                ),
            RecordSeverity::Warning
            )
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            records
            );

    QCOMPARE(
        buckets.size(),
        2
        );

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral("11:02")
        );

    QCOMPARE(
        buckets.at(0).traceCount,
        1
        );

    QCOMPARE(
        buckets.at(0).debugCount,
        1
        );

    QCOMPARE(
        buckets.at(0).infoCount,
        1
        );

    QCOMPARE(
        buckets.at(0).warningCount,
        1
        );

    QCOMPARE(
        buckets.at(0).errorCount,
        1
        );

    QCOMPARE(
        buckets.at(0).criticalCount,
        1
        );

    QCOMPARE(
        buckets.at(0).totalCount(),
        6
        );

    QCOMPARE(
        buckets.at(1).label,
        QStringLiteral("11:03")
        );

    QCOMPARE(
        buckets.at(1).traceCount,
        0
        );

    QCOMPARE(
        buckets.at(1).debugCount,
        0
        );

    QCOMPARE(
        buckets.at(1).infoCount,
        0
        );

    QCOMPARE(
        buckets.at(1).warningCount,
        1
        );

    QCOMPARE(
        buckets.at(1).errorCount,
        0
        );

    QCOMPARE(
        buckets.at(1).criticalCount,
        0
        );

    QCOMPARE(
        buckets.at(1).totalCount(),
        1
        );
}

void EventTimelineAnalyzerTests::
    groupRecordsByMinuteSkipsInvalidTimestamps()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            QStringLiteral("not-a-timestamp"),
            RecordSeverity::Warning
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:04:01.000Z"
                ),
            RecordSeverity::Error
            )
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            records
            );

    QCOMPARE(
        buckets.size(),
        1
        );

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral("11:04")
        );

    QCOMPARE(
        buckets.at(0).errorCount,
        1
        );
}

void EventTimelineAnalyzerTests::
    groupRecordsByMinuteFillsMissingMinutes()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:02:15.000Z"
                ),
            RecordSeverity::Info
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-07T11:05:20.000Z"
                ),
            RecordSeverity::Warning
            )
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            records
            );

    QCOMPARE(
        buckets.size(),
        4
        );

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

void EventTimelineAnalyzerTests::
    filteredRecordsPreserveFullTimelineRange()
{
    const QVector<InvestigationRecord>
        allRecords = {
            makeRecord(
                QStringLiteral(
                    "2026-07-07T11:02:15.000Z"
                    ),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral(
                    "2026-07-07T11:03:25.000Z"
                    ),
                RecordSeverity::Warning
                ),
            makeRecord(
                QStringLiteral(
                    "2026-07-07T11:05:20.000Z"
                    ),
                RecordSeverity::Info
                )
        };

    const QVector<InvestigationRecord>
        filteredRecords = {
            allRecords.at(1)
        };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            filteredRecords,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-07-07T11:02:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-07-07T11:05:00.000Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QCOMPARE(
        buckets.size(),
        4
        );

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

void EventTimelineAnalyzerTests::
    groupRecordsByMinuteHandlesMidnightBoundary()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            QStringLiteral(
                "2026-07-07T23:59:30.000Z"
                ),
            RecordSeverity::Info
            ),
        makeRecord(
            QStringLiteral(
                "2026-07-08T00:01:10.000Z"
                ),
            RecordSeverity::Error
            )
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            records
            );

    QCOMPARE(
        buckets.size(),
        3
        );

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

void EventTimelineAnalyzerTests::
    groupRecordsByMinuteCountsUnspecifiedSeverity()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            QStringLiteral(
                "2026-08-12T08:20:00.000Z"
                )
            ),
        makeRecord(
            QStringLiteral(
                "2026-08-12T08:20:15.000Z"
                ),
            RecordSeverity::Info
            ),
        makeRecord(
            QStringLiteral(
                "2026-08-12T08:20:30.000Z"
                )
            )
    };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByMinute(
            records
            );

    QCOMPARE(
        buckets.size(),
        1
        );

    QCOMPARE(
        buckets.first().infoCount,
        1
        );

    QCOMPARE(
        buckets.first().unspecifiedCount,
        2
        );

    QCOMPARE(
        buckets.first().totalCount(),
        3
        );
}

QTEST_MAIN(EventTimelineAnalyzerTests)

#include "EventTimelineAnalyzerTests.moc"
