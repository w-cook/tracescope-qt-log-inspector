#include "InvestigationEventDetailPanel.h"

#include <algorithm>
#include <utility>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include "../../domain/RecordSeverity.h"

InvestigationEventDetailPanel::
    InvestigationEventDetailPanel(
        QWidget *parent
        )
    : QGroupBox(
          tr("Selected Event Details"),
          parent
          ),
    m_detailText(
        new QPlainTextEdit(this)
        ),
    m_findingStatusCombo(
        new QComboBox(this)
        ),
    m_noteButton(
        new QPushButton(
            tr("Add Note"),
            this
            )
        ),
    m_bookmarkButton(
        new QPushButton(
            tr("Bookmark Event"),
            this
            )
        )
{
    m_detailText->setReadOnly(
        true
        );

    m_detailText->setPlaceholderText(
        tr(
            "Select a telemetry event to "
            "view its details."
            )
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        4
        );

    auto *stateLayout =
        new QHBoxLayout();

    stateLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    stateLayout->setSpacing(
        6
        );

    auto *findingStatusLabel =
        new QLabel(
            tr("Finding status:"),
            this
            );

    m_findingStatusCombo->addItem(
        tr("None"),
        static_cast<int>(
            FindingStatus::None
            )
        );

    m_findingStatusCombo->addItem(
        tr("Open"),
        static_cast<int>(
            FindingStatus::Open
            )
        );

    m_findingStatusCombo->addItem(
        tr("Resolved"),
        static_cast<int>(
            FindingStatus::Resolved
            )
        );

    m_findingStatusCombo->addItem(
        tr("Dismissed"),
        static_cast<int>(
            FindingStatus::Dismissed
            )
        );

    m_findingStatusCombo->setMinimumWidth(
        m_findingStatusCombo
            ->sizeHint()
            .width()
        + 8
        );

    m_findingStatusCombo->setToolTip(
        tr(
            "Set the investigation finding status "
            "for the selected event"
            )
        );

    m_noteButton->setToolTip(
        tr(
            "Add an analyst note to "
            "the selected event"
            )
        );

    m_bookmarkButton->setToolTip(
        tr(
            "Bookmark the selected event "
            "for later investigation"
            )
        );

    stateLayout->addWidget(
        findingStatusLabel
        );

    stateLayout->addWidget(
        m_findingStatusCombo
        );

    stateLayout->addStretch();

    stateLayout->addWidget(
        m_noteButton
        );

    stateLayout->addWidget(
        m_bookmarkButton
        );

    layout->addLayout(
        stateLayout
        );

    layout->addWidget(
        m_detailText
        );

    connect(
        m_findingStatusCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            emit findingStatusChangeRequested();
        }
        );

    connect(
        m_noteButton,
        &QPushButton::clicked,
        this,
        &InvestigationEventDetailPanel::
        noteEditRequested
        );

    connect(
        m_bookmarkButton,
        &QPushButton::clicked,
        this,
        &InvestigationEventDetailPanel::
        bookmarkToggleRequested
        );

    clearInvestigationState();
}

void InvestigationEventDetailPanel::
    displayRecord(
        const InvestigationRecord &record
        )
{
    QStringList lines;

    lines << QStringLiteral("Timestamp: ")
                 + (
                     record.timestamp.has_value()
                         ? record.timestamp
                               ->toString(
                                   Qt::ISODateWithMs
                                   )
                         : QString()
                     );

    lines << QStringLiteral("Level: ")
                 + (
                     record.severity.has_value()
                         ? recordSeverityToString(
                               record.severity.value()
                               )
                         : QString()
                     );

    lines << QStringLiteral("Subsystem: ")
                 + record.subsystem.value_or(
                     QString()
                     );

    lines << QStringLiteral("Event Code: ")
                 + record.eventCode.value_or(
                     QString()
                     );

    lines << QStringLiteral("Entity ID: ")
                 + record.entityId.value_or(
                     QString()
                     );

    lines << QString();
    lines << QStringLiteral("Message:");
    lines << record.message.value_or(
        QString()
        );

    if (!record.customAttributes.isEmpty()) {
        lines << QString();
        lines << QStringLiteral(
            "Custom Attributes:"
            );

        QStringList attributeKeys =
            record.customAttributes.keys();

        std::sort(
            attributeKeys.begin(),
            attributeKeys.end(),
            [](
                const QString &left,
                const QString &right
                ) {
                return left.compare(
                           right,
                           Qt::CaseInsensitive
                           )
                       < 0;
            }
            );

        for (const QString &key
             : std::as_const(attributeKeys)) {
            lines
                << QStringLiteral("%1: %2")
                       .arg(
                           key,
                           record
                               .customAttributes
                               .value(key)
                               .toString()
                           );
        }
    }

    m_detailText->setPlainText(
        lines.join(
            QStringLiteral("\n")
            )
        );
}

void InvestigationEventDetailPanel::
    clearRecord()
{
    m_detailText->clear();
}

void InvestigationEventDetailPanel::
    clearInvestigationState()
{
    {
        const QSignalBlocker blocker(
            m_findingStatusCombo
            );

        const int noneIndex =
            m_findingStatusCombo->findData(
                static_cast<int>(
                    FindingStatus::None
                    )
                );

        m_findingStatusCombo->setCurrentIndex(
            noneIndex
            );
    }

    m_findingStatusCombo->setEnabled(
        false
        );

    m_noteButton->setEnabled(
        false
        );

    m_noteButton->setText(
        tr("Add Note")
        );

    m_noteButton->setToolTip(
        tr(
            "Add an analyst note to "
            "the selected event"
            )
        );

    m_bookmarkButton->setEnabled(
        false
        );

    m_bookmarkButton->setText(
        tr("Bookmark Event")
        );
}

void InvestigationEventDetailPanel::
    setInvestigationState(
        const InvestigationRecordState &state
        )
{
    {
        const QSignalBlocker blocker(
            m_findingStatusCombo
            );

        const int statusIndex =
            m_findingStatusCombo->findData(
                static_cast<int>(
                    state.findingStatus
                    )
                );

        m_findingStatusCombo->setCurrentIndex(
            statusIndex
            );
    }

    m_findingStatusCombo->setEnabled(
        true
        );

    const bool hasNote =
        !state.note.trimmed().isEmpty();

    m_noteButton->setEnabled(
        true
        );

    m_noteButton->setText(
        hasNote
            ? tr("View/Edit Note")
            : tr("Add Note")
        );

    m_noteButton->setToolTip(
        hasNote
            ? state.note
            : tr(
                  "Add an analyst note to "
                  "the selected event"
                  )
        );

    m_bookmarkButton->setEnabled(
        true
        );

    m_bookmarkButton->setText(
        state.bookmarked
            ? tr("Remove Bookmark")
            : tr("Bookmark Event")
        );
}

FindingStatus
    InvestigationEventDetailPanel::
    selectedFindingStatus() const
{
    if (!m_findingStatusCombo
             ->currentData()
             .isValid()) {
        return FindingStatus::None;
    }

    return static_cast<FindingStatus>(
        m_findingStatusCombo
            ->currentData()
            .toInt()
        );
}