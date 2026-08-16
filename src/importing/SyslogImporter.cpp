#include "SyslogImporter.h"

#include <limits>
#include <optional>
#include <utility>

#include <QDateTime>
#include <QTimeZone>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtGlobal>
#include <QJsonArray>

#include "ImportDiagnostic.h"
#include "JsonObjectRecordMapper.h"

namespace
{
struct ParsedSyslogLine
{
    bool success = false;
    bool usedLegacyYearInference = false;

    QJsonObject values;
    QString errorMessage;
};

RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber
    )
{
    RecordSourceMetadata source;

    source.sourcePath = sourcePath;
    source.recordNumber = recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(sourcePath).fileName();
    }

    return source;
}

void appendDiagnostic(
    ImportResult &result,
    const QString &code,
    const QString &message,
    ImportDiagnosticSeverity severity,
    const std::optional<RecordSourceMetadata> &source =
    std::nullopt
    )
{
    ImportDiagnostic diagnostic;

    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.severity = severity;
    diagnostic.source = source;

    result.diagnostics.append(
        diagnostic
        );
}

std::optional<int> parsePriority(
    const QString &line,
    qsizetype &payloadStart
    )
{
    if (!line.startsWith(
            QLatin1Char('<')
            )) {
        return std::nullopt;
    }

    const qsizetype closingBracket =
        line.indexOf(
            QLatin1Char('>')
            );

    if (closingBracket < 2
        || closingBracket > 4) {
        return std::nullopt;
    }

    const QString priorityText =
        line.mid(
            1,
            closingBracket - 1
            );

    static const QRegularExpression
        priorityPattern(
            QStringLiteral(
                R"(^\d{1,3}$)"
                )
            );

    if (!priorityPattern
             .match(priorityText)
             .hasMatch()) {
        return std::nullopt;
    }

    bool conversionSucceeded = false;

    const int priority =
        priorityText.toInt(
            &conversionSucceeded
            );

    if (!conversionSucceeded
        || priority < 0
        || priority > 191) {
        return std::nullopt;
    }

    payloadStart =
        closingBracket + 1;

    return priority;
}

QString canonicalSeverity(
    int severityCode
    )
{
    switch (severityCode) {
    case 0:
    case 1:
    case 2:
        return QStringLiteral(
            "CRITICAL"
            );

    case 3:
        return QStringLiteral(
            "ERROR"
            );

    case 4:
        return QStringLiteral(
            "WARN"
            );

    case 5:
    case 6:
        return QStringLiteral(
            "INFO"
            );

    case 7:
        return QStringLiteral(
            "DEBUG"
            );
    }

    return {};
}

QString syslogSeverityName(
    int severityCode
    )
{
    switch (severityCode) {
    case 0:
        return QStringLiteral(
            "Emergency"
            );

    case 1:
        return QStringLiteral(
            "Alert"
            );

    case 2:
        return QStringLiteral(
            "Critical"
            );

    case 3:
        return QStringLiteral(
            "Error"
            );

    case 4:
        return QStringLiteral(
            "Warning"
            );

    case 5:
        return QStringLiteral(
            "Notice"
            );

    case 6:
        return QStringLiteral(
            "Informational"
            );

    case 7:
        return QStringLiteral(
            "Debug"
            );
    }

    return {};
}

void insertCommonPriorityFields(
    QJsonObject &values,
    int priority
    )
{
    const int facilityCode =
        priority / 8;

    const int severityCode =
        priority % 8;

    values.insert(
        QStringLiteral("priority"),
        priority
        );

    values.insert(
        QStringLiteral("facilityCode"),
        facilityCode
        );

    values.insert(
        QStringLiteral("severityCode"),
        severityCode
        );

    values.insert(
        QStringLiteral("syslogSeverity"),
        syslogSeverityName(
            severityCode
            )
        );

    values.insert(
        QStringLiteral("level"),
        canonicalSeverity(
            severityCode
            )
        );
}

