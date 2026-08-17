#include <QApplication>
#include <QCoreApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(
        QStringLiteral("TraceScope")
        );

    QCoreApplication::setApplicationName(
        QStringLiteral("TraceScope")
        );

    MainWindow window;
    window.show();

    return app.exec();
}