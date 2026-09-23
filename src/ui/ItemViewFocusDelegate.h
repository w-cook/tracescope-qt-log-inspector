#pragma once

#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

/*
 * Preserve keyboard/current-cell focus for item views
 * without relying on the platform's native dotted /
 * dashed focus rectangle.
 *
 * Selection painting remains completely native.
 * Only State_HasFocus is removed from Qt's ordinary
 * item painting, after which a solid current-cell
 * outline is drawn explicitly.
 */
class ItemViewFocusDelegate
    : public QStyledItemDelegate
{
public:
    explicit ItemViewFocusDelegate(
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

        const bool hasFocus =
            itemOption.state
            & QStyle::State_HasFocus;

        itemOption.state &=
            ~QStyle::State_HasFocus;

        QStyledItemDelegate::paint(
            painter,
            itemOption,
            index
            );

        if (hasFocus) {
            drawCurrentCellIndicator(
                painter,
                option
                );
        }
    }

protected:
    static void drawCurrentCellIndicator(
        QPainter *painter,
        const QStyleOptionViewItem &option
        )
    {
        if (painter == nullptr
            || !(option.state
                 & QStyle::State_HasFocus)) {
            return;
        }

        QPalette::ColorGroup colorGroup;

        if (!(option.state
              & QStyle::State_Enabled)) {
            colorGroup =
                QPalette::Disabled;
        } else if (
            option.state
            & QStyle::State_Active
            ) {
            colorGroup =
                QPalette::Active;
        } else {
            colorGroup =
                QPalette::Inactive;
        }

        const bool selected =
            option.state
            & QStyle::State_Selected;

        /*
         * Against a selected cell, use highlighted text
         * so the indicator contrasts with the native
         * selection fill.
         *
         * Against an ordinary cell, use the platform's
         * highlight color.
         */
        const QColor indicatorColor =
            option.palette.color(
                colorGroup,
                selected
                    ? QPalette::HighlightedText
                    : QPalette::Highlight
                );

        painter->save();

        QPen pen(
            indicatorColor
            );

        pen.setStyle(
            Qt::SolidLine
            );

        pen.setWidth(
            1
            );

        painter->setPen(
            pen
            );

        painter->setBrush(
            Qt::NoBrush
            );

        painter->drawRect(
            option.rect.adjusted(
                0,
                0,
                -1,
                -1
                )
            );

        painter->restore();
    }
};