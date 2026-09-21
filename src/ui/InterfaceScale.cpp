#include "InterfaceScale.h"

#include <algorithm>

#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

namespace
{
constexpr qreal StandardLogicalDpi =
    96.0;

constexpr qreal MinimumUserFactor =
    0.50;

constexpr qreal MaximumUserFactor =
    2.00;
}

qreal InterfaceScale::s_userFactor =
    1.0;

qreal InterfaceScale::userFactor()
{
    return s_userFactor;
}

void InterfaceScale::setUserFactor(
    qreal factor
    )
{
    s_userFactor =
        std::clamp(
            factor,
            MinimumUserFactor,
            MaximumUserFactor
            );
}

QScreen *InterfaceScale::screenFor(
    const QWidget *widget
    )
{
    if (
        widget != nullptr
        && widget->screen() != nullptr
        ) {
        return widget->screen();
    }

    return QGuiApplication::
        primaryScreen();
}

qreal InterfaceScale::
    systemResidualFactor(
        const QWidget *widget
        )
{
#ifdef Q_OS_WIN
    QScreen *screen =
        screenFor(widget);

    if (screen == nullptr) {
        return 1.0;
    }

    /*
     * After Floor rounding, Qt exposes the fraction
     * removed from the raster DPR through effective
     * logical DPI.
     *
     * 120 / 96 = 1.25, for example.
     */
    return screen->logicalDotsPerInch()
           / StandardLogicalDpi;
#else
    Q_UNUSED(widget);

    /*
     * Do not duplicate Qt's normal high-DPI scaling
     * on platforms where TraceScope does not force
     * the Windows integer-DPR policy.
     */
    return 1.0;
#endif
}

qreal InterfaceScale::geometryFactor(
    const QWidget *widget
    )
{
    return systemResidualFactor(
               widget
               )
           * userFactor();
}

qreal InterfaceScale::nativeFactor()
{
    return userFactor();
}

int InterfaceScale::pixels(
    int value,
    const QWidget *widget
    )
{
    if (value == 0) {
        return 0;
    }

    return qRound(
        static_cast<qreal>(value)
        * geometryFactor(widget)
        );
}

QSize InterfaceScale::size(
    int width,
    int height,
    const QWidget *widget
    )
{
    return QSize(
        pixels(
            width,
            widget
            ),
        pixels(
            height,
            widget
            )
        );
}

QMargins InterfaceScale::margins(
    int left,
    int top,
    int right,
    int bottom,
    const QWidget *widget
    )
{
    return QMargins(
        pixels(
            left,
            widget
            ),
        pixels(
            top,
            widget
            ),
        pixels(
            right,
            widget
            ),
        pixels(
            bottom,
            widget
            )
        );
}