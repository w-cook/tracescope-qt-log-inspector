#pragma once

#include <optional>

#include <QString>

#include "../importing/ImportProfile.h"
#include "../sources/SourceFamilyConfiguration.h"
#include "../sources/SourcePhysicalIdentity.h"

enum class PersistedInvestigationSessionBackingMode
{
    SourceBacked,
    SnapshotBacked,
    Hybrid
};

struct PersistedInvestigationExternalSourceBinding
{
    QString sourcePath;

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    quint64 sourceGeneration = 0;

    std::optional<SourcePhysicalIdentity>
        sourceIdentity;
};

struct PersistedInvestigationSessionBacking
{
    PersistedInvestigationSessionBackingMode
        mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    /*
     * Workspace-relative whenever the workspace save
     * layer can provide a relative reference.
     *
     * SourceBacked sessions may also carry this as a
     * self-contained fallback without becoming Hybrid.
     */
    std::optional<QString>
        snapshotReference;

    std::optional<
        PersistedInvestigationExternalSourceBinding>
        externalSourceBinding;

    /*
     * Only SourceBacked sessions persist their import
     * profile here.
     *
     * SnapshotBacked and Hybrid sessions get their
     * authoritative profile from the .tsinv snapshot.
     */
    std::optional<ImportProfile>
        sourceImportProfile;
};