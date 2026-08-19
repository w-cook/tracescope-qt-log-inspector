#include <QtTest>

#include "../src/controllers/InvestigationController.h"

class InvestigationControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void setRecordsUpdatesModels();
    void filtersVisibleRecords();
    void filtersMultipleSeverities();
    void filtersMultipleSubsystems();
    void filtersByInclusiveTimeRange();
    void filtersMultipleEventCodes();
    void filtersMultipleEntities();
    void filtersMultipleCustomFieldValues();
    void combinesMultipleCustomFieldFilters();
    void customFieldFilterExcludesMissingValues();
    void combinesEventCodeAndEntityFilters();
    void returnsSortedSubsystems();
    void mapsSortedProxyIndexToSourceRecord();
    void timeRangeExcludesRecordsWithoutTimestamp();
    void navigatesVisibleIssuesInProxyOrder();
    void navigatesAdjacentVisibleEvents();
    void issueNavigationRespectsFiltering();
};

static QVector<InvestigationRecord> sampleRecords()
{
    InvestigationRecord startup;
    startup.recordId =
        QStringLiteral("record-startup");

    startup.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    startup.severity =
        RecordSeverity::Info;

    startup.subsystem =
        QStringLiteral("Startup");

    startup.eventCode =
        QStringLiteral("SESSION_START");

    startup.entityId =
        QStringLiteral("node-a");

    startup.message =
        QStringLiteral("Started");

    startup.customAttributes.insert(
        QStringLiteral("region"),
        QStringLiteral("east")
        );

    startup.customAttributes.insert(
        QStringLiteral("rack"),
        QStringLiteral("rack-1")
        );

    InvestigationRecord tracking;
    tracking.recordId =
        QStringLiteral("record-tracking");

    tracking.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:02:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    tracking.severity =
        RecordSeverity::Warning;

    tracking.subsystem =
        QStringLiteral("Tracking");

    tracking.eventCode =
        QStringLiteral("TRACK_LOST");

    tracking.entityId =
        QStringLiteral("target-42");

    tracking.message =
        QStringLiteral("Track lost");

    tracking.customAttributes.insert(
        QStringLiteral("region"),
        QStringLiteral("west")
        );

    tracking.customAttributes.insert(
        QStringLiteral("rack"),
        QStringLiteral("rack-1")
        );

    tracking.customAttributes.insert(
        QStringLiteral("ticket"),
        QStringLiteral("INC-42")
        );

    InvestigationRecord comms;
    comms.recordId =
        QStringLiteral("record-comms");

    comms.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:01:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    comms.severity =
        RecordSeverity::Error;

    comms.subsystem =
        QStringLiteral("Comms");

    comms.eventCode =
        QStringLiteral("PACKET_DROP");

    comms.entityId =
        QStringLiteral("node-a");

    comms.message =
        QStringLiteral("Packet loss");

    comms.customAttributes.insert(
        QStringLiteral("region"),
        QStringLiteral("east")
        );

    comms.customAttributes.insert(
        QStringLiteral("rack"),
        QStringLiteral("rack-2")
        );

    return {
        startup,
        tracking,
        comms
    };
}

void InvestigationControllerTests::
    setRecordsUpdatesModels()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    QCOMPARE(
        controller.totalRecordCount(),
        3
        );

    QCOMPARE(
        controller.sourceModel()->rowCount(),
        3
        );

    QCOMPARE(
        controller.proxyModel()->rowCount(),
        3
        );
}

void InvestigationControllerTests::
    filtersVisibleRecords()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setFilters(
        QStringLiteral("WARN"),
        QString(),
        QString()
        );

    const QVector<InvestigationRecord> records =
        controller.visibleRecords();

    QCOMPARE(records.size(), 1);

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-tracking")
        );
}

void InvestigationControllerTests::
    filtersMultipleSeverities()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setFilters(
        QStringList {
            QStringLiteral("warn"),
            QStringLiteral(" ERROR "),
            QStringLiteral("warn")
        },
        QString(),
        QString()
        );

    const QVector<InvestigationRecord> records =
        controller.visibleRecords();

    QCOMPARE(
        controller
            .proxyModel()
            ->severityFilters(),
        QStringList({
            QStringLiteral("WARN"),
            QStringLiteral("ERROR")
        })
        );

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-tracking")
        );

    QCOMPARE(
        records[1].recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationControllerTests::
    filtersMultipleSubsystems()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setFilters(
        QStringList(),
        QStringList {
            QStringLiteral(" Startup "),
            QStringLiteral("Comms"),
            QStringLiteral("Startup")
        },
        QString()
        );

    QCOMPARE(
        controller
            .proxyModel()
            ->subsystemFilters(),
        QStringList({
            QStringLiteral("Startup"),
            QStringLiteral("Comms")
        })
        );

    const QVector<InvestigationRecord> records =
        controller.recordsForAnalysis();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-startup")
        );

    QCOMPARE(
        records[1].recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationControllerTests::
    filtersByInclusiveTimeRange()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    const QDateTime startTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:01:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QDateTime endTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:02:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    controller.setTimeRangeFilter(
        startTime,
        endTime
        );

    const QVector<InvestigationRecord> records =
        controller.recordsForAnalysis();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-tracking")
        );

    QCOMPARE(
        records[1].recordId,
        QStringLiteral("record-comms")
        );

    controller.setTimeRangeFilter(
        std::nullopt,
        std::nullopt
        );

    QCOMPARE(
        controller
            .recordsForAnalysis()
            .size(),
        3
        );
}

