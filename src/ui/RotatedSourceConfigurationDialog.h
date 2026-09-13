#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

#include "../sources/RotatedSourceDiscoveryService.h"
#include "../sources/RotatedSourceRule.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QWidget;
class RotatedSourceSettingsStore;

class RotatedSourceConfigurationDialog final
    : public QDialog
{
    Q_OBJECT

public:
    explicit RotatedSourceConfigurationDialog(
        const QString &activeSourcePath,
        RotatedSourceRule initialRule,
        bool includeRotatedSources,
        RotatedSourceSettingsStore
            *rotatedSourceSettingsStore,
        QWidget *parent = nullptr
        );

    RotatedSourceRule configuredRule() const;

    const QVector<RotatedSourceMatch> &
    rotatedSources() const;

    bool includeRotatedSources() const;

private:
    RotatedSourceSettingsStore
        *m_rotatedSourceSettingsStore =
        nullptr;

    QString m_activeSourcePath;

    RotatedSourceRule m_workingRule;

    QComboBox *m_savedCustomRuleComboBox;
    QPushButton *m_saveCustomRuleButton;
    QPushButton *m_deleteCustomRuleButton;

    QVector<RotatedSourceMatch>
        m_rotatedSources;

    QComboBox *m_namingSchemeComboBox;
    QLabel *m_summaryLabel;
    QTableWidget *m_sourceTable;

    QWidget *m_customOptionsWidget;

    QLineEdit *m_customRegexEdit;
    QSpinBox *m_captureGroupSpinBox;

    QComboBox *m_orderValueTypeComboBox;
    QComboBox *m_orderDirectionComboBox;

    QLabel *m_dateTimeFormatLabel;
    QLineEdit *m_dateTimeFormatEdit;

    QCheckBox *m_includeRotatedSourcesCheckBox;

    QDialogButtonBox *m_buttonBox;
    QPushButton *m_useSourceFamilyButton;

    void buildLayout();
    void populateControls();

    void updateWorkingRuleFromControls();
    void updateCustomControlVisibility();
    void refreshDiscovery();
    void refreshSourceTable();

    void populateSavedCustomRules();
    void applySelectedSavedCustomRule();
    void updateSavedCustomRuleSelection();

    void saveCustomRule();
    void deleteSelectedCustomRule();
};