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

    QLineEdit *timestampPathEdit;
    QLineEdit *severityPathEdit;
    QLineEdit *subsystemPathEdit;
    QLineEdit *eventCodePathEdit;
    QLineEdit *entityIdPathEdit;
    QLineEdit *messagePathEdit;

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

    void updateWorkingProfile();
    void updateSourceState();
    void updateFormatSuggestion();
    void updateValidationState();
    void updateImportAvailability();
};