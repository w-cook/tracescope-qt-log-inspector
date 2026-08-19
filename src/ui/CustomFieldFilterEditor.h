#pragma once

#include <QWidget>
#include <QStringList>

#include "../models/InvestigationFilterProxyModel.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

class CustomFieldFilterEditor
    : public QWidget
{
    Q_OBJECT

public:
    explicit CustomFieldFilterEditor(
        QWidget *parent = nullptr
        );

    void setAvailableFields(
        const QStringList &fields
        );

    void setFilters(
        const CustomFieldFilterMap &filters
        );

    const CustomFieldFilterMap &
    filters() const;

    void addFilter(
        const QString &field,
        const QString &value
        );

    void clearFilters();

signals:
    void filtersChanged();

private:
    QComboBox *fieldCombo;
    QLineEdit *valueEdit;
    QPushButton *addButton;

    QWidget *activeFiltersWidget;
    QVBoxLayout *activeFiltersLayout;

    CustomFieldFilterMap m_filters;

    void addCurrentFilter();
    void removeFilter(
        const QString &field,
        const QString &value
        );

    void rebuildActiveFilters();
    void updateAddButton();
};