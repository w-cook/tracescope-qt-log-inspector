#include "HybridInvestigationCandidateBuilder.h"

#include <QSet>

#include <utility>

#include "../importing/ImportResultSourceGeneration.h"

HybridInvestigationCandidateBuildResult
HybridInvestigationCandidateBuilder::build(
    const InvestigationSessionSnapshot &snapshot,
    ImportResult replayResult,
    quint64 replaySourceGeneration
    ) const
{
    HybridInvestigationCandidateBuildResult
        result;

    result.replaySourceGeneration =
        replaySourceGeneration;

    /*
     * A cancelled replay does not represent a complete
     * reconstruction candidate. Never allow a partial
     * replay to replace the active investigation.
     */
    if (replayResult.cancelled) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "The external source replay was "
                "cancelled before reconstruction "
                "completed."
                );

        return result;
    }

    /*
     * The saved snapshot is the durable evidence
     * floor. Preserve its record order exactly.
     */
    result.importResult.records =
        snapshot.records;

    result.importResult.diagnostics =
        snapshot.diagnostics;

    result.importResult.processedRecordCount =
        snapshot.processedRecordCount;

    result.importResult.sourceTruncated =
        snapshot.sourceTruncated
        || replayResult.sourceTruncated;

    QSet<QString> admittedRecordIds;

    admittedRecordIds.reserve(
        result.importResult.records.size()
        + replayResult.records.size()
        );

    for (const InvestigationRecord &record
         : std::as_const(result.importResult.records)) {
        if (!record.recordId.isEmpty()) {
            admittedRecordIds.insert(
                record.recordId
                );
        }
    }

    /*
     * A static replay sees the currently available
     * physical file as generation zero.
     *
     * Hybrid reconstruction knows which logical source
     * generation that physical file actually belongs
     * to, so rebase records, stable IDs, and replay
     * diagnostics before comparing identities.
     */
    rebaseImportResultSourceGeneration(
        replayResult,
        replaySourceGeneration
        );

    for (InvestigationRecord &record
         : replayResult.records) {
        if (admittedRecordIds.contains(
                record.recordId
                )) {
            ++result
                  .duplicateReplayRecordCount;

            continue;
        }

        admittedRecordIds.insert(
            record.recordId
            );

        result.importResult.records.append(
            std::move(record)
            );

        ++result.appendedReplayRecordCount;
    }

    for (ImportDiagnostic &diagnostic
         : replayResult.diagnostics) {
        result.importResult.diagnostics.append(
            std::move(diagnostic)
            );
    }

    result.replaySkippedRecordCount =
        replayResult.skippedRecordCount();

    /*
     * Conservative Phase 15 processed-count rule:
     *
     * Preserve the snapshot's already-established
     * processed count and add only replay records that
     * were newly admitted.
     *
     * A full-file replay may encounter skipped records
     * that were already encountered before the snapshot
     * was captured. Without a durable identity for a
     * skipped record, counting those again would inflate
     * the investigation on every reconstruction.
     *
     * replaySkippedRecordCount remains available to the
     * caller for diagnostics/telemetry without silently
     * changing the durable count.
     */
    result.importResult.processedRecordCount +=
        result.appendedReplayRecordCount;

    return result;
}