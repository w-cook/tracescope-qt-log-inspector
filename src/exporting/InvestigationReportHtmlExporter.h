#pragma once

#include <QString>

#include "InvestigationReportSnapshot.h"

class InvestigationReportHtmlExporter
{
public:
    bool exportToFile(
        const InvestigationReportSnapshot &snapshot,
        const QString &filePath
        ) const;
};