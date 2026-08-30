#include "RecentItemsStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <algorithm>

namespace
{
const QString RecentFilesKey =
    QStringLiteral("recent/files");

const QString RecentProfilesKey =
    QStringLiteral("recent/profiles");
}

const QString RecentWorkspacesKey =
    QStringLiteral("recent/workspaces");

RecentItemsStore::RecentItemsStore(
    QSettings &settings
    )
    : m_settings(settings)
{
}

QStringList RecentItemsStore::recentFiles() const
{
    return recentItems(
        RecentFilesKey
        );
}

QStringList RecentItemsStore::recentProfiles() const
{
    return recentItems(
        RecentProfilesKey
        );
}

void RecentItemsStore::addRecentFile(
    const QString &filePath
    )
{
    addRecentItem(
        RecentFilesKey,
        filePath
        );
}

void RecentItemsStore::addRecentProfile(
    const QString &filePath
    )
{
    addRecentItem(
        RecentProfilesKey,
        filePath
        );
}

void RecentItemsStore::removeRecentFile(
    const QString &filePath
    )
{
    removeRecentItem(
        RecentFilesKey,
        filePath
        );
}

void RecentItemsStore::removeRecentProfile(
    const QString &filePath
    )
{
    removeRecentItem(
        RecentProfilesKey,
        filePath
        );
}

QStringList RecentItemsStore::recentItems(
    const QString &settingsKey
    ) const
{
    return m_settings
        .value(settingsKey)
        .toStringList();
}

void RecentItemsStore::addRecentItem(
    const QString &settingsKey,
    const QString &filePath
    )
{
    const QString normalized =
        normalizedPath(filePath);

    if (normalized.isEmpty()) {
        return;
    }

    QStringList items =
        recentItems(settingsKey);

    items.erase(
        std::remove_if(
            items.begin(),
            items.end(),
            [&normalized](
                const QString &existing
                ) {
                return pathsEqual(
                    existing,
                    normalized
                    );
            }
            ),
        items.end()
        );

    items.prepend(
        normalized
        );

    while (
        items.size()
        > MaxRecentItems
        ) {
        items.removeLast();
    }

    m_settings.setValue(
        settingsKey,
        items
        );

    m_settings.sync();
}

void RecentItemsStore::removeRecentItem(
    const QString &settingsKey,
    const QString &filePath
    )
{
    const QString normalized =
        normalizedPath(filePath);

    QStringList items =
        recentItems(settingsKey);

    items.erase(
        std::remove_if(
            items.begin(),
            items.end(),
            [&normalized](
                const QString &existing
                ) {
                return pathsEqual(
                    existing,
                    normalized
                    );
            }
            ),
        items.end()
        );

    m_settings.setValue(
        settingsKey,
        items
        );

    m_settings.sync();
}

QString RecentItemsStore::normalizedPath(
    const QString &filePath
    )
{
    const QString trimmed =
        filePath.trimmed();

    if (trimmed.isEmpty()) {
        return {};
    }

    return QDir::cleanPath(
        QFileInfo(trimmed)
            .absoluteFilePath()
        );
}

bool RecentItemsStore::pathsEqual(
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

QStringList RecentItemsStore::
    recentWorkspaces() const
{
    return recentItems(
        RecentWorkspacesKey
        );
}

void RecentItemsStore::
    addRecentWorkspace(
        const QString &filePath
        )
{
    addRecentItem(
        RecentWorkspacesKey,
        filePath
        );
}

void RecentItemsStore::
    removeRecentWorkspace(
        const QString &filePath
        )
{
    removeRecentItem(
        RecentWorkspacesKey,
        filePath
        );
}