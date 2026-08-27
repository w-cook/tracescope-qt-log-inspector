#pragma once

#include <optional>
#include <utility>

#include <QString>

#include "../analysis/BurstDetectionSettings.h"
#include "../analysis/InvestigationSessionComparison.h"
#include "InvestigationSession.h"

struct InvestigationComparisonSourceSnapshot
{
    QString sessionId;

    InvestigationSessionSourceMetadata
        sourceMetadata;
};

class InvestigationComparisonSnapshot
{
public:
    InvestigationComparisonSnapshot(
        QString id,
        InvestigationComparisonSourceSnapshot baselineSource,
        InvestigationComparisonSourceSnapshot comparisonSource,
        std::optional<BurstDetectionSettings>
            requestedBurstSettings,
        InvestigationSessionComparison analysis
        )
        : m_id(
              std::move(id)
              ),
        m_baselineSource(
            std::move(baselineSource)
            ),
        m_comparisonSource(
            std::move(comparisonSource)
            ),
        m_requestedBurstSettings(
            std::move(requestedBurstSettings)
            ),
        m_analysis(
            std::move(analysis)
            )
    {
    }

    const QString &id() const
    {
        return m_id;
    }

    const InvestigationComparisonSourceSnapshot &
    baselineSource() const
    {
        return m_baselineSource;
    }

    const InvestigationComparisonSourceSnapshot &
    comparisonSource() const
    {
        return m_comparisonSource;
    }

    bool burstComparisonRequested() const
    {
        return m_requestedBurstSettings
            .has_value();
    }

    const std::optional<BurstDetectionSettings> &
    requestedBurstSettings() const
    {
        return m_requestedBurstSettings;
    }

    const InvestigationSessionComparison &
    analysis() const
    {
        return m_analysis;
    }

private:
    QString m_id;

    InvestigationComparisonSourceSnapshot
        m_baselineSource;

    InvestigationComparisonSourceSnapshot
        m_comparisonSource;

    std::optional<BurstDetectionSettings>
        m_requestedBurstSettings;

    InvestigationSessionComparison
        m_analysis;
};