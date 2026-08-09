#pragma once

#include <QDialog>
#include <QString>

#include "../importing/ImportFormatSuggestionService.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportProfileValidator.h"

class QDialogButtonBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTableWidget;
class QComboBox;

class ImportConfigurationDialog final
    : public QDialog
{
    Q_OBJECT

public:
    explicit ImportConfigurationDialog(
        QWidget *parent = nullptr
        );

    QString selectedFilePath() const;

    void setSelectedFilePath(
        const QString &filePath
        );

    ImportProfile configuredProfile() const;

protected:
    void dragEnterEvent(
        QDragEnterEvent *event
        ) override;

    void dropEvent(
        QDropEvent *event
        ) override;

private:
    QLineEdit *filePathEdit;
    QPushButton *browseButton;
    QLabel *formatSuggestionLabel;

    QLineEdit *profileNameEdit;
    QCheckBox *preserveUnmappedCheckBox;

    QLineEdit *timestampPathEdit;
    QLineEdit *severityPathEdit;
    QLineEdit *subsystemPathEdit;
    QLineEdit *eventCodePathEdit;
    QLineEdit *entityIdPathEdit;
    QLineEdit *messagePathEdit;

    QTableWidget *customFieldTable;
    QPushButton *addCustomFieldButton;
    QPushButton *removeCustomFieldButton;

    QTableWidget *severityAliasTable;
    QPushButton *addSeverityAliasButton;
    QPushButton *removeSeverityAliasButton;

    QTableWidget *timestampRuleTable;
    QPushButton *addTimestampRuleButton;
    QPushButton *removeTimestampRuleButton;

    QLabel *validationLabel;

    QDialogButtonBox *buttonBox;
    QPushButton *importButton;

    ImportFormatSuggestionService
        formatSuggestionService;

    ImportProfileValidator
        profileValidator;

    ImportProfile workingProfile;

    void buildLayout();
    void browseForFile();

    void populateProfileControls();

    void populateCustomFieldMappings();

    void addCustomFieldMapping();
    void removeSelectedCustomFieldMapping();
    void updateCustomFieldMappings();

    void populateSeverityAliases();
    void addSeverityAlias();
    void removeSelectedSeverityAlias();
    void updateSeverityAliases();

    void populateTimestampRules();
    void addTimestampRule();
    void removeSelectedTimestampRule();
    void updateTimestampRules();

    QComboBox *createSeverityCombo(
        RecordSeverity severity
        );

    QComboBox *createTimestampRuleTypeCombo(
        TimestampRuleType type
        );

    void updateWorkingProfile();
    void updateSourceState();
    void updateFormatSuggestion();
    void updateValidationState();
    void updateImportAvailability();
};