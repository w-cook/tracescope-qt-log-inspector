#pragma once

#include <optional>

#include <QString>

#include "../persistence/InvestigationSessionSnapshot.h"
#include "../sources/SourceFamilyConfiguration.h"
#include "../sources/SourcePhysicalIdentity.h"

enum class InvestigationSessionBackingMode
{
    SourceBacked,
    SnapshotBacked,
    Hybrid
};

struct InvestigationExternalSourceBinding
{
    /*
     * Current physical location from which TraceScope
     * reads the external source.
     *
     * This may change after a verified relocation.
     */
    QString sourcePath;

    /*
     * Stable logical identity of this source.
     *
     * For ordinary sessions this begins as the same
     * normalized absolute path as sourcePath.
     *
     * A verified rename/move may change sourcePath,
     * but must never change logicalSourceKey.
     */
    QString logicalSourceKey;

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    /*
     * Logical same-path generation currently associated
     * with the active external source.
     */
    quint64 sourceGeneration = 0;

    /*
     * Physical identity of that active generation when
     * known. This is persisted with workspace Hybrid
     * bindings later.
     */
    std::optional<SourcePhysicalIdentity>
        sourceIdentity;
};

struct InvestigationSnapshotBinding
{
    /*
     * Path to the durable TraceScope investigation
     * snapshot backing this open session.
     */
    QString snapshotPath;

    InvestigationSnapshotSourceFidelity
        sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;
};

class InvestigationSessionBacking
{
public:
    static InvestigationSessionBacking
    sourceBacked(
        InvestigationExternalSourceBinding
            externalSource
        );

    static InvestigationSessionBacking
    snapshotBacked(
        InvestigationSnapshotBinding snapshot
        );

    static InvestigationSessionBacking hybrid(
        InvestigationExternalSourceBinding
            externalSource,
        InvestigationSnapshotBinding snapshot
        );

    InvestigationSessionBackingMode mode() const;

    bool hasExternalSource() const;
    bool hasSnapshot() const;

    const InvestigationExternalSourceBinding *
    externalSource() const;

    InvestigationExternalSourceBinding *
    externalSource();

    const InvestigationSnapshotBinding *
    snapshot() const;

    InvestigationSnapshotBinding *
    snapshot();

    /*
     * Replace the durable path of an already attached
     * snapshot without changing backing mode.
     *
     * SourceBacked sessions have no runtime snapshot
     * binding, so this returns false for them.
     */
    bool updateSnapshotPath(
        QString snapshotPath
        );

    /*
     * Attaching/replacing a snapshot changes:
     *
     * SourceBacked -> Hybrid
     * SnapshotBacked -> SnapshotBacked
     * Hybrid -> Hybrid
     */
    void attachSnapshot(
        InvestigationSnapshotBinding snapshot
        );

    /*
     * A snapshot cannot be detached if it is the
     * session's only backing.
     *
     * Hybrid -> SourceBacked
     */
    bool detachSnapshot();

    /*
     * Connecting/replacing an external source changes:
     *
     * SnapshotBacked -> Hybrid
     * SourceBacked -> SourceBacked
     * Hybrid -> Hybrid
     */
    void connectExternalSource(
        InvestigationExternalSourceBinding
            externalSource
        );

    /*
     * An external source cannot be disconnected if it
     * is the session's only backing.
     *
     * Hybrid -> SnapshotBacked
     */
    bool disconnectExternalSource();

private:
    InvestigationSessionBacking() = default;

    std::optional<
        InvestigationExternalSourceBinding>
        m_externalSource;

    std::optional<
        InvestigationSnapshotBinding>
        m_snapshot;
};