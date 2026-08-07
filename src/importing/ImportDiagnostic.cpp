#include "ImportDiagnostic.h"

QString importDiagnosticSeverityToString(
    ImportDiagnosticSeverity severity
    )
{
    switch (severity) {
    case ImportDiagnosticSeverity::Information:
        return QStringLiteral("INFO");
    case ImportDiagnosticSeverity::Warning:
        return QStringLiteral("WARNING");
    case ImportDiagnosticSeverity::Error:
        return QStringLiteral("ERROR");
    }

    return {};
}