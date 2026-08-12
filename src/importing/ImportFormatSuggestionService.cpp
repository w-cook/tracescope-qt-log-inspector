#include "ImportFormatSuggestionService.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

#include "BuiltInImportProfilePresets.h"

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

ImportFormatSuggestion syslogSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("syslog"),
        QStringLiteral(
            "Syslog (RFC 5424 / RFC 3164)"
            ),
        reason
    };
}

ImportFormatSuggestion apacheCommonSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("regex-text"),
        QStringLiteral(
            "Apache Common Access Log"
            ),
        reason,
        BuiltInImportProfilePresetIds::
        ApacheCommon
    };
}

ImportFormatSuggestion
apacheNginxCombinedSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("regex-text"),
        QStringLiteral(
            "Apache/Nginx Combined Access Log"
            ),
        reason,
        BuiltInImportProfilePresetIds::
        ApacheNginxCombined
    };
}

bool looksLikeApacheCommonRecord(
    const QString &line
    )
{
    static const QRegularExpression pattern(
        QStringLiteral(
            R"(^\S+\s+\S+\s+\S+\s+\[\d{1,2}/[A-Za-z]{3}/\d{4}:\d{2}:\d{2}:\d{2}\s+[+-]\d{4}\]\s+"[^"]*"\s+\d{3}\s+(?:\d+|-)$)"
            )
        );

    return pattern.match(line).hasMatch();
}

bool looksLikeApacheNginxCombinedRecord(
    const QString &line
    )
{
    static const QRegularExpression pattern(
        QStringLiteral(
            R"(^\S+\s+\S+\s+\S+\s+\[\d{1,2}/[A-Za-z]{3}/\d{4}:\d{2}:\d{2}:\d{2}\s+[+-]\d{4}\]\s+"[^"]*"\s+\d{3}\s+(?:\d+|-)\s+"[^"]*"\s+"[^"]*"$)"
            )
        );

    return pattern.match(line).hasMatch();
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

bool looksLikeRfc5424Record(
    const QString &line
    )
{
    static const QRegularExpression pattern(
        QStringLiteral(
            R"(^<(?<priority>\d{1,3})>(?<version>\d{1,3})\s+\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+(?:-|(?:\[.*\]))(?:\s.*)?$)"
            )
        );

    const QRegularExpressionMatch match =
        pattern.match(line);

    if (!match.hasMatch()) {
        return false;
    }

    bool priorityValid = false;
    const int priority =
        match.captured(
                 QStringLiteral("priority")
                 )
            .toInt(
                &priorityValid
                );

    bool versionValid = false;
    const int version =
        match.captured(
                 QStringLiteral("version")
                 )
            .toInt(
                &versionValid
                );

    return priorityValid
           && priority >= 0
           && priority <= 191
           && versionValid
           && version > 0;
}

bool looksLikeRfc3164Record(
    const QString &line
    )
{
    static const QRegularExpression pattern(
        QStringLiteral(
            R"(^<(?<priority>\d{1,3})>[A-Z][a-z]{2}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}\s+\S+(?:\s+.*)?$)"
            )
        );

    const QRegularExpressionMatch match =
        pattern.match(line);

    if (!match.hasMatch()) {
        return false;
    }

    bool priorityValid = false;

    const int priority =
        match.captured(
                 QStringLiteral("priority")
                 )
            .toInt(
                &priorityValid
                );

    return priorityValid
           && priority >= 0
           && priority <= 191;
}

bool looksLikeSyslogRecord(
    const QString &line
    )
{
    return looksLikeRfc5424Record(line)
    || looksLikeRfc3164Record(line);
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

    if (suffix == QStringLiteral(
            "syslog"
            )) {
        return syslogSuggestion(
            QStringLiteral(
                "The file extension indicates "
                "Syslog content."
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
    bool allSamplesLookLikeSyslog = true;
    bool allSamplesLookLikeApacheCommon = true;
    bool allSamplesLookLikeApacheNginxCombined = true;

    while (!file.atEnd()
           && sampledRecords < maximumSamples) {
        const QByteArray rawLine =
            file.readLine().trimmed();

        if (rawLine.isEmpty()) {
            continue;
        }

        const QString line =
            QString::fromUtf8(
                rawLine
                );

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
                line
                )) {
            allSamplesLookLikeKeyValue =
                false;
        }

        if (!looksLikeSyslogRecord(
                line
                )) {
            allSamplesLookLikeSyslog =
                false;
        }

        if (!looksLikeApacheCommonRecord(
                line
                )) {
            allSamplesLookLikeApacheCommon =
                false;
        }

        if (!looksLikeApacheNginxCombinedRecord(
                line
                )) {
            allSamplesLookLikeApacheNginxCombined =
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

    if (allSamplesLookLikeSyslog) {
        return syslogSuggestion(
            QStringLiteral(
                "Sampled non-empty source records "
                "match RFC 5424 or RFC 3164 "
                "Syslog structure."
                )
            );
    }

    if (allSamplesLookLikeApacheNginxCombined) {
        return apacheNginxCombinedSuggestion(
            QStringLiteral(
                "Sampled non-empty source records "
                "match the standard combined "
                "web access-log structure."
                )
            );
    }

    if (allSamplesLookLikeApacheCommon) {
        return apacheCommonSuggestion(
            QStringLiteral(
                "Sampled non-empty source records "
                "match Apache Common Log Format."
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