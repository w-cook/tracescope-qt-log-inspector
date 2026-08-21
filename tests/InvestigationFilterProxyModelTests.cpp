#include <QtTest>

#include <QHash>
#include <QSet>
#include <QSignalSpy>

#include "../src/domain/InvestigationRecordState.h"
#include "../src/models/InvestigationFilterProxyModel.h"
#include "../src/models/InvestigationTableModel.h"

class InvestigationFilterProxyModelTests
    : public QObject
{
    Q_OBJECT

private slots:
    void emptyFiltersExposeAllRecords();

    void filtersBySeverity();
    void filtersBySubsystem();

    void searchesCanonicalFieldsCaseInsensitively();
    void searchesCustomAttributes();
    void searchesNormalizedSeverityWithRawSource();
    void searchesNormalizedTimestampWithRawSource();

    void sortsUsingTypedTimestampValues();
    void mapsProxyRowsBackToSourceRecords();

    void filtersByBookmark();
    void updatesBookmarkFilterWhenBookmarksChange();

    void filtersByFindingStatus();
    void findingStatusFilterRespondsToStateChanges();

    void completeFilterStateUsesSingleModelReset();
};

static QVector<InvestigationRecord>
sampleRecords()
{
    InvestigationRecord startup;

    startup.recordId =
        QStringLiteral(
            "record-startup"
            );

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
        QStringLiteral(
            "Startup"
            );

    startup.eventCode =
        QStringLiteral(
            "SESSION_START"
            );

    startup.entityId =
        QStringLiteral(
            "SYS-001"
            );

    startup.message =
        QStringLiteral(
            "Telemetry session initialized"
            );

    startup.customAttributes.insert(
        QStringLiteral(
            "host"
            ),
        QStringLiteral(
            "server-01"
            )
        );

    InvestigationRecord tracking;

    tracking.recordId =
        QStringLiteral(
            "record-tracking"
            );

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
        QStringLiteral(
            "Tracking"
            );

    tracking.eventCode =
        QStringLiteral(
            "TRACK_LOST"
            );

    tracking.entityId =
        QStringLiteral(
            "TRK-402"
            );

    tracking.message =
        QStringLiteral(
            "Track 402 lost"
            );

    tracking.customAttributes.insert(
        QStringLiteral(
            "host"
            ),
        QStringLiteral(
            "server-02"
            )
        );

    InvestigationRecord comms;

    comms.recordId =
        QStringLiteral(
            "record-comms"
            );

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
        QStringLiteral(
            "Comms"
            );

    comms.eventCode =
        QStringLiteral(
            "PACKET_DROP"
            );

    comms.entityId =
        QStringLiteral(
            "LINK-A"
            );

    comms.message =
        QStringLiteral(
            "Packet loss exceeded threshold"
            );

    comms.customAttributes.insert(
        QStringLiteral(
            "region"
            ),
        QStringLiteral(
            "west"
            )
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

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    QCOMPARE(
        proxyModel.rowCount(),
        3
        );
}

void InvestigationFilterProxyModelTests::
    filtersBySeverity()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSeverityFilter(
        QStringLiteral(
            "WARN"
            )
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    QCOMPARE(
        proxyModel
            .data(
                proxyModel.index(
                    0,
                    3
                    )
                )
            .toString(),
        QStringLiteral(
            "TRACK_LOST"
            )
        );
}

void InvestigationFilterProxyModelTests::
    filtersBySubsystem()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSubsystemFilter(
        QStringLiteral(
            "Comms"
            )
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    QCOMPARE(
        proxyModel
            .data(
                proxyModel.index(
                    0,
                    1
                    )
                )
            .toString(),
        QStringLiteral(
            "ERROR"
            )
        );
}

void InvestigationFilterProxyModelTests::
    searchesCanonicalFieldsCaseInsensitively()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSearchText(
        QStringLiteral(
            "  trk-402  "
            )
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    QCOMPARE(
        proxyModel
            .data(
                proxyModel.index(
                    0,
                    3
                    )
                )
            .toString(),
        QStringLiteral(
            "TRACK_LOST"
            )
        );
}

void InvestigationFilterProxyModelTests::
    searchesCustomAttributes()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSearchText(
        QStringLiteral(
            "WEST"
            )
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
            "record-comms"
            )
        );
}

