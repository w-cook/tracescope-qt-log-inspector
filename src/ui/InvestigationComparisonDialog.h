#pragma once

#include <optional>

#include <QDialog>
#include <QString>

#include "../analysis/BurstDetectionSettings.h"

class InvestigationWorkspace;

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

    std::optional<BurstDetectionSettings>
    burstSettings() const;

private:
    void populateSessions(
        const QString &initialBaselineSessionId,
        const QString &initialComparisonSessionId
        );

    void applySharedBurstDefaults();

    void updateValidation();

    InvestigationWorkspace *m_workspace =
        nullptr;

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