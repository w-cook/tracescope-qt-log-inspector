#pragma once

#include <QDialog>
#include <QString>
#include <QSet>
#include <QFutureWatcher>

#include "../importing/ImportFormatSuggestionService.h"
#include "../importing/ImportPreviewService.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportProfileSerialization.h"
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
class QPlainTextEdit;
class QTimer;

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

    QLabel *previewSummaryLabel;
    QTableWidget *previewTable;
    QPlainTextEdit *rawSourcePreview;

    QPushButton *refreshPreviewButton;

    QFutureWatcher<ImportPreviewResult>
        *previewWatcher = nullptr;

    QTimer *previewRefreshTimer;

    QLineEdit *profileNameEdit;
    QComboBox *importerComboBox;
    QLineEdit *recordPathEdit;
    QPlainTextEdit *regexPatternEdit;
    QCheckBox *preserveUnmappedCheckBox;

    QPushButton *newProfileFromSourceButton;
    QPushButton *loadProfileButton;
    QPushButton *saveProfileButton;

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

    ImportPreviewService
        previewService;

    ImportProfileSerializer
        profileSerializer;

    ImportProfileValidator
        profileValidator;

    ImportProfile workingProfile;
    bool profileIsUserConfigured = false;
    QString previewSourcePath;
    QString customFieldDetectionSourcePath;
    QSet<QString> autoDetectedCustomFieldKeys;

    void buildLayout();
    void browseForFile();

    void detectCustomFieldMappings();

    void populateProfileControls();

    void populateImporterOptions();
    void updateFormatSpecificControls();

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
    void updateValidationState(
        bool refreshPreview = true
        );
    void updateImportAvailability();
    void updatePreview();

    void clearPreview(
        const QString &message
        );

    void updateRawSourcePreview(
        int row
        );

    void saveProfile();
    void loadProfile();

    void schedulePreviewRefresh();

    void startManualPreview();

    void displayPreviewResult(
        const QString &filePath,
        const ImportPreviewResult &preview
        );

    void applyDetectedCustomFieldMappings(
        const ImportPreviewResult &preview
        );

    void createProfileFromSource(
        bool userInitiated
        );
};