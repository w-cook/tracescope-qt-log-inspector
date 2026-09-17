#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "../sources/RotatedSourceRule.h"

class QSettings;

struct SavedRotatedSourceRule
{
    QString name;
    RotatedSourceRule rule;
};

struct RememberedRotatedSourceConfiguration
{
    QString sourcePath;

    RotatedSourceRule rule;

    bool includeRotatedSources = false;
};

class RotatedSourceSettingsStore
{
public:
    static constexpr int
        MaxRememberedSources = 50;

    explicit RotatedSourceSettingsStore(
        QSettings &settings
        );

    QVector<SavedRotatedSourceRule>
    savedCustomRules() const;

    bool saveCustomRule(
        SavedRotatedSourceRule savedRule
        );

    bool removeCustomRule(
        const QString &name
        );

    std::optional<
        RememberedRotatedSourceConfiguration
        >
    rememberedSourceConfiguration(
        const QString &sourcePath
        ) const;

    bool rememberSourceConfiguration(
        RememberedRotatedSourceConfiguration
            configuration
        );

    bool forgetSourceConfiguration(
        const QString &sourcePath
        );

private:
    QSettings &m_settings;

    void writeCustomRules(
        const QVector<SavedRotatedSourceRule>
            &rules
        );

    QVector<
        RememberedRotatedSourceConfiguration
        >
    rememberedSourceConfigurations() const;

    void writeRememberedSourceConfigurations(
        const QVector<
            RememberedRotatedSourceConfiguration
            > &configurations
        );

    static QString normalizedPath(
        const QString &sourcePath
        );

    static bool pathsEqual(
        const QString &left,
        const QString &right
        );
};