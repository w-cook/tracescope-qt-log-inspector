#pragma once

#include <QString>
#include <QStringList>

#include "RotatedSourceRule.h"

struct SourceFamilyConfiguration
{
    bool includeRotatedSources = false;

    RotatedSourceRule rotationRule;

    /*
     * Physical rotated members in chronological
     * order: oldest -> newest.
     *
     * The active source is deliberately not stored
     * here. InvestigationSession continues to own
     * the active source path separately.
     */
    QStringList rotatedSourcePaths;

    QStringList orderedSourcePaths(
        const QString &activeSourcePath
        ) const
    {
        QStringList paths;

        if (includeRotatedSources) {
            paths = rotatedSourcePaths;
        }

        paths.append(
            activeSourcePath
            );

        return paths;
    }
};