void insertIfPresent(
    QJsonObject &values,
    const QString &key,
    const QString &value
    )
{
    if (value.isEmpty()
        || value == QStringLiteral("-")) {
        return;
    }

    values.insert(
        key,
        value
        );
}

bool takeToken(
    const QString &text,
    qsizetype &position,
    QString &token
    )
{
    if (position >= text.size()) {
        return false;
    }

    const qsizetype nextSpace =
        text.indexOf(
            QLatin1Char(' '),
            position
            );

    if (nextSpace < 0) {
        return false;
    }

    token =
        text.mid(
            position,
            nextSpace - position
            );

    if (token.isEmpty()) {
        return false;
    }

    position =
        nextSpace + 1;

    return true;
}

bool parseStructuredDataAndMessage(
    const QString &payload,
    qsizetype position,
    QString &structuredData,
    QString &message
    )
{
    if (position >= payload.size()) {
        return false;
    }

    if (payload.at(position)
        == QLatin1Char('-')) {
        structuredData =
            QStringLiteral("-");

        ++position;

        if (position == payload.size()) {
            return true;
        }

        if (payload.at(position)
            != QLatin1Char(' ')) {
            return false;
        }

        message =
            payload.mid(
                position + 1
                );

        return true;
    }

    if (payload.at(position)
        != QLatin1Char('[')) {
        return false;
    }

    const qsizetype structuredDataStart =
        position;

    while (position < payload.size()
           && payload.at(position)
                  == QLatin1Char('[')) {
        ++position;

        bool insideQuotes = false;
        bool escaped = false;
        bool closingBracketFound = false;

        while (position < payload.size()) {
            const QChar character =
                payload.at(position);

            if (escaped) {
                escaped = false;
                ++position;
                continue;
            }

            if (insideQuotes
                && character
                       == QLatin1Char('\\')) {
                escaped = true;
                ++position;
                continue;
            }

            if (character
                == QLatin1Char('"')) {
                insideQuotes =
                    !insideQuotes;

                ++position;
                continue;
            }

            if (!insideQuotes
                && character
                       == QLatin1Char(']')) {
                ++position;

                closingBracketFound = true;
                break;
            }

            ++position;
        }

        if (!closingBracketFound
            || insideQuotes
            || escaped) {
            return false;
        }
    }

    structuredData =
        payload.mid(
            structuredDataStart,
            position - structuredDataStart
            );

    if (position == payload.size()) {
        return true;
    }

    if (payload.at(position)
        != QLatin1Char(' ')) {
        return false;
    }

    message =
        payload.mid(
            position + 1
            );

    return true;
}

