#include "InterfaceScaleStyle.h"

#include <algorithm>

#include "InterfaceScale.h"

InterfaceScaleStyle::
    InterfaceScaleStyle()
#ifdef Q_OS_WIN
    : QProxyStyle(
          QStringLiteral("windowsvista")
          )
#else
    : QProxyStyle()
#endif
{
}

int InterfaceScaleStyle::pixelMetric(
    PixelMetric metric,
    const QStyleOption *option,
    const QWidget *widget
    ) const
{
    const int nativeValue =
        QProxyStyle::pixelMetric(
            metric,
            option,
            widget
            );

    /*
     * Negative metrics have special semantic
     * meanings in Qt. Never scale those.
     */
    const qreal nativeScaleFactor =
        InterfaceScale::nativeFactor();

    if (
        nativeValue <= 0
        || qFuzzyCompare(
            nativeScaleFactor,
            1.0
            )
        ) {
        return nativeValue;
    }

    return std::max(
        1,
        qRound(
            nativeValue
            * nativeScaleFactor
            )
        );
}

int InterfaceScaleStyle::layoutSpacing(
    QSizePolicy::ControlType control1,
    QSizePolicy::ControlType control2,
    Qt::Orientation orientation,
    const QStyleOption *option,
    const QWidget *widget
    ) const
{
    const int nativeSpacing =
        QProxyStyle::layoutSpacing(
            control1,
            control2,
            orientation,
            option,
            widget
            );

    const qreal nativeScaleFactor =
        InterfaceScale::nativeFactor();

    /*
     * -1 means the style does not provide an
     * explicit spacing preference.
     */
    if (
        nativeSpacing < 0
        || qFuzzyCompare(
            nativeScaleFactor,
            1.0
            )
        ) {
        return nativeSpacing;
    }

    return std::max(
        1,
        qRound(
            nativeSpacing
            * nativeScaleFactor
            )
        );
}