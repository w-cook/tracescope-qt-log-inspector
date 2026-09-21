#include <QApplication>
#include <QCoreApplication>

#include "MainWindow.h"
#include "ui/NativeStyleAnimationSuppressor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    NativeStyleAnimationSuppressor
        nativeStyleAnimationSuppressor;

    app.installEventFilter(
        &nativeStyleAnimationSuppressor
        );

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