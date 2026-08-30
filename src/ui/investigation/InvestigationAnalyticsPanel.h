#pragma once

#include <QDateTime>
#include <QVector>
#include <QWidget>

#include "../../analysis/InvestigationAnalyticsAnalyzer.h"
#include "../../analysis/InvestigationBurst.h"
#include "../../analysis/InvestigationBurstAnalyzer.h"
#include "../../analysis/InvestigationCadenceAnalyzer.h"
#include "../../domain/InvestigationRecord.h"
#include "../../workspace/InvestigationPresentationState.h"

class InvestigationSession;
class QLabel;
class QGroupBox;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QSplitter;

class InvestigationAnalyticsPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationAnalyticsPanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    void updateRecords(
        const QVector<InvestigationRecord> &records
        );

    void clear();

    InvestigationAnalyticsPresentationState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationAnalyticsPresentationState &state
        );

signals:
    void burstDrillDownRequested(
        const QDateTime &startTimestamp,
        const QDateTime &endTimestamp
        );

private:
    void restoreSelectedTab();

    void updateOverview();

    void updateBursts();

    void updateBurstDetail(
        int row
        );

    void showBurstSettingsDialog();

    void requestBurstDrillDown(
        int row
        );

    InvestigationSession *m_session =
        nullptr;

    QVector<InvestigationRecord>
        m_records;

    QVector<InvestigationBurst>
        m_currentBursts;

    InvestigationAnalyticsAnalyzer
        m_analyticsAnalyzer;

    InvestigationBurstAnalyzer
        m_burstAnalyzer;

    InvestigationCadenceAnalyzer
        m_cadenceAnalyzer;

    QTabWidget *m_tabs =
        nullptr;

    QWidget *m_overviewPage =
        nullptr;

    QWidget *m_burstsPage =
        nullptr;

    QLabel *m_overviewEmptyLabel =
        nullptr;

    QGroupBox *m_eventCodeGroup =
        nullptr;

    QTableWidget *m_eventCodeTable =
        nullptr;

    QGroupBox *m_entityGroup =
        nullptr;

    QTableWidget *m_entityTable =
        nullptr;

    QPushButton *m_burstSettingsButton =
        nullptr;

    QTableWidget *m_burstTable =
        nullptr;

    QPlainTextEdit *m_burstDetailText =
        nullptr;

    QSplitter *m_overviewSplitter =
        nullptr;

    QSplitter *m_burstSplitter =
        nullptr;
};