#include "InvestigationSessionSummaryPanel.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QSizePolicy>

#include "../../workspace/InvestigationSession.h"

InvestigationSessionSummaryPanel::
    InvestigationSessionSummaryPanel(
        InvestigationSession *session,
        QWidget *parent
        )
    : QWidget(parent),
    m_session(session),
    m_label(
        new QLabel(this)
        )
{
    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->addWidget(
        m_label
        );

    m_label->setMinimumWidth(
        0
        );

    m_label->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Preferred
        );

    setSummaryText(
        tr("No log file loaded.")
        );
}

void InvestigationSessionSummaryPanel::refresh(
    const QVector<InvestigationRecord>
        &visibleRecords
    )
{
    if (m_session == nullptr) {
        setSummaryText(
            tr("No log file loaded.")
            );

        return;
    }

    InvestigationController *controller =
        m_session
            ->investigationController();

    if (controller == nullptr) {
        setSummaryText(
            tr("No log file loaded.")
            );

        return;
    }

    const QString sourcePath =
        m_session
            ->sourceMetadata()
            .sourcePath;

    if (!m_session->hasSeverityData()) {
        setSummaryText(
            QString(
                "TOTAL: %1 visible of %2 events from %3"
                )
                .arg(
                    visibleRecords.size()
                    )
                .arg(
                    controller
                        ->totalRecordCount()
                    )
                .arg(
                    sourcePath
                    )
            );

        return;
    }

    int traceCount = 0;
    int debugCount = 0;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;

    for (const InvestigationRecord &record
         : visibleRecords) {
        if (!record.severity.has_value()) {
            continue;
        }

        switch (record.severity.value()) {
        case RecordSeverity::Trace:
            ++traceCount;
            break;

        case RecordSeverity::Debug:
            ++debugCount;
            break;

        case RecordSeverity::Info:
            ++infoCount;
            break;

        case RecordSeverity::Warning:
            ++warningCount;
            break;

        case RecordSeverity::Error:
            ++errorCount;
            break;

        case RecordSeverity::Critical:
            ++criticalCount;
            break;
        }
    }

    setSummaryText(
        QString(
            "Showing %1 of %2 events from %3 | "
            "TRACE: %4 | DEBUG: %5 | INFO: %6 | "
            "WARN: %7 | ERROR: %8 | CRITICAL: %9"
            )
            .arg(
                visibleRecords.size()
                )
            .arg(
                controller
                    ->totalRecordCount()
                )
            .arg(
                sourcePath
                )
            .arg(traceCount)
            .arg(debugCount)
            .arg(infoCount)
            .arg(warningCount)
            .arg(errorCount)
            .arg(criticalCount)
        );
}

void InvestigationSessionSummaryPanel::
    setSummaryText(
        const QString &text
        )
{
    m_fullText =
        text;

    m_label->setToolTip(
        m_fullText
        );

    updateDisplayedText();
}

void InvestigationSessionSummaryPanel::
    updateDisplayedText()
{
    if (m_label == nullptr) {
        return;
    }

    const int availableWidth =
        m_label->width();

    if (availableWidth <= 0) {
        m_label->setText(
            m_fullText
            );

        return;
    }

    const QFontMetrics metrics(
        m_label->font()
        );

    m_label->setText(
        metrics.elidedText(
            m_fullText,
            Qt::ElideMiddle,
            availableWidth
            )
        );
}

void InvestigationSessionSummaryPanel::
    resizeEvent(
        QResizeEvent *event
        )
{
    QWidget::resizeEvent(
        event
        );

    updateDisplayedText();
}