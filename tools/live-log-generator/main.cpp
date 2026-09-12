#include "LiveLogScenarioLoader.h"
#include "LiveLogScenarioPlayer.h"
#include "LogRecordRendererFactory.h"

#include <cmath>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

namespace
{
int fail(
    const QString &message
    )
{
    QTextStream error(stderr);

    error << "Error: "
          << message
          << '\n';

    return 1;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    QCoreApplication::setApplicationName(
        QStringLiteral(
            "TraceScopeLiveLogGenerator"
            )
        );

    QCoreApplication::setApplicationVersion(
        QStringLiteral("0.1.0")
        );

    QCommandLineParser parser;

    parser.setApplicationDescription(
        QStringLiteral(
            "Standalone deterministic live-log "
            "generator for TraceScope."
            )
        );

    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption scenarioOption(
        QStringLiteral("scenario"),
        QStringLiteral(
            "Path to the live-log scenario JSON file."
            ),
        QStringLiteral("path")
        );

    const QCommandLineOption outputOption(
        QStringLiteral("output"),
        QStringLiteral(
            "Path to the generated log file."
            ),
        QStringLiteral("path")
        );

    const QCommandLineOption formatOption(
        QStringLiteral("format"),
        QStringLiteral(
            "Output format identifier."
            ),
        QStringLiteral("format-id")
        );

    const QCommandLineOption speedOption(
        QStringLiteral("speed"),
        QStringLiteral(
            "Playback speed multiplier."
            ),
        QStringLiteral("multiplier"),
        QStringLiteral("1")
        );

    const QCommandLineOption loopOption(
        QStringLiteral("loop"),
        QStringLiteral(
            "Continuously replay the scenario."
            )
        );

    const QCommandLineOption loopModeOption(
        QStringLiteral("loop-mode"),
        QStringLiteral(
            "Loop behavior: append or restart."
            ),
        QStringLiteral("mode"),
        QStringLiteral("append")
        );

    parser.addOption(scenarioOption);
    parser.addOption(outputOption);
    parser.addOption(formatOption);
    parser.addOption(speedOption);
    parser.addOption(loopOption);
    parser.addOption(loopModeOption);

    parser.process(application);

    if (!parser.isSet(scenarioOption)) {
        return fail(
            QStringLiteral(
                "Missing required --scenario option."
                )
            );
    }

    if (!parser.isSet(outputOption)) {
        return fail(
            QStringLiteral(
                "Missing required --output option."
                )
            );
    }

    if (!parser.isSet(formatOption)) {
        return fail(
            QStringLiteral(
                "Missing required --format option."
                )
            );
    }

    bool speedOk = false;

    const double speed =
        parser.value(speedOption)
            .toDouble(&speedOk);

    if (!speedOk
        || !std::isfinite(speed)
        || speed <= 0.0) {
        return fail(
            QStringLiteral(
                "--speed must be a finite number "
                "greater than 0."
                )
            );
    }

    const QString scenarioPath =
        parser.value(scenarioOption);

    const QString outputPath =
        parser.value(outputOption);

    const QString formatId =
        parser.value(formatOption)
            .trimmed()
            .toLower();

    const bool loop =
        parser.isSet(loopOption);

    const QString loopMode =
        parser.value(loopModeOption)
            .trimmed()
            .toLower();

    LiveLogLoopBehavior loopBehavior =
        LiveLogLoopBehavior::Append;

    if (loopMode
        == QStringLiteral("append")) {
        loopBehavior =
            LiveLogLoopBehavior::Append;
    }
    else if (loopMode
             == QStringLiteral("restart")) {
        loopBehavior =
            LiveLogLoopBehavior::Restart;
    }
    else {
        return fail(
            QStringLiteral(
                "--loop-mode must be either "
                "'append' or 'restart'."
                )
            );
    }

    const LiveLogScenarioLoadResult loadResult =
        LiveLogScenarioLoader().loadFile(
            scenarioPath
            );

    if (!loadResult.isSuccess()) {
        return fail(
            QStringLiteral(
                "%1: %2"
                )
                .arg(
                    loadResult.errorCode,
                    loadResult.errorMessage
                    )
            );
    }

    const LiveLogScenario &scenario =
        *loadResult.scenario;

    LogRecordRendererCreateResult
        rendererResult =
        LogRecordRendererFactory::create(
            formatId,
            scenario
            );

    if (!rendererResult.isSuccess()) {
        return fail(
            QStringLiteral(
                "%1: %2"
                )
                .arg(
                    rendererResult.errorCode,
                    rendererResult.errorMessage
                    )
            );
    }

    QTextStream output(stdout);

    output
        << "Scenario: "
        << scenario.name
        << '\n'
        << "Steps: "
        << scenario.steps.size()
        << '\n'
        << "Output: "
        << outputPath
        << '\n'
        << "Format: "
        << formatId
        << '\n'
        << "Speed: "
        << speed
        << "x\n"
        << "Loop: "
        << (
               loop
                   ? "yes"
                   : "no"
               )
        << '\n';

    if (loop) {
        output
            << "Loop mode: "
            << loopMode
            << '\n';
    }

    LiveLogScenarioPlayer player;

    const ILogRecordRenderer &renderer =
        *rendererResult.renderer;

    LiveLogScenarioPlayerOptions playOptions;

    playOptions.outputPath =
        outputPath;

    playOptions.speed =
        speed;

    playOptions.loop =
        loop;

    playOptions.loopBehavior =
        loopBehavior;

    playOptions.scenarioStart =
        QDateTime::currentDateTimeUtc();

    output
        << "Playback started at "
        << playOptions.scenarioStart
               .toString(
                   Qt::ISODateWithMs
                   )
        << '\n';

    output.flush();

    const LiveLogScenarioPlayResult
        playResult =
        player.play(
            scenario,
            renderer,
            playOptions
            );

    if (!playResult.isSuccess()) {
        return fail(
            QStringLiteral(
                "%1: %2"
                )
                .arg(
                    playResult.errorCode,
                    playResult.errorMessage
                    )
            );
    }

    output
        << "Playback completed."
        << '\n';

    output.flush();

    return 0;
}