#pragma once

#include <QString>

#include "InvestigationReportSnapshot.h"

class InvestigationReportHtmlRenderer
{
public:
    QString render(
        const InvestigationReportSnapshot &snapshot
        ) const;
};