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

ImportFormatSuggestion delimitedTextSuggestion(
    const QString &importerId,
    const QString &displayName,
    const QString &reason
    )
{
    return {
        importerId,
        displayName,
        reason
    };
}

ImportFormatSuggestion structuredJsonSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("structured-json"),
        QStringLiteral("Structured JSON"),
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

    if (suffix == QStringLiteral("json")) {
        return structuredJsonSuggestion(
            QStringLiteral(
                "The file extension indicates "
                "a structured JSON document."
                )
            );
    }

    if (suffix == QStringLiteral("csv")) {
        return delimitedTextSuggestion(
            QStringLiteral("csv"),
            QStringLiteral("CSV"),
            QStringLiteral(
                "The file extension indicates "
                "comma-separated values."
                )
            );
    }

    if (suffix == QStringLiteral("tsv")) {
        return delimitedTextSuggestion(
            QStringLiteral("tsv"),
            QStringLiteral("TSV"),
            QStringLiteral(
                "The file extension indicates "
                "tab-separated values."
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