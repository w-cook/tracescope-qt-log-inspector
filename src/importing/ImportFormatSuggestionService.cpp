#include "ImportFormatSuggestionService.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>

namespace
{
ImportFormatSuggestion jsonLinesSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("json-lines"),
        QStringLiteral("JSON Lines"),
        reason
    };
}
}

ImportFormatSuggestion
ImportFormatSuggestionService::suggestForFile(
    const QString &filePath
    ) const
{
    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()
        || !fileInfo.isFile()) {
        return {};
    }

    const QString suffix =
        fileInfo.suffix().toCaseFolded();

    if (suffix == QStringLiteral("jsonl")
        || suffix == QStringLiteral("ndjson")) {
        return jsonLinesSuggestion(
            QStringLiteral(
                "The file extension indicates "
                "newline-delimited JSON."
                )
            );
    }

    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )) {
        return {};
    }

    constexpr int maximumSamples = 5;

    int sampledRecords = 0;

    while (!file.atEnd()
           && sampledRecords < maximumSamples) {
        const QByteArray line =
            file.readLine().trimmed();

        if (line.isEmpty()) {
            continue;
        }

        ++sampledRecords;

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                line,
                &parseError
                );

        if (parseError.error
                != QJsonParseError::NoError
            || !document.isObject()) {
            return {};
        }
    }

    if (sampledRecords == 0) {
        return {};
    }

    return jsonLinesSuggestion(
        QStringLiteral(
            "Sampled non-empty source records "
            "are individual JSON objects."
            )
        );
}