QJsonObject parseStructuredDataFields(
    const QString &structuredData
    )
{
    QJsonObject elements;

    qsizetype position = 0;

    while (position < structuredData.size()) {
        if (structuredData.at(position)
            != QLatin1Char('[')) {
            return {};
        }

        ++position;

        const qsizetype idStart =
            position;

        while (position < structuredData.size()
               && structuredData.at(position)
                      != QLatin1Char(' ')
               && structuredData.at(position)
                      != QLatin1Char(']')) {
            ++position;
        }

        const QString sdId =
            structuredData.mid(
                idStart,
                position - idStart
                );

        if (sdId.isEmpty()) {
            return {};
        }

        QJsonObject parameters;

        while (position < structuredData.size()
               && structuredData.at(position)
                      != QLatin1Char(']')) {
            if (structuredData.at(position)
                != QLatin1Char(' ')) {
                return {};
            }

            ++position;

            const qsizetype nameStart =
                position;

            while (position < structuredData.size()
                   && structuredData.at(position)
                          != QLatin1Char('=')) {
                if (structuredData.at(position)
                        == QLatin1Char(' ')
                    || structuredData.at(position)
                           == QLatin1Char(']')) {
                    return {};
                }

                ++position;
            }

            if (position
                >= structuredData.size()) {
                return {};
            }

            const QString parameterName =
                structuredData.mid(
                    nameStart,
                    position - nameStart
                    );

            if (parameterName.isEmpty()) {
                return {};
            }

            ++position;

            if (position
                    >= structuredData.size()
                || structuredData.at(position)
                       != QLatin1Char('"')) {
                return {};
            }

            ++position;

            QString parameterValue;
            bool closingQuoteFound = false;

            while (position
                   < structuredData.size()) {
                const QChar character =
                    structuredData.at(position);

                if (character
                    == QLatin1Char('"')) {
                    ++position;
                    closingQuoteFound = true;
                    break;
                }

                if (character
                    == QLatin1Char('\\')) {
                    if (position + 1
                        >= structuredData.size()) {
                        parameterValue.append(
                            character
                            );

                        ++position;
                        continue;
                    }

                    const QChar escaped =
                        structuredData.at(
                            position + 1
                            );

                    if (escaped
                            == QLatin1Char('"')
                        || escaped
                               == QLatin1Char('\\')
                        || escaped
                               == QLatin1Char(']')) {
                        parameterValue.append(
                            escaped
                            );

                        position += 2;
                        continue;
                    }

                    /*
                     * RFC 5424 says an unknown
                     * escape preserves the
                     * backslash literally.
                     */
                    parameterValue.append(
                        character
                        );

                    ++position;
                    continue;
                }

                parameterValue.append(
                    character
                    );

                ++position;
            }

            if (!closingQuoteFound) {
                return {};
            }

            const QJsonValue existing =
                parameters.value(
                    parameterName
                    );

            if (existing.isUndefined()) {
                parameters.insert(
                    parameterName,
                    parameterValue
                    );
            } else if (existing.isArray()) {
                QJsonArray values =
                    existing.toArray();

                values.append(
                    parameterValue
                    );

                parameters.insert(
                    parameterName,
                    values
                    );
            } else {
                QJsonArray values;

                values.append(
                    existing
                    );

                values.append(
                    parameterValue
                    );

                parameters.insert(
                    parameterName,
                    values
                    );
            }
        }

        if (position
                >= structuredData.size()
            || structuredData.at(position)
                   != QLatin1Char(']')) {
            return {};
        }

        ++position;

        /*
         * RFC 5424 does not permit the same
         * SD-ID more than once in one message.
         */
        if (elements.contains(sdId)) {
            return {};
        }

        elements.insert(
            sdId,
            parameters
            );
    }

    return elements;
}

ParsedSyslogLine parseRfc5424(
    const QString &line,
    int priority,
    qsizetype payloadStart
    )
{
    ParsedSyslogLine result;

    const QString payload =
        line.mid(
            payloadStart
            );

    qsizetype position = 0;

    QString version;
    QString timestamp;
    QString hostname;
    QString appName;
    QString processId;
    QString messageId;

    if (!takeToken(
            payload,
            position,
            version
            )
        || !takeToken(
            payload,
            position,
            timestamp
            )
        || !takeToken(
            payload,
            position,
            hostname
            )
        || !takeToken(
            payload,
            position,
            appName
            )
        || !takeToken(
            payload,
            position,
            processId
            )
        || !takeToken(
            payload,
            position,
            messageId
            )) {
        return result;
    }

    bool versionIsNumber = false;

    const int versionNumber =
        version.toInt(
            &versionIsNumber
            );

    if (!versionIsNumber
        || versionNumber <= 0
        || version.size() > 3) {
        return result;
    }

    QString structuredData;
    QString message;

    if (!parseStructuredDataAndMessage(
            payload,
            position,
            structuredData,
            message
            )) {
        result.errorMessage =
            QStringLiteral(
                "RFC 5424 structured data "
                "is malformed."
                );

        return result;
    }

    if (!message.isEmpty()
        && message.front()
               == QChar(0xfeff)) {
        message.remove(
            0,
            1
            );
    }

    insertCommonPriorityFields(
        result.values,
        priority
        );

    result.values.insert(
        QStringLiteral("syslogFormat"),
        QStringLiteral("RFC 5424")
        );

    result.values.insert(
        QStringLiteral("syslogVersion"),
        versionNumber
        );

    insertIfPresent(
        result.values,
        QStringLiteral("timestamp"),
        timestamp
        );

    insertIfPresent(
        result.values,
        QStringLiteral("hostname"),
        hostname
        );

    insertIfPresent(
        result.values,
        QStringLiteral("subsystem"),
        appName
        );

    insertIfPresent(
        result.values,
        QStringLiteral("processId"),
        processId
        );

    insertIfPresent(
        result.values,
        QStringLiteral("eventCode"),
        messageId
        );

    if (structuredData
        != QStringLiteral("-")) {
        result.values.insert(
            QStringLiteral(
                "structuredData"
                ),
            structuredData
            );
    }

    const QJsonObject structuredFields =
        parseStructuredDataFields(
            structuredData
            );

    if (!structuredFields.isEmpty()) {
        result.values.insert(
            QStringLiteral(
                "structuredDataFields"
                ),
            structuredFields
            );
    }

    insertIfPresent(
        result.values,
        QStringLiteral("message"),
        message
        );

    result.success = true;

    return result;
}

