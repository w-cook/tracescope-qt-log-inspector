#include "RecordSeverity.h"

QString recordSeverityToString(RecordSeverity severity)
{
    switch (severity) {
    case RecordSeverity::Trace:
        return QStringLiteral("TRACE");
    case RecordSeverity::Debug:
        return QStringLiteral("DEBUG");
    case RecordSeverity::Info:
        return QStringLiteral("INFO");
    case RecordSeverity::Warning:
        return QStringLiteral("WARN");
    case RecordSeverity::Error:
        return QStringLiteral("ERROR");
    case RecordSeverity::Critical:
        return QStringLiteral("CRITICAL");
    }

    return {};
}

std::optional<RecordSeverity> parseRecordSeverity(const QString &value)
{
    const QString normalized = value.trimmed().toUpper();

    if (normalized == QStringLiteral("TRACE")) {
        return RecordSeverity::Trace;
    }

    if (normalized == QStringLiteral("DEBUG")) {
        return RecordSeverity::Debug;
    }

    if (normalized == QStringLiteral("INFO")
        || normalized == QStringLiteral("INFORMATION")) {
        return RecordSeverity::Info;
    }

    if (normalized == QStringLiteral("WARN")
        || normalized == QStringLiteral("WARNING")) {
        return RecordSeverity::Warning;
    }

    if (normalized == QStringLiteral("ERROR")) {
        return RecordSeverity::Error;
    }

    if (normalized == QStringLiteral("CRITICAL")
        || normalized == QStringLiteral("FATAL")) {
        return RecordSeverity::Critical;
    }

    return std::nullopt;
}