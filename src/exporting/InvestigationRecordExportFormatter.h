#pragma once

#include <QString>

#include "../domain/InvestigationRecord.h"

class InvestigationRecordExportFormatter
{
public:
    QString toStructuredJson(
        const InvestigationRecord &record
        ) const;

    QString toFormattedText(
        const InvestigationRecord &record
        ) const;
};