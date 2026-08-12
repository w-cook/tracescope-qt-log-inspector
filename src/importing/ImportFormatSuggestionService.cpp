#include "ImportFormatSuggestionService.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

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

ImportFormatSuggestion keyValueSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("key-value"),
        QStringLiteral("Key-Value / logfmt"),
        reason
    };
}

bool looksLikeKeyValueRecord(
    const QString &line
    )
{
    static const QRegularExpression
        assignmentPattern(
            QStringLiteral(
                R"((?:^|\s)[A-Za-z_][A-Za-z0-9_.:-]*=(?:"(?:\\.|[^"])*"|\S+))"
                )
            );

    QRegularExpressionMatchIterator matches =
        assignmentPattern.globalMatch(line);

    int assignmentCount = 0;
    qsizetype firstMatchStart = -1;

    while (matches.hasNext()) {
        const QRegularExpressionMatch match =
            matches.next();

        if (assignmentCount == 0) {
            firstMatchStart =
                match.capturedStart();
        }

        ++assignmentCount;
    }

    /*
     * Keep format suggestion conservative:
     * logfmt-style records normally begin with
     * an assignment and contain multiple fields.
     *
     * A single key=value fragment embedded in an
     * otherwise arbitrary text log should not cause
     * TraceScope to assume key-value format.
     */
    return firstMatchStart == 0
           && assignmentCount >= 2;
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

    bool allSamplesAreJsonObjects = true;
    bool allSamplesLookLikeKeyValue = true;

    while (!file.atEnd()
           && sampledRecords < maximumSamples) {
        const QByteArray rawLine =
            file.readLine().trimmed();

        if (rawLine.isEmpty()) {
            continue;
        }

        ++sampledRecords;

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                rawLine,
                &parseError
                );

        const bool isJsonObject =
            parseError.error
                == QJsonParseError::NoError
            && document.isObject();

        if (!isJsonObject) {
            allSamplesAreJsonObjects =
                false;
        }

        if (!looksLikeKeyValueRecord(
                QString::fromUtf8(rawLine)
                )) {
            allSamplesLookLikeKeyValue =
                false;
        }
    }

    if (sampledRecords == 0) {
        return {};
    }

    if (allSamplesAreJsonObjects) {
        return jsonLinesSuggestion(
            QStringLiteral(
                "Sampled non-empty source records "
                "are individual JSON objects."
                )
            );
    }

    if (allSamplesLookLikeKeyValue) {
        return keyValueSuggestion(
            QStringLiteral(
                "Sampled non-empty source records "
                "contain logfmt-style key-value assignments."
                )
            );
    }

    return {};
}