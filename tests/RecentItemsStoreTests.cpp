#include <QtTest>

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>

#include "../src/preferences/RecentItemsStore.h"

class RecentItemsStoreTests : public QObject
{
    Q_OBJECT

private slots:
    void startsEmpty();
    void storesMostRecentFileFirst();
    void movesDuplicateFileToFront();
    void keepsFilesProfilesAndWorkspacesSeparate();
    void limitsRecentItems();
    void removesItems();
    void persistsAcrossStoreInstances();
};

void RecentItemsStoreTests::startsEmpty()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    QVERIFY(
        store.recentFiles().isEmpty()
        );

    QVERIFY(
        store.recentProfiles().isEmpty()
        );

    QVERIFY(
        store.recentWorkspaces().isEmpty()
        );
}

void RecentItemsStoreTests::
    storesMostRecentFileFirst()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    const QString first =
        directory.filePath(
            QStringLiteral("first.jsonl")
            );

    const QString second =
        directory.filePath(
            QStringLiteral("second.xml")
            );

    store.addRecentFile(first);
    store.addRecentFile(second);

    const QStringList files =
        store.recentFiles();

    QCOMPARE(files.size(), 2);

    QCOMPARE(
        files[0],
        QDir::cleanPath(
            QFileInfo(second)
                .absoluteFilePath()
            )
        );

    QCOMPARE(
        files[1],
        QDir::cleanPath(
            QFileInfo(first)
                .absoluteFilePath()
            )
        );
}

void RecentItemsStoreTests::
    movesDuplicateFileToFront()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    const QString first =
        directory.filePath(
            QStringLiteral("first.jsonl")
            );

    const QString second =
        directory.filePath(
            QStringLiteral("second.jsonl")
            );

    store.addRecentFile(first);
    store.addRecentFile(second);
    store.addRecentFile(first);

    const QStringList files =
        store.recentFiles();

    QCOMPARE(files.size(), 2);

    QCOMPARE(
        QFileInfo(files[0]).fileName(),
        QStringLiteral("first.jsonl")
        );

    QCOMPARE(
        QFileInfo(files[1]).fileName(),
        QStringLiteral("second.jsonl")
        );
}

void RecentItemsStoreTests::
    keepsFilesProfilesAndWorkspacesSeparate()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    store.addRecentFile(
        directory.filePath(
            QStringLiteral("session.jsonl")
            )
        );

    store.addRecentProfile(
        directory.filePath(
            QStringLiteral("profile.json")
            )
        );

    store.addRecentWorkspace(
        directory.filePath(
            QStringLiteral(
                "investigation-workspace.json"
                )
            )
        );

    const QStringList recentFiles =
        store.recentFiles();

    const QStringList recentProfiles =
        store.recentProfiles();

    const QStringList recentWorkspaces =
        store.recentWorkspaces();

    QCOMPARE(
        recentFiles.size(),
        1
        );

    QCOMPARE(
        recentProfiles.size(),
        1
        );

    QCOMPARE(
        recentWorkspaces.size(),
        1
        );

    QCOMPARE(
        QFileInfo(
            recentFiles.first()
            ).fileName(),
        QStringLiteral("session.jsonl")
        );

    QCOMPARE(
        QFileInfo(
            recentProfiles.first()
            ).fileName(),
        QStringLiteral("profile.json")
        );

    QCOMPARE(
        QFileInfo(
            recentWorkspaces.first()
            ).fileName(),
        QStringLiteral(
            "investigation-workspace.json"
            )
        );
}

void RecentItemsStoreTests::
    limitsRecentItems()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    for (
        int index = 0;
        index
        < RecentItemsStore::MaxRecentItems
              + 3;
        ++index
        ) {
        store.addRecentFile(
            directory.filePath(
                QStringLiteral(
                    "session-%1.jsonl"
                    )
                    .arg(index)
                )
            );
    }

    const QStringList files =
        store.recentFiles();

    QCOMPARE(
        files.size(),
        RecentItemsStore::MaxRecentItems
        );

    QCOMPARE(
        QFileInfo(files.first()).fileName(),
        QStringLiteral("session-12.jsonl")
        );
}

void RecentItemsStoreTests::removesItems()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    RecentItemsStore store(settings);

    const QString filePath =
        directory.filePath(
            QStringLiteral("session.jsonl")
            );

    const QString profilePath =
        directory.filePath(
            QStringLiteral("profile.json")
            );

    const QString workspacePath =
        directory.filePath(
            QStringLiteral(
                "workspace.json"
                )
            );

    store.addRecentFile(
        filePath
        );

    store.addRecentProfile(
        profilePath
        );

    store.addRecentWorkspace(
        workspacePath
        );

    store.removeRecentFile(
        filePath
        );

    store.removeRecentProfile(
        profilePath
        );

    store.removeRecentWorkspace(
        workspacePath
        );

    QVERIFY(
        store.recentFiles().isEmpty()
        );

    QVERIFY(
        store.recentProfiles().isEmpty()
        );

    QVERIFY(
        store.recentWorkspaces().isEmpty()
        );
}

void RecentItemsStoreTests::
    persistsAcrossStoreInstances()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString settingsPath =
        directory.filePath(
            QStringLiteral("settings.ini")
            );

    const QString recentPath =
        directory.filePath(
            QStringLiteral("session.jsonl")
            );

    {
        QSettings settings(
            settingsPath,
            QSettings::IniFormat
            );

        RecentItemsStore store(settings);

        store.addRecentFile(
            recentPath
            );

        store.addRecentWorkspace(
            directory.filePath(
                QStringLiteral(
                    "workspace.json"
                    )
                )
            );
    }

    {
        QSettings settings(
            settingsPath,
            QSettings::IniFormat
            );

        RecentItemsStore store(settings);

        const QStringList recentFiles =
            store.recentFiles();

        QCOMPARE(
            recentFiles.size(),
            1
            );

        QCOMPARE(
            QFileInfo(
                recentFiles.first()
                ).fileName(),
            QStringLiteral("session.jsonl")
            );

        const QStringList recentWorkspaces =
            store.recentWorkspaces();

        QCOMPARE(
            recentWorkspaces.size(),
            1
            );

        QCOMPARE(
            QFileInfo(
                recentWorkspaces.first()
                ).fileName(),
            QStringLiteral(
                "workspace.json"
                )
            );
    }
}

QTEST_MAIN(RecentItemsStoreTests)

#include "RecentItemsStoreTests.moc"