#include "InvestigationEventDetailPanel.h"

#include <algorithm>
#include <utility>

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QAction>
#include <QMenu>

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

    /*
     * ---------------------------------------------------------
     * Selected-event context menu
     * ---------------------------------------------------------
     *
     * Preserve the standard QPlainTextEdit context menu
     * so users can still copy selected text normally,
     * then append whole-record export actions.
     */
    m_detailText->setContextMenuPolicy(
        Qt::CustomContextMenu
        );

    connect(
        m_detailText,
        &QPlainTextEdit::
        customContextMenuRequested,
        this,
        [this](const QPoint &position) {
            QMenu *menu =
                m_detailText
                    ->createStandardContextMenu();

            menu->addSeparator();

            QAction *copyJsonAction =
                menu->addAction(
                    tr("Copy Event as JSON")
                    );

            QAction *copyTextAction =
                menu->addAction(
                    tr(
                        "Copy Event as "
                        "Formatted Text"
                        )
                    );

            const bool hasRecord =
                !m_detailText
                     ->toPlainText()
                     .isEmpty();

            copyJsonAction->setEnabled(
                hasRecord
                );

            copyTextAction->setEnabled(
                hasRecord
                );

            connect(
                copyJsonAction,
                &QAction::triggered,
                this,
                &InvestigationEventDetailPanel::
                copyStructuredJsonRequested
                );

            connect(
                copyTextAction,
                &QAction::triggered,
                this,
                &InvestigationEventDetailPanel::
                copyFormattedTextRequested
                );

            menu->exec(
                m_detailText->mapToGlobal(
                    position
                    )
                );

            delete menu;
        }
        );

    auto *layout =
        new QVBoxLayout(this);

    layout->setSpacing(
        4
        );

    /*
     * ---------------------------------------------------------
     * Investigation-state controls
     * ---------------------------------------------------------
     */
    m_stateLayout =
        new QGridLayout();

    m_stateLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    m_stateLayout->setHorizontalSpacing(
        6
        );

    m_stateLayout->setVerticalSpacing(
        4
        );

    m_findingStatusLabel =
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

    /*
     * Start in the normal single-row presentation:
     *
     * Finding status: [Status]
     *                   ... [Add Note] [Bookmark Event]
     *
     * updateResponsiveControls() moves the action
     * buttons to a second row only when the panel
     * becomes too narrow.
     */
    m_stateLayout->addWidget(
        m_findingStatusLabel,
        0,
        0
        );

    m_stateLayout->addWidget(
        m_findingStatusCombo,
        0,
        1
        );

    m_stateLayout->setColumnStretch(
        2,
        1
        );

    m_stateLayout->addWidget(
        m_noteButton,
        0,
        3
        );

    m_stateLayout->addWidget(
        m_bookmarkButton,
        0,
        4
        );

    layout->addLayout(
        m_stateLayout
        );

    layout->addWidget(
        m_detailText
        );

    /*
     * ---------------------------------------------------------
     * Investigation-state signals
     * ---------------------------------------------------------
     */
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

    /*
     * Apply the appropriate presentation once the
     * widget receives its initial geometry.
     */
    updateResponsiveControls();
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

InvestigationScrollState
    InvestigationEventDetailPanel::
    capturePresentationState() const
{
    InvestigationScrollState state;

    if (m_detailText == nullptr) {
        return state;
    }

    if (m_detailText->horizontalScrollBar()
        != nullptr) {
        state.horizontalValue =
            m_detailText
                ->horizontalScrollBar()
                ->value();
    }

    if (m_detailText->verticalScrollBar()
        != nullptr) {
        state.verticalValue =
            m_detailText
                ->verticalScrollBar()
                ->value();
    }

    return state;
}

void InvestigationEventDetailPanel::
    restorePresentationState(
        const InvestigationScrollState &state
        )
{
    if (m_detailText == nullptr) {
        return;
    }

    if (QScrollBar *horizontal =
        m_detailText
            ->horizontalScrollBar();
        horizontal != nullptr) {
        horizontal->setValue(
            std::clamp(
                state.horizontalValue,
                horizontal->minimum(),
                horizontal->maximum()
                )
            );
    }

    if (QScrollBar *vertical =
        m_detailText
            ->verticalScrollBar();
        vertical != nullptr) {
        vertical->setValue(
            std::clamp(
                state.verticalValue,
                vertical->minimum(),
                vertical->maximum()
                )
            );
    }
}

void InvestigationEventDetailPanel::
    updateResponsiveControls()
{
    if (m_stateLayout == nullptr) {
        return;
    }

    const int requiredWideWidth =
        m_findingStatusLabel
            ->sizeHint()
            .width()
        + m_findingStatusCombo
              ->sizeHint()
              .width()
        + m_noteButton
              ->sizeHint()
              .width()
        + m_bookmarkButton
              ->sizeHint()
              .width()
        + 6 * 3
        + 20;

    const bool compact =
        contentsRect().width()
        < requiredWideWidth;

    if (compact
        == m_compactControls) {
        return;
    }

    m_compactControls =
        compact;

    m_stateLayout->removeWidget(
        m_findingStatusLabel
        );

    m_stateLayout->removeWidget(
        m_findingStatusCombo
        );

    m_stateLayout->removeWidget(
        m_noteButton
        );

    m_stateLayout->removeWidget(
        m_bookmarkButton
        );

    for (int column = 0;
         column < 5;
         ++column) {
        m_stateLayout->setColumnStretch(
            column,
            0
            );
    }

    if (!compact) {
        /*
         * Normal width:
         *
         * Finding status: [Status]
         *                   ... [Add Note] [Bookmark Event]
         */
        m_stateLayout->addWidget(
            m_findingStatusLabel,
            0,
            0
            );

        m_stateLayout->addWidget(
            m_findingStatusCombo,
            0,
            1
            );

        m_stateLayout->setColumnStretch(
            2,
            1
            );

        m_stateLayout->addWidget(
            m_noteButton,
            0,
            3
            );

        m_stateLayout->addWidget(
            m_bookmarkButton,
            0,
            4
            );
    } else {
        /*
         * Narrow width:
         *
         * Finding status: [Status]
         * [Add Note] [Bookmark Event]
         */
        m_stateLayout->addWidget(
            m_findingStatusLabel,
            0,
            0
            );

        m_stateLayout->addWidget(
            m_findingStatusCombo,
            0,
            1
            );

        m_stateLayout->addWidget(
            m_noteButton,
            1,
            0
            );

        m_stateLayout->addWidget(
            m_bookmarkButton,
            1,
            1
            );

        m_stateLayout->setColumnStretch(
            1,
            1
            );
    }

    m_stateLayout->invalidate();
}

void InvestigationEventDetailPanel::
    resizeEvent(
        QResizeEvent *event
        )
{
    QGroupBox::resizeEvent(
        event
        );

    updateResponsiveControls();
}