#pragma once

#include <QString>
#include <QtTypes>
#include <QVector>

#include "RotatedSourceRule.h"

struct RotatedSourceMatch
{
    QString filePath;
    QString fileName;

    qint64 rotationIndex = 0;
};

struct RotatedSourceDiscoveryResult
{
    bool succeeded = true;
    QString errorMessage;

    /*
     * Rotated physical sources only.
     *
     * Ordering is oldest -> newest so callers can
     * append the active source afterward when they
     * need complete chronological source-family
     * ordering.
     */
    QVector<RotatedSourceMatch> rotatedSources;
};

class RotatedSourceDiscoveryService
{
public:
    static RotatedSourceDiscoveryResult discover(
        const QString &activeSourcePath,
        const RotatedSourceRule &rule
        );
};