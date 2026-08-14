#include "TelemetryIssueAnalyzer.h"

#include <QMap>
#include <algorithm>

QVector<TelemetryIssueGroup> TelemetryIssueAnalyzer::groupWarningsAndErrorsBySubsystem(
        const QVector<InvestigationRecord> &records
        ) const
{
    QMap<QString, TelemetryIssueGroup>
        groupsBySubsystem;

    for (const InvestigationRecord &record
         : records) {
        if (!record.severity.has_value()) {
            continue;
        }

        const RecordSeverity severity =
            record.severity.value();

        if (severity != RecordSeverity::Warning
            && severity != RecordSeverity::Error
            && severity != RecordSeverity::Critical) {
            continue;
        }

        const QString subsystem =
            record.subsystem.has_value()
                    && !record.subsystem->isEmpty()
                ? record.subsystem.value()
                : QStringLiteral("(No subsystem)");

        auto iterator =
            groupsBySubsystem.find(
                subsystem
                );

        if (iterator ==
            groupsBySubsystem.end()) {
            TelemetryIssueGroup group;
            group.subsystem = subsystem;

            iterator =
                groupsBySubsystem.insert(
                    subsystem,
                    group
                    );
        }

        if (severity ==
            RecordSeverity::Warning) {
            ++iterator->warningCount;
        } else {
            ++iterator->errorCount;
        }
    }

    QVector<TelemetryIssueGroup> groups;

    groups.reserve(
        groupsBySubsystem.size()
        );

    for (const TelemetryIssueGroup &group
         : groupsBySubsystem) {
        groups.append(group);
    }

    std::sort(
        groups.begin(),
        groups.end(),
        [](
            const TelemetryIssueGroup &left,
            const TelemetryIssueGroup &right
            ) {
            if (left.totalCount()
                == right.totalCount()) {
                return left.subsystem
                       < right.subsystem;
            }

            return left.totalCount()
                   > right.totalCount();
        }
        );

    return groups;
}