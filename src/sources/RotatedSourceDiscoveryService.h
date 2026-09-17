#pragma once

#include <QString>
#include <QtTypes>
#include <QVector>

#include "RotatedSourceRule.h"

struct RotatedSourceMatch
{
    QString filePath;
    QString fileName;

    /*
     * Populated for the built-in numeric rotation
     * schemes. Other schemes leave this at zero.
     */
    qint64 rotationIndex = 0;

    /*
     * The captured value used to establish physical
     * source ordering.
     */
    QString orderingValue;
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

struct RotatedSourceRuleSuggestion
{
    RotatedSourceRule rule;
    int matchCount = 0;
};

struct RotatedSourceRuleSuggestionResult
{
    bool succeeded = true;
    QString errorMessage;

    QVector<RotatedSourceRuleSuggestion>
        suggestions;
};

class RotatedSourceDiscoveryService
{
public:
    static RotatedSourceDiscoveryResult discover(
        const QString &activeSourcePath,
        const RotatedSourceRule &rule
        );

    static RotatedSourceRuleSuggestionResult
    suggestRules(
        const QString &activeSourcePath
        );
};