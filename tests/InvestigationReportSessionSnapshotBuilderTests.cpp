#include <QtTest/QtTest>

#include <memory>
#include <utility>

#include "../src/exporting/InvestigationReportSessionSnapshotBuilder.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{

QDateTime testTimestamp(
    int secondsFromStart
    )
{
    const QDateTime start =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-03T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    return start.addSecs(
        secondsFromStart
        );
}

InvestigationRecord makeRecord(
    const QString &recordId,
    int secondsFromStart,
    RecordSeverity severity =
    RecordSeverity::Warning,
    const QString &subsystem =
    QStringLiteral("Gateway"),
    const QString &eventCode =
    QStringLiteral("REQUEST_FAILED"),
    const QString &entityId =
    QStringLiteral("node-a")
    )
{
    InvestigationRecord record;

    record.recordId =
        recordId;

    record.timestamp =
        testTimestamp(
            secondsFromStart
            );

    record.severity =
        severity;

    record.subsystem =
        subsystem;

    record.eventCode =
        eventCode;

    record.entityId =
        entityId;

    record.message =
        QStringLiteral("Message for %1")
            .arg(recordId);

    record.customAttributes.insert(
        QStringLiteral("runId"),
        QStringLiteral("run-8472")
        );

    record.rawSource =
        QStringLiteral(
            "{\"record\":\"%1\"}"
            )
            .arg(recordId);

    record.source.sourcePath =
        QStringLiteral(
            "C:/private/logs/report-test.jsonl"
            );

    record.source.sourceName =
        QStringLiteral("report-test.jsonl");

    record.source.recordNumber =
        secondsFromStart + 1;

    return record;
}

std::unique_ptr<InvestigationSession>
makeSession(
    QVector<InvestigationRecord> records,
    bool includeDiagnostic = false
    )
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Report Test Profile");

    profile.importerId =
        QStringLiteral("json-lines");

    profile.regexPattern =
        QStringLiteral("test-pattern");

    profile.preserveUnmappedFields =
        false;

    ImportResult result;

    result.records =
        std::move(records);

    /*
     * Leave two processed records intentionally
     * unimported so report count capture can be
     * verified.
     */
    result.processedRecordCount =
        result.records.size() + 2;

    result.sourceTruncated =
        true;

    if (includeDiagnostic) {
        ImportDiagnostic diagnostic;

        diagnostic.code =
            QStringLiteral("TEST_WARNING");

        diagnostic.message =
            QStringLiteral(
                "Synthetic report test diagnostic"
                );

        diagnostic.severity =
            ImportDiagnosticSeverity::Warning;

        result.diagnostics.append(
            diagnostic
            );
    }

    return std::make_unique<InvestigationSession>(
        QStringLiteral("session-report-test"),
        QStringLiteral(
            "C:/private/logs/report-test.jsonl"
            ),
        std::move(profile),
        std::move(result)
        );
}

void synchronizeStateIndicators(
    InvestigationSession &session
    )
{
    InvestigationStateStore *stateStore =
        session.investigationStateStore();

    session
        .investigationController()
        ->proxyModel()
        ->setInvestigationStateIndicators(
            stateStore->bookmarkedRecordIds(),
            stateStore->notedRecordIds(),
            stateStore->findingStatuses()
            );
}

const InvestigationReportEvidenceRecord *
findEvidence(
    const QVector<InvestigationReportEvidenceRecord>
        &records,
    const QString &recordId
    )
{
    for (const InvestigationReportEvidenceRecord
             &record : records) {
        if (record.record.recordId
            == recordId) {
            return &record;
        }
    }

    return nullptr;
}

}

class InvestigationReportSessionSnapshotBuilderTests
    : public QObject
{
    Q_OBJECT

private slots:
    void capturesSourceFiltersAndDistinctPopulations();
    void capturesCompleteAnalyticsAndDeterministicTimeline();
    void capturesAutoAndManualBurstSettings();
    void capturesAndDeduplicatesSupportingEvidence();
    void canSuppressAdditionalBurstEvidence();
    void technicalAppendixAndSnapshotRemainImmutable();
};

