#pragma once

#include <QString>

#include <optional>

#include "../domain/RecordSourceMetadata.h"

enum class ImportDiagnosticSeverity
{
    Information,
    Warning,
    Error
};

QString importDiagnosticSeverityToString(
    ImportDiagnosticSeverity severity
    );

struct ImportDiagnostic
{
    QString code;
    QString message;
    ImportDiagnosticSeverity severity =
        ImportDiagnosticSeverity::Information;

    std::optional<RecordSourceMetadata> source;
};