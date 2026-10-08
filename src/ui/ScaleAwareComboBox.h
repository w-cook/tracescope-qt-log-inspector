
#pragma once

#include <QComboBox>

class ScaleAwareComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

    QSize minimumSizeHint() const override
    {
        const QSize nativeMinimum =
            QComboBox::minimumSizeHint();

        return QSize(
            nativeMinimum.width(),
            QComboBox::sizeHint().height()
            );
    }
};