void InvestigationReportSessionSnapshotBuilderTests::
    capturesSourceFiltersAndDistinctPopulations()
{
    QVector<InvestigationRecord> records {
        makeRecord(
            QStringLiteral("startup"),
            0,
            RecordSeverity::Info,
            QStringLiteral("Gateway"),
            QStringLiteral("START"),
            QStringLiteral("node-a")
            ),
        makeRecord(
            QStringLiteral("warning"),
            1,
            RecordSeverity::Warning,
            QStringLiteral("Gateway"),
            QStringLiteral("DEGRADED"),
            QStringLiteral("node-a")
            ),
        makeRecord(
            QStringLiteral("worker"),
            2,
            RecordSeverity::Error,
            QStringLiteral("Worker"),
            QStringLiteral("FAIL"),
            QStringLiteral("node-b")
            )
    };

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            std::move(records),
            true
            );

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setBookmarked(
        QStringLiteral("warning"),
        true
        );

    stateStore->setFindingStatus(
        QStringLiteral("warning"),
        FindingStatus::Open
        );

    synchronizeStateIndicators(
        *session
        );

    InvestigationController *controller =
        session->investigationController();

    controller->setEntityFilters(
        QStringList {
            QStringLiteral("node-a")
        }
        );

    controller
        ->proxyModel()
        ->setBookmarkedOnly(
            true
            );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot snapshot =
        builder.build(
            *session,
            QStringLiteral("Degraded Gateway"),
            true,
            false
            );

    QCOMPARE(
        snapshot.source.sessionId,
        QStringLiteral("session-report-test")
        );

    QCOMPARE(
        snapshot.source.documentTitle,
        QStringLiteral("Degraded Gateway")
        );

    /*
     * The shareable session summary retains the file
     * name but does not expose the local workstation
     * path as a report-source field.
     */
    QCOMPARE(
        snapshot.source.sourceName,
        QStringLiteral("report-test.jsonl")
        );

    QCOMPARE(
        snapshot.source.importProfileName,
        QStringLiteral("Report Test Profile")
        );

    QCOMPARE(
        snapshot.source.importerId,
        QStringLiteral("json-lines")
        );

    QCOMPARE(
        snapshot.source.processedRecordCount,
        5
        );

    QCOMPARE(
        snapshot.source.importedRecordCount,
        3
        );

    QCOMPARE(
        snapshot.source.skippedRecordCount,
        2
        );

    QVERIFY(
        snapshot.source.sourceTruncated
        );

    QCOMPARE(
        snapshot.source.diagnostics.size(),
        1
        );

    QVERIFY(
        !snapshot.source
             .technicalImportProfile
             .has_value()
        );

    QCOMPARE(
        snapshot.filters.entityIds,
        QStringList {
            QStringLiteral("node-a")
        }
        );

    QVERIFY(
        snapshot.filters.bookmarkedOnly
        );

    QCOMPARE(
        snapshot.recordContext.totalRecordCount,
        3
        );

    /*
     * Visible population:
     * node-a AND bookmarked.
     */
    QCOMPARE(
        snapshot.recordContext.visibleRecordCount,
        1
        );

    /*
     * Analysis population:
     * node-a, ignoring bookmark-only review state.
     */
    QCOMPARE(
        snapshot.recordContext.analysisRecordCount,
        2
        );

    QCOMPARE(
        snapshot.visibleSeveritySummary.totalCount(),
        1
        );

    QCOMPARE(
        snapshot.analysisSeveritySummary.totalCount(),
        2
        );

    QCOMPARE(
        snapshot.analysisSeveritySummary.infoCount,
        1
        );

    QCOMPARE(
        snapshot.analysisSeveritySummary.warningCount,
        1
        );
}

