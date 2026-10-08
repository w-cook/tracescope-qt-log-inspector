#include "CustomFieldFilterEditor.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <utility>

#include "InterfaceScale.h"

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
        InterfaceScale::pixels(
            4,
            this
            )
        );

    inputLayout =
        new QHBoxLayout();

    inputLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    inputLayout->setSpacing(
        InterfaceScale::pixels(
            6,
            this
            )
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
        InterfaceScale::pixels(
            180,
            fieldCombo
            )
        );

    valueEdit->setMinimumWidth(
        InterfaceScale::pixels(
            220,
            valueEdit
            )
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

    fieldValueSpacing =
        new QSpacerItem(
            InterfaceScale::pixels(
                8,
                this
                ),
            0,
            QSizePolicy::Fixed,
            QSizePolicy::Minimum
            );

    inputLayout->addItem(
        fieldValueSpacing
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
        InterfaceScale::pixels(
            2,
            activeFiltersWidget
            )
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
    refreshInterfaceScale()
{
    if (layout() != nullptr) {
        layout()->setSpacing(
            InterfaceScale::pixels(
                4,
                this
                )
            );

        layout()->invalidate();
    }

    if (inputLayout != nullptr) {
        inputLayout->setSpacing(
            InterfaceScale::pixels(
                6,
                this
                )
            );

        inputLayout->invalidate();
    }

    fieldCombo->setMinimumWidth(
        InterfaceScale::pixels(
            180,
            fieldCombo
            )
        );

    valueEdit->setMinimumWidth(
        InterfaceScale::pixels(
            220,
            valueEdit
            )
        );

    if (fieldValueSpacing != nullptr) {
        fieldValueSpacing->changeSize(
            InterfaceScale::pixels(
                8,
                this
                ),
            0,
            QSizePolicy::Fixed,
            QSizePolicy::Minimum
            );
    }

    activeFiltersLayout->setSpacing(
        InterfaceScale::pixels(
            2,
            activeFiltersWidget
            )
        );

    /*
     * Active rows own scale-sensitive spacing that
     * was calculated when each row was constructed.
     * Rebuilding them is presentation-only and does
     * not alter the filter model or emit a change.
     */
    rebuildActiveFilters();

    updateGeometry();
    update();
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
                InterfaceScale::pixels(
                    6,
                    row
                    )
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