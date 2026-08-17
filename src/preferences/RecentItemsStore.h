#pragma once

#include <QString>
#include <QStringList>

class QSettings;

class RecentItemsStore
{
public:
    static constexpr int MaxRecentItems = 10;

    explicit RecentItemsStore(
        QSettings &settings
        );

    QStringList recentFiles() const;
    QStringList recentProfiles() const;

    void addRecentFile(
        const QString &filePath
        );

    void addRecentProfile(
        const QString &filePath
        );

    void removeRecentFile(
        const QString &filePath
        );

    void removeRecentProfile(
        const QString &filePath
        );

private:
    QSettings &m_settings;

    QStringList recentItems(
        const QString &settingsKey
        ) const;

    void addRecentItem(
        const QString &settingsKey,
        const QString &filePath
        );

    void removeRecentItem(
        const QString &settingsKey,
        const QString &filePath
        );

    static QString normalizedPath(
        const QString &filePath
        );

    static bool pathsEqual(
        const QString &left,
        const QString &right
        );
};