void InvestigationFilterProxyModelTests::
    searchesNormalizedSeverityWithRawSource()
{
    InvestigationRecord record;

    record.recordId =
        QStringLiteral(
            "record-alias"
            );

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

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSearchText(
        QStringLiteral(
            "WARN"
            )
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
        QStringLiteral(
            "record-timestamp"
            );

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

    proxyModel.setSourceModel(
        &sourceModel
        );

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

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.sort(
        0,
        Qt::AscendingOrder
        );

    const QModelIndex
        firstSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(
                0,
                0
                )
            );

    const QModelIndex
        secondSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(
                1,
                0
                )
            );

    const QModelIndex
        thirdSourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(
                2,
                0
                )
            );

    QCOMPARE(
        sourceModel
            .recordAt(
                firstSourceIndex.row()
                )
            ->recordId,
        QStringLiteral(
            "record-startup"
            )
        );

    QCOMPARE(
        sourceModel
            .recordAt(
                secondSourceIndex.row()
                )
            ->recordId,
        QStringLiteral(
            "record-comms"
            )
        );

    QCOMPARE(
        sourceModel
            .recordAt(
                thirdSourceIndex.row()
                )
            ->recordId,
        QStringLiteral(
            "record-tracking"
            )
        );
}

void InvestigationFilterProxyModelTests::
    mapsProxyRowsBackToSourceRecords()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setSeverityFilter(
        QStringLiteral(
            "ERROR"
            )
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

    QVERIFY(
        sourceIndex.isValid()
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
            "record-comms"
            )
        );
}

void InvestigationFilterProxyModelTests::
    filtersByBookmark()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setBookmarkedRecordIds(
        QSet<QString> {
            QStringLiteral(
                "record-tracking"
                ),
            QStringLiteral(
                "record-comms"
                )
        }
        );

    proxyModel.setBookmarkedOnly(
        true
        );

    QCOMPARE(
        proxyModel.rowCount(),
        2
        );

    QSet<QString> visibleRecordIds;

    for (
        int proxyRow = 0;
        proxyRow < proxyModel.rowCount();
        ++proxyRow
        ) {
        const QModelIndex sourceIndex =
            proxyModel.mapToSource(
                proxyModel.index(
                    proxyRow,
                    0
                    )
                );

        const InvestigationRecord *record =
            sourceModel.recordAt(
                sourceIndex.row()
                );

        QVERIFY(
            record != nullptr
            );

        visibleRecordIds.insert(
            record->recordId
            );
    }

    QVERIFY(
        visibleRecordIds.contains(
            QStringLiteral(
                "record-tracking"
                )
            )
        );

    QVERIFY(
        visibleRecordIds.contains(
            QStringLiteral(
                "record-comms"
                )
            )
        );

    QVERIFY(
        !visibleRecordIds.contains(
            QStringLiteral(
                "record-startup"
                )
            )
        );
}

void InvestigationFilterProxyModelTests::
    updatesBookmarkFilterWhenBookmarksChange()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    proxyModel.setBookmarkedRecordIds(
        QSet<QString> {
            QStringLiteral(
                "record-startup"
                ),
            QStringLiteral(
                "record-tracking"
                )
        }
        );

    proxyModel.setBookmarkedOnly(
        true
        );

    QCOMPARE(
        proxyModel.rowCount(),
        2
        );

    proxyModel.setBookmarkedRecordIds(
        QSet<QString> {
            QStringLiteral(
                "record-comms"
                )
        }
        );

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    const QModelIndex sourceIndex =
        proxyModel.mapToSource(
            proxyModel.index(
                0,
                0
                )
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
            "record-comms"
            )
        );
}

void InvestigationFilterProxyModelTests::
    filtersByFindingStatus()
{
    QVector<InvestigationRecord> records =
        sampleRecords();

    /*
     * This record intentionally receives no
     * finding status. It verifies that records
     * with FindingStatus::None are excluded when
     * an explicit finding-status filter is active.
     */
    InvestigationRecord unclassified;

    unclassified.recordId =
        QStringLiteral(
            "record-unclassified"
            );

    unclassified.message =
        QStringLiteral(
            "No finding assigned"
            );

    records.append(
        unclassified
        );

    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        records
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    QHash<QString, FindingStatus>
        findingStatuses;

    findingStatuses.insert(
        QStringLiteral(
            "record-startup"
            ),
        FindingStatus::Open
        );

    findingStatuses.insert(
        QStringLiteral(
            "record-tracking"
            ),
        FindingStatus::Resolved
        );

    findingStatuses.insert(
        QStringLiteral(
            "record-comms"
            ),
        FindingStatus::Dismissed
        );

    proxyModel.setInvestigationStateIndicators(
        {},
        {},
        findingStatuses
        );

    proxyModel.setFindingStatusFilters({
        QStringLiteral(
            "OPEN"
            ),
        QStringLiteral(
            "RESOLVED"
            )
    });

    QCOMPARE(
        proxyModel.rowCount(),
        2
        );

    QSet<QString> visibleRecordIds;

    for (
        int proxyRow = 0;
        proxyRow < proxyModel.rowCount();
        ++proxyRow
        ) {
        const QModelIndex sourceIndex =
            proxyModel.mapToSource(
                proxyModel.index(
                    proxyRow,
                    0
                    )
                );

        const InvestigationRecord *record =
            sourceModel.recordAt(
                sourceIndex.row()
                );

        QVERIFY(
            record != nullptr
            );

        visibleRecordIds.insert(
            record->recordId
            );
    }

    QVERIFY(
        visibleRecordIds.contains(
            QStringLiteral(
                "record-startup"
                )
            )
        );

    QVERIFY(
        visibleRecordIds.contains(
            QStringLiteral(
                "record-tracking"
                )
            )
        );

    QVERIFY(
        !visibleRecordIds.contains(
            QStringLiteral(
                "record-comms"
                )
            )
        );

    QVERIFY(
        !visibleRecordIds.contains(
            QStringLiteral(
                "record-unclassified"
                )
            )
        );
}

