#pragma once

#include <QString>
#include <QtTypes>

#include "ImportResult.h"

/*
 * Rebase every record and source-backed diagnostic in
 * an import result to the supplied logical source
 * generation.
 *
 * Record IDs are regenerated because source generation
 * participates in stable record identity.
 */
void rebaseImportResultSourceGeneration(
    ImportResult &result,
    quint64 sourceGeneration
    );

/*
 * Rebase only records/diagnostics whose source path
 * matches sourcePath.
 *
 * This is needed for source-family imports: persisted
 * source generation belongs to the active source path,
 * not to independently identifiable rotated files.
 */
void rebaseImportResultSourceGenerationForPath(
    ImportResult &result,
    const QString &sourcePath,
    quint64 sourceGeneration
    );