void InvestigationReportSessionSnapshotBuilderTests::
    capturesCompleteAnalyticsAndDeterministicTimeline()
{
    QVector<InvestigationRecord> records;

    for (int index = 0;
         index < 70;
         ++index) {
        records.append(
            makeRecord(
                QStringLiteral("record-%1")
                    .arg(index),
                index,
                RecordSeverity::Info,
                index % 2 == 0
                    ? QStringLiteral("Gateway")
                    : QStringLiteral("Worker"),
                index % 2 == 0
                    ? QStringLiteral("REQUEST")
                    : QStringLiteral("RESPONSE"),
                QStringLiteral("entity-%1")
                    .arg(index % 12)
                )
            );
    }

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            std::move(records)
            );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot first =
        builder.build(
            *session,
            QStringLiteral("Timeline Test"),
            true,
            false
            );

    const InvestigationReportSessionSnapshot second =
        builder.build(
            *session,
            QStringLiteral("Timeline Test"),
            true,
            false
            );

    QVERIFY(
        first.timeline.available
        );

    /*
     * Seventy one-second-spaced records cannot fit
     * within the report's 60-bucket limit at 1-second
     * resolution. The deterministic ladder therefore
     * selects 5-second buckets.
     */
    QCOMPARE(
        first.timeline.intervalMilliseconds,
        5000
        );

    QVERIFY(
        first.timeline.buckets.size() <= 60
        );

    QCOMPARE(
        first.timeline.intervalMilliseconds,
        second.timeline.intervalMilliseconds
        );

    QCOMPARE(
        first.timeline.buckets.size(),
        second.timeline.buckets.size()
        );

    QVERIFY(
        first.subsystemTrends.available
        );

    QCOMPARE(
        first.subsystemTrends.intervalMilliseconds,
        first.timeline.intervalMilliseconds
        );

    QCOMPARE(
        first.eventCodeFrequencies.size(),
        2
        );

    QCOMPARE(
        first.subsystemFrequencies.size(),
        2
        );

    /*
     * The interactive UI currently limits its entity
     * display to Top 10, but the report snapshot should
     * preserve the complete deterministic collection.
     */
    QCOMPARE(
        first.entityFrequencies.size(),
        12
        );
}

void InvestigationReportSessionSnapshotBuilderTests::
    capturesAutoAndManualBurstSettings()
{
    QVector<InvestigationRecord> records;

    for (int index = 0;
         index < 6;
         ++index) {
        records.append(
            makeRecord(
                QStringLiteral("burst-%1")
                    .arg(index),
                index,
                RecordSeverity::Warning
                )
            );
    }

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            records
            );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot automatic =
        builder.build(
            *session,
            QStringLiteral("Automatic Burst Test"),
            true,
            false
            );

    QVERIFY(
        automatic.burstAnalysis.available
        );

    QCOMPARE(
        automatic.burstAnalysis.timingMode,
        InvestigationReportBurstTimingMode::Auto
        );

    QCOMPARE(
        automatic.burstAnalysis.cadence.timestampCount,
        6
        );

    QCOMPARE(
        automatic.burstAnalysis.cadence.positiveGapCount,
        5
        );

    QCOMPARE(
        automatic
            .burstAnalysis
            .effectiveSettings
            .windowMilliseconds,
        automatic
            .burstAnalysis
            .cadence
            .recommendedBurstWindowMilliseconds
        );

    QCOMPARE(
        automatic
            .burstAnalysis
            .effectiveSettings
            .mergeGapMilliseconds,
        automatic
            .burstAnalysis
            .cadence
            .recommendedMergeGapMilliseconds
        );

    QVERIFY(
        !automatic
             .burstAnalysis
             .bursts
             .isEmpty()
        );

    BurstDetectionSettings manualSettings;

    manualSettings.windowMilliseconds =
        2000;

    manualSettings.mergeGapMilliseconds =
        750;

    manualSettings.elevatedEventThreshold =
        3;

    manualSettings.errorCriticalThreshold =
        2;

    session->setBurstTimingMode(
        InvestigationBurstTimingMode::Manual
        );

    session->setBurstDetectionSettings(
        manualSettings
        );

    const InvestigationReportSessionSnapshot manual =
        builder.build(
            *session,
            QStringLiteral("Manual Burst Test"),
            true,
            false
            );

    QCOMPARE(
        manual.burstAnalysis.timingMode,
        InvestigationReportBurstTimingMode::Manual
        );

    QCOMPARE(
        manual
            .burstAnalysis
            .configuredSettings
            .windowMilliseconds,
        2000
        );

    QCOMPARE(
        manual
            .burstAnalysis
            .effectiveSettings
            .windowMilliseconds,
        2000
        );

    QCOMPARE(
        manual
            .burstAnalysis
            .effectiveSettings
            .mergeGapMilliseconds,
        750
        );

    QCOMPARE(
        manual
            .burstAnalysis
            .effectiveSettings
            .elevatedEventThreshold,
        3
        );

    QCOMPARE(
        manual
            .burstAnalysis
            .effectiveSettings
            .errorCriticalThreshold,
        2
        );
}

