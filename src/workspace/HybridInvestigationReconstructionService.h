#pragma once

#include <QString>

#include "../importing/ILogImporter.h"
#include "../persistence/InvestigationSessionSnapshot.h"
#include "InvestigationSessionBacking.h"

struct HybridInvestigationReconstructionResult
{
    bool succeeded = true;

    QString errorMessage;

    /*
     * False is not itself an error for a Hybrid
     * investigation. The durable snapshot remains a
     * valid reconstruction when the external source
     * is temporarily unavailable.
     */
    bool externalSourceAvailable = false;

    bool sourceGenerationChanged = false;

    InvestigationExternalSourceBinding
        externalSourceBinding;

    ImportResult importResult;

    qint64 duplicateReplayRecordCount = 0;
    qint64 appendedReplayRecordCount = 0;
    qint64 replaySkippedRecordCount = 0;
};

class HybridInvestigationReconstructionService
{
public:
    HybridInvestigationReconstructionResult
    reconstruct(
        const InvestigationSessionSnapshot &snapshot,
        const InvestigationExternalSourceBinding
            &externalSourceBinding,
        const ILogImporter &importer
        ) const;
};