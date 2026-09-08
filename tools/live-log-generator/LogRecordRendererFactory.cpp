#include "LogRecordRendererFactory.h"

#include <variant>

#include "renderers/AccessLogRenderer.h"
#include "renderers/DelimitedTextRenderer.h"
#include "renderers/IisW3cRenderer.h"
#include "renderers/JsonLinesRenderer.h"
#include "renderers/KeyValueTextRenderer.h"
#include "renderers/RegexTextRenderer.h"
#include "renderers/StructuredJsonRenderer.h"
#include "renderers/StructuredXmlRenderer.h"
#include "renderers/SyslogRenderer.h"
#include "renderers/WindowsEventXmlRenderer.h"

namespace
{
bool isValidRegexTextValue(
    const QString &value
    )
{
    return !value.contains(
               QLatin1Char(']')
               )
           && !value.contains(
               QLatin1Char('\r')
               )
           && !value.contains(
               QLatin1Char('\n')
               );
}

LogRecordRendererCreateResult failure(
    const QString &code,
    const QString &message
    )
{
    LogRecordRendererCreateResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

bool containsWhitespace(
    const QString &value
    )
{
    for (const QChar character : value) {
        if (character.isSpace()) {
            return true;
        }
    }

    return false;
}

bool containsLineBreakOrQuote(
    const QString &value
    )
{
    return value.contains(
               QLatin1Char('\r')
               )
           || value.contains(
               QLatin1Char('\n')
               )
           || value.contains(
               QLatin1Char('"')
               );
}

bool containsOnlyDigits(
    const QString &value
    )
{
    if (value.isEmpty()) {
        return false;
    }

    for (const QChar character : value) {
        if (!character.isDigit()) {
            return false;
        }
    }

    return true;
}
}

QStringList
LogRecordRendererFactory::supportedFormats()
{
    return {
        QStringLiteral("jsonl"),
        QStringLiteral("csv"),
        QStringLiteral("tsv"),
        QStringLiteral("logfmt"),
        QStringLiteral("regex-text"),
        QStringLiteral("iis-w3c"),
        QStringLiteral("apache-common"),
        QStringLiteral("apache-combined"),
        QStringLiteral("nginx-combined"),
        QStringLiteral("syslog-rfc5424"),
        QStringLiteral("syslog-rfc3164"),
        QStringLiteral("structured-json"),
        QStringLiteral("structured-xml"),
        QStringLiteral("windows-event-xml")
    };
}

LogRecordRendererCreateResult
LogRecordRendererFactory::create(
    const QString &formatId,
    const LiveLogScenario &scenario
    )
{
    const QString normalized =
        formatId.trimmed().toLower();

    LogRecordRendererCreateResult result;

    if (normalized
        == QStringLiteral("jsonl")) {
        result.renderer =
            std::make_unique<
                JsonLinesRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("logfmt")) {
        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            for (auto iterator =
                 recordStep
                     ->record
                     .attributes
                     .constBegin();
                 iterator !=
                 recordStep
                     ->record
                     .attributes
                     .constEnd();
                 ++iterator) {
                if (KeyValueTextRenderer::
                    isValidAttributeKey(
                        iterator.key()
                        )) {
                    continue;
                }

                return failure(
                    QStringLiteral(
                        "INVALID_LOGFMT_ATTRIBUTE_KEY"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "attribute key '%2', which "
                        "cannot be rendered as logfmt. "
                        "Keys must be non-empty and "
                        "cannot contain whitespace or '='."
                        )
                        .arg(stepIndex + 1)
                        .arg(iterator.key())
                    );
            }
        }

        result.renderer =
            std::make_unique<
                KeyValueTextRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("regex-text")) {
        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            const LiveLogRecord &record =
                recordStep->record;

            const QStringList canonicalValues {
                record.severity,
                record.subsystem,
                record.eventCode,
                record.entityId
            };

            for (const QString &value
                 : canonicalValues) {
                if (isValidRegexTextValue(
                        value
                        )) {
                    continue;
                }

                return failure(
                    QStringLiteral(
                        "INVALID_REGEX_TEXT_VALUE"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "a canonical value that cannot "
                        "be represented by the generator's "
                        "regex-text format."
                        )
                        .arg(stepIndex + 1)
                    );
            }

            if (record.message.contains(
                    QLatin1Char('\r')
                    )
                || record.message.contains(
                    QLatin1Char('\n')
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_REGEX_TEXT_MESSAGE"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "a line break in its message. "
                        "Regex-text generator records "
                        "must remain single-line records."
                        )
                        .arg(stepIndex + 1)
                    );
            }

