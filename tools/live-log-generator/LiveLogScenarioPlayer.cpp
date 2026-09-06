#include "LiveLogScenarioPlayer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#include <QFile>

namespace
{
LiveLogScenarioPlayResult failure(
    const QString &code,
    const QString &message
    )
{
    LiveLogScenarioPlayResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

qint64 scaledDuration(
    qint64 durationMs,
    double speed
    )
{
    if (durationMs <= 0) {
        return 0;
    }

    return static_cast<qint64>(
        std::llround(
            static_cast<double>(
                durationMs
                ) / speed
            )
        );
}

LiveLogScenarioPlayResult writeBytes(
    QFile &file,
    const QByteArray &bytes
    )
{
    qint64 offset = 0;

    while (offset < bytes.size()) {
        const qint64 written =
            file.write(
                bytes.constData() + offset,
                bytes.size() - offset
                );

        if (written <= 0) {
            return failure(
                QStringLiteral(
                    "OUTPUT_WRITE_FAILED"
                    ),
                QStringLiteral(
                    "Could not write output file '%1': %2"
                    )
                    .arg(
                        file.fileName(),
                        file.errorString()
                        )
                );
        }

        offset += written;
    }

    if (!file.flush()) {
        return failure(
            QStringLiteral(
                "OUTPUT_FLUSH_FAILED"
                ),
            QStringLiteral(
                "Could not flush output file '%1': %2"
                )
                .arg(
                    file.fileName(),
                    file.errorString()
                    )
            );
    }

    return {};
}

LiveLogScenarioPlayResult initializeFile(
    QFile &file,
    const ILogRecordRenderer &renderer
    )
{
    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )) {
        return failure(
            QStringLiteral(
                "OUTPUT_FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "Could not open output file '%1': %2"
                )
                .arg(
                    file.fileName(),
                    file.errorString()
                    )
            );
    }

    return writeBytes(
        file,
        renderer.initialContent()
        );
}
}

LiveLogScenarioPlayer::LiveLogScenarioPlayer(
    SleepFunction sleepFunction
    )
    : sleepFunction(
          std::move(sleepFunction)
          )
{
    if (!this->sleepFunction) {
        this->sleepFunction =
            [](qint64 milliseconds) {
                if (milliseconds <= 0) {
                    return;
                }

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(
                        milliseconds
                        )
                    );
            };
    }
}

