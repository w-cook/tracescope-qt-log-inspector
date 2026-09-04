#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct InvestigationReportSessionSelection
{
    QString sessionId;
    QString documentTitle;
    bool selected = false;
};

struct InvestigationReportComparisonSelection
{
    QString comparisonId;
    QString documentTitle;

    QString baselineSessionId;
    QString comparisonSessionId;

    bool selected = false;
};

enum class InvestigationReportSelectionOriginType
{
    None,
    Session,
    Comparison
};

struct InvestigationReportSelectionOrigin
{
    InvestigationReportSelectionOriginType type =
        InvestigationReportSelectionOriginType::None;

    QString documentId;
};

class InvestigationReportSelectionModel
{
public:
    InvestigationReportSelectionModel(
        QVector<InvestigationReportSessionSelection>
            sessions,
        QVector<InvestigationReportComparisonSelection>
            comparisons,
        InvestigationReportSelectionOrigin origin
        );

    const QVector<InvestigationReportSessionSelection> &
    sessions() const;

    const QVector<InvestigationReportComparisonSelection> &
    comparisons() const;

    bool setSessionSelected(
        const QString &sessionId,
        bool selected
        );

    bool setComparisonSelected(
        const QString &comparisonId,
        bool selected
        );

    QStringList selectedSessionIds() const;

    QStringList selectedComparisonIds() const;

    bool hasSelection() const;

private:
    void applyInitialSelection(
        const InvestigationReportSelectionOrigin &origin
        );

    void selectOpenComparisonDependencies(
        const InvestigationReportComparisonSelection
            &comparison
        );

    QVector<InvestigationReportSessionSelection>
        m_sessions;

    QVector<InvestigationReportComparisonSelection>
        m_comparisons;
};