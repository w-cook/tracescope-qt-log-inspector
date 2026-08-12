#include "TelemetryIssueAnalyzer.h"

#include <QMap>
#include <algorithm>

QVector<TelemetryIssueGroup> TelemetryIssueAnalyzer::groupWarningsAndErrorsBySubsystem(
    const QVector<TelemetryEvent> &events
    ) const
{
    QMap<QString, TelemetryIssueGroup> groupsBySubsystem;

    for (const TelemetryEvent &event : events) {
        if (event.level != "WARN"
            && event.level != "ERROR"
            && event.level != "CRITICAL") {
            continue;
        }

        const QString subsystem = event.subsystem.isEmpty()
                                      ? "(No subsystem)"
                                      : event.subsystem;

        if (!groupsBySubsystem.contains(subsystem)) {
            TelemetryIssueGroup group;
            group.subsystem = subsystem;
            groupsBySubsystem.insert(subsystem, group);
        }

        TelemetryIssueGroup group = groupsBySubsystem.value(subsystem);

        if (event.level == "WARN") {
            ++group.warningCount;
        } else if (
            event.level == "ERROR"
            || event.level == "CRITICAL"
            ) {
            ++group.errorCount;
        }

        groupsBySubsystem.insert(subsystem, group);
    }

    QVector<TelemetryIssueGroup> groups;

    for (const TelemetryIssueGroup &group : groupsBySubsystem) {
        groups.push_back(group);
    }

    std::sort(
        groups.begin(),
        groups.end(),
        [](const TelemetryIssueGroup &left, const TelemetryIssueGroup &right) {
            if (left.totalCount() == right.totalCount()) {
                return left.subsystem < right.subsystem;
            }

            return left.totalCount() > right.totalCount();
        }
        );

    return groups;
}