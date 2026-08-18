#include <QtTest>

#include "../src/controllers/InvestigationController.h"

class InvestigationControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void setRecordsUpdatesModels();
    void filtersVisibleRecords();
    void filtersMultipleSeverities();
    void returnsSortedSubsystems();
    void mapsSortedProxyIndexToSourceRecord();
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
    startup.message =
        QStringLiteral("Started");

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
    tracking.message =
        QStringLiteral("Track lost");

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
    comms.message =
        QStringLiteral("Packet loss");

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

QTEST_MAIN(InvestigationControllerTests)

#include "InvestigationControllerTests.moc"