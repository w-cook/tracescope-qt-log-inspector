#include "LiveLogGeneratorLauncherWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(
        argc,
        argv
        );

    QApplication::setApplicationName(
        QStringLiteral(
            "TraceScopeLiveLogGeneratorLauncher"
            )
        );

    QApplication::setApplicationVersion(
        QStringLiteral("0.1.0")
        );

    LiveLogGeneratorLauncherWindow window;
    window.show();

    return application.exec();
}