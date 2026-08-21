#include <QtTest>
#include <QSignalSpy>

#include "../src/models/InvestigationFilterProxyModel.h"
#include "../src/models/InvestigationTableModel.h"

class InvestigationFilterProxyModelTests : public QObject
{
    Q_OBJECT

private slots:
    void emptyFiltersExposeAllRecords();
    void filtersBySeverity();
    void filtersBySubsystem();
    void filtersByBookmark();
    void updatesBookmarkFilterWhenBookmarksChange();
    void searchesCanonicalFieldsCaseInsensitively();
    void searchesCustomAttributes();
    void searchesNormalizedSeverityWithRawSource();
    void searchesNormalizedTimestampWithRawSource();
    void sortsUsingTypedTimestampValues();
    void mapsProxyRowsBackToSourceRecords();
    void completeFilterStateUsesSingleModelReset();
};

static QVector<InvestigationRecord> sampleRecords()
{
    InvestigationRecord startup;
    startup.recordId = QStringLiteral("record-startup");
    startup.timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-08T10:00:00.000Z"),
        Qt::ISODateWithMs
        );
    startup.severity = RecordSeverity::Info;
    startup.subsystem = QStringLiteral("Startup");
    startup.eventCode = QStringLiteral("SESSION_START");
    startup.entityId = QStringLiteral("SYS-001");
    startup.message =
        QStringLiteral("Telemetry session initialized");
    startup.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("server-01")
        );

    InvestigationRecord tracking;
    tracking.recordId = QStringLiteral("record-tracking");
    tracking.timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-08T10:02:00.000Z"),
        Qt::ISODateWithMs
        );
    tracking.severity = RecordSeverity::Warning;
    tracking.subsystem = QStringLiteral("Tracking");
    tracking.eventCode = QStringLiteral("TRACK_LOST");
    tracking.entityId = QStringLiteral("TRK-402");
    tracking.message =
        QStringLiteral("Track 402 lost");
    tracking.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("server-02")
        );

    InvestigationRecord comms;
    comms.recordId = QStringLiteral("record-comms");
    comms.timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-08T10:01:00.000Z"),
        Qt::ISODateWithMs
        );
    comms.severity = RecordSeverity::Error;
    comms.subsystem = QStringLiteral("Comms");
    comms.eventCode = QStringLiteral("PACKET_DROP");
    comms.entityId = QStringLiteral("LINK-A");
    comms.message =
        QStringLiteral("Packet loss exceeded threshold");
    comms.customAttributes.insert(
        QStringLiteral("region"),
        QStringLiteral("west")
        );

    return {
        startup,
        tracking,
        comms
    };
}

void InvestigationFilterProxyModelTests::
    emptyFiltersExposeAllRecords()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    QCOMPARE(proxyModel.rowCount(), 3);
}

void InvestigationFilterProxyModelTests::
    filtersBySeverity()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSeverityFilter(
        QStringLiteral("WARN")
        );

    QCOMPARE(proxyModel.rowCount(), 1);

    QCOMPARE(
        proxyModel.data(
                      proxyModel.index(0, 3)
                      ).toString(),
        QStringLiteral("TRACK_LOST")
        );
}

void InvestigationFilterProxyModelTests::
    filtersBySubsystem()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSubsystemFilter(
        QStringLiteral("Comms")
        );

    QCOMPARE(proxyModel.rowCount(), 1);

    QCOMPARE(
        proxyModel.data(
                      proxyModel.index(0, 1)
                      ).toString(),
        QStringLiteral("ERROR")
        );
}

void InvestigationFilterProxyModelTests::
    filtersByBookmark()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setBookmarkedRecordIds({
        QStringLiteral("record-tracking"),
        QStringLiteral("record-comms")
    });

    proxyModel.setBookmarkedOnly(true);

    QCOMPARE(
        proxyModel.rowCount(),
        2
        );
}

void InvestigationFilterProxyModelTests::
    updatesBookmarkFilterWhenBookmarksChange()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setBookmarkedRecordIds({
        QStringLiteral("record-startup"),
        QStringLiteral("record-tracking")
    });

    proxyModel.setBookmarkedOnly(true);

    QCOMPARE(
        proxyModel.rowCount(),
        2
        );

    proxyModel.setBookmarkedRecordIds({
        QStringLiteral("record-comms")
    });

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    const QModelIndex sourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(0, 0)
            );

    QCOMPARE(
        sourceModel
            .recordAt(sourceIndex.row())
            ->recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationFilterProxyModelTests::
    searchesCanonicalFieldsCaseInsensitively()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSearchText(
        QStringLiteral("  trk-402  ")
        );

    QCOMPARE(proxyModel.rowCount(), 1);

    QCOMPARE(
        proxyModel.data(
                      proxyModel.index(0, 3)
                      ).toString(),
        QStringLiteral("TRACK_LOST")
        );
}