void InvestigationControllerTests::
    filtersMultipleEventCodes()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setEventCodeFilters(
        QStringList {
            QStringLiteral(" TRACK_LOST "),
            QStringLiteral("PACKET_DROP"),
            QStringLiteral("TRACK_LOST")
        }
        );

    const QVector<InvestigationRecord> records =
        controller.recordsForAnalysis();

    QCOMPARE(
        controller
            .proxyModel()
            ->eventCodeFilters(),
        QStringList({
            QStringLiteral("TRACK_LOST"),
            QStringLiteral("PACKET_DROP")
        })
        );

    QCOMPARE(
        records.size(),
        2
        );
}

void InvestigationControllerTests::
    filtersMultipleEntities()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setEntityFilters(
        QStringList {
            QStringLiteral("node-a")
        }
        );

    const QVector<InvestigationRecord> records =
        controller.recordsForAnalysis();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-startup")
        );

    QCOMPARE(
        records[1].recordId,
        QStringLiteral("record-comms")
        );
}
void InvestigationControllerTests::
    filtersMultipleCustomFieldValues()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    CustomFieldFilterMap filters;

    filters.insert(
        QStringLiteral("region"),
        QStringList {
            QStringLiteral("east")
        }
        );

    controller.setCustomFieldFilters(
        filters
        );

    const QVector<InvestigationRecord>
        records =
        controller.recordsForAnalysis();

    QCOMPARE(records.size(), 2);

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-startup")
        );

    QCOMPARE(
        records[1].recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationControllerTests::
    combinesMultipleCustomFieldFilters()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    CustomFieldFilterMap filters;

    filters.insert(
        QStringLiteral("region"),
        QStringList {
            QStringLiteral("east"),
            QStringLiteral("west")
        }
        );

    filters.insert(
        QStringLiteral("rack"),
        QStringList {
            QStringLiteral("rack-2")
        }
        );

    controller.setCustomFieldFilters(
        filters
        );

    const QVector<InvestigationRecord>
        records =
        controller.recordsForAnalysis();

    QCOMPARE(records.size(), 1);

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationControllerTests::
    customFieldFilterExcludesMissingValues()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    CustomFieldFilterMap filters;

    filters.insert(
        QStringLiteral("ticket"),
        QStringList {
            QStringLiteral("INC-42")
        }
        );

    controller.setCustomFieldFilters(
        filters
        );

    const QVector<InvestigationRecord>
        records =
        controller.recordsForAnalysis();

    QCOMPARE(records.size(), 1);

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-tracking")
        );
}

void InvestigationControllerTests::
    combinesEventCodeAndEntityFilters()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setEventCodeFilters(
        QStringList {
            QStringLiteral("TRACK_LOST"),
            QStringLiteral("PACKET_DROP")
        }
        );

    controller.setEntityFilters(
        QStringList {
            QStringLiteral("node-a")
        }
        );

    const QVector<InvestigationRecord> records =
        controller.recordsForAnalysis();

    QCOMPARE(
        records.size(),
        1
        );

    QCOMPARE(
        records[0].recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationControllerTests::
    returnsSortedSubsystems()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    const QStringList subsystems =
        controller.availableSubsystems();

    QCOMPARE(
        subsystems,
        QStringList({
            QStringLiteral("Comms"),
            QStringLiteral("Startup"),
            QStringLiteral("Tracking")
        })
        );
}

void InvestigationControllerTests::
    mapsSortedProxyIndexToSourceRecord()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.proxyModel()->sort(
        0,
        Qt::DescendingOrder
        );

    const QModelIndex proxyIndex =
        controller.proxyModel()->index(
            0,
            0
            );

    const InvestigationRecord *record =
        controller.recordForProxyIndex(
            proxyIndex
            );

    QVERIFY(record != nullptr);

    QCOMPARE(
        record->recordId,
        QStringLiteral("record-tracking")
        );
}

