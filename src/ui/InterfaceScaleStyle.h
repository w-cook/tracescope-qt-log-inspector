#pragma once

#include <QProxyStyle>

class InterfaceScaleStyle
    : public QProxyStyle
{
public:
    InterfaceScaleStyle();

    int pixelMetric(
        PixelMetric metric,
        const QStyleOption *option = nullptr,
        const QWidget *widget = nullptr
        ) const override;

    int layoutSpacing(
        QSizePolicy::ControlType control1,
        QSizePolicy::ControlType control2,
        Qt::Orientation orientation,
        const QStyleOption *option = nullptr,
        const QWidget *widget = nullptr
        ) const override;
};