LiveLogScenarioPlayResult
LiveLogScenarioPlayer::play(
    const LiveLogScenario &scenario,
    const ILogRecordRenderer &renderer,
    const LiveLogScenarioPlayerOptions &options
    ) const
{
    if (options.outputPath.trimmed().isEmpty()) {
        return failure(
            QStringLiteral(
                "INVALID_OUTPUT_PATH"
                ),
            QStringLiteral(
                "The output path must not be empty."
                )
            );
    }

    if (!std::isfinite(options.speed)
        || options.speed <= 0.0) {
        return failure(
            QStringLiteral(
                "INVALID_PLAYBACK_SPEED"
                ),
            QStringLiteral(
                "Playback speed must be a finite "
                "number greater than 0."
                )
            );
    }

    const QDateTime scenarioStart =
        options.scenarioStart.isValid()
            ? options.scenarioStart.toUTC()
            : QDateTime::currentDateTimeUtc();

    QFile file(options.outputPath);

    LiveLogScenarioPlayResult result =
        initializeFile(
            file,
            renderer
            );

    if (!result.isSuccess()) {
        return result;
    }

    int rotationIndex = 0;

    for (const LiveLogScenarioStep &step
         : scenario.steps) {
        if (std::holds_alternative<
                LiveLogWaitStep
                >(step)) {
            const LiveLogWaitStep &waitStep =
                std::get<LiveLogWaitStep>(
                    step
                    );

            sleepFunction(
                scaledDuration(
                    waitStep.durationMs,
                    options.speed
                    )
                );

            continue;
        }

        if (std::holds_alternative<
                LiveLogRecordStep
                >(step)) {
            const LiveLogRecordStep &recordStep =
                std::get<LiveLogRecordStep>(
                    step
                    );

            const QByteArray rendered =
                renderer.renderRecord(
                    recordStep.record,
                    scenarioStart
                    );

            if (recordStep.write.mode ==
                LiveLogWriteMode::Normal) {
                result =
                    writeBytes(
                        file,
                        rendered
                        );

                if (!result.isSuccess()) {
                    return result;
                }

                continue;
            }

            if (rendered.size() < 2) {
                return failure(
                    QStringLiteral(
                        "PARTIAL_RECORD_TOO_SMALL"
                        ),
                    QStringLiteral(
                        "The rendered record is too "
                        "small for a partial write."
                        )
                    );
            }

            const qint64 calculatedSplit =
                std::llround(
                    static_cast<double>(
                        rendered.size()
                        )
                    * recordStep.write.splitFraction
                    );

            const qsizetype split =
                static_cast<qsizetype>(
                    std::clamp<qint64>(
                        calculatedSplit,
                        1,
                        rendered.size() - 1
                        )
                    );

            result =
                writeBytes(
                    file,
                    rendered.left(split)
                    );

            if (!result.isSuccess()) {
                return result;
            }

            sleepFunction(
                scaledDuration(
                    recordStep.write.holdMs,
                    options.speed
                    )
                );

            result =
                writeBytes(
                    file,
                    rendered.mid(split)
                    );

            if (!result.isSuccess()) {
                return result;
            }

            continue;
        }

        if (std::holds_alternative<
                LiveLogTruncateStep
                >(step)) {
            if (!file.resize(0)
                || !file.seek(0)) {
                return failure(
                    QStringLiteral(
                        "OUTPUT_TRUNCATE_FAILED"
                        ),
                    QStringLiteral(
                        "Could not truncate output file '%1': %2"
                        )
                        .arg(
                            file.fileName(),
                            file.errorString()
                            )
                    );
            }

            result =
                writeBytes(
                    file,
                    renderer.initialContent()
                    );

            if (!result.isSuccess()) {
                return result;
            }

            continue;
        }

        if (std::holds_alternative<
                LiveLogReplaceStep
                >(step)) {
            file.close();

            if (QFile::exists(
                    options.outputPath
                    )
                && !QFile::remove(
                    options.outputPath
                    )) {
                return failure(
                    QStringLiteral(
                        "OUTPUT_REPLACE_FAILED"
                        ),
                    QStringLiteral(
                        "Could not remove output file "
                        "'%1' during replacement."
                        )
                        .arg(
                            options.outputPath
                            )
                    );
            }

            file.setFileName(
                options.outputPath
                );

            result =
                initializeFile(
                    file,
                    renderer
                    );

            if (!result.isSuccess()) {
                return result;
            }

            continue;
        }

        if (std::holds_alternative<
                LiveLogRotateStep
                >(step)) {
            file.close();

            ++rotationIndex;

            const QString rotatedPath =
                QStringLiteral("%1.%2")
                    .arg(
                        options.outputPath
                        )
                    .arg(
                        rotationIndex
                        );

            if (QFile::exists(rotatedPath)
                && !QFile::remove(
                    rotatedPath
                    )) {
                return failure(
                    QStringLiteral(
                        "ROTATED_FILE_REMOVE_FAILED"
                        ),
                    QStringLiteral(
                        "Could not remove existing "
                        "rotated file '%1'."
                        )
                        .arg(rotatedPath)
                    );
            }

            if (!QFile::rename(
                    options.outputPath,
                    rotatedPath
                    )) {
                return failure(
                    QStringLiteral(
                        "OUTPUT_ROTATE_FAILED"
                        ),
                    QStringLiteral(
                        "Could not rotate '%1' to '%2'."
                        )
                        .arg(
                            options.outputPath,
                            rotatedPath
                            )
                    );
            }

            file.setFileName(
                options.outputPath
                );

            result =
                initializeFile(
                    file,
                    renderer
                    );

            if (!result.isSuccess()) {
                return result;
            }
        }
    }

    file.close();

    return {};
}