#include "InvestigationReportExportDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace
{

constexpr int DocumentListMinimumHeight =
    90;

constexpr int DocumentListMaximumHeight =
    180;

void configureDocumentList(
    QListWidget *list
    )
{
    list->setSelectionMode(
        QAbstractItemView::NoSelection
        );

    list->setMinimumHeight(
        DocumentListMinimumHeight
        );

    list->setMaximumHeight(
        DocumentListMaximumHeight
        );

    list->setAlternatingRowColors(
        true
        );
}

QListWidgetItem *makeCheckableItem(
    const QString &title,
    const QString &id,
    bool selected,
    QListWidget *list
    )
{
    auto *item =
        new QListWidgetItem(
            title,
            list
            );

    item->setData(
        Qt::UserRole,
        id
        );

    item->setFlags(
        item->flags()
        | Qt::ItemIsUserCheckable
        );

    item->setCheckState(
        selected
            ? Qt::Checked
            : Qt::Unchecked
        );

    return item;
}

}

InvestigationReportExportDialog::
    InvestigationReportExportDialog(
        QVector<InvestigationReportSessionSelection>
            sessions,
        QVector<InvestigationReportComparisonSelection>
            comparisons,
        InvestigationReportSelectionOrigin origin,
        const QString &initialTitle,
        QWidget *parent
        )
    : QDialog(parent),
    m_selectionModel(
        std::move(sessions),
        std::move(comparisons),
        std::move(origin)
        ),
    m_titleEdit(
        new QLineEdit(this)
        ),
    m_contextEdit(
        new QPlainTextEdit(this)
        ),
    m_sessionsList(
        new QListWidget(this)
        ),
    m_comparisonsList(
        new QListWidget(this)
        ),
    m_supportingEvidenceCheck(
        new QCheckBox(
            tr(
                "Include supporting evidence records"
                ),
            this
            )
        ),
    m_technicalAppendixCheck(
        new QCheckBox(
            tr(
                "Include technical import/profile appendix"
                ),
            this
            )
        ),
    m_buttons(
        new QDialogButtonBox(
            QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel,
            this
            )
        )
{
    setWindowTitle(
        tr("Export Investigation Report")
        );

    setMinimumWidth(
        600
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        10
        );

    auto *description =
        new QLabel(
            tr(
                "Create a self-contained investigation report "
                "from the selected sessions and comparisons. "
                "The report captures immutable investigation "
                "state when export begins."
                ),
            this
            );

    description->setWordWrap(
        true
        );

    layout->addWidget(
        description
        );

    /*
     * -------------------------------------------------
     * Report
     * -------------------------------------------------
     */

    auto *reportGroup =
        new QGroupBox(
            tr("Report"),
            this
            );

    auto *reportLayout =
        new QFormLayout(
            reportGroup
            );

    m_titleEdit->setText(
        initialTitle.trimmed()
        );

    m_titleEdit->setPlaceholderText(
        tr("Investigation report title")
        );

    m_contextEdit->setPlaceholderText(
        tr(
            "Optional investigation context, purpose, "
            "or background for report readers"
            )
        );

    m_contextEdit->setMaximumHeight(
        90
        );

    reportLayout->addRow(
        tr("Title:"),
        m_titleEdit
        );

    reportLayout->addRow(
        tr("Investigation context:"),
        m_contextEdit
        );

    layout->addWidget(
        reportGroup
        );

    /*
     * -------------------------------------------------
     * Included workspace documents
     * -------------------------------------------------
     */

    auto *documentsGroup =
        new QGroupBox(
            tr("Included Workspace Documents"),
            this
            );

    auto *documentsLayout =
        new QVBoxLayout(
            documentsGroup
            );

    configureDocumentList(
        m_sessionsList
        );

    configureDocumentList(
        m_comparisonsList
        );

    auto *sessionsLabel =
        new QLabel(
            tr("Sessions"),
            documentsGroup
            );

    auto sessionsFont =
        sessionsLabel->font();

    sessionsFont.setBold(
        true
        );

    sessionsLabel->setFont(
        sessionsFont
        );

    documentsLayout->addWidget(
        sessionsLabel
        );

    documentsLayout->addWidget(
        m_sessionsList
        );

    if (!m_selectionModel
             .comparisons()
             .isEmpty()) {
        auto *comparisonsLabel =
            new QLabel(
                tr("Comparisons"),
                documentsGroup
                );

        auto comparisonsFont =
            comparisonsLabel->font();

        comparisonsFont.setBold(
            true
            );

        comparisonsLabel->setFont(
            comparisonsFont
            );

        documentsLayout->addWidget(
            comparisonsLabel
            );

        documentsLayout->addWidget(
            m_comparisonsList
            );
    } else {
        m_comparisonsList->hide();
    }

    layout->addWidget(
        documentsGroup
        );

    /*
     * -------------------------------------------------
     * Report detail
     * -------------------------------------------------
     */

    auto *detailGroup =
        new QGroupBox(
            tr("Report Detail"),
            this
            );

    auto *detailLayout =
        new QVBoxLayout(
            detailGroup
            );

    m_supportingEvidenceCheck->setChecked(
        true
        );

    m_technicalAppendixCheck->setChecked(
        true
        );

    detailLayout->addWidget(
        m_supportingEvidenceCheck
        );

    detailLayout->addWidget(
        m_technicalAppendixCheck
        );

    layout->addWidget(
        detailGroup
        );

    layout->addWidget(
        m_buttons
        );

    QPushButton *exportButton =
        m_buttons->button(
            QDialogButtonBox::Ok
            );

    exportButton->setText(
        tr("Export...")
        );

    connect(
        m_buttons,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept
        );

    connect(
        m_buttons,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
        );

    connect(
        m_titleEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            updateValidation();
        }
        );

    connect(
        m_sessionsList,
        &QListWidget::itemChanged,
        this,
        [this](
            QListWidgetItem *item
            ) {
            if (item == nullptr) {
                return;
            }

            m_selectionModel
                .setSessionSelected(
                    item
                        ->data(Qt::UserRole)
                        .toString(),
                    item->checkState()
                        == Qt::Checked
                    );

            updateValidation();
        }
        );

    connect(
        m_comparisonsList,
        &QListWidget::itemChanged,
        this,
        [this](
            QListWidgetItem *item
            ) {
            if (item == nullptr) {
                return;
            }

            m_selectionModel
                .setComparisonSelected(
                    item
                        ->data(Qt::UserRole)
                        .toString(),
                    item->checkState()
                        == Qt::Checked
                    );

            /*
             * Selecting a comparison may have selected
             * its currently open source sessions.
             */
            syncSelectionFromModel();
            updateValidation();
        }
        );

    populateDocumentLists();
    updateValidation();
}

