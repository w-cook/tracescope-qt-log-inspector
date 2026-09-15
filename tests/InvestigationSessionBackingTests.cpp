#include <QtTest/QtTest>

#include "../src/workspace/InvestigationSessionBacking.h"

class InvestigationSessionBackingTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sourceBackedCanBecomeHybridAndReturn();
    void snapshotBackedCanBecomeHybridAndReturn();
    void cannotDetachOnlySnapshotBacking();
    void cannotDisconnectOnlyExternalBacking();
};

void InvestigationSessionBackingTests::
    sourceBackedCanBecomeHybridAndReturn()
{
    InvestigationExternalSourceBinding
        externalSource;

    externalSource.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::
        sourceBacked(
            externalSource
            );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        backing.hasExternalSource()
        );

    QVERIFY(
        !backing.hasSnapshot()
        );

    InvestigationSnapshotBinding snapshot;

    snapshot.snapshotPath =
        QStringLiteral(
            "gateway.tsinv"
            );

    snapshot.sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    backing.attachSnapshot(
        snapshot
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        backing.hasExternalSource()
        );

    QVERIFY(
        backing.hasSnapshot()
        );

    QVERIFY(
        backing.detachSnapshot()
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        backing.hasExternalSource()
        );

    QVERIFY(
        !backing.hasSnapshot()
        );
}

void InvestigationSessionBackingTests::
    snapshotBackedCanBecomeHybridAndReturn()
{
    InvestigationSnapshotBinding snapshot;

    snapshot.snapshotPath =
        QStringLiteral(
            "gateway.tsinv"
            );

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::
        snapshotBacked(
            snapshot
            );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        !backing.hasExternalSource()
        );

    QVERIFY(
        backing.hasSnapshot()
        );

    InvestigationExternalSourceBinding
        externalSource;

    externalSource.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    backing.connectExternalSource(
        externalSource
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        backing.disconnectExternalSource()
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        !backing.hasExternalSource()
        );

    QVERIFY(
        backing.hasSnapshot()
        );
}

void InvestigationSessionBackingTests::
    cannotDetachOnlySnapshotBacking()
{
    InvestigationSnapshotBinding snapshot;

    snapshot.snapshotPath =
        QStringLiteral(
            "gateway.tsinv"
            );

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::
        snapshotBacked(
            snapshot
            );

    QVERIFY(
        !backing.detachSnapshot()
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        backing.hasSnapshot()
        );
}

void InvestigationSessionBackingTests::
    cannotDisconnectOnlyExternalBacking()
{
    InvestigationExternalSourceBinding
        externalSource;

    externalSource.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    InvestigationSessionBacking backing =
        InvestigationSessionBacking::
        sourceBacked(
            externalSource
            );

    QVERIFY(
        !backing.disconnectExternalSource()
        );

    QCOMPARE(
        backing.mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        backing.hasExternalSource()
        );
}

QTEST_MAIN(
    InvestigationSessionBackingTests
    )

#include "InvestigationSessionBackingTests.moc"