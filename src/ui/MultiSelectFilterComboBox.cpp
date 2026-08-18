#include "MultiSelectFilterComboBox.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSet>
#include <QStandardItem>
#include <QStandardItemModel>

MultiSelectFilterComboBox::
    MultiSelectFilterComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setModel(
        new QStandardItemModel(this)
        );

    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);

    lineEdit()->setReadOnly(true);
    lineEdit()->setCursor(
        Qt::ArrowCursor
        );

    view()
        ->viewport()
        ->installEventFilter(this);

    updateDisplayText();
}

void MultiSelectFilterComboBox::
    setEmptySelectionText(
        const QString &text
        )
{
    m_emptySelectionText = text;

    updateDisplayText();
}

void MultiSelectFilterComboBox::
    addFilterItem(
        const QString &text,
        const QString &value
        )
{
    auto *standardModel =
        qobject_cast<QStandardItemModel *>(
            model()
            );

    if (standardModel == nullptr) {
        return;
    }

    auto *item =
        new QStandardItem(text);

    item->setData(
        value,
        Qt::UserRole
        );

    item->setCheckable(true);

    item->setCheckState(
        Qt::Unchecked
        );

    standardModel->appendRow(
        item
        );

    updateDisplayText();
}

QStringList
MultiSelectFilterComboBox::selectedValues() const
{
    QStringList values;

    const auto *standardModel =
        qobject_cast<
            const QStandardItemModel *
            >(model());

    if (standardModel == nullptr) {
        return values;
    }

    for (
        int row = 0;
        row < standardModel->rowCount();
        ++row
        ) {
        const QStandardItem *item =
            standardModel->item(row);

        if (item == nullptr
            || item->checkState()
                   != Qt::Checked) {
            continue;
        }

        values.append(
            item
                ->data(Qt::UserRole)
                .toString()
            );
    }

    return values;
}

void MultiSelectFilterComboBox::
    setSelectedValues(
        const QStringList &values
        )
{
    auto *standardModel =
        qobject_cast<QStandardItemModel *>(
            model()
            );

    if (standardModel == nullptr) {
        return;
    }

    QSet<QString> selected;

    for (const QString &value : values) {
        selected.insert(value);
    }

    bool changed = false;

    for (
        int row = 0;
        row < standardModel->rowCount();
        ++row
        ) {
        QStandardItem *item =
            standardModel->item(row);

        if (item == nullptr) {
            continue;
        }

        const Qt::CheckState targetState =
            selected.contains(
                item
                    ->data(Qt::UserRole)
                    .toString()
                )
                ? Qt::Checked
                : Qt::Unchecked;

        if (item->checkState()
            == targetState) {
            continue;
        }

        item->setCheckState(
            targetState
            );

        changed = true;
    }

    updateDisplayText();

    if (changed) {
        emit selectionChanged();
    }
}

void MultiSelectFilterComboBox::
    clearSelection()
{
    setSelectedValues(
        QStringList()
        );
}

bool MultiSelectFilterComboBox::eventFilter(
    QObject *watched,
    QEvent *event
    )
{
    if (watched == view()->viewport()
        && event->type()
               == QEvent::MouseButtonRelease) {
        auto *mouseEvent =
            static_cast<QMouseEvent *>(
                event
                );

        if (mouseEvent->button()
            != Qt::LeftButton) {
            return QComboBox::eventFilter(
                watched,
                event
                );
        }

        const QModelIndex index =
            view()->indexAt(
                mouseEvent
                    ->position()
                    .toPoint()
                );

        if (!index.isValid()) {
            return QComboBox::eventFilter(
                watched,
                event
                );
        }

        auto *standardModel =
            qobject_cast<QStandardItemModel *>(
                model()
                );

        if (standardModel == nullptr) {
            return QComboBox::eventFilter(
                watched,
                event
                );
        }

        QStandardItem *item =
            standardModel->itemFromIndex(
                index
                );

        if (item == nullptr) {
            return true;
        }

        item->setCheckState(
            item->checkState()
                    == Qt::Checked
                ? Qt::Unchecked
                : Qt::Checked
            );

        updateDisplayText();

        emit selectionChanged();

        /*
         * Consume the release so QComboBox does
         * not treat this as a normal single-item
         * selection and close the popup.
         */
        return true;
    }

    return QComboBox::eventFilter(
        watched,
        event
        );
}

void MultiSelectFilterComboBox::
    updateDisplayText()
{
    QStringList labels;

    const auto *standardModel =
        qobject_cast<
            const QStandardItemModel *
            >(model());

    if (standardModel != nullptr) {
        for (
            int row = 0;
            row < standardModel->rowCount();
            ++row
            ) {
            const QStandardItem *item =
                standardModel->item(row);

            if (item != nullptr
                && item->checkState()
                       == Qt::Checked) {
                labels.append(
                    item->text()
                    );
            }
        }
    }

    QString displayText;

    if (labels.isEmpty()) {
        displayText =
            m_emptySelectionText;
    } else if (labels.size() <= 2) {
        displayText =
            labels.join(
                QStringLiteral(", ")
                );
    } else {
        displayText =
            tr("%1 selected")
                .arg(labels.size());
    }

    lineEdit()->setText(
        displayText
        );

    const QString toolTip =
        labels.isEmpty()
            ? m_emptySelectionText
            : labels.join(
                  QStringLiteral(", ")
                  );

    setToolTip(toolTip);
}