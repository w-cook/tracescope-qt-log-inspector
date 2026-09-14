#include "RotatedSourceDiscoveryService.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTime>
#include <QTimeZone>

#include <algorithm>
#include <optional>
#include <utility>

namespace
{
struct DiscoveryConfiguration
{
    QRegularExpression expression;

    int orderCaptureGroup = 1;

    RotatedSourceOrderValueType
        orderValueType =
        RotatedSourceOrderValueType::
        Lexicographic;

    RotatedSourceOrderDirection
        orderDirection =
        RotatedSourceOrderDirection::
        Ascending;

    QString dateTimeFormat;

    bool numericRotationScheme = false;
};

struct DiscoveryCandidate
{
    RotatedSourceMatch match;

    qint64 numericOrderValue = 0;
    QString lexicalOrderValue;
    QDateTime dateTimeOrderValue;
};

RotatedSourceDiscoveryResult failure(
    const QString &message
    )
{
    RotatedSourceDiscoveryResult result;

    result.succeeded = false;
    result.errorMessage = message;

    return result;
}

RotatedSourceRuleSuggestionResult
suggestionFailure(
    const QString &message
    )
{
    RotatedSourceRuleSuggestionResult result;

    result.succeeded = false;
    result.errorMessage = message;

    return result;
}

QRegularExpression numericSuffixExpression(
    const QFileInfo &activeSourceInfo
    )
{
    const QString escapedFileName =
        QRegularExpression::escape(
            activeSourceInfo.fileName()
            );

    return QRegularExpression(
        QStringLiteral(
            "^%1\\.(\\d+)$"
            )
            .arg(
                escapedFileName
                )
        );
}

QRegularExpression
numericBeforeExtensionExpression(
    const QFileInfo &activeSourceInfo
    )
{
    const QString suffix =
        activeSourceInfo.suffix();

    if (suffix.isEmpty()) {
        return numericSuffixExpression(
            activeSourceInfo
            );
    }

    const QString escapedBaseName =
        QRegularExpression::escape(
            activeSourceInfo.completeBaseName()
            );

    const QString escapedSuffix =
        QRegularExpression::escape(
            suffix
            );

    return QRegularExpression(
        QStringLiteral(
            "^%1\\.(\\d+)\\.%2$"
            )
            .arg(
                escapedBaseName,
                escapedSuffix
                )
        );
}

QRegularExpression
isoDateTimeBeforeExtensionExpression(
    const QFileInfo &activeSourceInfo
    )
{
    const QString escapedBaseName =
        QRegularExpression::escape(
            activeSourceInfo.completeBaseName()
            );

    const QString suffix =
        activeSourceInfo.suffix();

    /*
     * Supported built-in examples:
     *
     * service-2026-09-12.log
     * service-2026-09-12T14-30-05.log
     *
     * The timestamp form avoids ':' so it remains
     * portable to Windows file systems.
     */
    const QString orderingPattern =
        QStringLiteral(
            "(\\d{4}-\\d{2}-\\d{2}"
            "(?:T\\d{2}-\\d{2}-\\d{2})?)"
            );

    if (suffix.isEmpty()) {
        return QRegularExpression(
            QStringLiteral(
                "^%1-%2$"
                )
                .arg(
                    escapedBaseName,
                    orderingPattern
                    )
            );
    }

    const QString escapedSuffix =
        QRegularExpression::escape(
            suffix
            );

    return QRegularExpression(
        QStringLiteral(
            "^%1-%2\\.%3$"
            )
            .arg(
                escapedBaseName,
                orderingPattern,
                escapedSuffix
                )
        );
}

bool completeMatch(
    const QRegularExpressionMatch &match,
    const QString &fileName
    )
{
    return match.hasMatch()
    && match.capturedStart(0) == 0
        && match.capturedLength(0)
               == fileName.size();
}

std::optional<QDateTime>
parseDateTimeValue(
    const QString &value,
    const QString &format
    )
{
    QDateTime dateTime =
        QDateTime::fromString(
            value,
            format
            );

    if (dateTime.isValid()) {
        return dateTime;
    }

    const QDate date =
        QDate::fromString(
            value,
            format
            );

    if (!date.isValid()) {
        return std::nullopt;
    }

    return QDateTime(
        date,
        QTime(
            0,
            0
            ),
        QTimeZone::UTC
        );
}

std::optional<DiscoveryConfiguration>
configurationForRule(
    const QFileInfo &activeSourceInfo,
    const RotatedSourceRule &rule,
    QString &errorMessage
    )
{
    DiscoveryConfiguration configuration;

    switch (rule.namingScheme) {
    case RotatedSourceNamingScheme::Disabled:
        return std::nullopt;

    case RotatedSourceNamingScheme::NumericSuffix:
        configuration.expression =
            numericSuffixExpression(
                activeSourceInfo
                );

        configuration.orderValueType =
            RotatedSourceOrderValueType::
            Numeric;

        configuration.orderDirection =
            rule.numericOrderDirection;

        configuration.numericRotationScheme =
            true;

        break;

    case RotatedSourceNamingScheme::
        NumericBeforeExtension:
        configuration.expression =
            numericBeforeExtensionExpression(
                activeSourceInfo
                );

        configuration.orderValueType =
            RotatedSourceOrderValueType::
            Numeric;

        configuration.orderDirection =
            rule.numericOrderDirection;

        configuration.numericRotationScheme =
            true;

        break;

    case RotatedSourceNamingScheme::
        IsoDateTimeBeforeExtension:
        configuration.expression =
            isoDateTimeBeforeExtensionExpression(
                activeSourceInfo
                );

        /*
         * ISO-style zero-padded values sort in the
         * same order lexicographically as they do
         * chronologically.
         */
        configuration.orderValueType =
            RotatedSourceOrderValueType::
            Lexicographic;

        configuration.orderDirection =
            RotatedSourceOrderDirection::
            Ascending;

        break;

    case RotatedSourceNamingScheme::CustomRegex:
        if (rule
                .customRegularExpression
                .trimmed()
                .isEmpty()) {
            errorMessage =
                QStringLiteral(
                    "A custom rotated-source rule "
                    "requires a regular expression."
                    );

            return std::nullopt;
        }

        configuration.expression =
            QRegularExpression(
                rule.customRegularExpression
                );

        configuration.orderCaptureGroup =
            rule.customOrderCaptureGroup;

        configuration.orderValueType =
            rule.customOrderValueType;

        configuration.orderDirection =
            rule.customOrderDirection;

        configuration.dateTimeFormat =
            rule.customDateTimeFormat;

        break;
    }

    if (!configuration.expression.isValid()) {
        errorMessage =
            QStringLiteral(
                "The rotated-source regular "
                "expression is invalid: %1"
                )
                .arg(
                    configuration
                        .expression
                        .errorString()
                    );

        return std::nullopt;
    }

    if (configuration.orderCaptureGroup <= 0
        || configuration.orderCaptureGroup
               > configuration
                     .expression
                     .captureCount()) {
        errorMessage =
            QStringLiteral(
                "The rotated-source ordering "
                "capture group is not present in "
                "the configured regular expression."
                );

        return std::nullopt;
    }

    if (configuration.orderValueType
            == RotatedSourceOrderValueType::
            DateTime
        && configuration
               .dateTimeFormat
               .trimmed()
               .isEmpty()) {
        errorMessage =
            QStringLiteral(
                "A date/time rotated-source rule "
                "requires a date/time format."
                );

        return std::nullopt;
    }

    return configuration;
}

bool populateOrderingValue(
    DiscoveryCandidate &candidate,
    const DiscoveryConfiguration &configuration,
    QString &errorMessage
    )
{
    const QString &value =
        candidate.match.orderingValue;

    switch (configuration.orderValueType) {
    case RotatedSourceOrderValueType::Numeric: {
        bool converted = false;

        const qint64 numericValue =
            value.toLongLong(
                &converted
                );

        if (!converted) {
            errorMessage =
                QStringLiteral(
                    "The rotated source '%1' "
                    "matched the configured rule, "
                    "but ordering value '%2' is "
                    "not a valid integer."
                    )
                    .arg(
                        candidate.match.fileName,
                        value
                        );

            return false;
        }

        candidate.numericOrderValue =
            numericValue;

        if (configuration
                .numericRotationScheme) {
            if (numericValue <= 0) {
                return false;
            }

            candidate.match.rotationIndex =
                numericValue;
        }

        return true;
    }

    case RotatedSourceOrderValueType::
        Lexicographic:
        candidate.lexicalOrderValue =
            value;

        return true;

    case RotatedSourceOrderValueType::DateTime: {
        const std::optional<QDateTime>
            dateTime =
            parseDateTimeValue(
                value,
                configuration
                    .dateTimeFormat
                );

        if (!dateTime.has_value()) {
            errorMessage =
                QStringLiteral(
                    "The rotated source '%1' "
                    "matched the configured rule, "
                    "but ordering value '%2' could "
                    "not be parsed with format '%3'."
                    )
                    .arg(
                        candidate.match.fileName,
                        value,
                        configuration
                            .dateTimeFormat
                        );

            return false;
        }

        candidate.dateTimeOrderValue =
            dateTime.value();

        return true;
    }
    }

    return false;
}

int compareCandidates(
    const DiscoveryCandidate &left,
    const DiscoveryCandidate &right,
    RotatedSourceOrderValueType valueType
    )
{
    switch (valueType) {
    case RotatedSourceOrderValueType::Numeric:
        if (left.numericOrderValue
            < right.numericOrderValue) {
            return -1;
        }

        if (left.numericOrderValue
            > right.numericOrderValue) {
            return 1;
        }

        break;

    case RotatedSourceOrderValueType::
        Lexicographic: {
        const int comparison =
            QString::compare(
                left.lexicalOrderValue,
                right.lexicalOrderValue,
                Qt::CaseSensitive
                );

        if (comparison < 0) {
            return -1;
        }

        if (comparison > 0) {
            return 1;
        }

        break;
    }

    case RotatedSourceOrderValueType::DateTime:
        if (left.dateTimeOrderValue
            < right.dateTimeOrderValue) {
            return -1;
        }

        if (left.dateTimeOrderValue
            > right.dateTimeOrderValue) {
            return 1;
        }

        break;
    }

    return QString::compare(
        left.match.fileName,
        right.match.fileName,
        Qt::CaseSensitive
        );
}
}

