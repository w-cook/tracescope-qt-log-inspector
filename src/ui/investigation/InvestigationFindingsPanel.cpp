#include "InvestigationFindingsPanel.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTextOption>
#include <QVBoxLayout>
#include <QScrollBar>

#include "../../domain/InvestigationRecord.h"
#include "../../domain/InvestigationRecordState.h"
#include "../../workspace/InvestigationSession.h"
#include "../../workspace/InvestigationStateStore.h"

namespace
{

QString findingStatusDisplayText(
    FindingStatus status
    )
{
    switch (status) {
    case FindingStatus::Open:
        return QObject::tr("Open");

    case FindingStatus::Resolved:
        return QObject::tr("Resolved");

    case FindingStatus::Dismissed:
        return QObject::tr("Dismissed");

    case FindingStatus::None:
        return QString();
    }

    return QString();
}

class FindingTextDelegate
    : public QStyledItemDelegate
{
public:
    explicit FindingTextDelegate(
        QObject *parent = nullptr
        )
        : QStyledItemDelegate(parent)
    {
    }

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
        ) const override
    {
        QStyleOptionViewItem itemOption(
            option
            );

        initStyleOption(
            &itemOption,
            index
            );

        QStyle *style =
            itemOption.widget != nullptr
                ? itemOption.widget->style()
                : QApplication::style();

        const QString text =
            itemOption.text;

        itemOption.text.clear();

        style->drawControl(
            QStyle::CE_ItemViewItem,
            &itemOption,
            painter,
            itemOption.widget
            );

        QTextDocument document;

        document.setDocumentMargin(
            0.0
            );

        document.setDefaultFont(
            option.font
            );

        QTextOption textOption;

        textOption.setWrapMode(
            QTextOption::
            WrapAtWordBoundaryOrAnywhere
            );

        document.setDefaultTextOption(
            textOption
            );

        document.setPlainText(
            text
            );

        constexpr int horizontalPadding = 8;
        constexpr int verticalPadding = 4;

        document.setTextWidth(
            std::max(
                1,
                option.rect.width()
                    - horizontalPadding
                )
            );

        QPalette::ColorGroup colorGroup;

        if (!(itemOption.state
              & QStyle::State_Enabled)) {
            colorGroup =
                QPalette::Disabled;
        } else if (
            itemOption.state
            & QStyle::State_Active
            ) {
            colorGroup =
                QPalette::Active;
        } else {
            colorGroup =
                QPalette::Inactive;
        }

        const bool selected =
            itemOption.state
            & QStyle::State_Selected;

        const QColor textColor =
            selected
                ? itemOption.palette.color(
                      colorGroup,
                      QPalette::HighlightedText
                      )
                : itemOption.palette.color(
                      colorGroup,
                      QPalette::Text
                      );

        QAbstractTextDocumentLayout::PaintContext
            context;

        context.palette =
            itemOption.palette;

        context.palette.setColor(
            QPalette::Text,
            textColor
            );

        context.palette.setColor(
            QPalette::WindowText,
            textColor
            );

        context.clip =
            QRectF(
                0,
                0,
                document.textWidth(),
                std::max(
                    1,
                    option.rect.height()
                        - verticalPadding
                    )
                );

        painter->save();

        painter->translate(
            option.rect.left() + 4,
            option.rect.top() + 2
            );

        document
            .documentLayout()
            ->draw(
                painter,
                context
                );

        painter->restore();
    }

    QSize sizeHint(
        const QStyleOptionViewItem &option,
        const QModelIndex &index
        ) const override
    {
        QStyleOptionViewItem itemOption(
            option
            );

        initStyleOption(
            &itemOption,
            index
            );

        QTextDocument document;

        document.setDocumentMargin(
            0.0
            );

        document.setDefaultFont(
            itemOption.font
            );

        QTextOption textOption;

        textOption.setWrapMode(
            QTextOption::
            WrapAtWordBoundaryOrAnywhere
            );

        document.setDefaultTextOption(
            textOption
            );

        document.setPlainText(
            itemOption.text
            );

        document.setTextWidth(
            std::max(
                1,
                itemOption.rect.width() - 8
                )
            );

        return QSize(
            itemOption.rect.width(),
            static_cast<int>(
                document.size().height()
                ) + 6
            );
    }
};

}

