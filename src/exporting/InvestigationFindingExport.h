#pragma once

#include "../domain/InvestigationRecord.h"
#include "../domain/InvestigationRecordState.h"

enum class InvestigationFindingExportScope
{
    All,
    Filtered,
    Bookmarked
};

struct InvestigationFindingExport
{
    InvestigationRecord record;

    FindingStatus status =
        FindingStatus::None;

    QString note;

    bool bookmarked = false;
};