#include <QtTest>

#include <QSettings>
#include <QTemporaryDir>

#include "../src/preferences/RotatedSourceSettingsStore.h"

class RotatedSourceSettingsStoreTests
    : public QObject
{
    Q_OBJECT

private slots:
    void savesAndRestoresCustomRules();
    void replacesCustomRuleByName();
    void removesCustomRule();
    void remembersSourceConfiguration();
    void replacesRememberedSourceConfiguration();
    void forgetsSourceConfiguration();
};

void RotatedSourceSettingsStoreTests::
    savesAndRestoresCustomRules()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    SavedRotatedSourceRule savedRule;

    savedRule.name =
        QStringLiteral(
            "Warehouse archives"
            );

    savedRule.rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    savedRule.rule.customRegularExpression =
        QStringLiteral(
            "^warehouse-sync-test-archive-(\\d{3})\\.jsonl$"
            );

    savedRule.rule.customOrderCaptureGroup =
        1;

    savedRule.rule.customOrderValueType =
        RotatedSourceOrderValueType::
        Numeric;

    savedRule.rule.customOrderDirection =
        RotatedSourceOrderDirection::
        Descending;

    QVERIFY(
        store.saveCustomRule(
            savedRule
            )
        );

    const auto rules =
        store.savedCustomRules();

    QCOMPARE(
        rules.size(),
        1
        );

    QCOMPARE(
        rules.first().name,
        QStringLiteral(
            "Warehouse archives"
            )
        );

    QCOMPARE(
        rules.first()
            .rule
            .customRegularExpression,
        savedRule
            .rule
            .customRegularExpression
        );

    QCOMPARE(
        rules.first()
            .rule
            .customOrderValueType,
        RotatedSourceOrderValueType::
        Numeric
        );

    QCOMPARE(
        rules.first()
            .rule
            .customOrderDirection,
        RotatedSourceOrderDirection::
        Descending
        );
}

void RotatedSourceSettingsStoreTests::
    replacesCustomRuleByName()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    SavedRotatedSourceRule first;

    first.name =
        QStringLiteral(
            "Warehouse"
            );

    first.rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    first.rule.customRegularExpression =
        QStringLiteral(
            "^old-(\\d+)\\.log$"
            );

    QVERIFY(
        store.saveCustomRule(
            first
            )
        );

    SavedRotatedSourceRule replacement =
        first;

    replacement.name =
        QStringLiteral(
            "warehouse"
            );

    replacement.rule.customRegularExpression =
        QStringLiteral(
            "^new-(\\d+)\\.log$"
            );

    QVERIFY(
        store.saveCustomRule(
            replacement
            )
        );

    const auto rules =
        store.savedCustomRules();

    QCOMPARE(
        rules.size(),
        1
        );

    QCOMPARE(
        rules.first()
            .rule
            .customRegularExpression,
        QStringLiteral(
            "^new-(\\d+)\\.log$"
            )
        );
}

void RotatedSourceSettingsStoreTests::
    removesCustomRule()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    SavedRotatedSourceRule savedRule;

    savedRule.name =
        QStringLiteral("Rule");

    savedRule.rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    savedRule.rule.customRegularExpression =
        QStringLiteral(
            "^archive-(\\d+)\\.log$"
            );

    QVERIFY(
        store.saveCustomRule(
            savedRule
            )
        );

    QVERIFY(
        store.removeCustomRule(
            QStringLiteral(
                "rule"
                )
            )
        );

    QVERIFY(
        store.savedCustomRules()
            .isEmpty()
        );
}

void RotatedSourceSettingsStoreTests::
    remembersSourceConfiguration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    RememberedRotatedSourceConfiguration
        configuration;

    configuration.sourcePath =
        directory.filePath(
            QStringLiteral(
                "warehouse-sync-test-current.jsonl"
                )
            );

    configuration.rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    configuration.rule.customRegularExpression =
        QStringLiteral(
            "^warehouse-sync-test-archive-(\\d{3})\\.jsonl$"
            );

    configuration.rule.customOrderValueType =
        RotatedSourceOrderValueType::
        Numeric;

    configuration.rule.customOrderDirection =
        RotatedSourceOrderDirection::
        Descending;

    configuration.includeRotatedSources =
        true;

    QVERIFY(
        store.rememberSourceConfiguration(
            configuration
            )
        );

    const auto restored =
        store.rememberedSourceConfiguration(
            configuration.sourcePath
            );

    QVERIFY(
        restored.has_value()
        );

    QVERIFY(
        restored
            ->includeRotatedSources
        );

    QCOMPARE(
        restored
            ->rule
            .namingScheme,
        RotatedSourceNamingScheme::
        CustomRegex
        );

    QCOMPARE(
        restored
            ->rule
            .customRegularExpression,
        configuration
            .rule
            .customRegularExpression
        );
}

void RotatedSourceSettingsStoreTests::
    replacesRememberedSourceConfiguration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    RememberedRotatedSourceConfiguration
        first;

    first.sourcePath =
        sourcePath;

    first.rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    first.includeRotatedSources =
        true;

    QVERIFY(
        store.rememberSourceConfiguration(
            first
            )
        );

    RememberedRotatedSourceConfiguration
        replacement;

    replacement.sourcePath =
        sourcePath;

    replacement.rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericBeforeExtension;

    replacement.rule.numericOrderDirection =
        RotatedSourceOrderDirection::
        Ascending;

    replacement.includeRotatedSources =
        false;

    QVERIFY(
        store.rememberSourceConfiguration(
            replacement
            )
        );

    const auto restored =
        store.rememberedSourceConfiguration(
            sourcePath
            );

    QVERIFY(
        restored.has_value()
        );

    QCOMPARE(
        restored
            ->rule
            .namingScheme,
        RotatedSourceNamingScheme::
        NumericBeforeExtension
        );

    QCOMPARE(
        restored
            ->rule
            .numericOrderDirection,
        RotatedSourceOrderDirection::
        Ascending
        );

    QVERIFY(
        !restored
             ->includeRotatedSources
        );
}

void RotatedSourceSettingsStoreTests::
    forgetsSourceConfiguration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    QSettings settings(
        directory.filePath(
            QStringLiteral(
                "settings.ini"
                )
            ),
        QSettings::IniFormat
        );

    RotatedSourceSettingsStore store(
        settings
        );

    RememberedRotatedSourceConfiguration
        configuration;

    configuration.sourcePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    configuration.rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    QVERIFY(
        store.rememberSourceConfiguration(
            configuration
            )
        );

    QVERIFY(
        store.forgetSourceConfiguration(
            configuration.sourcePath
            )
        );

    QVERIFY(
        !store
             .rememberedSourceConfiguration(
                 configuration.sourcePath
                 )
             .has_value()
        );
}

QTEST_MAIN(
    RotatedSourceSettingsStoreTests
    )

#include "RotatedSourceSettingsStoreTests.moc"