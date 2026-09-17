#pragma once

#include <QString>

#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"

#include "LiveStructuredJsonRecordBatch.h"

class LiveStructuredJsonImportAdapter
{
public:
    explicit LiveStructuredJsonImportAdapter(
        ImportProfile profile
        );

    bool isSupported() const;

    ImportResult importBatch(
        const LiveStructuredJsonRecordBatch &batch,
        const QString &sourcePath
        ) const;

private:
    ImportProfile m_profile;
};