InvestigationReportConfiguration
    InvestigationReportExportDialog::
    configuration() const
{
    InvestigationReportConfiguration configuration;

    configuration.title =
        m_titleEdit
            ->text()
            .trimmed();

    configuration.context =
        m_contextEdit
            ->toPlainText()
            .trimmed();

    configuration.sessionIds =
        m_selectionModel
            .selectedSessionIds();

    configuration.comparisonIds =
        m_selectionModel
            .selectedComparisonIds();

    configuration.includeSupportingEvidence =
        m_supportingEvidenceCheck
            ->isChecked();

    configuration.includeTechnicalAppendix =
        m_technicalAppendixCheck
            ->isChecked();

    return configuration;
}

void InvestigationReportExportDialog::
    populateDocumentLists()
{
    {
        const QSignalBlocker blocker(
            m_sessionsList
            );

        m_sessionsList->clear();

        for (const InvestigationReportSessionSelection
                 &session
             : m_selectionModel.sessions()) {
            makeCheckableItem(
                session.documentTitle,
                session.sessionId,
                session.selected,
                m_sessionsList
                );
        }
    }

    {
        const QSignalBlocker blocker(
            m_comparisonsList
            );

        m_comparisonsList->clear();

        for (const InvestigationReportComparisonSelection
                 &comparison
             : m_selectionModel.comparisons()) {
            makeCheckableItem(
                comparison.documentTitle,
                comparison.comparisonId,
                comparison.selected,
                m_comparisonsList
                );
        }
    }
}

void InvestigationReportExportDialog::
    syncSelectionFromModel()
{
    const QStringList selectedSessions =
        m_selectionModel
            .selectedSessionIds();

    const QStringList selectedComparisons =
        m_selectionModel
            .selectedComparisonIds();

    const QSignalBlocker sessionBlocker(
        m_sessionsList
        );

    const QSignalBlocker comparisonBlocker(
        m_comparisonsList
        );

    for (int index = 0;
         index < m_sessionsList->count();
         ++index) {
        QListWidgetItem *item =
            m_sessionsList->item(
                index
                );

        if (item == nullptr) {
            continue;
        }

        const QString id =
            item
                ->data(Qt::UserRole)
                .toString();

        item->setCheckState(
            selectedSessions.contains(id)
                ? Qt::Checked
                : Qt::Unchecked
            );
    }

    for (int index = 0;
         index < m_comparisonsList->count();
         ++index) {
        QListWidgetItem *item =
            m_comparisonsList->item(
                index
                );

        if (item == nullptr) {
            continue;
        }

        const QString id =
            item
                ->data(Qt::UserRole)
                .toString();

        item->setCheckState(
            selectedComparisons.contains(id)
                ? Qt::Checked
                : Qt::Unchecked
            );
    }
}

void InvestigationReportExportDialog::
    updateValidation()
{
    const bool valid =
        !m_titleEdit
             ->text()
             .trimmed()
             .isEmpty()
        && m_selectionModel.hasSelection();

    if (QPushButton *exportButton =
        m_buttons->button(
            QDialogButtonBox::Ok
            )) {
        exportButton->setEnabled(
            valid
            );
    }
}