InvestigationFindingsPanel::
    InvestigationFindingsPanel(
        QWidget *parent
        )
    : QWidget(parent),
    m_summaryLabel(
        new QLabel(
            tr(
                "Open: 0    Resolved: 0    "
                "Dismissed: 0"
                ),
            this
            )
        ),
    m_table(
        new QTableWidget(
            0,
            4,
            this
            )
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

    layout->setSpacing(
        4
        );

    m_summaryLabel->setToolTip(
        tr(
            "Summary of explicitly classified "
            "investigation findings"
            )
        );

    layout->addWidget(
        m_summaryLabel
        );

    m_table->setHorizontalHeaderLabels({
        tr("Status"),
        tr("#"),
        tr("Time"),
        tr("Finding")
    });

    m_table
        ->horizontalHeaderItem(1)
        ->setToolTip(
            tr("Source record number")
            );

    m_table
        ->horizontalHeaderItem(2)
        ->setToolTip(
            tr(
                "Event time. Hover a value to see "
                "the complete timestamp."
                )
            );

    m_table->setAlternatingRowColors(
        true
        );

    m_table->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    m_table->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    m_table->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    m_table->setSortingEnabled(
        false
        );

    m_table->setItemDelegateForColumn(
        3,
        new FindingTextDelegate(
            m_table
            )
        );

    m_table->setWordWrap(
        true
        );

    m_table->setTextElideMode(
        Qt::ElideNone
        );

    m_table
        ->verticalHeader()
        ->setVisible(
            false
            );

    m_table
        ->verticalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
            );

    m_table
        ->verticalHeader()
        ->setMinimumSectionSize(
            m_table
                ->fontMetrics()
                .height()
            + 8
            );

    m_table
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents
            );

    m_table
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Fixed
            );

    m_table->setColumnWidth(
        1,
        58
        );

    m_table
        ->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents
            );

    m_table
        ->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::Stretch
            );

    m_table->setToolTip(
        tr(
            "Review conclusions recorded during "
            "this investigation"
            )
        );

    connect(
        m_table,
        &QTableWidget::cellDoubleClicked,
        this,
        [this](
            int row,
            int
            ) {
            activateRow(
                row
                );
        }
        );

    layout->addWidget(
        m_table,
        1
        );
}

void InvestigationFindingsPanel::setSession(
    InvestigationSession *session
    )
{
    m_session =
        session;

    refresh();
}

InvestigationSession *
InvestigationFindingsPanel::session() const
{
    return m_session;
}

void InvestigationFindingsPanel::refresh()
{
    m_table->setRowCount(
        0
        );

    int openCount = 0;
    int resolvedCount = 0;
    int dismissedCount = 0;

    if (m_session == nullptr) {
        m_summaryLabel->setText(
            tr(
                "Open: 0    Resolved: 0    "
                "Dismissed: 0"
                )
            );

        return;
    }

    const InvestigationStateStore *stateStore =
        m_session
            ->investigationStateStore();

    const QVector<InvestigationRecord> &records =
        m_session
            ->investigationController()
            ->allRecords();

    for (const InvestigationRecord &record
         : records) {
        if (record.recordId.isEmpty()) {
            continue;
        }

        const InvestigationRecordState state =
            stateStore->stateForRecord(
                record.recordId
                );

        /*
         * Bookmarking or adding a note alone does
         * not make an event a finding.
         */
        if (state.findingStatus
            == FindingStatus::None) {
            continue;
        }

        switch (state.findingStatus) {
        case FindingStatus::Open:
            ++openCount;
            break;

        case FindingStatus::Resolved:
            ++resolvedCount;
            break;

        case FindingStatus::Dismissed:
            ++dismissedCount;
            break;

        case FindingStatus::None:
            break;
        }

        const int row =
            m_table->rowCount();

        m_table->insertRow(
            row
            );

        auto *statusItem =
            new QTableWidgetItem(
                findingStatusDisplayText(
                    state.findingStatus
                    )
                );

        auto *sourceRecordItem =
            new QTableWidgetItem(
                QString::number(
                    record
                        .source
                        .recordNumber
                    )
                );

        QString timestampText;
        QString fullTimestampText;

        if (record.timestamp.has_value()) {
            timestampText =
                record.timestamp
                    ->toString(
                        QStringLiteral(
                            "HH:mm:ss.zzz"
                            )
                        );

            fullTimestampText =
                record.timestamp
                    ->toString(
                        Qt::ISODateWithMs
                        );
        }

        auto *timestampItem =
            new QTableWidgetItem(
                timestampText
                );

        if (!fullTimestampText.isEmpty()) {
            timestampItem->setToolTip(
                fullTimestampText
                );
        }

        const QString trimmedNote =
            state.note.trimmed();

        QString findingText;

        if (!trimmedNote.isEmpty()) {
            findingText =
                trimmedNote;
        } else {
            QString eventDescription;

            const QString eventCode =
                record.eventCode.value_or(
                    QString()
                    );

            const QString message =
                record.message.value_or(
                    QString()
                    );

            if (!eventCode.isEmpty()
                && !message.isEmpty()) {
                eventDescription =
                    QStringLiteral("%1 — %2")
                        .arg(
                            eventCode,
                            message
                            );
            } else if (!eventCode.isEmpty()) {
                eventDescription =
                    eventCode;
            } else if (!message.isEmpty()) {
                eventDescription =
                    message;
            } else {
                eventDescription =
                    tr(
                        "No event description"
                        );
            }

            findingText =
                tr("(No analyst note) — %1")
                    .arg(
                        eventDescription
                        );
        }

        auto *findingItem =
            new QTableWidgetItem(
                findingText
                );

        /*
         * Every cell carries the stable identity so
         * future presentation changes do not affect
         * navigation.
         */
        statusItem->setData(
            Qt::UserRole,
            record.recordId
            );

        sourceRecordItem->setData(
            Qt::UserRole,
            record.recordId
            );

        timestampItem->setData(
            Qt::UserRole,
            record.recordId
            );

        findingItem->setData(
            Qt::UserRole,
            record.recordId
            );

        if (!trimmedNote.isEmpty()) {
            findingItem->setToolTip(
                state.note
                );
        } else {
            findingItem->setToolTip(
                tr(
                    "This finding has no analyst "
                    "note yet."
                    )
                );
        }

        m_table->setItem(
            row,
            0,
            statusItem
            );

        m_table->setItem(
            row,
            1,
            sourceRecordItem
            );

        m_table->setItem(
            row,
            2,
            timestampItem
            );

        m_table->setItem(
            row,
            3,
            findingItem
            );
    }

    m_summaryLabel->setText(
        tr(
            "Open: %1    Resolved: %2    "
            "Dismissed: %3"
            )
            .arg(openCount)
            .arg(resolvedCount)
            .arg(dismissedCount)
        );

    m_table->resizeRowsToContents();

    m_table->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );
}