void InvestigationReportSessionSnapshotBuilderTests::
    capturesAndDeduplicatesSupportingEvidence()
{
    QVector<InvestigationRecord> records;

    for (int index = 0;
         index < 6;
         ++index) {
        records.append(
            makeRecord(
                QStringLiteral("evidence-%1")
                    .arg(index),
                index,
                RecordSeverity::Warning
                )
            );
    }

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            records
            );

    InvestigationStateStore *stateStore =
        session->investigationStateStore();

    stateStore->setBookmarked(
        QStringLiteral("evidence-0"),
        true
        );

    stateStore->setNote(
        QStringLiteral("evidence-0"),
        QStringLiteral(
            "First visible symptom"
            )
        );

    stateStore->setFindingStatus(
        QStringLiteral("evidence-0"),
        FindingStatus::Open
        );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot snapshot =
        builder.build(
            *session,
            QStringLiteral("Evidence Test"),
            true,
            false
            );

    QVERIFY(
        !snapshot
             .burstAnalysis
             .bursts
             .isEmpty()
        );

    /*
     * All six records contribute to the detected burst.
     * evidence-0 is also stateful, but must occur only
     * once in the evidence collection.
     */
    QCOMPARE(
        snapshot.evidenceRecords.size(),
        6
        );

    int evidenceZeroCount = 0;

    for (const InvestigationReportEvidenceRecord
             &evidence : snapshot.evidenceRecords) {
        if (evidence.record.recordId
            == QStringLiteral("evidence-0")) {
            ++evidenceZeroCount;
        }
    }

    QCOMPARE(
        evidenceZeroCount,
        1
        );

    const InvestigationReportEvidenceRecord *evidence =
        findEvidence(
            snapshot.evidenceRecords,
            QStringLiteral("evidence-0")
            );

    QVERIFY(
        evidence != nullptr
        );

    QVERIFY(
        evidence->state.bookmarked
        );

    QCOMPARE(
        evidence->state.findingStatus,
        FindingStatus::Open
        );

    QCOMPARE(
        evidence->state.note,
        QStringLiteral(
            "First visible symptom"
            )
        );

    QVERIFY(
        evidence->burstEvidence
        );

    /*
     * Evidence remains in deterministic source order.
     */
    for (int index = 0;
         index < snapshot.evidenceRecords.size();
         ++index) {
        QCOMPARE(
            snapshot
                .evidenceRecords
                .at(index)
                .record
                .recordId,
            QStringLiteral("evidence-%1")
                .arg(index)
            );
    }
}

