#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    QCoreApplication::setApplicationName("TraceScopeLiveLogGenerator");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Standalone deterministic live-log generator for TraceScope."
    );
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(application);

    QTextStream output(stdout);
    output << "TraceScope live-log generator scaffold.\n";

    return 0;
}