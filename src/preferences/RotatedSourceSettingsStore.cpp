#include "RotatedSourceSettingsStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <algorithm>
#include <utility>

namespace
{
const QString CustomRulesKey =
    QStringLiteral(
        "rotation/customRules"
        );

const QString SourceConfigurationsKey =
    QStringLiteral(
        "rotation/sourceConfigurations"
        );

const QString VersionKey =
    QStringLiteral("version");

const QString NameKey =
    QStringLiteral("name");

const QString SourcePathKey =
    QStringLiteral("sourcePath");

const QString IncludeRotatedSourcesKey =
    QStringLiteral(
        "includeRotatedSources"
        );

const QString NamingSchemeKey =
    QStringLiteral(
        "rule/namingScheme"
        );

const QString CustomRegularExpressionKey =
    QStringLiteral(
        "rule/customRegularExpression"
        );

const QString CustomOrderCaptureGroupKey =
    QStringLiteral(
        "rule/customOrderCaptureGroup"
        );

const QString CustomOrderValueTypeKey =
    QStringLiteral(
        "rule/customOrderValueType"
        );

const QString CustomOrderDirectionKey =
    QStringLiteral(
        "rule/customOrderDirection"
        );

const QString CustomDateTimeFormatKey =
    QStringLiteral(
        "rule/customDateTimeFormat"
        );

const QString NumericOrderDirectionKey =
    QStringLiteral(
        "rule/numericOrderDirection"
        );

constexpr int CurrentVersion = 1;

bool validNamingScheme(
    int value
    )
{
    return value
               >= static_cast<int>(
                   RotatedSourceNamingScheme::
                   Disabled
                   )
           && value
                  <= static_cast<int>(
                      RotatedSourceNamingScheme::
                      CustomRegex
                      );
}

bool validOrderValueType(
    int value
    )
{
    return value
               >= static_cast<int>(
                   RotatedSourceOrderValueType::
                   Numeric
                   )
           && value
                  <= static_cast<int>(
                      RotatedSourceOrderValueType::
                      DateTime
                      );
}

bool validOrderDirection(
    int value
    )
{
    return value
               >= static_cast<int>(
                   RotatedSourceOrderDirection::
                   Ascending
                   )
           && value
                  <= static_cast<int>(
                      RotatedSourceOrderDirection::
                      Descending
                      );
}

void writeRule(
    QSettings &settings,
    const RotatedSourceRule &rule
    )
{
    settings.setValue(
        NamingSchemeKey,
        static_cast<int>(
            rule.namingScheme
            )
        );

    settings.setValue(
        NumericOrderDirectionKey,
        static_cast<int>(
            rule.numericOrderDirection
            )
        );

    settings.setValue(
        CustomRegularExpressionKey,
        rule.customRegularExpression
        );

    settings.setValue(
        CustomOrderCaptureGroupKey,
        rule.customOrderCaptureGroup
        );

    settings.setValue(
        CustomOrderValueTypeKey,
        static_cast<int>(
            rule.customOrderValueType
            )
        );

    settings.setValue(
        CustomOrderDirectionKey,
        static_cast<int>(
            rule.customOrderDirection
            )
        );

    settings.setValue(
        CustomDateTimeFormatKey,
        rule.customDateTimeFormat
        );
}

std::optional<RotatedSourceRule>
readRule(
    QSettings &settings
    )
{
    const int namingScheme =
        settings
            .value(
                NamingSchemeKey,
                static_cast<int>(
                    RotatedSourceNamingScheme::
                    Disabled
                    )
                )
            .toInt();

    const int numericOrderDirection =
        settings
            .value(
                NumericOrderDirectionKey,
                static_cast<int>(
                    RotatedSourceOrderDirection::
                    Descending
                    )
                )
            .toInt();

    const int orderValueType =
        settings
            .value(
                CustomOrderValueTypeKey,
                static_cast<int>(
                    RotatedSourceOrderValueType::
                    Lexicographic
                    )
                )
            .toInt();

    const int orderDirection =
        settings
            .value(
                CustomOrderDirectionKey,
                static_cast<int>(
                    RotatedSourceOrderDirection::
                    Ascending
                    )
                )
            .toInt();

    if (!validNamingScheme(
            namingScheme
            )
        || !validOrderDirection(
            numericOrderDirection
            )
        || !validOrderValueType(
            orderValueType
            )
        || !validOrderDirection(
            orderDirection
            )) {
        return std::nullopt;
    }

    RotatedSourceRule rule;

    rule.namingScheme =
        static_cast<
            RotatedSourceNamingScheme
            >(
            namingScheme
            );

    rule.numericOrderDirection =
        static_cast<
            RotatedSourceOrderDirection
            >(
            numericOrderDirection
            );

    rule.customRegularExpression =
        settings
            .value(
                CustomRegularExpressionKey
                )
            .toString();

    rule.customOrderCaptureGroup =
        settings
            .value(
                CustomOrderCaptureGroupKey,
                1
                )
            .toInt();

    rule.customOrderValueType =
        static_cast<
            RotatedSourceOrderValueType
            >(
            orderValueType
            );

    rule.customOrderDirection =
        static_cast<
            RotatedSourceOrderDirection
            >(
            orderDirection
            );

    rule.customDateTimeFormat =
        settings
            .value(
                CustomDateTimeFormatKey
                )
            .toString();

    if (rule.customOrderCaptureGroup <= 0) {
        return std::nullopt;
    }

    return rule;
}

bool validSavedCustomRule(
    const SavedRotatedSourceRule &savedRule
    )
{
    if (savedRule.name
            .trimmed()
            .isEmpty()) {
        return false;
    }

    if (savedRule.rule.namingScheme
        != RotatedSourceNamingScheme::
        CustomRegex) {
        return false;
    }

    if (savedRule
            .rule
            .customRegularExpression
            .trimmed()
            .isEmpty()) {
        return false;
    }

    if (savedRule
            .rule
            .customOrderCaptureGroup
        <= 0) {
        return false;
    }

    if (savedRule
                .rule
                .customOrderValueType
            == RotatedSourceOrderValueType::
            DateTime
        && savedRule
               .rule
               .customDateTimeFormat
               .trimmed()
               .isEmpty()) {
        return false;
    }

    return true;
}
}

