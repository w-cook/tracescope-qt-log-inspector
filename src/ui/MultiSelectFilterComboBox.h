#pragma once

#include <QComboBox>
#include <QString>
#include <QStringList>

class MultiSelectFilterComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit MultiSelectFilterComboBox(
        QWidget *parent = nullptr
        );

    void setEmptySelectionText(
        const QString &text
        );

    void addFilterItem(
        const QString &text,
        const QString &value
        );

    QStringList selectedValues() const;

    void setSelectedValues(
        const QStringList &values
        );

    void clearSelection();

signals:
    void selectionChanged();

protected:
    bool eventFilter(
        QObject *watched,
        QEvent *event
        ) override;

private:
    QString m_emptySelectionText;

    void updateDisplayText();
};