            for (auto iterator =
                 record.attributes.constBegin();
                 iterator
                 != record.attributes.constEnd();
                 ++iterator) {
                if (!isValidRegexTextValue(
                        iterator.key()
                        )
                    || !isValidRegexTextValue(
                        iterator.value().toString()
                        )) {
                    return failure(
                        QStringLiteral(
                            "INVALID_REGEX_TEXT_ATTRIBUTE"
                            ),
                        QStringLiteral(
                            "Scenario step %1 contains "
                            "attribute '%2' with a key or "
                            "value that cannot be represented "
                            "by the generator's regex-text "
                            "format."
                            )
                            .arg(stepIndex + 1)
                            .arg(iterator.key())
                        );
                }
            }
        }

        result.renderer =
            std::make_unique<
                RegexTextRenderer
                >(
                DelimitedTextRenderer::
                attributeKeysForScenario(
                    scenario
                    )
                );

        return result;
    }

    if (normalized
        == QStringLiteral("iis-w3c")) {
        const QStringList iisAttributeKeys({
            QStringLiteral("serverIp"),
            QStringLiteral("method"),
            QStringLiteral("path"),
            QStringLiteral("query"),
            QStringLiteral("serverPort"),
            QStringLiteral("username"),
            QStringLiteral("clientIp"),
            QStringLiteral("userAgent"),
            QStringLiteral("referer"),
            QStringLiteral("status"),
            QStringLiteral("substatus"),
            QStringLiteral("win32Status"),
            QStringLiteral("timeTakenMs")
        });

        for (const LiveLogScenarioStep &scenarioStep :
             scenario.steps) {
            if (!std::holds_alternative<
                    LiveLogRecordStep
                    >(scenarioStep)) {
                continue;
            }

            const LiveLogRecord &record =
                std::get<LiveLogRecordStep>(
                    scenarioStep
                    ).record;

            for (const QString &key :
                 iisAttributeKeys) {
                const auto iterator =
                    record.attributes.constFind(
                        key
                        );

                if (iterator
                        == record.attributes.constEnd()
                    || !iterator.value().isValid()
                    || iterator.value().isNull()) {
                    continue;
                }

                const QString value =
                    iterator.value().toString();

                if (containsWhitespace(value)) {
                    return {
                        nullptr,
                        QStringLiteral(
                            "INVALID_IIS_W3C_ATTRIBUTE"
                            ),
                        QStringLiteral(
                            "IIS W3C attribute '%1' "
                            "contains whitespace and cannot "
                            "be represented safely."
                            )
                            .arg(key)
                    };
                }
            }
        }

        return {
            std::make_unique<
                IisW3cRenderer
                >(),
            {},
            {}
        };
    }

    if (normalized
            == QStringLiteral("apache-common")
        || normalized
               == QStringLiteral("apache-combined")
        || normalized
               == QStringLiteral("nginx-combined")) {

        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            const LiveLogRecord &record =
                recordStep->record;

            const QStringList requiredKeys({
                QStringLiteral("clientIp"),
                QStringLiteral("method"),
                QStringLiteral("path"),
                QStringLiteral("status")
            });

            for (const QString &key :
                 requiredKeys) {
                const auto iterator =
                    record.attributes.constFind(
                        key
                        );

                if (iterator
                        == record.attributes.constEnd()
                    || !iterator.value().isValid()
                    || iterator.value().isNull()
                    || iterator.value()
                           .toString()
                           .isEmpty()) {
                    return failure(
                        QStringLiteral(
                            "MISSING_ACCESS_LOG_ATTRIBUTE"
                            ),
                        QStringLiteral(
                            "Scenario step %1 is missing "
                            "required access-log attribute "
                            "'%2'."
                            )
                            .arg(stepIndex + 1)
                            .arg(key)
                        );
                }
            }

            const QStringList tokenKeys({
                QStringLiteral("clientIp"),
                QStringLiteral("method"),
                QStringLiteral("path"),
                QStringLiteral("query"),
                QStringLiteral("username")
            });

            for (const QString &key :
                 tokenKeys) {
                const auto iterator =
                    record.attributes.constFind(
                        key
                        );

                if (iterator
                        == record.attributes.constEnd()
                    || !iterator.value().isValid()
                    || iterator.value().isNull()) {
                    continue;
                }

                const QString value =
                    iterator.value().toString();

                if (containsWhitespace(value)
                    || containsLineBreakOrQuote(
                        value
                        )) {
                    return failure(
                        QStringLiteral(
                            "INVALID_ACCESS_LOG_ATTRIBUTE"
                            ),
                        QStringLiteral(
                            "Scenario step %1 contains "
                            "access-log attribute '%2' "
                            "that cannot be represented "
                            "safely."
                            )
                            .arg(stepIndex + 1)
                            .arg(key)
                        );
                }
            }

            if (normalized
                    == QStringLiteral("apache-combined")
                || normalized
                       == QStringLiteral("nginx-combined")) {
                const QStringList quotedKeys({
                    QStringLiteral("referer"),
                    QStringLiteral("userAgent")
                });

                for (const QString &key :
                     quotedKeys) {
                    const auto iterator =
                        record.attributes.constFind(
                            key
                            );

                    if (iterator
                            == record.attributes.constEnd()
                        || !iterator.value().isValid()
                        || iterator.value().isNull()) {
                        continue;
                    }

                    if (containsLineBreakOrQuote(
                            iterator.value()
                                .toString()
                            )) {
                        return failure(
                            QStringLiteral(
                                "INVALID_ACCESS_LOG_ATTRIBUTE"
                                ),
                            QStringLiteral(
                                "Scenario step %1 contains "
                                "access-log attribute '%2' "
                                "that cannot be represented "
                                "safely."
                                )
                                .arg(stepIndex + 1)
                                .arg(key)
                            );
                    }
                }
            }

            const QString status =
                record.attributes
                    .value(
                        QStringLiteral("status")
                        )
                    .toString();

            if (status.size() != 3
                || !containsOnlyDigits(status)) {
                return failure(
                    QStringLiteral(
                        "INVALID_ACCESS_LOG_STATUS"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "HTTP status '%2'. Access-log "
                        "status values must contain "
                        "exactly three digits."
                        )
                        .arg(stepIndex + 1)
                        .arg(status)
                    );
            }

            const auto bytesIterator =
                record.attributes.constFind(
                    QStringLiteral(
                        "responseBytes"
                        )
                    );

            if (bytesIterator
                    != record.attributes.constEnd()
                && bytesIterator.value().isValid()
                && !bytesIterator.value().isNull()) {
                const QString bytes =
                    bytesIterator.value()
                        .toString();

                if (!bytes.isEmpty()
                    && !containsOnlyDigits(bytes)) {
                    return failure(
                        QStringLiteral(
                            "INVALID_ACCESS_LOG_RESPONSE_BYTES"
                            ),
                        QStringLiteral(
                            "Scenario step %1 contains "
                            "responseBytes '%2'. Response "
                            "bytes must be a non-negative "
                            "integer when present."
                            )
                            .arg(stepIndex + 1)
                            .arg(bytes)
                        );
                }
            }
        }

        AccessLogFormat accessFormat =
            AccessLogFormat::ApacheCommon;

        if (normalized
            == QStringLiteral(
                "apache-combined"
                )) {
            accessFormat =
                AccessLogFormat::ApacheCombined;
        } else if (normalized
                   == QStringLiteral(
                       "nginx-combined"
                       )) {
            accessFormat =
                AccessLogFormat::NginxCombined;
        }

        result.renderer =
            std::make_unique<
                AccessLogRenderer
                >(
                accessFormat
                );

        return result;
    }

    if (normalized
        == QStringLiteral("syslog-rfc5424")) {
        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            const LiveLogRecord &record =
                recordStep->record;

            if (!record.subsystem.isEmpty()
                && !SyslogRenderer::
                   isValidRfc5424AppName(
                       record.subsystem
                       )) {
                return failure(
                    QStringLiteral(
                        "INVALID_RFC5424_APP_NAME"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "subsystem '%2', which cannot "
                        "be represented as an RFC 5424 "
                        "APP-NAME."
                        )
                        .arg(
                            QString::number(
                                stepIndex + 1
                                ),
                            record.subsystem
                            )
                    );
            }

            if (!record.eventCode.isEmpty()
                && !SyslogRenderer::
                   isValidRfc5424MessageId(
                       record.eventCode
                       )) {
                return failure(
                    QStringLiteral(
                        "INVALID_RFC5424_MESSAGE_ID"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "event code '%2', which cannot "
                        "be represented as an RFC 5424 "
                        "MSGID."
                        )
                        .arg(
                            QString::number(
                                stepIndex + 1
                                ),
                            record.eventCode
                            )
                    );
            }

            if (record.message.contains(
                    QLatin1Char('\n')
                    )
                || record.message.contains(
                    QLatin1Char('\r')
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_SYSLOG_MESSAGE"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "a line break in its message. "
                        "Syslog generator records must "
                        "remain single-line messages."
                        )
                        .arg(
                            stepIndex + 1
                            )
                    );
            }

            for (auto iterator =
                 record.attributes.constBegin();
                 iterator !=
                 record.attributes.constEnd();
                 ++iterator) {
                if (SyslogRenderer::
                    isValidRfc5424StructuredDataName(
                        iterator.key()
                        )) {
                    continue;
                }

                return failure(
                    QStringLiteral(
                        "INVALID_RFC5424_STRUCTURED_DATA_NAME"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "attribute key '%2', which "
                        "cannot be represented as an "
                        "RFC 5424 structured-data "
                        "parameter name."
                        )
                        .arg(
                            QString::number(
                                stepIndex + 1
                                ),
                            iterator.key()
                            )
                    );
            }
        }

        result.renderer =
            std::make_unique<
                SyslogRenderer
                >(
                SyslogFormat::Rfc5424
                );

        return result;
    }

    if (normalized
        == QStringLiteral("syslog-rfc3164")) {
        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            const LiveLogRecord &record =
                recordStep->record;

            if (!record.subsystem.isEmpty()
                && !SyslogRenderer::
                   isValidRfc3164Tag(
                       record.subsystem
                       )) {
                return failure(
                    QStringLiteral(
                        "INVALID_RFC3164_TAG"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "subsystem '%2', which cannot "
                        "be represented as an RFC 3164 "
                        "tag."
                        )
                        .arg(
                            QString::number(
                                stepIndex + 1
                                ),
                            record.subsystem
                            )
                    );
            }

            if (record.message.contains(
                    QLatin1Char('\n')
                    )
                || record.message.contains(
                    QLatin1Char('\r')
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_SYSLOG_MESSAGE"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "a line break in its message. "
                        "Syslog generator records must "
                        "remain single-line messages."
                        )
                        .arg(
                            stepIndex + 1
                            )
                    );
            }
        }

        result.renderer =
            std::make_unique<
                SyslogRenderer
                >(
                SyslogFormat::Rfc3164
                );

        return result;
    }

    if (normalized
        == QStringLiteral("structured-json")) {
        result.renderer =
            std::make_unique<
                StructuredJsonRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("structured-xml")) {
        result.renderer =
            std::make_unique<
                StructuredXmlRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("windows-event-xml")) {
        result.renderer =
            std::make_unique<
                WindowsEventXmlRenderer
                >();

        return result;
    }

    const QStringList attributeKeys =
        DelimitedTextRenderer::
        attributeKeysForScenario(
            scenario
            );

    if (normalized
        == QStringLiteral("csv")) {
        result.renderer =
            std::make_unique<
                DelimitedTextRenderer
                >(
                QLatin1Char(','),
                attributeKeys
                );

        return result;
    }

    if (normalized
        == QStringLiteral("tsv")) {
        result.renderer =
            std::make_unique<
                DelimitedTextRenderer
                >(
                QLatin1Char('\t'),
                attributeKeys
                );

        return result;
    }

    return failure(
        QStringLiteral(
            "UNSUPPORTED_OUTPUT_FORMAT"
            ),
        QStringLiteral(
            "Unsupported format '%1'. "
            "Supported formats: %2."
            )
            .arg(
                normalized,
                supportedFormats().join(
                    QStringLiteral(", ")
                    )
                )
        );
}