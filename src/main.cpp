#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QSettings>

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
    window.show();

    return app.exec();
}