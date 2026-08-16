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
    void groupRecordsByFiveMinuteInterval();
    void groupRecordsByHundredMilliseconds();
    void windowedIntervalMaterializesOnlyRequestedBuckets();
    void millisecondBucketCountDoesNotMaterializeTimeline();
    void intervalGroupingPreservesEmptyBuckets();
    void intervalGroupingRejectsInvalidInterval();
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

void EventTimelineAnalyzerTests::
    groupRecordsByFiveMinuteInterval()
{
    const QVector<InvestigationRecord>
        records = {
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:01:00.000Z"
                    ),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:04:59.000Z"
                    ),
                RecordSeverity::Warning
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:05:00.000Z"
                    ),
                RecordSeverity::Error
                )
        };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByInterval(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-15T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-15T10:09:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            5
            );

    QCOMPARE(
        buckets.size(),
        2
        );

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral("10:00")
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
        buckets.at(1).label,
        QStringLiteral("10:05")
        );

    QCOMPARE(
        buckets.at(1).errorCount,
        1
        );
}

void EventTimelineAnalyzerTests::
    groupRecordsByHundredMilliseconds()
{
    const QVector<InvestigationRecord>
        records = {
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:00:00.015Z"
                    ),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:00:00.099Z"
                    ),
                RecordSeverity::Warning
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:00:00.100Z"
                    ),
                RecordSeverity::Error
                )
        };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer
            .groupRecordsByIntervalMilliseconds(
                records,
                QDateTime::fromString(
                    QStringLiteral(
                        "2026-08-15T10:00:00.000Z"
                        ),
                    Qt::ISODateWithMs
                    ),
                QDateTime::fromString(
                    QStringLiteral(
                        "2026-08-15T10:00:00.199Z"
                        ),
                    Qt::ISODateWithMs
                    ),
                100
                );

    QCOMPARE(
        buckets.size(),
        2
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
        buckets.at(1).errorCount,
        1
        );

    QCOMPARE(
        buckets.at(0).label,
        QStringLiteral(
            "10:00:00.000"
            )
        );

    QCOMPARE(
        buckets.at(1).label,
        QStringLiteral(
            "10:00:00.100"
            )
        );
}

void EventTimelineAnalyzerTests::
    windowedIntervalMaterializesOnlyRequestedBuckets()
{
    const QVector<InvestigationRecord>
        records = {
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:00:00.003Z"
                    ),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:00:05.007Z"
                    ),
                RecordSeverity::Error
                )
        };

    EventTimelineAnalyzer analyzer;

    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-15T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QDateTime lastTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-15T10:10:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const auto buckets =
        analyzer
            .groupRecordsByIntervalWindowMilliseconds(
                records,
                firstTimestamp,
                lastTimestamp,
                1,
                5000,
                20
                );

    QCOMPARE(
        buckets.size(),
        20
        );

    QCOMPARE(
        buckets.first().label,
        QStringLiteral(
            "10:00:05.000"
            )
        );

    QCOMPARE(
        buckets.at(7).errorCount,
        1
        );
}

void EventTimelineAnalyzerTests::
    millisecondBucketCountDoesNotMaterializeTimeline()
{
    EventTimelineAnalyzer analyzer;

    const QDateTime firstTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-15T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QDateTime lastTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-15T14:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const qint64 bucketCount =
        analyzer
            .intervalBucketCountMilliseconds(
                firstTimestamp,
                lastTimestamp,
                1
                );

    QCOMPARE(
        bucketCount,
        qint64(14'400'001)
        );
}

void EventTimelineAnalyzerTests::
    intervalGroupingPreservesEmptyBuckets()
{
    const QVector<InvestigationRecord>
        records = {
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:01:00.000Z"
                    ),
                RecordSeverity::Info
                ),
            makeRecord(
                QStringLiteral(
                    "2026-08-15T10:16:00.000Z"
                    ),
                RecordSeverity::Warning
                )
        };

    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByInterval(
            records,
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-15T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-15T10:19:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            5
            );

    QCOMPARE(
        buckets.size(),
        4
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
    intervalGroupingRejectsInvalidInterval()
{
    EventTimelineAnalyzer analyzer;

    const auto buckets =
        analyzer.groupRecordsByInterval(
            {},
            QDateTime::currentDateTimeUtc(),
            QDateTime::currentDateTimeUtc(),
            0
            );

    QVERIFY(
        buckets.isEmpty()
        );
}

QTEST_MAIN(EventTimelineAnalyzerTests)

#include "EventTimelineAnalyzerTests.moc"
