#pragma once

#include <QDialog>
#include <QString>

#include "../importing/ImportFormatSuggestionService.h"

class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QDragEnterEvent;
class QDropEvent;

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
    QDialogButtonBox *buttonBox;
    QPushButton *importButton;
    QLabel *formatSuggestionLabel;

    ImportFormatSuggestionService
        formatSuggestionService;

    void buildLayout();
    void browseForFile();
    void updateSourceState();
    void updateFormatSuggestion();
};