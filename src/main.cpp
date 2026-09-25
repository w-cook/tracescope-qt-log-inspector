#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>

#include <algorithm>
#include <memory>

#include "MainWindow.h"

#include "preferences/InterfaceScaleSettingsStore.h"
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

    InterfaceScale::initializeApplication(
        &app
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

    QSettings startupSettings;

    InterfaceScaleSettingsStore
        interfaceScaleSettingsStore(
            startupSettings
            );

    InterfaceScale::applyUserFactor(
        &app,
        interfaceScaleSettingsStore
            .userFactor()
        );


    MainWindow window;

    /*
     * Respect the scaled preferred size when possible,
     * but never request an initial window larger than
     * the available desktop area.
     */
    if (QScreen *screen =
        QGuiApplication::primaryScreen();
        screen != nullptr) {

        const QRect available =
            screen->availableGeometry();

        constexpr int screenMargin = 48;

        const int maximumWidth =
            std::max(
                1,
                available.width() - screenMargin
                );

        const int maximumHeight =
            std::max(
                1,
                available.height() - screenMargin
                );

        /*
         * The main window also has a minimum width.
         * Ensure it cannot defeat the screen constraint
         * on smaller displays.
         */
        if (window.minimumWidth() > maximumWidth) {
            window.setMinimumWidth(maximumWidth);
        }

        window.resize(
            std::min(window.width(), maximumWidth),
            std::min(window.height(), maximumHeight)
            );

        window.move(
            available.center()
            - QPoint(
                window.width() / 2,
                window.height() / 2
                )
            );
    }

    window.show();

    return app.exec();
}