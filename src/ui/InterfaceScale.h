#pragma once

#include <QMargins>
#include <QSize>
#include <QtGlobal>

class QScreen;
class QWidget;

class InterfaceScale
{
public:
    /*
     * User-selected multiplier relative to the
     * operating system's recommended UI size.
     *
     * 1.00 = system size
     * 0.75 = 75% of system size
     * 1.25 = 125% of system size
     */
    static qreal userFactor();

    static void setUserFactor(
        qreal factor
        );

    /*
     * Fraction of the OS scale that remains after
     * Qt's raster DPR has been rounded down.
     *
     * Windows 125%:
     *     raster DPR 1
     *     residual   1.25
     *
     * Windows 200%:
     *     raster DPR 2
     *     residual   1.00
     *
     * Windows 250%:
     *     raster DPR 2
     *     residual   1.25
     */
    static qreal systemResidualFactor(
        const QWidget *widget = nullptr
        );

    /*
     * Factor for TraceScope-owned logical-pixel
     * constants.
     */
    static qreal geometryFactor(
        const QWidget *widget = nullptr
        );

    /*
     * Qt already applies the OS DPI to fonts and
     * native style metrics. These therefore need
     * only the user's relative adjustment.
     */
    static qreal nativeFactor();

    static int pixels(
        int value,
        const QWidget *widget = nullptr
        );

    static QSize size(
        int width,
        int height,
        const QWidget *widget = nullptr
        );

    static QMargins margins(
        int left,
        int top,
        int right,
        int bottom,
        const QWidget *widget = nullptr
        );

private:
    static QScreen *screenFor(
        const QWidget *widget
        );

    static qreal s_userFactor;
};