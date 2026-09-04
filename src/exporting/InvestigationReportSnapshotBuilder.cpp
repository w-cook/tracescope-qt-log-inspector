#include "InvestigationReportSnapshotBuilder.h"

#include <algorithm>
#include <utility>

#include <QSet>

#include "InvestigationReportSessionSnapshotBuilder.h"

#include "../workspace/InvestigationComparisonSnapshot.h"
#include "../workspace/InvestigationSession.h"

namespace
{

const InvestigationReportSessionInput *
findSessionInput(
    const QVector<InvestigationReportSessionInput> &inputs,
    const QString &sessionId
    )
{
    for (const InvestigationReportSessionInput &input
         : inputs) {
        if (input.session != nullptr
            && input.session->id() == sessionId) {
            return &input;
        }
    }

    return nullptr;
}

const InvestigationReportComparisonInput *
findComparisonInput(
    const QVector<InvestigationReportComparisonInput> &inputs,
    const QString &comparisonId
    )
{
    for (const InvestigationReportComparisonInput &input
         : inputs) {
        if (input.snapshot != nullptr
            && input.snapshot->id() == comparisonId) {
            return &input;
        }
    }

    return nullptr;
}

QString comparisonSourceName(
    const InvestigationComparisonSourceSnapshot &source
    )
{
    const QString sourceName =
        source.sourceMetadata
            .sourceName
            .trimmed();

    return sourceName.isEmpty()
               ? source.sessionId
               : sourceName;
}

InvestigationReportComparisonSnapshot
buildComparisonSnapshot(
    const InvestigationComparisonSnapshot &comparison,
    const QString &documentTitle
    )
{
    InvestigationReportComparisonSnapshot snapshot;

    snapshot.comparisonId =
        comparison.id();

    snapshot.documentTitle =
        documentTitle;

    snapshot.baselineSessionId =
        comparison.baselineSource().sessionId;

    snapshot.comparisonSessionId =
        comparison.comparisonSource().sessionId;

    snapshot.baselineSourceName =
        comparisonSourceName(
            comparison.baselineSource()
            );

    snapshot.comparisonSourceName =
        comparisonSourceName(
            comparison.comparisonSource()
            );

    snapshot.requestedBurstSettings =
        comparison.requestedBurstSettings();

    /*
     * Critical report invariant:
     *
     * Copy the already-immutable Phase 12 comparison
     * result. Report generation must never recalculate
     * the comparison from potentially newer live session
     * contents.
     */
    snapshot.analysis =
        comparison.analysis();

    return snapshot;
}

InvestigationReportSourceTimeCoverage
buildTimeCoverage(
    const InvestigationReportSessionSnapshot &session
    )
{
    InvestigationReportSourceTimeCoverage coverage;

    coverage.sessionId =
        session.source.sessionId;

    coverage.sourceName =
        session.source.sourceName;

    /*
     * Cross-source time coverage represents the
     * source-data investigation population used by
     * deterministic analysis, not annotation-only
     * bookmark/finding filtering.
     */
    coverage.timestampedRecordCount =
        session
            .recordContext
            .analysisTimestampedRecordCount;

    coverage.firstTimestamp =
        session
            .recordContext
            .analysisFirstTimestamp;

    coverage.lastTimestamp =
        session
            .recordContext
            .analysisLastTimestamp;

    return coverage;
}

QVector<InvestigationReportChronologyEntry>
buildCrossSourceChronology(
    const QVector<InvestigationReportSessionSnapshot>
        &sessions
    )
{
    struct OrderedEntry
    {
        InvestigationReportChronologyEntry entry;

        int sessionOrder = 0;
        qint64 sourceRecordNumber = 0;
    };

    QVector<OrderedEntry> orderedEntries;

    for (int sessionIndex = 0;
         sessionIndex < sessions.size();
         ++sessionIndex) {
        const InvestigationReportSessionSnapshot &session =
            sessions.at(sessionIndex);

        for (const InvestigationReportEvidenceRecord
                 &evidence : session.evidenceRecords) {
            if (!evidence.record.timestamp.has_value()
                || !evidence.record.timestamp->isValid()) {
                continue;
            }

            OrderedEntry ordered;

            ordered.sessionOrder =
                sessionIndex;

            ordered.sourceRecordNumber =
                evidence.record.source.recordNumber;

            InvestigationReportChronologyEntry &entry =
                ordered.entry;

            entry.sessionId =
                session.source.sessionId;

            entry.sourceName =
                session.source.sourceName;

            entry.recordId =
                evidence.record.recordId;

            entry.timestamp =
                evidence.record.timestamp
                    ->toUTC();

            entry.severity =
                evidence.record.severity;

            entry.subsystem =
                evidence.record.subsystem;

            entry.eventCode =
                evidence.record.eventCode;

            entry.entityId =
                evidence.record.entityId;

            entry.message =
                evidence.record.message;

            entry.state =
                evidence.state;

            entry.burstEvidence =
                evidence.burstEvidence;

            orderedEntries.append(
                std::move(ordered)
                );
        }
    }

    std::stable_sort(
        orderedEntries.begin(),
        orderedEntries.end(),
        [](
            const OrderedEntry &left,
            const OrderedEntry &right
            ) {
            if (left.entry.timestamp
                != right.entry.timestamp) {
                return left.entry.timestamp
                       < right.entry.timestamp;
            }

            /*
             * Equal timestamps are common in telemetry.
             * Preserve report session order before
             * falling back to source record position.
             */
            if (left.sessionOrder
                != right.sessionOrder) {
                return left.sessionOrder
                       < right.sessionOrder;
            }

            if (left.sourceRecordNumber
                != right.sourceRecordNumber) {
                return left.sourceRecordNumber
                       < right.sourceRecordNumber;
            }

            return left.entry.recordId
                   < right.entry.recordId;
        }
        );

    QVector<InvestigationReportChronologyEntry>
        chronology;

    chronology.reserve(
        orderedEntries.size()
        );

    for (OrderedEntry &ordered
         : orderedEntries) {
        chronology.append(
            std::move(ordered.entry)
            );
    }

    return chronology;
}

}

