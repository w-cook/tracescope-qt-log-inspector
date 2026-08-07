#include "RecordTimestamp.h"

std::optional<QDateTime> parseRecordTimestamp(const QString &value)
{
    const QString normalized = value.trimmed();

    if (normalized.isEmpty()) {
        return std::nullopt;
    }

    QDateTime timestamp = QDateTime::fromString(
        normalized,
        Qt::ISODateWithMs
        );

    if (!timestamp.isValid()) {
        timestamp = QDateTime::fromString(
            normalized,
            Qt::ISODate
            );
    }

    if (!timestamp.isValid()) {
        return std::nullopt;
    }

    return timestamp;
}