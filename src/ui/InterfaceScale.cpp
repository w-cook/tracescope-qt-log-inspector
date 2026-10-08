#include "InterfaceScale.h"

#include <algorithm>
#include <cmath>

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QLayout>
#include <QScreen>
#include <QStyle>
#include <QWidget>

namespace
{
constexpr qreal StandardLogicalDpi =
    96.0;

constexpr qreal MinimumUserFactor =
    0.50;

constexpr qreal MaximumUserFactor =
    2.00;

const QVector<qreal> PresetUserFactors{
    0.50,
    0.75,
    0.80,
    0.90,
    1.00,
    1.10,
    1.20,
    1.25,
    1.50,
    1.75,
    2.00
};

QFont systemApplicationFont;

bool systemApplicationFontInitialized =
    false;
}

qreal InterfaceScale::s_userFactor =
    1.0;

InterfaceScale::InterfaceScale(
    QObject *parent
    )
    : QObject(parent)
{
}

InterfaceScale *InterfaceScale::instance()
{
    static InterfaceScale scale;

    return &scale;
}

void InterfaceScale::initializeApplication(
    QApplication *application
    )
{
    if (
        application == nullptr
        || systemApplicationFontInitialized
        ) {
        return;
    }

    /*
     * QApplication already reflects the OS logical
     * DPI here. Preserve that exact font as the
     * permanent system-relative baseline.
     */
    systemApplicationFont =
        application->font();

    systemApplicationFontInitialized =
        true;
}

qreal InterfaceScale::
    normalizedUserFactor(
        qreal factor
        )
{
    if (!std::isfinite(
            factor
            )) {
        return 1.0;
    }

    return std::clamp(
        factor,
        MinimumUserFactor,
        MaximumUserFactor
        );
}

bool InterfaceScale::applyUserFactor(
    QApplication *application,
    qreal factor
    )
{
    const qreal normalizedFactor =
        normalizedUserFactor(
            factor
            );

    if (qFuzzyCompare(
            normalizedFactor,
            s_userFactor
            )) {
        return false;
    }

    s_userFactor =
        normalizedFactor;

    refreshApplicationPresentation(
        application
        );

    emit instance()->userFactorChanged(
        s_userFactor
        );

    return true;
}

void InterfaceScale::
    refreshApplicationPresentation(
        QApplication *application
        )
{
    if (application == nullptr) {
        return;
    }

    if (!systemApplicationFontInitialized) {
        initializeApplication(
            application
            );
    }

    /*
     * Always scale from the untouched OS-derived
     * baseline. Never multiply the current font,
     * because repeated live changes would otherwise
     * compound.
     */
    QFont scaledFont =
        systemApplicationFont;

    if (scaledFont.pointSizeF() > 0.0) {
        scaledFont.setPointSizeF(
            scaledFont.pointSizeF()
            * nativeFactor()
            );
    } else if (
        scaledFont.pixelSize() > 0
        ) {
        scaledFont.setPixelSize(
            std::max(
                1,
                qRound(
                    scaledFont.pixelSize()
                    * nativeFactor()
                    )
                )
            );
    }

    application->setFont(
        scaledFont
        );

    /*
     * InterfaceScaleStyle reads nativeFactor()
     * dynamically. Re-polish existing widgets so
     * native size hints and style metrics are
     * recalculated for the new factor.
     */
    const QWidgetList widgets =
        application->allWidgets();

    for (QWidget *widget : widgets) {
        if (widget == nullptr) {
            continue;
        }

        if (QStyle *style =
            widget->style();
            style != nullptr) {
            style->unpolish(
                widget
                );

            style->polish(
                widget
                );
        }

        if (QLayout *layout =
            widget->layout();
            layout != nullptr) {
            layout->invalidate();
        }

        widget->updateGeometry();
        widget->update();
    }
}

qreal InterfaceScale::userFactor()
{
    return s_userFactor;
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

    return screen->logicalDotsPerInch()
           / StandardLogicalDpi;
#else
    Q_UNUSED(widget);

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

const QVector<qreal> &
InterfaceScale::presetUserFactors()
{
    return PresetUserFactors;
}