#pragma once

#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"

enum class InvestigationSnapshotSourceFidelity
{
    /*
     * The saved normalized interpretation is complete,
     * but the original source bytes are not available
     * for arbitrary future re-import.
     */
    NormalizedOnly,

    /*
     * Reserved for the later source-capture expansion.
     * A snapshot with this fidelity retains enough
     * original source material for full re-import.
     */
    CompleteSource
};

struct InvestigationSessionSnapshot
{
    inline static constexpr int
        CurrentSchemaVersion = 1;

    int schemaVersion =
        CurrentSchemaVersion;

    InvestigationSnapshotSourceFidelity
        sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    /*
     * The interpretation that produced the saved
     * normalized record set.
     */
    ImportProfile importProfile;

    /*
     * Complete normalized investigation evidence in
     * deterministic investigation order.
     */
    QVector<InvestigationRecord> records;

    /*
     * These are part of the saved import result rather
     * than workspace presentation state. Preserving
     * them lets a reopened snapshot reproduce the
     * original session's import counts and diagnostics.
     */
    QVector<ImportDiagnostic> diagnostics;

    qint64 processedRecordCount = 0;

    bool sourceTruncated = false;
};