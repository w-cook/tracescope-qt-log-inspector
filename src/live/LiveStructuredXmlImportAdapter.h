#pragma once

#include <QString>

#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"

#include "LiveStructuredXmlRecordBatch.h"

class LiveStructuredXmlImportAdapter
{
public:
    explicit LiveStructuredXmlImportAdapter(
        ImportProfile profile
        );

    bool isSupported() const;

    ImportResult importBatch(
        const LiveStructuredXmlRecordBatch &batch,
        const QString &sourcePath
        ) const;

private:
    ImportProfile m_profile;
};