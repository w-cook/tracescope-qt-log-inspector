#pragma once

#include <QString>

#include "../importing/ImportResult.h"
#include "../persistence/InvestigationSessionSnapshot.h"

struct HybridInvestigationCandidateBuildResult
{
    bool succeeded = true;

    QString errorMessage;

    ImportResult importResult;

    qint64 duplicateReplayRecordCount = 0;
    qint64 appendedReplayRecordCount = 0;
    qint64 replaySkippedRecordCount = 0;

    quint64 replaySourceGeneration = 0;
};

class HybridInvestigationCandidateBuilder
{
public:
    HybridInvestigationCandidateBuildResult build(
        const InvestigationSessionSnapshot &snapshot,
        ImportResult replayResult,
        quint64 replaySourceGeneration
        ) const;
};