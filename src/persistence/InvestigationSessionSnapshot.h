#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "../domain/InvestigationRecord.h"
#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"
#include "../sources/SourceFamilyConfiguration.h"
#include "../sources/SourcePhysicalIdentity.h"

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

/*
 * Frozen source-continuity information describing the
 * external source from which snapshot evidence originated.
 *
 * This is provenance/reconnect information only. Its
 * presence does not mean that the external source currently
 * controls a SnapshotBacked investigation.
 */
struct InvestigationSnapshotSourceContinuity
{
    /*
     * Physical path observed when this snapshot was
     * captured. A verified relocation may later replace
     * this path while preserving logicalSourceKey.
     */
    QString sourcePath;

    /*
     * Stable identity of the logical source. Unlike
     * sourcePath, this survives verified relocation.
     */
    QString logicalSourceKey;

    /*
     * Durable source-family discovery configuration.
     * rotatedSourcePaths contains transient discovery
     * results and must not be persisted.
     */
    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    /*
     * Generation identifies same-logical-source
     * replacement/truncation history. Relocation alone
     * does not advance this value.
     */
    quint64 sourceGeneration = 0;

    /*
     * Optional physical evidence used to conservatively
     * verify that a later path still represents the same
     * underlying source.
     */
    std::optional<SourcePhysicalIdentity>
        sourceIdentity;
};

struct InvestigationSessionSnapshot
{
    inline static constexpr int
        CurrentSchemaVersion = 2;

    int schemaVersion =
        CurrentSchemaVersion;

    InvestigationSnapshotSourceFidelity
        sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    /*
     * Optional persisted source provenance introduced in
     * schema version 2. Version-1 snapshots deserialize
     * with no source continuity information.
     */
    std::optional<
        InvestigationSnapshotSourceContinuity>
        sourceContinuity;

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