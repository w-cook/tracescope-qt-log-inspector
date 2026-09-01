#pragma once

#include <optional>

#include <QDateTime>
#include <QGroupBox>
#include <QString>
#include <QVector>

#include "../../analysis/EventTimelineAnalyzer.h"
#include "../../analysis/InvestigationAnalyticsAnalyzer.h"
#include "../../domain/InvestigationRecord.h"
#include "../../workspace/InvestigationPresentationState.h"

class InvestigationSession;
class QChartView;
class QComboBox;
class QLabel;
class QScrollBar;
class QWidget;
class QResizeEvent;
class QTimer;

class InvestigationTimelinePanel
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit InvestigationTimelinePanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    /*
     * Replace the record collection represented by
     * the timeline.
     *
     * MainWindow currently supplies the same
     * recordsForAnalysis() collection that drove
     * the legacy timeline implementation.
     */
    void updateRecords(
        const QVector<InvestigationRecord> &records
        );

    void clear();

    InvestigationTimelinePresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationTimelinePresentationState &state
        );

signals:
    /*
     * The timeline determines exactly what the
     * clicked bar represents, but it does not own
     * the filter controls.
     *
     * The surrounding investigation surface applies
     * this requested drill-down as one coordinated
     * filter transaction.
     */
    void bucketDrillDownRequested(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp,
        const QString &severity,
        const QString &subsystem
        );

protected:
    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    void rebuildBreakdownControls();

    void render();

    void showEmptyTimeline();

    void updateRangeLabel(
        int scrollValue
        );

    void requestBucketDrillDown(
        int visibleBucketIndex,
        const QString &severity,
        const QString &subsystem
        );

    std::optional<QDateTime>
    effectiveFirstTimestamp() const;

    std::optional<QDateTime>
    effectiveLastTimestamp() const;

    qint64 effectiveTimelineIntervalMilliseconds(
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp
        ) const;

    int responsiveVisibleBucketCount(
        const QDateTime &firstTimestamp,
        const QDateTime &lastTimestamp,
        qint64 intervalMilliseconds
        ) const;

    InvestigationSession *m_session =
        nullptr;

    QVector<InvestigationRecord>
        m_records;

    EventTimelineAnalyzer
        m_timelineAnalyzer;

    InvestigationAnalyticsAnalyzer
        m_analyticsAnalyzer;

    QChartView *m_chartView =
        nullptr;

    QComboBox *m_intervalCombo =
        nullptr;

    QWidget *m_breakdownWidget =
        nullptr;

    QComboBox *m_breakdownCombo =
        nullptr;

    QWidget *m_subsystemShowWidget =
        nullptr;

    QComboBox *m_subsystemLimitCombo =
        nullptr;

    QScrollBar *m_scrollBar =
        nullptr;

    QLabel *m_rangeLabel =
        nullptr;

    bool m_scaleValid =
        false;

    qint64 m_scaleIntervalMilliseconds =
        0;

    int m_scaleMaximum =
        1;

    QTimer *m_resizeRenderTimer =
        nullptr;
};