#pragma once

#include <optional>

#include <QVector>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QStringList>

#include "InvestigationPresentationState.h"
#include "InvestigationSessionBackingPersistence.h"
#include "WorkspaceDocumentLayoutState.h"

#include "../analysis/BurstDetectionSettings.h"
#include "../analysis/InvestigationSessionComparison.h"
#include "../domain/InvestigationRecordState.h"
#include "../importing/ImportProfile.h"
#include "../sources/RotatedSourceRule.h"

struct PersistedInvestigationRecordState
{
    QString recordId;

    bool bookmarked = false;
    QString note;

    FindingStatus findingStatus =
        FindingStatus::None;
};

struct PersistedInvestigationFilterState
{
    QStringList severities;
    QStringList subsystems;
    QString searchText;

    QStringList eventCodes;
    QStringList entityIds;

    std::optional<QDateTime> startTime;
    std::optional<QDateTime> endTime;

    QMap<QString, QStringList>
        customFieldFilters;

    QStringList findingStatuses;

    bool bookmarkedOnly = false;
};

struct PersistedInvestigationComparisonSource
{
    QString sessionId;

    QString sourcePath;
    QString sourceName;

    qint64 sourceSizeBytes = 0;

    QDateTime sourceLastModified;
    QDateTime importedAtUtc;
};

struct PersistedInvestigationComparison
{
    QString comparisonId;

    PersistedInvestigationComparisonSource
        baselineSource;

    PersistedInvestigationComparisonSource
        comparisonSource;

    std::optional<BurstDetectionSettings>
        requestedBurstSettings;

    InvestigationSessionComparison analysis;

    InvestigationComparisonPresentationState
        presentationState;
};

struct PersistedSourceFamilyConfiguration
{
    bool includeRotatedSources = false;

    RotatedSourceRule rotationRule;
};

struct PersistedInvestigationSession
{
    QString sessionId;

    PersistedInvestigationSessionBacking
        backing;

    /*
     * Temporary compatibility mirrors.
     *
     * These keep the existing workspace-open path
     * compiling while schema-v2 restoration is wired
     * in. Schema v2 JSON will not persist these fields.
     *
     * Schema-v1 deserialization will populate both
     * these mirrors and the normalized backing object.
     */
    QString sourcePath;

    ImportProfile importProfile;

    PersistedSourceFamilyConfiguration
        sourceFamilyConfiguration;

    QVector<PersistedInvestigationRecordState>
        recordStates;

    PersistedInvestigationFilterState
        filterState;

    InvestigationSessionPresentationState
        presentationState;
};

struct WorkspacePersistenceState
{
    inline static constexpr int
        CurrentSchemaVersion = 2;

    int schemaVersion =
        CurrentSchemaVersion;

    QVector<PersistedInvestigationSession>
        sessions;

    QVector<PersistedInvestigationComparison>
        comparisons;

    WorkspaceDocumentLayoutState
        documentLayout;
};