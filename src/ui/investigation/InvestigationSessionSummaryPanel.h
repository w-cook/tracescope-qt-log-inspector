#pragma once

#include <QVector>
#include <QWidget>
#include <QString>

#include "../../domain/InvestigationRecord.h"

class InvestigationSession;
class QLabel;
class QResizeEvent;

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

protected:
    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    void setSummaryText(
        const QString &text
        );

    void updateDisplayedText();

    InvestigationSession *m_session = nullptr;
    QLabel *m_label = nullptr;

    QString m_fullText;
};