RotatedSourceDiscoveryResult
RotatedSourceDiscoveryService::discover(
    const QString &activeSourcePath,
    const RotatedSourceRule &rule
    )
{
    RotatedSourceDiscoveryResult result;

    if (rule.namingScheme
        == RotatedSourceNamingScheme::Disabled) {
        return result;
    }

    const QFileInfo activeSourceInfo(
        activeSourcePath
        );

    if (!activeSourceInfo.exists()
        || !activeSourceInfo.isFile()) {
        return failure(
            QStringLiteral(
                "The active source file does not exist "
                "or is not a regular file."
                )
            );
    }

    QString configurationError;

    const std::optional<DiscoveryConfiguration>
        configuration =
        configurationForRule(
            activeSourceInfo,
            rule,
            configurationError
            );

    if (!configuration.has_value()) {
        return failure(
            configurationError
            );
    }

    const QDir directory(
        activeSourceInfo.absolutePath()
        );

    const QFileInfoList entries =
        directory.entryInfoList(
            QDir::Files
                | QDir::NoDotAndDotDot,
            QDir::Name
            );

    QVector<DiscoveryCandidate>
        candidates;

    for (const QFileInfo &entry
         : entries) {
        /*
         * Never rediscover the active source itself as
         * one of its archived siblings.
         */
        if (entry.absoluteFilePath()
            == activeSourceInfo
                   .absoluteFilePath()) {
            continue;
        }

        const QRegularExpressionMatch match =
            configuration
                ->expression
                .match(
                    entry.fileName()
                    );

        if (!completeMatch(
                match,
                entry.fileName()
                )) {
            continue;
        }

        DiscoveryCandidate candidate;

        candidate.match.filePath =
            entry.absoluteFilePath();

        candidate.match.fileName =
            entry.fileName();

        candidate.match.orderingValue =
            match.captured(
                configuration
                    ->orderCaptureGroup
                );

        QString orderingError;

        if (!populateOrderingValue(
                candidate,
                configuration.value(),
                orderingError
                )) {
            /*
             * Numeric built-in rotation index zero is
             * intentionally ignored. Any other failure
             * represents a configured rule that cannot
             * establish deterministic ordering.
             */
            if (configuration
                    ->numericRotationScheme
                && orderingError.isEmpty()) {
                continue;
            }

            return failure(
                orderingError
                );
        }

        candidates.append(
            std::move(
                candidate
                )
            );
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [&configuration](
            const DiscoveryCandidate &left,
            const DiscoveryCandidate &right
            ) {
            int comparison =
                compareCandidates(
                    left,
                    right,
                    configuration
                        ->orderValueType
                    );

            if (configuration
                    ->orderDirection
                == RotatedSourceOrderDirection::
                Descending) {
                comparison =
                    -comparison;
            }

            return comparison < 0;
        }
        );

    result.rotatedSources.reserve(
        candidates.size()
        );

    for (DiscoveryCandidate &candidate
         : candidates) {
        result.rotatedSources.append(
            std::move(
                candidate.match
                )
            );
    }

    return result;
}

