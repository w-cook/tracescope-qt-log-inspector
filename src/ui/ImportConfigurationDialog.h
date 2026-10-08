#pragma once

#include <QDialog>
#include <QFutureWatcher>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "../importing/ImportFormatSuggestionService.h"
#include "../importing/ImportPreviewService.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportProfileSerialization.h"
#include "../importing/ImportProfileValidator.h"
#include "../preferences/RecentItemsStore.h"
#include "../sources/RotatedSourceDiscoveryService.h"
#include "../sources/RotatedSourceRule.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDragEnterEvent;
class QDropEvent;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QTableWidget;
class QTimer;
class QVBoxLayout;

class RotatedSourceSettingsStore;

class ImportConfigurationDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ImportConfigurationDialog(
        QWidget *parent = nullptr,
        RecentItemsStore *recentItemsStore = nullptr,
        RotatedSourceSettingsStore *rotatedSourceSettingsStore = nullptr
        );

    QString selectedFilePath() const;

    void setSelectedFilePath(
        const QString &filePath
        );

    ImportProfile configuredProfile() const;

    bool includeRotatedSources() const;

    const RotatedSourceRule &
    rotatedSourceRule() const;

    const QVector<RotatedSourceMatch> &
    rotatedSources() const;

protected:
    void dragEnterEvent(
        QDragEnterEvent *event
        ) override;

    void dropEvent(
        QDropEvent *event
        ) override;

    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    RecentItemsStore *recentItemsStore = nullptr;
    RotatedSourceSettingsStore *rotatedSourceSettingsStore = nullptr;

    QLineEdit *filePathEdit;
    QPushButton *browseButton;
    QLabel *formatSuggestionLabel;
    QCheckBox *includeRotatedSourcesCheckBox;
    QPushButton *reviewRotatedSourcesButton;

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
    QPushButton *recentProfilesButton;
    QMenu *recentProfilesMenu;

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

    ImportFormatSuggestionService formatSuggestionService;
    ImportPreviewService previewService;
    ImportProfileSerializer profileSerializer;
    ImportProfileValidator profileValidator;

    ImportProfile workingProfile;
    bool profileIsUserConfigured = false;

    QString rotationConfigurationSourcePath;
    RotatedSourceRule rotatedSourceRuleState;
    QVector<RotatedSourceMatch> rotatedSourceMatches;
    bool rotatedSourceRuleWasAutomaticallySuggested =
        false;

    QString previewSourceKey;
    QString customFieldDetectionSourceKey;
    QSet<QString> autoDetectedCustomFieldKeys;

    QWidget *responsiveBody = nullptr;
    QVBoxLayout *responsiveBodyLayout = nullptr;
    QScrollArea *responsiveProfileArea = nullptr;
    QGroupBox *responsivePreviewGroup = nullptr;
    QWidget *responsiveContent = nullptr;

    bool usingCompactLayout = false;
    bool responsiveUpdateQueued = false;
    int lastCompactTabIndex = 0;
    QList<int> lastWideSplitterSizes;

    QLabel *responsiveIntroLabel = nullptr;
    QGroupBox *responsiveSourceGroup = nullptr;
    QScrollArea *responsiveOuterScroll = nullptr;

    void buildLayout();
    void browseForFile();

    void updateRotatedSourceState();
    void updateRotatedSourceControls();
    void reviewRotatedSources();
    void refineSuggestedNumericRotationDirectionFromTimestamps();

    bool previewIncludesRotations() const;
    QStringList previewSourcePaths() const;
    QString previewSelectionKey() const;

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

    void loadProfileFromPath(
        const QString &filePath
        );

    void schedulePreviewRefresh();

    void startManualPreview();
    void cancelManualPreview();

    void displayPreviewResult(
        const QString &sourceKey,
        const ImportPreviewResult &preview,
        bool showSourceColumn,
        int physicalSourceCount
        );

    void applyDetectedCustomFieldMappings(
        const ImportPreviewResult &preview
        );

    void createProfileFromSource(
        bool userInitiated
        );

    void refreshRecentProfilesMenu();

    void updateResponsiveLayout();
};
