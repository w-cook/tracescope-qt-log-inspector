#include "ImportFormatSuggestionService.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QXmlStreamReader>

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

ImportFormatSuggestion xmlSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("xml"),
        QStringLiteral("Structured XML"),
        reason
    };
}

ImportFormatSuggestion windowsEventXmlSuggestion(
    const QString &reason,
    const QString &presetId
    )
{
    return {
        QStringLiteral("xml"),
        QStringLiteral("Windows Event XML"),
        reason,
        presetId
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

ImportFormatSuggestion iisW3cSuggestion(
    const QString &reason
    )
{
    return {
        QStringLiteral("iis-w3c"),
        QStringLiteral(
            "IIS W3C Extended Log"
            ),
        reason,
        BuiltInImportProfilePresetIds::
        IisW3c
    };
}

bool looksLikeIisW3cFieldsDirective(
    const QString &line
    )
{
    if (!line.startsWith(
            QStringLiteral("#Fields:"),
            Qt::CaseInsensitive
            )) {
        return false;
    }

    const QString fields =
        line.mid(
                QStringLiteral("#Fields:")
                    .size()
                )
            .trimmed();

    if (fields.isEmpty()) {
        return false;
    }

    const QStringList tokens =
        fields.split(
            QRegularExpression(
                QStringLiteral(R"(\s+)")
                ),
            Qt::SkipEmptyParts
            );

    const bool hasDate =
        tokens.contains(
            QStringLiteral("date"),
            Qt::CaseInsensitive
            );

    const bool hasTime =
        tokens.contains(
            QStringLiteral("time"),
            Qt::CaseInsensitive
            );

    const bool hasIisRequestField =
        tokens.contains(
            QStringLiteral("cs-method"),
            Qt::CaseInsensitive
            )
        || tokens.contains(
            QStringLiteral("cs-uri-stem"),
            Qt::CaseInsensitive
            );

    const bool hasIisResponseField =
        tokens.contains(
            QStringLiteral("sc-status"),
            Qt::CaseInsensitive
            )
        || tokens.contains(
            QStringLiteral("time-taken"),
            Qt::CaseInsensitive
            );

    return hasDate
           && hasTime
           && hasIisRequestField
           && hasIisResponseField;
}

bool isWindowsEventNamespace(
    const QString &namespaceUri
    )
{
    return namespaceUri
           == QStringLiteral(
               "http://schemas.microsoft.com/"
               "win/2004/08/events/event"
               );
}

ImportFormatSuggestion
detectWindowsEventXml(
    const QString &filePath
    )
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )) {
        return {};
    }

    QXmlStreamReader reader(&file);

    bool rootSeen = false;
    bool collectionRoot = false;

    constexpr int maximumStartElements = 32;

    int inspectedStartElements = 0;

    while (!reader.atEnd()
           && inspectedStartElements
                  < maximumStartElements) {
        reader.readNext();

        if (!reader.isStartElement()) {
            continue;
        }

        ++inspectedStartElements;

        const QString elementName =
            reader.name().toString();

        const QString namespaceUri =
            reader.namespaceUri()
                .toString();

        if (!rootSeen) {
            rootSeen = true;

            /*
             * A single rendered Windows event uses
             * Event as the document root.
             */
            if (elementName
                    == QStringLiteral("Event")
                && isWindowsEventNamespace(
                    namespaceUri
                    )) {
                return windowsEventXmlSuggestion(
                    QStringLiteral(
                        "The XML document contains "
                        "a Windows Event record."
                        ),
                    BuiltInImportProfilePresetIds::
                    WindowsEventXml
                    );
            }

            /*
             * TraceScope also supports collections
             * wrapped in an Events element.
             */
            collectionRoot =
                elementName
                == QStringLiteral("Events");

            if (!collectionRoot) {
                return {};
            }

            continue;
        }

        if (collectionRoot
            && elementName
                   == QStringLiteral("Event")
            && isWindowsEventNamespace(
                namespaceUri
                )) {
            return windowsEventXmlSuggestion(
                QStringLiteral(
                    "The XML document contains "
                    "a collection of Windows Event "
                    "records."
                    ),
                BuiltInImportProfilePresetIds::
                WindowsEventXmlCollection
                );
        }
    }

    return {};
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

    if (suffix == QStringLiteral("xml")) {
        const ImportFormatSuggestion
            windowsEventSuggestion =
            detectWindowsEventXml(
                filePath
                );

        if (windowsEventSuggestion
                .hasSuggestion()) {
            return windowsEventSuggestion;
        }

        return xmlSuggestion(
            QStringLiteral(
                "The file extension indicates "
                "a structured XML document."
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

    constexpr int maximumIisHeaderLines = 32;

    int inspectedHeaderLines = 0;

    while (!file.atEnd()
           && inspectedHeaderLines
                  < maximumIisHeaderLines) {
        const QString line =
            QString::fromUtf8(
                file.readLine()
                    .trimmed()
                );

        if (line.isEmpty()) {
            continue;
        }

        ++inspectedHeaderLines;

        if (looksLikeIisW3cFieldsDirective(
                line
                )) {
            return iisW3cSuggestion(
                QStringLiteral(
                    "The source contains an IIS-style "
                    "W3C #Fields directive."
                    )
                );
        }

        /*
     * Once actual record data begins, there is
     * no reason to continue searching for a
     * normal W3C header.
     */
        if (!line.startsWith(
                QLatin1Char('#')
                )) {
            break;
        }
    }

    file.seek(0);

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