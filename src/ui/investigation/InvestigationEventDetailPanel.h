#pragma once

#include <QGroupBox>

#include "../../domain/InvestigationRecord.h"
#include "../../domain/InvestigationRecordState.h"
#include "../../workspace/InvestigationPresentationState.h"

class QComboBox;
class QPlainTextEdit;
class QPushButton;
class QGridLayout;
class QResizeEvent;
class QLabel;

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

    InvestigationScrollState
    capturePresentationState() const;

    void restorePresentationState(
        const InvestigationScrollState &state
        );

signals:
    void findingStatusChangeRequested();
    void noteEditRequested();
    void bookmarkToggleRequested();

protected:
    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    void updateResponsiveControls();

    QGridLayout *m_stateLayout =
        nullptr;

    QLabel *m_findingStatusLabel =
        nullptr;

    bool m_compactControls =
        false;

    QPlainTextEdit *m_detailText = nullptr;

    QComboBox *m_findingStatusCombo = nullptr;

    QPushButton *m_noteButton = nullptr;
    QPushButton *m_bookmarkButton = nullptr;
};