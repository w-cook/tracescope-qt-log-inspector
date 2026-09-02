#include <QtTest/QtTest>

#include <memory>
#include <utility>

#include "../src/exporting/InvestigationFindingExportSnapshotBuilder.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{
InvestigationRecord makeRecord(
    const QString &recordId,
    qint64 recordNumber
    )
{
    InvestigationRecord record;

    record.recordId =
        recordId;

    record.message =
        QStringLiteral("Message for %1")
            .arg(recordId);

    record.source.sourcePath =
        QStringLiteral(
            "C:/logs/findings-export-test.jsonl"
            );

    record.source.sourceName =
        QStringLiteral(
            "findings-export-test.jsonl"
            );

    record.source.recordNumber =
        recordNumber;

    return record;
}

std::unique_ptr<InvestigationSession>
makeSession(
    const QVector<InvestigationRecord> &records
    )
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Findings Export Test"
            );

    profile.importerId =
        QStringLiteral("json-lines");

    ImportResult result;

    result.records = records;

    result.processedRecordCount =
        records.size();

    return std::make_unique<
        InvestigationSession
        >(
        QStringLiteral(
            "findings-export-test.jsonl"
            ),
        std::move(profile),
        std::move(result)
        );
}
}

class InvestigationFindingExportSnapshotBuilderTests
    : public QObject
{
    Q_OBJECT

private slots:
    void includesOnlyRecordsWithFindingStatus();
    void capturesFindingStatusNoteAndBookmark();
    void preservesCompleteInvestigationRecord();
    void preservesSourceRecordOrder();
    void snapshotDoesNotChangeAfterSessionStateChanges();
};

void
    InvestigationFindingExportSnapshotBuilderTests::
    includesOnlyRecordsWithFindingStatus()
{
    QVector<InvestigationRecord> records{
        makeRecord(
            QStringLiteral("none"),
            1
            ),
        makeRecord(
            QStringLiteral("note-only"),
            2
            ),
        makeRecord(
            QStringLiteral("bookmark-only"),
            3
            ),
        makeRecord(
            QStringLiteral("finding"),
            4
            )
    };

    std::unique_ptr<InvestigationSession>
        session =
        makeSession(records);

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setNote(
        QStringLiteral("note-only"),
        QStringLiteral("Analyst note")
        );

    stateStore->setBookmarked(
        QStringLiteral("bookmark-only"),
        true
        );

    stateStore->setFindingStatus(
        QStringLiteral("finding"),
        FindingStatus::Open
        );

    InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        findings =
        builder.build(*session);

    QCOMPARE(findings.size(), 1);

    QCOMPARE(
        findings[0].record.recordId,
        QStringLiteral("finding")
        );

    QCOMPARE(
        findings[0].status,
        FindingStatus::Open
        );
}

void
    InvestigationFindingExportSnapshotBuilderTests::
    capturesFindingStatusNoteAndBookmark()
{
    QVector<InvestigationRecord> records{
        makeRecord(
            QStringLiteral("finding"),
            12
            )
    };

    std::unique_ptr<InvestigationSession>
        session =
        makeSession(records);

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setFindingStatus(
        QStringLiteral("finding"),
        FindingStatus::Resolved
        );

    stateStore->setNote(
        QStringLiteral("finding"),
        QStringLiteral(
            "Confirmed timeout during "
            "database failover.\n"
            "Recovered after retry."
            )
        );

    stateStore->setBookmarked(
        QStringLiteral("finding"),
        true
        );

    InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        findings =
        builder.build(*session);

    QCOMPARE(findings.size(), 1);

    const InvestigationFindingExport &finding =
        findings[0];

    QCOMPARE(
        finding.status,
        FindingStatus::Resolved
        );

    QCOMPARE(
        finding.note,
        QStringLiteral(
            "Confirmed timeout during "
            "database failover.\n"
            "Recovered after retry."
            )
        );

    QVERIFY(finding.bookmarked);
}

