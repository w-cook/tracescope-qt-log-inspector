#include "CustomFieldFilterEditor.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <utility>

CustomFieldFilterEditor::
    CustomFieldFilterEditor(
        QWidget *parent
        )
    : QWidget(parent),
    fieldCombo(
        new QComboBox(this)
        ),
    valueEdit(
        new QLineEdit(this)
        ),
    addButton(
        new QPushButton(
            tr("Add"),
            this
            )
        ),
    activeFiltersWidget(
        new QWidget(this)
        ),
    activeFiltersLayout(
        new QVBoxLayout(
            activeFiltersWidget
            )
        )
{
    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    mainLayout->setSpacing(
        4
        );

    auto *inputLayout =
        new QHBoxLayout();

    inputLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    inputLayout->setSpacing(
        6
        );

    auto *fieldLabel =
        new QLabel(
            tr("Custom field:"),
            this
            );

    auto *valueLabel =
        new QLabel(
            tr("Value:"),
            this
            );

    fieldCombo->setMinimumWidth(
        180
        );

    valueEdit->setMinimumWidth(
        220
        );

    valueEdit->setPlaceholderText(
        tr("Exact value")
        );

    addButton->setEnabled(
        false
        );

    inputLayout->addWidget(
        fieldLabel
        );

    inputLayout->addWidget(
        fieldCombo
        );

    inputLayout->addSpacing(
        8
        );

    inputLayout->addWidget(
        valueLabel
        );

    inputLayout->addWidget(
        valueEdit,
        1
        );

    inputLayout->addWidget(
        addButton
        );

    mainLayout->addLayout(
        inputLayout
        );

    activeFiltersLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    activeFiltersLayout->setSpacing(
        2
        );

    mainLayout->addWidget(
        activeFiltersWidget
        );

    activeFiltersWidget->setVisible(
        false
        );

    connect(
        fieldCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            updateAddButton();
        }
        );

    connect(
        valueEdit,
        &QLineEdit::textChanged,
        this,
        [this]() {
            updateAddButton();
        }
        );

    connect(
        valueEdit,
        &QLineEdit::returnPressed,
        this,
        [this]() {
            if (addButton->isEnabled()) {
                addCurrentFilter();
            }
        }
        );

    connect(
        addButton,
        &QPushButton::clicked,
        this,
        [this]() {
            addCurrentFilter();
        }
        );
}

void CustomFieldFilterEditor::
    setAvailableFields(
        const QStringList &fields
        )
{
    const QString currentField =
        fieldCombo
            ->currentData()
            .toString();

    const QSignalBlocker blocker(
        fieldCombo
        );

    fieldCombo->clear();

    for (const QString &field
         : fields) {
        if (field.isEmpty()) {
            continue;
        }

        fieldCombo->addItem(
            field,
            field
            );
    }

    const int previousIndex =
        fieldCombo->findData(
            currentField
            );

    if (previousIndex >= 0) {
        fieldCombo->setCurrentIndex(
            previousIndex
            );
    }

    /*
     * Remove filters for fields that are no
     * longer available. This can occur after a
     * session reload changes the imported schema.
     */
    for (
        auto iterator =
        m_filters.begin();
        iterator != m_filters.end();
        ) {
        if (fieldCombo->findData(
                iterator.key()
                ) < 0) {
            iterator =
                m_filters.erase(
                    iterator
                    );
        } else {
            ++iterator;
        }
    }

    rebuildActiveFilters();
    updateAddButton();
}

void CustomFieldFilterEditor::
    setFilters(
        const CustomFieldFilterMap &filters
        )
{
    CustomFieldFilterMap normalized;

    for (
        auto filterIterator =
        filters.constBegin();
        filterIterator !=
        filters.constEnd();
        ++filterIterator
        ) {
        const QString &field =
            filterIterator.key();

        /*
         * Do not restore stale criteria for a
         * field that does not exist in the current
         * investigation.
         */
        if (fieldCombo->findData(
                field
                ) < 0) {
            continue;
        }

        QStringList values;

        for (const QString &value
             : filterIterator.value()) {
            if (value.isEmpty()
                || values.contains(
                    value
                    )) {
                continue;
            }

            values.append(
                value
                );
        }

        if (!values.isEmpty()) {
            normalized.insert(
                field,
                values
                );
        }
    }

    m_filters =
        std::move(normalized);

    rebuildActiveFilters();
    updateAddButton();
}