void InvestigationControllerTests::
    timeRangeExcludesRecordsWithoutTimestamp()
{
    InvestigationController controller;

    QVector<InvestigationRecord> records =
        sampleRecords();

    InvestigationRecord untimed;
    untimed.recordId =
        QStringLiteral("record-untimed");
    untimed.message =
        QStringLiteral("No timestamp");

    records.append(
        untimed
        );

    controller.setRecords(
        records
        );

    const QDateTime startTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    controller.setTimeRangeFilter(
        startTime,
        std::nullopt
        );

    const QVector<InvestigationRecord> filtered =
        controller.recordsForAnalysis();

    QCOMPARE(
        filtered.size(),
        3
        );

    for (const InvestigationRecord &record
         : filtered) {
        QVERIFY(
            record.timestamp.has_value()
            );
    }
}

void InvestigationControllerTests::
    navigatesVisibleIssuesInProxyOrder()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    /*
     * Source/proxy order:
     *
     * 0 Startup   INFO
     * 1 Tracking  WARN
     * 2 Comms     ERROR
     */

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            -1,
            1
            ),
        1
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            -1,
            -1
            ),
        2
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            1,
            1
            ),
        2
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            2,
            -1
            ),
        1
        );

    /*
     * Navigation wraps in both directions.
     */
    QCOMPARE(
        controller.adjacentIssueProxyRow(
            2,
            1
            ),
        1
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            1,
            -1
            ),
        2
        );

    controller.proxyModel()->sort(
        0,
        Qt::DescendingOrder
        );

    /*
     * Timestamp-descending proxy order:
     *
     * Tracking 10:02 WARN
     * Comms    10:01 ERROR
     * Startup  10:00 INFO
     */
    QCOMPARE(
        controller.adjacentIssueProxyRow(
            -1,
            1
            ),
        0
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            0,
            1
            ),
        1
        );
}

void InvestigationControllerTests::
    navigatesAdjacentVisibleEvents()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    /*
     * Unfiltered proxy order:
     *
     * 0 Startup
     * 1 Tracking
     * 2 Comms
     *
     * With no current selection, forward
     * navigation starts at the first visible
     * event and backward navigation starts at
     * the last visible event.
     */
    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            1
            ),
        0
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            -1
            ),
        2
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            1,
            1
            ),
        2
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            1,
            -1
            ),
        0
        );

    /*
     * Ordinary adjacent-event navigation does
     * not wrap at either end.
     */
    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            0,
            -1
            ),
        -1
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            2,
            1
            ),
        -1
        );

    /*
     * Navigation operates on proxy rows, so
     * active filters reduce the navigable range
     * to only currently visible records.
     */
    controller.setFilters(
        QStringList {
            QStringLiteral("WARN"),
            QStringLiteral("ERROR")
        },
        QStringList(),
        QString()
        );

    QCOMPARE(
        controller.proxyModel()->rowCount(),
        2
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            1
            ),
        0
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            -1
            ),
        1
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            0,
            1
            ),
        1
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            1,
            -1
            ),
        0
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            0,
            -1
            ),
        -1
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            1,
            1
            ),
        -1
        );

    /*
     * No visible records means there is nowhere
     * to navigate in either direction.
     */
    controller.setFilters(
        QStringList(),
        QStringList(),
        QStringLiteral(
            "does-not-match-any-record"
            )
        );

    QCOMPARE(
        controller.proxyModel()->rowCount(),
        0
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            1
            ),
        -1
        );

    QCOMPARE(
        controller.adjacentVisibleProxyRow(
            -1,
            -1
            ),
        -1
        );
}

void InvestigationControllerTests::
    issueNavigationRespectsFiltering()
{
    InvestigationController controller;

    controller.setRecords(
        sampleRecords()
        );

    controller.setFilters(
        QStringList {
            QStringLiteral("ERROR")
        },
        QStringList(),
        QString()
        );

    QCOMPARE(
        controller.proxyModel()->rowCount(),
        1
        );

    /*
     * Only the visible ERROR remains navigable.
     */
    QCOMPARE(
        controller.adjacentIssueProxyRow(
            -1,
            1
            ),
        0
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            0,
            1
            ),
        0
        );

    /*
     * A filter containing no warning/error-class
     * records leaves nothing to navigate.
     */
    controller.setFilters(
        QStringList {
            QStringLiteral("INFO")
        },
        QStringList(),
        QString()
        );

    QCOMPARE(
        controller.proxyModel()->rowCount(),
        1
        );

    QCOMPARE(
        controller.adjacentIssueProxyRow(
            -1,
            1
            ),
        -1
        );
}

QTEST_MAIN(InvestigationControllerTests)

#include "InvestigationControllerTests.moc"