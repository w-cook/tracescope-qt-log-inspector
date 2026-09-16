#pragma once

#include <memory>
#include <optional>
#include <variant>

#include <QString>
#include <QtTypes>

#include "../importing/ImportExecutionContext.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"
#include "../persistence/InvestigationSessionSnapshot.h"
#include "../sources/SourceFamilyConfiguration.h"
#include "../sources/SourcePhysicalIdentity.h"
#include "InvestigationSession.h"
#include "WorkspacePersistenceState.h"

/*
 * Preparation result for a Source-backed session.
 *
 * This contains only value data. No InvestigationSession,
 * QObject, model, or other thread-affine object is created
 * during preparation.
 */
struct PreparedSourceBackedInvestigationSession
{
    QString sourcePath;

    ImportProfile importProfile;

    ImportResult importResult;

    SourceFamilyConfiguration
        sourceFamilyConfiguration;

    quint64 sourceGeneration = 0;

    SourcePhysicalIdentity sourceIdentity;

    qint64 initialLiveFollowByteOffset = 0;
};

/*
 * Snapshot-backed preparation consists entirely of the
 * resolved snapshot path plus the loaded durable snapshot.
 */
struct PreparedSnapshotBackedInvestigationSession
{
    QString snapshotPath;

    InvestigationSessionSnapshot snapshot;
};

/*
 * Hybrid preparation retains the durable snapshot and the
 * external-source binding that should be installed.
 *
 * When the current external source is available,
 * reconstructedImportResult contains the already-replayed
 * snapshot + source candidate.
 *
 * When the external source is unavailable, it remains
 * empty and the durable snapshot alone is materialized.
 */
struct PreparedHybridInvestigationSession
{
    QString snapshotPath;

    InvestigationSessionSnapshot snapshot;

    InvestigationExternalSourceBinding
        externalSourceBinding;

    std::optional<ImportResult>
        reconstructedImportResult;
};

using InvestigationSessionRestorationPreparedData =
    std::variant<
        PreparedSourceBackedInvestigationSession,
        PreparedSnapshotBackedInvestigationSession,
        PreparedHybridInvestigationSession
        >;

struct InvestigationSessionRestorationPreparationResult
{
    bool succeeded = false;

    QString errorMessage;

    std::optional<
        InvestigationSessionRestorationPreparedData>
        preparedData;
};

struct InvestigationSessionRestorationResult
{
    bool succeeded = false;

    QString errorMessage;

    std::unique_ptr<InvestigationSession> session;
};

class InvestigationSessionRestorationService
{
public:
    /*
     * File I/O, import, replay, source identity checks,
     * snapshot loading, and reconstruction happen here.
     *
     * This method deliberately creates no
     * InvestigationSession and is therefore suitable
     * for worker-thread execution.
     */
    InvestigationSessionRestorationPreparationResult
    prepare(
        const PersistedInvestigationSession
            &persistedSession,
        const QString &workspaceFilePath,
        const ImportExecutionContext
            &executionContext = {}
        ) const;

    /*
     * Construct the thread-affine investigation objects
     * from already-prepared value data.
     *
     * Call this on the thread that should own the
     * InvestigationSession's QObject/model graph.
     */
    InvestigationSessionRestorationResult
    materialize(
        const PersistedInvestigationSession
            &persistedSession,
        InvestigationSessionRestorationPreparationResult
            preparation
        ) const;

    /*
     * Convenience synchronous API retained for tests,
     * non-UI callers, and simple restoration paths.
     */
    InvestigationSessionRestorationResult restore(
        const PersistedInvestigationSession
            &persistedSession,
        const QString &workspaceFilePath,
        const ImportExecutionContext
            &executionContext = {}
        ) const;
};