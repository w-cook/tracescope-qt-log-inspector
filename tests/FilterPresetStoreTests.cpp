#include <QtTest>

#include <QSettings>
#include <QTemporaryDir>

#include "../src/preferences/FilterPresetStore.h"

class FilterPresetStoreTests : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsCompletePreset();
    void duplicateNameReplacesExistingPreset();
    void removesPreset();
    void rejectsEmptyName();
};

void FilterPresetStoreTests::roundTripsCompletePreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    FilterPresetStore store(settings);

    InvestigationFilterPreset preset;

    preset.name =
        QStringLiteral(
            "Critical database investigation"
            );

    preset.severities = {
        QStringLiteral("WARN"),
        QStringLiteral("ERROR")
    };

    preset.subsystems = {
        QStringLiteral("database"),
        QStringLiteral("storage")
    };

    preset.searchText =
        QStringLiteral("timeout");

    preset.eventCodes = {
        QStringLiteral("DB-100"),
        QStringLiteral("DB-200")
    };

    preset.entityIds = {
        QStringLiteral("node-7")
};

preset.timeRangeStart =
    QDateTime::fromString(
        QStringLiteral(
            "2026-08-19T12:00:00.000Z"
            ),
        Qt::ISODateWithMs
        );

preset.timeRangeEnd =
    QDateTime::fromString(
        QStringLiteral(
            "2026-08-19T12:30:00.000Z"
            ),
        Qt::ISODateWithMs
        );

preset.customFieldFilters.insert(
    QStringLiteral("region"),
    {
        QStringLiteral("east"),
        QStringLiteral("west")
    }
    );

preset.customFieldFilters.insert(
    QStringLiteral("queue"),
    {
        QStringLiteral("orders")
    }
    );

QVERIFY(
    store.savePreset(preset)
    );

const QVector<InvestigationFilterPreset>
    presets =
    store.presets();

QCOMPARE(
    presets.size(),
    1
    );

const InvestigationFilterPreset &loaded =
    presets.front();

QCOMPARE(
    loaded.name,
    preset.name
    );

QCOMPARE(
    loaded.severities,
    preset.severities
    );

QCOMPARE(
    loaded.subsystems,
    preset.subsystems
    );

QCOMPARE(
    loaded.searchText,
    preset.searchText
    );

QCOMPARE(
    loaded.eventCodes,
    preset.eventCodes
    );

QCOMPARE(
    loaded.entityIds,
    preset.entityIds
    );

QVERIFY(
    loaded.timeRangeStart.has_value()
    );

QCOMPARE(
    loaded.timeRangeStart.value(),
    preset.timeRangeStart.value()
    );

QVERIFY(
    loaded.timeRangeEnd.has_value()
    );

QCOMPARE(
    loaded.timeRangeEnd.value(),
    preset.timeRangeEnd.value()
    );

QCOMPARE(
    loaded.customFieldFilters,
    preset.customFieldFilters
    );
}

void FilterPresetStoreTests::
    duplicateNameReplacesExistingPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    FilterPresetStore store(settings);

    InvestigationFilterPreset first;

    first.name =
        QStringLiteral("Errors");

    first.severities = {
        QStringLiteral("ERROR")
};

QVERIFY(
    store.savePreset(first)
    );

InvestigationFilterPreset replacement;

replacement.name =
    QStringLiteral("errors");

replacement.searchText =
    QStringLiteral("connection");

QVERIFY(
    store.savePreset(replacement)
    );

const QVector<InvestigationFilterPreset>
    presets =
    store.presets();

QCOMPARE(
    presets.size(),
    1
    );

QCOMPARE(
    presets.front().name,
    QStringLiteral("errors")
    );

QCOMPARE(
    presets.front().searchText,
    QStringLiteral("connection")
    );

QVERIFY(
    presets.front().severities.isEmpty()
    );
}

void FilterPresetStoreTests::removesPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    FilterPresetStore store(settings);

    InvestigationFilterPreset first;
    first.name =
        QStringLiteral("First");

    InvestigationFilterPreset second;
    second.name =
        QStringLiteral("Second");

    QVERIFY(store.savePreset(first));
    QVERIFY(store.savePreset(second));

    QVERIFY(
        store.removePreset(
            QStringLiteral("FIRST")
            )
        );

    const QVector<InvestigationFilterPreset>
        presets =
        store.presets();

    QCOMPARE(
        presets.size(),
        1
        );

    QCOMPARE(
        presets.front().name,
        QStringLiteral("Second")
        );

    QVERIFY(
        !store.removePreset(
            QStringLiteral("Missing")
            )
        );
}

void FilterPresetStoreTests::rejectsEmptyName()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QSettings settings(
        directory.filePath(
            QStringLiteral("settings.ini")
            ),
        QSettings::IniFormat
        );

    FilterPresetStore store(settings);

    InvestigationFilterPreset preset;
    preset.name =
        QStringLiteral("   ");

    QVERIFY(
        !store.savePreset(preset)
        );

    QVERIFY(
        store.presets().isEmpty()
        );
}

QTEST_MAIN(FilterPresetStoreTests)

#include "FilterPresetStoreTests.moc"