QDate inferLegacyDate(
    int month,
    int day,
    const QDate &referenceDate
    )
{
    QDate bestDate;
    qint64 bestDistance =
        std::numeric_limits<qint64>::max();

    for (int year =
         referenceDate.year() - 1;
         year <=
         referenceDate.year() + 1;
         ++year) {
        const QDate candidate(
            year,
            month,
            day
            );

        if (!candidate.isValid()) {
            continue;
        }

        const qint64 distance =
            qAbs(
                candidate.daysTo(
                    referenceDate
                    )
                );

        if (distance < bestDistance) {
            bestDistance = distance;
            bestDate = candidate;
        }
    }

    return bestDate;
}

ParsedSyslogLine parseRfc3164(
    const QString &line,
    int priority,
    qsizetype payloadStart,
    const QDate &referenceDate
    )
{
    ParsedSyslogLine result;

    const QString payload =
        line.mid(
            payloadStart
            );

    static const QRegularExpression
        messagePattern(
            QStringLiteral(
                R"(^(?<month>[A-Z][a-z]{2})\s+(?<day>\d{1,2})\s+(?<time>\d{2}:\d{2}:\d{2})\s+(?<hostname>\S+)(?:\s+(?<msg>.*))?$)"
                )
            );

    const QRegularExpressionMatch match =
        messagePattern.match(
            payload
            );

    if (!match.hasMatch()) {
        return result;
    }

    const QString monthText =
        match.captured(
            QStringLiteral("month")
            );

    const QString dayText =
        match.captured(
            QStringLiteral("day")
            );

    const QString timeText =
        match.captured(
            QStringLiteral("time")
            );

    const QString timestampForParsing =
        QStringLiteral(
            "%1 %2 2000 %3"
            )
            .arg(
                monthText,
                dayText,
                timeText
                );

    const QDateTime parsedTimestamp =
        QDateTime::fromString(
            timestampForParsing,
            QStringLiteral(
                "MMM d yyyy HH:mm:ss"
                )
            );

    if (!parsedTimestamp.isValid()) {
        result.errorMessage =
            QStringLiteral(
                "RFC 3164 timestamp is invalid."
                );

        return result;
    }

    const QDate inferredDate =
        inferLegacyDate(
            parsedTimestamp.date().month(),
            parsedTimestamp.date().day(),
            referenceDate
            );

    if (!inferredDate.isValid()) {
        result.errorMessage =
            QStringLiteral(
                "RFC 3164 timestamp date "
                "could not be inferred."
                );

        return result;
    }

    const QDateTime inferredTimestamp(
        inferredDate,
        parsedTimestamp.time(),
        QTimeZone::systemTimeZone()
        );

    const QString originalTimestamp =
        QStringLiteral("%1 %2 %3")
            .arg(
                monthText,
                dayText,
                timeText
                );

    insertCommonPriorityFields(
        result.values,
        priority
        );

    result.values.insert(
        QStringLiteral("syslogFormat"),
        QStringLiteral("RFC 3164")
        );

    result.values.insert(
        QStringLiteral("timestamp"),
        inferredTimestamp.toString(
            Qt::ISODateWithMs
            )
        );

    result.values.insert(
        QStringLiteral("timestampOriginal"),
        originalTimestamp
        );

    result.values.insert(
        QStringLiteral("timestampYearInferred"),
        true
        );

    insertIfPresent(
        result.values,
        QStringLiteral("hostname"),
        match.captured(
            QStringLiteral("hostname")
            )
        );

    const QString rawMessage =
        match.captured(
            QStringLiteral("msg")
            );

    static const QRegularExpression
        taggedMessagePattern(
            QStringLiteral(
                R"(^(?<tag>[A-Za-z0-9_.-]+)(?:\[(?<pid>[^\]]+)\])?:\s?(?<content>.*)$)"
                )
            );

    const QRegularExpressionMatch
        taggedMatch =
        taggedMessagePattern.match(
            rawMessage
            );

    if (taggedMatch.hasMatch()) {
        insertIfPresent(
            result.values,
            QStringLiteral("subsystem"),
            taggedMatch.captured(
                QStringLiteral("tag")
                )
            );

        insertIfPresent(
            result.values,
            QStringLiteral("processId"),
            taggedMatch.captured(
                QStringLiteral("pid")
                )
            );

        insertIfPresent(
            result.values,
            QStringLiteral("message"),
            taggedMatch.captured(
                QStringLiteral("content")
                )
            );
    } else {
        insertIfPresent(
            result.values,
            QStringLiteral("message"),
            rawMessage
            );
    }

    result.usedLegacyYearInference = true;
    result.success = true;

    return result;
}

