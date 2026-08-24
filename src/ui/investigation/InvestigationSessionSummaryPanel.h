#pragma once

#include <QVector>
#include <QWidget>

#include "../../domain/InvestigationRecord.h"

class InvestigationSession;
class QLabel;

class InvestigationSessionSummaryPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationSessionSummaryPanel(
        InvestigationSession *session,
        QWidget *parent = nullptr
        );

    void refresh(
        const QVector<InvestigationRecord> &visibleRecords
        );

private:
    InvestigationSession *m_session = nullptr;
    QLabel *m_label = nullptr;
};