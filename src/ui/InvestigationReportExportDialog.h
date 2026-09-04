#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

#include "../exporting/InvestigationReportConfiguration.h"
#include "../exporting/InvestigationReportSelectionModel.h"

class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;

class InvestigationReportExportDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit InvestigationReportExportDialog(
        QVector<InvestigationReportSessionSelection> sessions,
        QVector<InvestigationReportComparisonSelection> comparisons,
        InvestigationReportSelectionOrigin origin,
        const QString &initialTitle,
        QWidget *parent = nullptr
        );

    InvestigationReportConfiguration
    configuration() const;

private:
    void populateDocumentLists();
    void syncSelectionFromModel();
    void updateValidation();

    InvestigationReportSelectionModel
        m_selectionModel;

    QLineEdit *m_titleEdit = nullptr;
    QPlainTextEdit *m_contextEdit = nullptr;

    QListWidget *m_sessionsList = nullptr;
    QListWidget *m_comparisonsList = nullptr;

    QCheckBox *m_supportingEvidenceCheck =
        nullptr;

    QCheckBox *m_technicalAppendixCheck =
        nullptr;

    QDialogButtonBox *m_buttons =
        nullptr;
};