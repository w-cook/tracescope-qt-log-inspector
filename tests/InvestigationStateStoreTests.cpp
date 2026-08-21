#include <QtTest>

#include <QSet>

#include "../src/workspace/InvestigationStateStore.h"

class InvestigationStateStoreTests : public QObject
{
    Q_OBJECT

private slots:
    void returnsDefaultStateForUnknownRecord();
    void storesBookmark();
    void removesOtherwiseEmptyStateWhenBookmarkCleared();
    void storesNote();
    void storesFindingStatus();
    void preservesIndependentStateProperties();
    void retainOnlyRemovesMissingRecords();
    void reportsBookmarkedRecordIds();
};

void InvestigationStateStoreTests::
    returnsDefaultStateForUnknownRecord()
{
    InvestigationStateStore store;

    const InvestigationRecordState state =
        store.stateForRecord(
            QStringLiteral("record-1")
            );

    QVERIFY(!state.bookmarked);
    QVERIFY(state.note.isEmpty());

    QCOMPARE(
        state.findingStatus,
        FindingStatus::None
        );

    QVERIFY(
        !store.hasStateForRecord(
            QStringLiteral("record-1")
            )
        );
}

void InvestigationStateStoreTests::
    storesBookmark()
{
    InvestigationStateStore store;

    store.setBookmarked(
        QStringLiteral("record-1"),
        true
        );

    QVERIFY(
        store.hasStateForRecord(
            QStringLiteral("record-1")
            )
        );

    QVERIFY(
        store.stateForRecord(
                 QStringLiteral("record-1")
                 ).bookmarked
        );
}

void InvestigationStateStoreTests::
    removesOtherwiseEmptyStateWhenBookmarkCleared()
{
    InvestigationStateStore store;

    const QString recordId =
        QStringLiteral("record-1");

    store.setBookmarked(
        recordId,
        true
        );

    QVERIFY(
        store.hasStateForRecord(recordId)
        );

    store.setBookmarked(
        recordId,
        false
        );

    QVERIFY(
        !store.hasStateForRecord(recordId)
        );
}

void InvestigationStateStoreTests::
    storesNote()
{
    InvestigationStateStore store;

    const QString recordId =
        QStringLiteral("record-1");

    store.setNote(
        recordId,
        QStringLiteral(
            "Failure begins immediately after reconnect."
            )
        );

    QCOMPARE(
        store.stateForRecord(recordId).note,
        QStringLiteral(
            "Failure begins immediately after reconnect."
            )
        );
}

void InvestigationStateStoreTests::
    storesFindingStatus()
{
    InvestigationStateStore store;

    const QString recordId =
        QStringLiteral("record-1");

    store.setFindingStatus(
        recordId,
        FindingStatus::Open
        );

    QCOMPARE(
        store.stateForRecord(recordId)
            .findingStatus,
        FindingStatus::Open
        );
}

void InvestigationStateStoreTests::
    preservesIndependentStateProperties()
{
    InvestigationStateStore store;

    const QString recordId =
        QStringLiteral("record-1");

    store.setBookmarked(
        recordId,
        true
        );

    store.setNote(
        recordId,
        QStringLiteral("Investigate timeout.")
        );

    store.setFindingStatus(
        recordId,
        FindingStatus::Open
        );

    InvestigationRecordState state =
        store.stateForRecord(recordId);

    QVERIFY(state.bookmarked);

    QCOMPARE(
        state.note,
        QStringLiteral("Investigate timeout.")
        );

    QCOMPARE(
        state.findingStatus,
        FindingStatus::Open
        );

    store.setFindingStatus(
        recordId,
        FindingStatus::Resolved
        );

    state =
        store.stateForRecord(recordId);

    QVERIFY(state.bookmarked);

    QCOMPARE(
        state.note,
        QStringLiteral("Investigate timeout.")
        );

    QCOMPARE(
        state.findingStatus,
        FindingStatus::Resolved
        );
}

void InvestigationStateStoreTests::
    retainOnlyRemovesMissingRecords()
{
    InvestigationStateStore store;

    store.setBookmarked(
        QStringLiteral("record-1"),
        true
        );

    store.setNote(
        QStringLiteral("record-2"),
        QStringLiteral("Keep this.")
        );

    store.setFindingStatus(
        QStringLiteral("record-3"),
        FindingStatus::Open
        );

    store.retainOnly(
        QSet<QString>{
            QStringLiteral("record-1"),
            QStringLiteral("record-3")
        }
        );

    QVERIFY(
        store.hasStateForRecord(
            QStringLiteral("record-1")
            )
        );

    QVERIFY(
        !store.hasStateForRecord(
            QStringLiteral("record-2")
            )
        );

    QVERIFY(
        store.hasStateForRecord(
            QStringLiteral("record-3")
            )
        );
}

void InvestigationStateStoreTests::
    reportsBookmarkedRecordIds()
{
    InvestigationStateStore store;

    store.setBookmarked(
        QStringLiteral("record-1"),
        true
        );

    store.setNote(
        QStringLiteral("record-2"),
        QStringLiteral("Not bookmarked")
        );

    store.setBookmarked(
        QStringLiteral("record-3"),
        true
        );

    const QSet<QString> recordIds =
        store.bookmarkedRecordIds();

    QCOMPARE(
        recordIds.size(),
        2
        );

    QVERIFY(
        recordIds.contains(
            QStringLiteral("record-1")
            )
        );

    QVERIFY(
        !recordIds.contains(
            QStringLiteral("record-2")
            )
        );

    QVERIFY(
        recordIds.contains(
            QStringLiteral("record-3")
            )
        );
}

QTEST_MAIN(InvestigationStateStoreTests)

#include "InvestigationStateStoreTests.moc"