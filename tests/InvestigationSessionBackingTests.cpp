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
    void updatesSnapshotPathWithoutChangingMode();
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

void InvestigationSessionBackingTests::
    updatesSnapshotPathWithoutChangingMode()
{
    InvestigationExternalSourceBinding
        externalSource;

    externalSource.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    /*
     * SourceBacked has no runtime snapshot binding.
     * A workspace-save fallback must not silently turn
     * it into Hybrid.
     */
    InvestigationSessionBacking sourceBacked =
        InvestigationSessionBacking::
        sourceBacked(
            externalSource
            );

    QVERIFY(
        !sourceBacked.updateSnapshotPath(
            QStringLiteral(
                "new-source-fallback.tsinv"
                )
            )
        );

    QCOMPARE(
        sourceBacked.mode(),
        InvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        !sourceBacked.hasSnapshot()
        );

    InvestigationSnapshotBinding snapshot;

    snapshot.snapshotPath =
        QStringLiteral(
            "old-snapshot.tsinv"
            );

    snapshot.sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    InvestigationSessionBacking snapshotBacked =
        InvestigationSessionBacking::
        snapshotBacked(
            snapshot
            );

    QVERIFY(
        snapshotBacked.updateSnapshotPath(
            QStringLiteral(
                "new-snapshot.tsinv"
                )
            )
        );

    QCOMPARE(
        snapshotBacked.mode(),
        InvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        snapshotBacked.snapshot()
        != nullptr
        );

    QCOMPARE(
        snapshotBacked
            .snapshot()
            ->snapshotPath,
        QStringLiteral(
            "new-snapshot.tsinv"
            )
        );

    QCOMPARE(
        snapshotBacked
            .snapshot()
            ->sourceFidelity,
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly
        );

    InvestigationSessionBacking hybrid =
        InvestigationSessionBacking::
        hybrid(
            externalSource,
            snapshot
            );

    QVERIFY(
        hybrid.updateSnapshotPath(
            QStringLiteral(
                "new-hybrid.tsinv"
                )
            )
        );

    QCOMPARE(
        hybrid.mode(),
        InvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        hybrid.hasExternalSource()
        );

    QVERIFY(
        hybrid.snapshot()
        != nullptr
        );

    QCOMPARE(
        hybrid
            .snapshot()
            ->snapshotPath,
        QStringLiteral(
            "new-hybrid.tsinv"
            )
        );

    /*
     * Empty paths must never erase a valid durable
     * binding.
     */
    QVERIFY(
        !hybrid.updateSnapshotPath(
            QString()
            )
        );

    QCOMPARE(
        hybrid
            .snapshot()
            ->snapshotPath,
        QStringLiteral(
            "new-hybrid.tsinv"
            )
        );
}

QTEST_MAIN(
    InvestigationSessionBackingTests
    )

#include "InvestigationSessionBackingTests.moc"