void InvestigationFindingsPanel::clear()
{
    m_session =
        nullptr;

    refresh();
}

InvestigationTablePresentationState
    InvestigationFindingsPanel::
    capturePresentationState() const
{
    InvestigationTablePresentationState
        state;

    if (m_table == nullptr) {
        return state;
    }

    const QModelIndex currentIndex =
        m_table->currentIndex();

    if (currentIndex.isValid()) {
        state.currentRow =
            currentIndex.row();

        state.currentColumn =
            currentIndex.column();
    }

    if (QScrollBar *horizontal =
        m_table->horizontalScrollBar();
        horizontal != nullptr) {
        state.scroll.horizontalValue =
            horizontal->value();
    }

    if (QScrollBar *vertical =
        m_table->verticalScrollBar();
        vertical != nullptr) {
        state.scroll.verticalValue =
            vertical->value();
    }

    return state;
}

void InvestigationFindingsPanel::
    restorePresentationState(
        const InvestigationTablePresentationState
            &state
        )
{
    if (m_table == nullptr) {
        return;
    }

    m_table->clearSelection();

    if (state.currentRow >= 0
        && state.currentRow
               < m_table->rowCount()
        && state.currentColumn >= 0
        && state.currentColumn
               < m_table->columnCount()) {
        m_table->setCurrentCell(
            state.currentRow,
            state.currentColumn
            );
    }

    if (QScrollBar *horizontal =
        m_table->horizontalScrollBar();
        horizontal != nullptr) {
        horizontal->setValue(
            std::clamp(
                state.scroll.horizontalValue,
                horizontal->minimum(),
                horizontal->maximum()
                )
            );
    }

    if (QScrollBar *vertical =
        m_table->verticalScrollBar();
        vertical != nullptr) {
        vertical->setValue(
            std::clamp(
                state.scroll.verticalValue,
                vertical->minimum(),
                vertical->maximum()
                )
            );
    }
}

void InvestigationFindingsPanel::activateRow(
    int row
    )
{
    if (row < 0
        || row >= m_table->rowCount()) {
        return;
    }

    QTableWidgetItem *item =
        m_table->item(
            row,
            0
            );

    if (item == nullptr) {
        return;
    }

    const QString recordId =
        item
            ->data(
                Qt::UserRole
                )
            .toString();

    if (recordId.isEmpty()) {
        return;
    }

    emit findingActivated(
        recordId
        );
}