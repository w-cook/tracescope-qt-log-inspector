#include "ImportResult.h"

#include <algorithm>

qint64 ImportResult::importedRecordCount() const
{
    return records.size();
}

qint64 ImportResult::skippedRecordCount() const
{
    return std::max(
        qint64(0),
        processedRecordCount - importedRecordCount()
        );
}

bool ImportResult::hasWarnings() const
{
    return std::any_of(
        diagnostics.cbegin(),
        diagnostics.cend(),
        [](const ImportDiagnostic &diagnostic) {
            return diagnostic.severity
                   == ImportDiagnosticSeverity::Warning;
        }
        );
}

bool ImportResult::hasErrors() const
{
    return std::any_of(
        diagnostics.cbegin(),
        diagnostics.cend(),
        [](const ImportDiagnostic &diagnostic) {
            return diagnostic.severity
                   == ImportDiagnosticSeverity::Error;
        }
        );
}