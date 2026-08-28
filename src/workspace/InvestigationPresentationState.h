#pragma once

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>
#include <Qt>

#include "../analysis/BurstDetectionSettings.h"
#include "InvestigationSession.h"

struct InvestigationScrollState
{
    int horizontalValue = 0;
    int verticalValue = 0;
};

struct InvestigationTablePresentationState
{
    int currentRow = -1;
    int currentColumn = -1;

    InvestigationScrollState scroll;
};

struct InvestigationEventTablePresentationState
{
    QString selectedRecordId;

    QVector<int> columnWidths;

    int sortColumn = -1;
    Qt::SortOrder sortOrder =
        Qt::AscendingOrder;

    InvestigationScrollState scroll;
};

struct InvestigationTimelinePresentationState
{
    /*
     * Zero means Auto, matching the timeline
     * control's existing data representation.
     */
    qint64 intervalMilliseconds = 0;

    InvestigationTimelineBreakdown breakdown =
        InvestigationTimelineBreakdown::Severity;

    int subsystemTrendLimit = 5;

    int horizontalScrollValue = 0;
};

struct InvestigationAnalyticsPresentationState
{
    InvestigationAnalyticsTab selectedTab =
        InvestigationAnalyticsTab::Overview;

    QVector<int> overviewSplitterSizes;

    InvestigationTablePresentationState
        eventCodeTable;

    InvestigationTablePresentationState
        entityTable;

    QVector<int> burstSplitterSizes;

    /*
     * Use timestamps rather than only a row number
     * so the selected burst has a semantic identity.
     */
    std::optional<QDateTime>
        selectedBurstStartTimestamp;

    std::optional<QDateTime>
        selectedBurstEndTimestamp;

    InvestigationTablePresentationState
        burstTable;

    InvestigationScrollState
        burstDetailScroll;
};

struct InvestigationReviewPresentationState
{
    InvestigationReviewTab selectedTab =
        InvestigationReviewTab::IssueSummary;

    InvestigationTablePresentationState
        issueSummaryTable;

    InvestigationTablePresentationState
        findingsTable;

    InvestigationAnalyticsPresentationState
        analytics;
};

struct InvestigationSessionPresentationState
{
    InvestigationEventTablePresentationState
        eventTable;

    InvestigationScrollState
        eventDetailScroll;

    InvestigationTimelinePresentationState
        timeline;

    InvestigationReviewPresentationState
        review;

    QVector<int> mainSplitterSizes;
    QVector<int> bottomSplitterSizes;

    InvestigationBurstTimingMode burstTimingMode =
        InvestigationBurstTimingMode::Auto;

    BurstDetectionSettings burstDetectionSettings;
};