RotatedSourceSettingsStore::
    RotatedSourceSettingsStore(
        QSettings &settings
        )
    : m_settings(settings)
{
}

QVector<SavedRotatedSourceRule>
    RotatedSourceSettingsStore::
    savedCustomRules() const
{
    QVector<SavedRotatedSourceRule>
        result;

    const int size =
        m_settings.beginReadArray(
            CustomRulesKey
            );

    result.reserve(
        size
        );

    for (int index = 0;
         index < size;
         ++index) {
        m_settings.setArrayIndex(
            index
            );

        const int version =
            m_settings
                .value(
                    VersionKey,
                    CurrentVersion
                    )
                .toInt();

        if (version
            != CurrentVersion) {
            continue;
        }

        SavedRotatedSourceRule
            savedRule;

        savedRule.name =
            m_settings
                .value(
                    NameKey
                    )
                .toString()
                .trimmed();

        const std::optional<
            RotatedSourceRule
            > rule =
            readRule(
                m_settings
                );

        if (!rule.has_value()) {
            continue;
        }

        savedRule.rule =
            rule.value();

        if (!validSavedCustomRule(
                savedRule
                )) {
            continue;
        }

        result.append(
            std::move(
                savedRule
                )
            );
    }

    m_settings.endArray();

    return result;
}

bool RotatedSourceSettingsStore::
    saveCustomRule(
        SavedRotatedSourceRule savedRule
        )
{
    savedRule.name =
        savedRule
            .name
            .trimmed();

    if (!validSavedCustomRule(
            savedRule
            )) {
        return false;
    }

    QVector<SavedRotatedSourceRule>
        rules =
        savedCustomRules();

    for (SavedRotatedSourceRule &existing
         : rules) {
        if (existing.name.compare(
                savedRule.name,
                Qt::CaseInsensitive
                )
            == 0) {
            existing =
                std::move(
                    savedRule
                    );

            writeCustomRules(
                rules
                );

            return true;
        }
    }

    rules.append(
        std::move(
            savedRule
            )
        );

    writeCustomRules(
        rules
        );

    return true;
}

bool RotatedSourceSettingsStore::
    removeCustomRule(
        const QString &name
        )
{
    const QString trimmedName =
        name.trimmed();

    if (trimmedName.isEmpty()) {
        return false;
    }

    QVector<SavedRotatedSourceRule>
        rules =
        savedCustomRules();

    for (qsizetype index = 0;
         index < rules.size();
         ++index) {
        if (rules[index]
                .name
                .compare(
                    trimmedName,
                    Qt::CaseInsensitive
                    )
            == 0) {
            rules.removeAt(
                index
                );

            writeCustomRules(
                rules
                );

            return true;
        }
    }

    return false;
}

void RotatedSourceSettingsStore::
    writeCustomRules(
        const QVector<
            SavedRotatedSourceRule
            > &rules
        )
{
    m_settings.remove(
        CustomRulesKey
        );

    m_settings.beginWriteArray(
        CustomRulesKey,
        rules.size()
        );

    for (qsizetype index = 0;
         index < rules.size();
         ++index) {
        m_settings.setArrayIndex(
            index
            );

        m_settings.setValue(
            VersionKey,
            CurrentVersion
            );

        m_settings.setValue(
            NameKey,
            rules[index].name
            );

        writeRule(
            m_settings,
            rules[index].rule
            );
    }

    m_settings.endArray();
    m_settings.sync();
}

std::optional<
    RememberedRotatedSourceConfiguration
    >
    RotatedSourceSettingsStore::
    rememberedSourceConfiguration(
        const QString &sourcePath
        ) const
{
    const QString normalized =
        normalizedPath(
            sourcePath
            );

    if (normalized.isEmpty()) {
        return std::nullopt;
    }

    const QVector<
        RememberedRotatedSourceConfiguration
        > configurations =
        rememberedSourceConfigurations();

    for (const auto &configuration
         : configurations) {
        if (pathsEqual(
                configuration.sourcePath,
                normalized
                )) {
            return configuration;
        }
    }

    return std::nullopt;
}

