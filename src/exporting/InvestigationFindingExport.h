#pragma once

#include "../domain/InvestigationRecord.h"
#include "../domain/InvestigationRecordState.h"

enum class InvestigationFindingExportScope
{
    All,
    Filtered,
    Bookmarked
};

struct InvestigationFindingExportCounts
{
    int all = 0;
    int filtered = 0;
    int bookmarked = 0;
};

struct InvestigationFindingExport
{
    InvestigationRecord record;

    FindingStatus status =
        FindingStatus::None;

    QString note;

    bool bookmarked = false;
};