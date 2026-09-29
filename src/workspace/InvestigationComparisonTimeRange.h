#pragma once

#include <optional>

#include <QDateTime>
#include <QVector>

#include "../domain/InvestigationRecord.h"

/*
 * A captured comparison time range.
 *
 * An absent scope means the complete imported
 * session is used. A present scope must contain
 * at least one valid inclusive boundary.
 */
struct InvestigationComparisonTimeRange
{
    std::optional<QDateTime> startTime;
    std::optional<QDateTime> endTime;

    bool isValid() const
    {
        if (!startTime.has_value()
            && !endTime.has_value()) {
            return false;
        }

        if (startTime.has_value()
            && !startTime->isValid()) {
            return false;
        }

        if (endTime.has_value()
            && !endTime->isValid()) {
            return false;
        }

        return !startTime.has_value()
               || !endTime.has_value()
               || *startTime <= *endTime;
    }
};

inline QVector<InvestigationRecord>
selectComparisonTimeRangeRecords(
    const QVector<InvestigationRecord> &records,
    const InvestigationComparisonTimeRange &range
    )
{
    QVector<InvestigationRecord> selected;

    for (const InvestigationRecord &record : records) {
        if (!record.timestamp.has_value()
            || !record.timestamp->isValid()) {
            continue;
        }

        const QDateTime &timestamp =
            *record.timestamp;

        if (range.startTime.has_value()
            && timestamp < *range.startTime) {
            continue;
        }

        if (range.endTime.has_value()
            && timestamp > *range.endTime) {
            continue;
        }

        selected.append(record);
    }

    return selected;
}