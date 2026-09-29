#pragma once

#include <optional>

#include <QDialog>
#include <QString>

#include "../analysis/BurstDetectionSettings.h"
#include "../workspace/InvestigationComparisonTimeRange.h"

class InvestigationSession;
class InvestigationWorkspace;

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QSpinBox;

class InvestigationComparisonDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit InvestigationComparisonDialog(
        InvestigationWorkspace *workspace,
        const QString &initialBaselineSessionId,
        const QString &initialComparisonSessionId,
        QWidget *parent = nullptr
        );

    QString baselineSessionId() const;
    QString comparisonSessionId() const;

    std::optional<InvestigationComparisonTimeRange>
    baselineTimeRange() const;

    std::optional<InvestigationComparisonTimeRange>
    comparisonTimeRange() const;

    std::optional<BurstDetectionSettings>
    burstSettings() const;

private:
    void populateSessions(
        const QString &initialBaselineSessionId,
        const QString &initialComparisonSessionId
        );

    void applySharedBurstDefaults();

    void updateValidation();

    const InvestigationSession *selectedSession(
        const QString &sessionId
        ) const;

    void refreshTimeRangeOptions();

    InvestigationWorkspace *m_workspace =
        nullptr;

    QCheckBox *m_baselineTimeRangeCheck = nullptr;
    QCheckBox *m_comparisonTimeRangeCheck = nullptr;

    QComboBox *m_baselineCombo =
        nullptr;

    QComboBox *m_comparisonCombo =
        nullptr;

    QGroupBox *m_burstGroup =
        nullptr;

    QDoubleSpinBox *m_windowSpin =
        nullptr;

    QDoubleSpinBox *m_mergeGapSpin =
        nullptr;

    QSpinBox *m_elevatedSpin =
        nullptr;

    QSpinBox *m_errorCriticalSpin =
        nullptr;

    QLabel *m_validationLabel =
        nullptr;

    QDialogButtonBox *m_buttons =
        nullptr;
};