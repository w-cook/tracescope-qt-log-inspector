#pragma once

#include <QWidget>

class InvestigationSession;
class QLabel;
class QTableWidget;

class InvestigationFindingsPanel
    : public QWidget
{
    Q_OBJECT

public:
    explicit InvestigationFindingsPanel(
        QWidget *parent = nullptr
        );

    void setSession(
        InvestigationSession *session
        );

    InvestigationSession *session() const;

    void refresh();

    void clear();

signals:
    void findingActivated(
        const QString &recordId
        );

private:
    void activateRow(
        int row
        );

    InvestigationSession *m_session =
        nullptr;

    QLabel *m_summaryLabel =
        nullptr;

    QTableWidget *m_table =
        nullptr;
};