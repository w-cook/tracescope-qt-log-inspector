#pragma once

#include <QString>

#include <optional>

enum class RecordSeverity
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

QString recordSeverityToString(RecordSeverity severity);
std::optional<RecordSeverity> parseRecordSeverity(const QString &value);