const CustomFieldFilterMap &
    CustomFieldFilterEditor::
    filters() const
{
    return m_filters;
}

void CustomFieldFilterEditor::
    addFilter(
        const QString &field,
        const QString &value
        )
{
    if (field.isEmpty()
        || value.isEmpty()) {
        return;
    }

    /*
     * Only accept fields belonging to the
     * currently active investigation.
     */
    if (fieldCombo->findData(field) < 0) {
        return;
    }

    QStringList &values =
        m_filters[field];

    if (values.contains(value)) {
        return;
    }

    values.append(
        value
        );

    rebuildActiveFilters();
    updateAddButton();

    emit filtersChanged();
}

void CustomFieldFilterEditor::
    clearFilters()
{
    if (m_filters.isEmpty()) {
        return;
    }

    m_filters.clear();

    rebuildActiveFilters();
    updateAddButton();

    /*
     * Programmatic clearing does not emit
     * filtersChanged(). MainWindow performs its
     * own single applyFilters() after Reset or
     * session restoration.
     */
}

void CustomFieldFilterEditor::
    addCurrentFilter()
{
    const QString field =
        fieldCombo
            ->currentData()
            .toString();

    const QString value =
        valueEdit->text();

    if (field.isEmpty()
        || value.isEmpty()) {
        return;
    }

    const bool alreadyExists =
        m_filters
            .value(field)
            .contains(value);

    addFilter(
        field,
        value
        );

    if (!alreadyExists) {
        valueEdit->clear();
    }

    updateAddButton();
}

void CustomFieldFilterEditor::
    removeFilter(
        const QString &field,
        const QString &value
        )
{
    auto filterIterator =
        m_filters.find(
            field
            );

    if (filterIterator
        == m_filters.end()) {
        return;
    }

    QStringList &values =
        filterIterator.value();

    if (!values.removeOne(
            value
            )) {
        return;
    }

    if (values.isEmpty()) {
        m_filters.erase(
            filterIterator
            );
    }

    rebuildActiveFilters();
    updateAddButton();

    emit filtersChanged();
}

void CustomFieldFilterEditor::
    rebuildActiveFilters()
{
    while (
        QLayoutItem *item =
        activeFiltersLayout
            ->takeAt(0)
        ) {
        if (QWidget *widget =
            item->widget()) {
            widget->deleteLater();
        }

        delete item;
    }

    for (
        auto filterIterator =
        m_filters.constBegin();
        filterIterator !=
        m_filters.constEnd();
        ++filterIterator
        ) {
        const QString field =
            filterIterator.key();

        for (const QString &value
             : filterIterator.value()) {
            auto *row =
                new QWidget(
                    activeFiltersWidget
                    );

            auto *rowLayout =
                new QHBoxLayout(
                    row
                    );

            rowLayout->setContentsMargins(
                0,
                0,
                0,
                0
                );

            rowLayout->setSpacing(
                6
                );

            auto *label =
                new QLabel(
                    QStringLiteral(
                        "%1 = %2"
                        )
                        .arg(
                            field,
                            value
                            ),
                    row
                    );

            /*
             * Arbitrary imported values may
             * contain characters that QLabel
             * would otherwise interpret as rich
             * text.
             */
            label->setTextFormat(
                Qt::PlainText
                );

            label->setTextInteractionFlags(
                Qt::TextSelectableByMouse
                );

            auto *removeButton =
                new QPushButton(
                    tr("Remove"),
                    row
                    );

            removeButton->setToolTip(
                tr(
                    "Remove this custom-field "
                    "filter criterion"
                    )
                );

            rowLayout->addWidget(
                label
                );

            rowLayout->addStretch();

            rowLayout->addWidget(
                removeButton
                );

            connect(
                removeButton,
                &QPushButton::clicked,
                this,
                [
                    this,
                    field,
                    value
            ]() {
                    removeFilter(
                        field,
                        value
                        );
                }
                );

            activeFiltersLayout
                ->addWidget(
                    row
                    );
        }
    }

    activeFiltersWidget->setVisible(
        !m_filters.isEmpty()
        );
}

void CustomFieldFilterEditor::
    updateAddButton()
{
    const bool hasField =
        fieldCombo->currentIndex()
            >= 0
        && !fieldCombo
                ->currentData()
                .toString()
                .isEmpty();

    const bool hasValue =
        !valueEdit
             ->text()
             .isEmpty();

    addButton->setEnabled(
        hasField
        && hasValue
        );
}