ParsedSyslogLine parseLine(
    const QString &line,
    const QDate &legacyReferenceDate
    )
{
    ParsedSyslogLine result;

    qsizetype payloadStart = 0;

    const std::optional<int> priority =
        parsePriority(
            line,
            payloadStart
            );

    if (!priority.has_value()) {
        result.errorMessage =
            QStringLiteral(
                "The record does not begin with "
                "a valid syslog PRI value."
                );

        return result;
    }

    result =
        parseRfc5424(
            line,
            *priority,
            payloadStart
            );

    if (result.success) {
        return result;
    }

    result =
        parseRfc3164(
            line,
            *priority,
            payloadStart,
            legacyReferenceDate
            );

    if (result.success) {
        return result;
    }

    if (result.errorMessage.isEmpty()) {
        result.errorMessage =
            QStringLiteral(
                "The record does not match "
                "supported RFC 5424 or RFC 3164 "
                "syslog structure."
                );
    }

    return result;
}

void processSyslogRecord(
    const QString &rawSource,
    const QString &sourcePath,
    qint64 recordNumber,
    const ImportProfile &profile,
    const QDate &legacyReferenceDate,
    ImportResult &result,
    bool &usedLegacyYearInference
    )
{
    if (rawSource.trimmed().isEmpty()) {
        return;
    }

    ++result.processedRecordCount;

    const RecordSourceMetadata source =
        createSourceMetadata(
            sourcePath,
            recordNumber
            );

    const ParsedSyslogLine parsed =
        parseLine(
            rawSource,
            legacyReferenceDate
            );

    if (!parsed.success) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "SYSLOG_RECORD_MALFORMED"
                ),
            parsed.errorMessage,
            ImportDiagnosticSeverity::Error,
            source
            );

        return;
    }

    usedLegacyYearInference =
        usedLegacyYearInference
        || parsed.usedLegacyYearInference;

    result.records.append(
        JsonObjectRecordMapper::mapRecord(
            parsed.values,
            rawSource,
            source,
            profile,
            result
            )
        );
}