void InvestigationFilterProxyModelTests::
    findingStatusFilterRespondsToStateChanges()
{
    InvestigationTableModel sourceModel;

    sourceModel.setRecords(
        sampleRecords()
        );

    InvestigationFilterProxyModel proxyModel;

    proxyModel.setSourceModel(
        &sourceModel
        );

    QHash<QString, FindingStatus>
        findingStatuses;

    findingStatuses.insert(
        QStringLiteral(
            "record-startup"
            ),
        FindingStatus::Open
        );

    proxyModel.setInvestigationStateIndicators(
        {},
        {},
        findingStatuses
        );

    proxyModel.setFindingStatusFilters({
        QStringLiteral(
            "OPEN"
            )
    });

    QCOMPARE(
        proxyModel.rowCount(),
        1
        );

    {
        const QModelIndex sourceIndex =
            proxyModel.mapToSource(
                proxyModel.index(
                    0,
                    0
                    )
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
                "record-startup"
                )
            );
    }

    /*
     * Simulate the analyst changing the currently
     * visible finding from Open to Resolved.
     * Because only Open findings are being shown,
     * it should immediately leave the proxy.
     */
    findingStatuses.insert(
        QStringLiteral(
            "record-startup"
            ),
        FindingStatus::Resolved
        );

    proxyModel.setInvestigationStateIndicators(
        {},
        {},
        findingStatuses
        );

    QCOMPARE(
        proxyModel.rowCount(),
        0
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
     * The target Tracking record must satisfy the
     * finding-status criterion that will be included
     * in the coordinated filter state.
     */
    QHash<QString, FindingStatus>
        findingStatuses;

    findingStatuses.insert(
        QStringLiteral(
            "record-tracking"
            ),
        FindingStatus::Open
        );

    proxyModel.setInvestigationStateIndicators(
        {},
        {},
        findingStatuses
        );

    /*
     * Materialize the initial unfiltered proxy
     * before observing the complete state change.
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
        QStringLiteral(
            "host"
            ),
        {
            QStringLiteral(
                "server-02"
                )
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
            QStringLiteral(
                " WARN "
                )
        },
        {
            QStringLiteral(
                "Tracking"
                )
        },
        QStringLiteral(
            " lost "
            ),
        {
            QStringLiteral(
                "TRACK_LOST"
                )
        },
        {
            QStringLiteral(
                "TRK-402"
                )
        },
        start,
        end,
        customFilters,
        {
            QStringLiteral(
                " OPEN "
                )
        },
        false
        );

    /*
     * The entire filter state should be installed
     * through one coordinated model reset.
     */
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

    QCOMPARE(
        proxyModel.findingStatusFilters(),
        QStringList({
            QStringLiteral(
                "OPEN"
                )
        })
        );

    QVERIFY(
        !proxyModel.bookmarkedOnly()
        );

    /*
     * Reapplying an equivalent normalized state
     * must not reset the model again.
     */
    proxyModel.setFilterState(
        {
            QStringLiteral(
                "WARN"
                )
        },
        {
            QStringLiteral(
                "Tracking"
                )
        },
        QStringLiteral(
            "lost"
            ),
        {
            QStringLiteral(
                "TRACK_LOST"
                )
        },
        {
            QStringLiteral(
                "TRK-402"
                )
        },
        start,
        end,
        customFilters,
        {
            QStringLiteral(
                "OPEN"
                )
        },
        false
        );

    QCOMPARE(
        resetSpy.count(),
        1
        );
}

QTEST_MAIN(
    InvestigationFilterProxyModelTests
    )

#include "InvestigationFilterProxyModelTests.moc"