void InvestigationReportSessionSnapshotBuilderTests::
    canSuppressAdditionalBurstEvidence()
{
    QVector<InvestigationRecord> records;

    for (int index = 0;
         index < 6;
         ++index) {
        records.append(
            makeRecord(
                QStringLiteral("record-%1")
                    .arg(index),
                index,
                RecordSeverity::Warning
                )
            );
    }

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            records
            );

    session
        ->investigationStateStore()
        ->setNote(
            QStringLiteral("record-0"),
            QStringLiteral(
                "Keep this investigator note"
                )
            );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot snapshot =
        builder.build(
            *session,
            QStringLiteral("Compact Evidence Test"),
            false,
            false
            );

    QVERIFY(
        !snapshot
             .burstAnalysis
             .bursts
             .isEmpty()
        );

    /*
     * Investigator-authored state is core report
     * content even when additional supporting evidence
     * is disabled.
     */
    QCOMPARE(
        snapshot.evidenceRecords.size(),
        1
        );

    QCOMPARE(
        snapshot
            .evidenceRecords
            .front()
            .record
            .recordId,
        QStringLiteral("record-0")
        );

    QCOMPARE(
        snapshot
            .evidenceRecords
            .front()
            .state
            .note,
        QStringLiteral(
            "Keep this investigator note"
            )
        );

    /*
     * The retained stateful record may still be marked
     * as burst evidence; the option suppresses only the
     * additional otherwise-unannotated burst records.
     */
    QVERIFY(
        snapshot
            .evidenceRecords
            .front()
            .burstEvidence
        );
}

void InvestigationReportSessionSnapshotBuilderTests::
    technicalAppendixAndSnapshotRemainImmutable()
{
    QVector<InvestigationRecord> records {
        makeRecord(
            QStringLiteral("finding"),
            0,
            RecordSeverity::Error
            )
    };

    std::unique_ptr<InvestigationSession> session =
        makeSession(
            std::move(records)
            );

    session
        ->investigationStateStore()
        ->setNote(
            QStringLiteral("finding"),
            QStringLiteral("Original note")
            );

    InvestigationReportSessionSnapshotBuilder
        builder;

    const InvestigationReportSessionSnapshot withTechnical =
        builder.build(
            *session,
            QStringLiteral("Immutable Test"),
            false,
            true
            );

    QVERIFY(
        withTechnical
            .source
            .technicalImportProfile
            .has_value()
        );

    QCOMPARE(
        withTechnical
            .source
            .technicalImportProfile
            ->name,
        QStringLiteral("Report Test Profile")
        );

    QCOMPARE(
        withTechnical
            .source
            .technicalImportProfile
            ->importerId,
        QStringLiteral("json-lines")
        );

    QVERIFY(
        !withTechnical
             .source
             .technicalImportProfile
             ->preserveUnmappedFields
        );

    QCOMPARE(
        withTechnical.evidenceRecords.size(),
        1
        );

    QVERIFY(
        withTechnical
            .evidenceRecords
            .front()
            .record
            .source
            .sourcePath
            .isEmpty()
        );

    QCOMPARE(
        withTechnical
            .evidenceRecords
            .front()
            .record
            .source
            .sourceName,
        QStringLiteral("report-test.jsonl")
        );

    QCOMPARE(
        withTechnical
            .evidenceRecords
            .front()
            .record
            .source
            .recordNumber,
        1
        );

    /*
     * Change the live investigation after capture.
     */
    session
        ->investigationStateStore()
        ->setNote(
            QStringLiteral("finding"),
            QStringLiteral("Changed later")
            );

    QCOMPARE(
        withTechnical
            .evidenceRecords
            .front()
            .state
            .note,
        QStringLiteral("Original note")
        );

    const InvestigationReportSessionSnapshot withoutTechnical =
        builder.build(
            *session,
            QStringLiteral("Immutable Test"),
            false,
            false
            );

    QVERIFY(
        !withoutTechnical
             .source
             .technicalImportProfile
             .has_value()
        );
}

QTEST_APPLESS_MAIN(
    InvestigationReportSessionSnapshotBuilderTests
    )

#include "InvestigationReportSessionSnapshotBuilderTests.moc"