bool RotatedSourceSettingsStore::
    rememberSourceConfiguration(
        RememberedRotatedSourceConfiguration
            configuration
        )
{
    configuration.sourcePath =
        normalizedPath(
            configuration.sourcePath
            );

    if (configuration
            .sourcePath
            .isEmpty()) {
        return false;
    }

    QVector<
        RememberedRotatedSourceConfiguration
        > configurations =
        rememberedSourceConfigurations();

    configurations.erase(
        std::remove_if(
            configurations.begin(),
            configurations.end(),
            [
                &configuration
    ](
                const auto &existing
                ) {
                return pathsEqual(
                    existing.sourcePath,
                    configuration.sourcePath
                    );
            }
            ),
        configurations.end()
        );

    configurations.prepend(
        std::move(
            configuration
            )
        );

    while (
        configurations.size()
        > MaxRememberedSources
        ) {
        configurations.removeLast();
    }

    writeRememberedSourceConfigurations(
        configurations
        );

    return true;
}

bool RotatedSourceSettingsStore::
    forgetSourceConfiguration(
        const QString &sourcePath
        )
{
    const QString normalized =
        normalizedPath(
            sourcePath
            );

    if (normalized.isEmpty()) {
        return false;
    }

    QVector<
        RememberedRotatedSourceConfiguration
        > configurations =
        rememberedSourceConfigurations();

    const qsizetype originalSize =
        configurations.size();

    configurations.erase(
        std::remove_if(
            configurations.begin(),
            configurations.end(),
            [
                &normalized
    ](
                const auto &existing
                ) {
                return pathsEqual(
                    existing.sourcePath,
                    normalized
                    );
            }
            ),
        configurations.end()
        );

    if (configurations.size()
        == originalSize) {
        return false;
    }

    writeRememberedSourceConfigurations(
        configurations
        );

    return true;
}

QVector<
    RememberedRotatedSourceConfiguration
    >
    RotatedSourceSettingsStore::
    rememberedSourceConfigurations() const
{
    QVector<
        RememberedRotatedSourceConfiguration
        > result;

    const int size =
        m_settings.beginReadArray(
            SourceConfigurationsKey
            );

    result.reserve(
        size
        );

    for (int index = 0;
         index < size;
         ++index) {
        m_settings.setArrayIndex(
            index
            );

        const int version =
            m_settings
                .value(
                    VersionKey,
                    CurrentVersion
                    )
                .toInt();

        if (version
            != CurrentVersion) {
            continue;
        }

        RememberedRotatedSourceConfiguration
            configuration;

        configuration.sourcePath =
            normalizedPath(
                m_settings
                    .value(
                        SourcePathKey
                        )
                    .toString()
                );

        if (configuration
                .sourcePath
                .isEmpty()) {
            continue;
        }

        const std::optional<
            RotatedSourceRule
            > rule =
            readRule(
                m_settings
                );

        if (!rule.has_value()) {
            continue;
        }

        configuration.rule =
            rule.value();

        configuration.includeRotatedSources =
            m_settings
                .value(
                    IncludeRotatedSourcesKey,
                    false
                    )
                .toBool();

        result.append(
            std::move(
                configuration
                )
            );
    }

    m_settings.endArray();

    return result;
}

void RotatedSourceSettingsStore::
    writeRememberedSourceConfigurations(
        const QVector<
            RememberedRotatedSourceConfiguration
            > &configurations
        )
{
    m_settings.remove(
        SourceConfigurationsKey
        );

    m_settings.beginWriteArray(
        SourceConfigurationsKey,
        configurations.size()
        );

    for (qsizetype index = 0;
         index < configurations.size();
         ++index) {
        m_settings.setArrayIndex(
            index
            );

        const auto &configuration =
            configurations[index];

        m_settings.setValue(
            VersionKey,
            CurrentVersion
            );

        m_settings.setValue(
            SourcePathKey,
            configuration.sourcePath
            );

        m_settings.setValue(
            IncludeRotatedSourcesKey,
            configuration
                .includeRotatedSources
            );

        writeRule(
            m_settings,
            configuration.rule
            );
    }

    m_settings.endArray();
    m_settings.sync();
}

QString RotatedSourceSettingsStore::
    normalizedPath(
        const QString &sourcePath
        )
{
    const QString trimmed =
        sourcePath.trimmed();

    if (trimmed.isEmpty()) {
        return {};
    }

    return QDir::cleanPath(
        QFileInfo(
            trimmed
            )
            .absoluteFilePath()
        );
}

bool RotatedSourceSettingsStore::
    pathsEqual(
        const QString &left,
        const QString &right
        )
{
#ifdef Q_OS_WIN
    return left.compare(
               right,
               Qt::CaseInsensitive
               )
           == 0;
#else
    return left == right;
#endif
}