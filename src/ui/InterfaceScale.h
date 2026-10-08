#pragma once

#include <QObject>
#include <QMargins>
#include <QSize>
#include <QtGlobal>
#include <QVector>

class QApplication;
class QScreen;
class QWidget;

class InterfaceScale
    : public QObject
{
    Q_OBJECT

public:
    static InterfaceScale *instance();

    /*
     * Capture the unmodified QApplication font before
     * any TraceScope-relative scaling is applied.
     *
     * This gives every later scale change the same
     * stable OS-derived baseline instead of repeatedly
     * scaling an already-scaled font.
     */
    static void initializeApplication(
        QApplication *application
        );

    /*
     * Apply a new user multiplier immediately.
     *
     * Returns true when the effective factor changed.
     */
    static bool applyUserFactor(
        QApplication *application,
        qreal factor
        );

    /*
     * User-selected multiplier relative to the
     * operating system's recommended UI size.
     *
     * 1.00 = system size
     * 0.75 = 75% of system size
     * 1.25 = 125% of system size
     */
    static qreal userFactor();

    static qreal systemResidualFactor(
        const QWidget *widget = nullptr
        );

    static qreal geometryFactor(
        const QWidget *widget = nullptr
        );

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

    static const QVector<qreal> &
    presetUserFactors();

signals:
    /*
     * Persistent UI surfaces listen to this and
     * recompute their TraceScope-authored geometry.
     *
     * Qt-native font/style metrics have already been
     * updated before this signal is emitted.
     */
    void userFactorChanged(
        qreal factor
        );

private:
    explicit InterfaceScale(
        QObject *parent = nullptr
        );

    static QScreen *screenFor(
        const QWidget *widget
        );

    static qreal normalizedUserFactor(
        qreal factor
        );

    static void refreshApplicationPresentation(
        QApplication *application
        );

    static qreal s_userFactor;
};