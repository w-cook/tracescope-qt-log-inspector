#pragma once

#include "../domain/InvestigationRecord.h"
#include "../domain/InvestigationRecordState.h"

struct InvestigationFindingExport
{
    InvestigationRecord record;

    FindingStatus status =
        FindingStatus::None;

    QString note;

    bool bookmarked = false;
};