void InvestigationFilterProxyModelTests::
    searchesCustomAttributes()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSearchText(
        QStringLiteral("WEST")
        );

    QCOMPARE(proxyModel.rowCount(), 1);

    const QModelIndex proxyIndex =
        proxyModel.index(0, 0);

    const QModelIndex sourceIndex =
        proxyModel.mapToSource(proxyIndex);

    const InvestigationRecord *record =
        sourceModel.recordAt(sourceIndex.row());

    QVERIFY(record != nullptr);

    QCOMPARE(
        record->recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationFilterProxyModelTests::
    searchesNormalizedSeverityWithRawSource()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-alias");

    record.severity =
        RecordSeverity::Warning;

    record.rawSource =
        QStringLiteral(
            R"({"level":"W","message":"Something happened"})"
            );

    InvestigationTableModel sourceModel;

    sourceModel.setRecords({
        record
    });

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSearchText(
        QStringLiteral("WARN")
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );
}

void InvestigationFilterProxyModelTests::
    searchesNormalizedTimestampWithRawSource()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-timestamp");

    record.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-14T12:34:56.000Z"
                ),
            Qt::ISODateWithMs
            );

    record.rawSource =
        QStringLiteral(
            R"({"time":"08/14/2026 12:34:56"})"
            );

    InvestigationTableModel sourceModel;

    sourceModel.setRecords({
        record
    });

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSearchText(
        QStringLiteral(
            "2026-08-14T12:34:56"
            )
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );
}

void InvestigationFilterProxyModelTests::
    sortsUsingTypedTimestampValues()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.sort(
        0,
        Qt::AscendingOrder
        );

    const QModelIndex firstSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(0, 0)
            );

    const QModelIndex secondSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(1, 0)
            );

    const QModelIndex thirdSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(2, 0)
            );

    QCOMPARE(
        sourceModel.recordAt(
                       firstSourceIndex.row()
                       )->recordId,
        QStringLiteral("record-startup")
        );

    QCOMPARE(
        sourceModel.recordAt(
                       secondSourceIndex.row()
                       )->recordId,
        QStringLiteral("record-comms")
        );

    QCOMPARE(
        sourceModel.recordAt(
                       thirdSourceIndex.row()
                       )->recordId,
        QStringLiteral("record-tracking")
        );
}

void InvestigationFilterProxyModelTests::
    mapsProxyRowsBackToSourceRecords()
{
    InvestigationTableModel sourceModel;
    sourceModel.setRecords(sampleRecords());

    InvestigationFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&sourceModel);

    proxyModel.setSeverityFilter(
        QStringLiteral("ERROR")
        );

    QVERIFY(proxyModel.rowCount() == 1);

    const QModelIndex proxyIndex =
        proxyModel.index(0, 0);

    const QModelIndex sourceIndex =
        proxyModel.mapToSource(proxyIndex);

    QVERIFY(sourceIndex.isValid());

    const InvestigationRecord *record =
        sourceModel.recordAt(
            sourceIndex.row()
            );

    QVERIFY(record != nullptr);

    QCOMPARE(
        record->recordId,
        QStringLiteral("record-comms")
        );
}

void InvestigationFilterProxyModelTests::
    completeFilterStateUsesSingleModelReset()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    /*
     * Materialize the initial unfiltered proxy
     * before observing the filter-state change.
     */
    QCOMPARE(
        proxyModel.rowCount(),
        3
        );

    QSignalSpy resetSpy(
        &proxyModel,
        &QAbstractItemModel::modelReset
        );

    CustomFieldFilterMap customFilters;

    customFilters.insert(
        QStringLiteral("host"),
        {
            QStringLiteral("server-02")
        }
        );

    const std::optional<QDateTime> start =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:01:30.000Z"
                ),
            Qt::ISODateWithMs
            );

    const std::optional<QDateTime> end =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-08T10:02:30.000Z"
                ),
            Qt::ISODateWithMs
            );

    proxyModel.setFilterState(
        {
            QStringLiteral(" WARN ")
        },
        {
            QStringLiteral("Tracking")
        },
        QStringLiteral(" lost "),
        {
            QStringLiteral("TRACK_LOST")
        },
        {
            QStringLiteral("TRK-402")
        },
        start,
        end,
        customFilters,
        false
        );

    QCOMPARE(
        resetSpy.count(),
        1
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    const QModelIndex proxyIndex =
        proxyModel.index(
            0,
            0
            );

    const QModelIndex sourceIndex =
        proxyModel.mapToSource(
            proxyIndex
            );

    const InvestigationRecord *record =
        sourceModel.recordAt(
            sourceIndex.row()
            );

    QVERIFY(
        record != nullptr
        );

    QCOMPARE(
        record->recordId,
        QStringLiteral(
            "record-tracking"
            )
        );

    /*
     * Reapplying an equivalent normalized state
     * must not reset the model again.
     */
    proxyModel.setFilterState(
        {
            QStringLiteral("WARN")
        },
        {
            QStringLiteral("Tracking")
        },
        QStringLiteral("lost"),
        {
            QStringLiteral("TRACK_LOST")
        },
        {
            QStringLiteral("TRK-402")
        },
        start,
        end,
        customFilters,
        false
        );

    QCOMPARE(
        resetSpy.count(),
        1
        );
}

QTEST_MAIN(InvestigationFilterProxyModelTests)

#include "InvestigationFilterProxyModelTests.moc"