void
    InvestigationFindingExportSnapshotBuilderTests::
    preservesCompleteInvestigationRecord()
{
    InvestigationRecord record =
        makeRecord(
            QStringLiteral("complete"),
            42
            );

    record.timestamp =
        QDateTime::fromString(
            "2026-09-02T11:24:36.125Z",
            Qt::ISODateWithMs
            );

    record.severity =
        RecordSeverity::Error;

    record.subsystem =
        QStringLiteral("Payments");

    record.eventCode =
        QStringLiteral("DB_TIMEOUT");

    record.entityId =
        QStringLiteral("order-1842");

    record.message =
        QStringLiteral(
            "Database request timed out"
            );

    record.customAttributes.insert(
        QStringLiteral("durationMs"),
        5032
        );

    record.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("app-02")
        );

    record.rawSource =
        QStringLiteral(
            R"({"level":"error","durationMs":5032})"
            );

    std::unique_ptr<InvestigationSession>
        session =
        makeSession({record});

    session
        ->investigationStateStore()
        ->setFindingStatus(
            QStringLiteral("complete"),
            FindingStatus::Dismissed
            );

    InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        findings =
        builder.build(*session);

    QCOMPARE(findings.size(), 1);

    const InvestigationRecord &exportedRecord =
        findings[0].record;

    QCOMPARE(
        exportedRecord.recordId,
        QStringLiteral("complete")
        );

    QCOMPARE(
        exportedRecord.timestamp.value(),
        record.timestamp.value()
        );

    QCOMPARE(
        exportedRecord.severity.value(),
        RecordSeverity::Error
        );

    QCOMPARE(
        exportedRecord.subsystem.value(),
        QStringLiteral("Payments")
        );

    QCOMPARE(
        exportedRecord.eventCode.value(),
        QStringLiteral("DB_TIMEOUT")
        );

    QCOMPARE(
        exportedRecord.entityId.value(),
        QStringLiteral("order-1842")
        );

    QCOMPARE(
        exportedRecord.message.value(),
        QStringLiteral(
            "Database request timed out"
            )
        );

    QCOMPARE(
        exportedRecord
            .customAttributes
            .value("durationMs")
            .toInt(),
        5032
        );

    QCOMPARE(
        exportedRecord
            .customAttributes
            .value("host")
            .toString(),
        QStringLiteral("app-02")
        );

    QCOMPARE(
        exportedRecord.source.sourcePath,
        record.source.sourcePath
        );

    QCOMPARE(
        exportedRecord.source.sourceName,
        record.source.sourceName
        );

    QCOMPARE(
        exportedRecord.source.recordNumber,
        42
        );

    QCOMPARE(
        exportedRecord.rawSource,
        record.rawSource
        );
}

void
    InvestigationFindingExportSnapshotBuilderTests::
    preservesSourceRecordOrder()
{
    QVector<InvestigationRecord> records{
        makeRecord(
            QStringLiteral("first"),
            10
            ),
        makeRecord(
            QStringLiteral("second"),
            20
            ),
        makeRecord(
            QStringLiteral("third"),
            30
            )
    };

    std::unique_ptr<InvestigationSession>
        session =
        makeSession(records);

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setFindingStatus(
        QStringLiteral("first"),
        FindingStatus::Resolved
        );

    stateStore->setFindingStatus(
        QStringLiteral("second"),
        FindingStatus::Open
        );

    stateStore->setFindingStatus(
        QStringLiteral("third"),
        FindingStatus::Dismissed
        );

    InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        findings =
        builder.build(*session);

    QCOMPARE(findings.size(), 3);

    QCOMPARE(
        findings[0].record.recordId,
        QStringLiteral("first")
        );

    QCOMPARE(
        findings[1].record.recordId,
        QStringLiteral("second")
        );

    QCOMPARE(
        findings[2].record.recordId,
        QStringLiteral("third")
        );
}

void
    InvestigationFindingExportSnapshotBuilderTests::
    snapshotDoesNotChangeAfterSessionStateChanges()
{
    InvestigationRecord record =
        makeRecord(
            QStringLiteral("finding"),
            7
            );

    record.message =
        QStringLiteral(
            "Original message"
            );

    std::unique_ptr<InvestigationSession>
        session =
        makeSession({record});

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setFindingStatus(
        QStringLiteral("finding"),
        FindingStatus::Open
        );

    stateStore->setNote(
        QStringLiteral("finding"),
        QStringLiteral("Original note")
        );

    stateStore->setBookmarked(
        QStringLiteral("finding"),
        true
        );

    InvestigationFindingExportSnapshotBuilder
        builder;

    const QVector<InvestigationFindingExport>
        snapshot =
        builder.build(*session);

    stateStore->setFindingStatus(
        QStringLiteral("finding"),
        FindingStatus::Resolved
        );

    stateStore->setNote(
        QStringLiteral("finding"),
        QStringLiteral("Changed note")
        );

    stateStore->setBookmarked(
        QStringLiteral("finding"),
        false
        );

    QCOMPARE(snapshot.size(), 1);

    QCOMPARE(
        snapshot[0].status,
        FindingStatus::Open
        );

    QCOMPARE(
        snapshot[0].note,
        QStringLiteral("Original note")
        );

    QVERIFY(snapshot[0].bookmarked);

    QCOMPARE(
        snapshot[0].record.message.value(),
        QStringLiteral("Original message")
        );
}

QTEST_MAIN(
    InvestigationFindingExportSnapshotBuilderTests
    )

#include "InvestigationFindingExportSnapshotBuilderTests.moc"