void appendLegacyYearInferenceDiagnostic(
    ImportResult &result,
    bool usedLegacyYearInference
    )
{
    if (!usedLegacyYearInference) {
        return;
    }

    appendDiagnostic(
        result,
        QStringLiteral(
            "SYSLOG_LEGACY_TIMESTAMP_INFERRED"
            ),
        QStringLiteral(
            "RFC 3164 timestamps do not include "
            "a year or timezone. TraceScope "
            "inferred the nearest year relative "
            "to the import reference date and "
            "interpreted the timestamp as local time."
            ),
        ImportDiagnosticSeverity::Information
        );
}
}

SyslogImporter::SyslogImporter(
    ImportProfile profile,
    QDate legacyReferenceDate
    )
    : profile(std::move(profile)),
    legacyReferenceDate(
        legacyReferenceDate.isValid()
            ? legacyReferenceDate
            : QDate::currentDate()
        )
{
    this->profile.importerId =
        QStringLiteral(
            "syslog"
            );
}

QString SyslogImporter::id() const
{
    return QStringLiteral(
        "syslog"
        );
}

QString SyslogImporter::displayName() const
{
    return QStringLiteral(
        "Syslog (RFC 5424 / RFC 3164)"
        );
}

ImportResult SyslogImporter::importLines(
    const QStringList &lines,
    const QString &sourcePath
    ) const
{
    ImportResult result;

    bool usedLegacyYearInference = false;

    for (qsizetype index = 0;
         index < lines.size();
         ++index) {
        processSyslogRecord(
            lines.at(index),
            sourcePath,
            index + 1,
            profile,
            legacyReferenceDate,
            result,
            usedLegacyYearInference
            );
    }

    appendLegacyYearInferenceDiagnostic(
        result,
        usedLegacyYearInference
        );

    return result;
}

ImportResult SyslogImporter::importFile(
    const QString &filePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )) {
        ImportResult result;

        appendDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The source file could not be opened: %1"
                )
                .arg(
                    file.errorString()
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                filePath,
                0
                )
            );

        return result;
    }

    constexpr qint64 progressReportByteInterval =
        256 * 1024;

    ImportResult result;

    const qint64 totalBytes =
        file.size();

    qint64 physicalLineNumber = 0;
    qint64 lastReportedBytes = 0;

    bool usedLegacyYearInference = false;

    executionContext.report({
        0,
        totalBytes,
        0
    });

    while (!file.atEnd()) {
        if (executionContext
                .cancellationRequested()) {
            result.cancelled = true;
            break;
        }

        QByteArray lineBytes =
            file.readLine();

        ++physicalLineNumber;

        QString rawSource =
            QString::fromUtf8(
                lineBytes
                );

        if (rawSource.endsWith('\n')) {
            rawSource.chop(1);
        }

        if (rawSource.endsWith('\r')) {
            rawSource.chop(1);
        }

        if (!rawSource.trimmed().isEmpty()) {
            if (maxProcessedRecords > 0
                && result.processedRecordCount >=
                       maxProcessedRecords) {
                result.sourceTruncated = true;
                break;
            }

            processSyslogRecord(
                rawSource,
                filePath,
                physicalLineNumber,
                profile,
                legacyReferenceDate,
                result,
                usedLegacyYearInference
                );
        }

        const qint64 bytesProcessed =
            file.pos();

        if (bytesProcessed
                - lastReportedBytes
            >= progressReportByteInterval) {
            executionContext.report({
                bytesProcessed,
                totalBytes,
                result.processedRecordCount
            });

            lastReportedBytes =
                bytesProcessed;
        }
    }

    appendLegacyYearInferenceDiagnostic(
        result,
        usedLegacyYearInference
        );

    executionContext.report({
        file.pos(),
        totalBytes,
        result.processedRecordCount
    });

    return result;
}