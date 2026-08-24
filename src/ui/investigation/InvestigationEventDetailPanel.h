#pragma once

#include <QGroupBox>

#include "../../domain/InvestigationRecord.h"
#include "../../domain/InvestigationRecordState.h"

class QComboBox;
class QPlainTextEdit;
class QPushButton;

class InvestigationEventDetailPanel
    : public QGroupBox
{
    Q_OBJECT

public:
    explicit InvestigationEventDetailPanel(
        QWidget *parent = nullptr
        );

    void displayRecord(
        const InvestigationRecord &record
        );

    void clearRecord();

    void clearInvestigationState();

    void setInvestigationState(
        const InvestigationRecordState &state
        );

    FindingStatus selectedFindingStatus() const;

signals:
    void findingStatusChangeRequested();
    void noteEditRequested();
    void bookmarkToggleRequested();

private:
    QPlainTextEdit *m_detailText = nullptr;

    QComboBox *m_findingStatusCombo = nullptr;

    QPushButton *m_noteButton = nullptr;
    QPushButton *m_bookmarkButton = nullptr;
};