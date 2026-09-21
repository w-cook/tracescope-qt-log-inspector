#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QGuiApplication>

#include <algorithm>
#include <memory>

#include "MainWindow.h"
#include "ui/InterfaceScale.h"
#include "ui/InterfaceScaleStyle.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    QGuiApplication::
        setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::
            Floor
            );
#endif

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(
        QStringLiteral("TraceScope")
        );

    QCoreApplication::setApplicationName(
        QStringLiteral("TraceScope")
        );

    /*
     * Temporary Phase 16 test value.
     *
     * 1.00 means "match the operating system".
     *
     * We will replace this with the persisted user
     * preference after validating the architecture.
     */
    InterfaceScale::setUserFactor(
        1.0
        );

#ifdef Q_OS_WIN
    auto interfaceScaleStyle =
        std::make_unique<
            InterfaceScaleStyle
            >();

    app.setStyle(
        interfaceScaleStyle.get()
        );

    /*
     * QApplication::setStyle() takes ownership of
     * the style object and reparents it to qApp.
     */
    interfaceScaleStyle.release();
#endif

    /*
     * Qt already scales the base font according to
     * the operating system's logical DPI. Apply only
     * the user's relative TraceScope adjustment.
     */
    const qreal nativeScaleFactor =
        InterfaceScale::nativeFactor();

    if (!qFuzzyCompare(
            nativeScaleFactor,
            1.0
            )) {
        QFont scaledFont =
            app.font();

        if (scaledFont.pointSizeF() > 0.0) {
            scaledFont.setPointSizeF(
                scaledFont.pointSizeF()
                * nativeScaleFactor
                );
        } else if (
            scaledFont.pixelSize() > 0
            ) {
            scaledFont.setPixelSize(
                std::max(
                    1,
                    qRound(
                        scaledFont.pixelSize()
                        * nativeScaleFactor
                        )
                    )
                );
        }

        app.setFont(
            scaledFont
            );
    }

    MainWindow window;
    window.show();

    return app.exec();
}