InvestigationReportSnapshot
InvestigationReportSnapshotBuilder::build(
    const InvestigationReportConfiguration &configuration,
    const QVector<InvestigationReportSessionInput>
        &availableSessions,
    const QVector<InvestigationReportComparisonInput>
        &availableComparisons,
    const QDateTime &generatedAtUtc
    ) const
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        configuration.title;

    snapshot.context =
        configuration.context;

    snapshot.generatedAtUtc =
        generatedAtUtc.toUTC();

    snapshot.supportingEvidenceIncluded =
        configuration.includeSupportingEvidence;

    snapshot.technicalAppendixIncluded =
        configuration.includeTechnicalAppendix;

    /*
     * -----------------------------------------------------
     * Selected sessions
     * -----------------------------------------------------
     */

    InvestigationReportSessionSnapshotBuilder
        sessionBuilder;

    QSet<QString> capturedSessionIds;

    for (const QString &sessionId
         : configuration.sessionIds) {
        if (sessionId.isEmpty()
            || capturedSessionIds.contains(
                sessionId
                )) {
            continue;
        }

        const InvestigationReportSessionInput *input =
            findSessionInput(
                availableSessions,
                sessionId
                );

        if (input == nullptr
            || input->session == nullptr) {
            continue;
        }

        snapshot.sessions.append(
            sessionBuilder.build(
                *input->session,
                input->documentTitle,
                configuration
                    .includeSupportingEvidence,
                configuration
                    .includeTechnicalAppendix
                )
            );

        capturedSessionIds.insert(
            sessionId
            );
    }

    /*
     * -----------------------------------------------------
     * Selected immutable comparisons
     * -----------------------------------------------------
     */

    QSet<QString> capturedComparisonIds;

    for (const QString &comparisonId
         : configuration.comparisonIds) {
        if (comparisonId.isEmpty()
            || capturedComparisonIds.contains(
                comparisonId
                )) {
            continue;
        }

        const InvestigationReportComparisonInput *input =
            findComparisonInput(
                availableComparisons,
                comparisonId
                );

        if (input == nullptr
            || input->snapshot == nullptr) {
            continue;
        }

        snapshot.comparisons.append(
            buildComparisonSnapshot(
                *input->snapshot,
                input->documentTitle
                )
            );

        capturedComparisonIds.insert(
            comparisonId
            );
    }

    /*
     * -----------------------------------------------------
     * Cross-source structures
     * -----------------------------------------------------
     */

    snapshot.sourceTimeCoverage.reserve(
        snapshot.sessions.size()
        );

    for (const InvestigationReportSessionSnapshot &session
         : std::as_const(snapshot.sessions)) {
        snapshot.sourceTimeCoverage.append(
            buildTimeCoverage(
                session
                )
            );
    }

    snapshot.crossSourceChronology =
        buildCrossSourceChronology(
            snapshot.sessions
            );

    return snapshot;
}