RotatedSourceRuleSuggestionResult
RotatedSourceDiscoveryService::suggestRules(
    const QString &activeSourcePath
    )
{
    RotatedSourceRuleSuggestionResult result;

    const QFileInfo activeSourceInfo(
        activeSourcePath
        );

    if (!activeSourceInfo.exists()
        || !activeSourceInfo.isFile()) {
        return suggestionFailure(
            QStringLiteral(
                "The active source file does not exist "
                "or is not a regular file."
                )
            );
    }

    QVector<RotatedSourceRule>
        candidateRules;

    RotatedSourceRule numericSuffixRule;

    numericSuffixRule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    candidateRules.append(
        numericSuffixRule
        );

    /*
     * Without an extension, NumericBeforeExtension
     * intentionally collapses to the same filename
     * shape as NumericSuffix, so do not suggest both.
     */
    if (!activeSourceInfo.suffix().isEmpty()) {
        RotatedSourceRule
            numericBeforeExtensionRule;

        numericBeforeExtensionRule.namingScheme =
            RotatedSourceNamingScheme::
            NumericBeforeExtension;

        candidateRules.append(
            numericBeforeExtensionRule
            );
    }

    RotatedSourceRule dateTimeRule;

    dateTimeRule.namingScheme =
        RotatedSourceNamingScheme::
        IsoDateTimeBeforeExtension;

    candidateRules.append(
        dateTimeRule
        );

    for (const RotatedSourceRule &rule
         : candidateRules) {
        const RotatedSourceDiscoveryResult
            discovery =
            discover(
                activeSourcePath,
                rule
                );

        if (!discovery.succeeded) {
            return suggestionFailure(
                discovery.errorMessage
                );
        }

        if (discovery
                .rotatedSources
                .isEmpty()) {
            continue;
        }

        RotatedSourceRuleSuggestion
            suggestion;

        suggestion.rule =
            rule;

        suggestion.matchCount =
            discovery.rotatedSources.size();

        result.suggestions.append(
            std::move(
                suggestion
                )
            );
    }

    /*
     * Prefer the rule with the strongest evidence.
     * Equal-count suggestions retain the stable
     * built-in priority established above.
     */
    std::stable_sort(
        result.suggestions.begin(),
        result.suggestions.end(),
        [](
            const RotatedSourceRuleSuggestion &left,
            const RotatedSourceRuleSuggestion &right
            ) {
            return left.matchCount
                   > right.matchCount;
        }
        );

    return result;
}