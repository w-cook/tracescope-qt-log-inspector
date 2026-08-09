#include <QtTest/QtTest>

#include "../src/importing/ImportProfile.h"

class ImportProfileTests : public QObject
{
    Q_OBJECT

private slots:
    void defaultProfileUsesCurrentSchema();
    void defaultProfileMatchesJsonLinesMappings();
    void profileStoresCustomFieldMappings();
    void profileStoresSeverityAliases();
    void profileStoresTimestampRules();
};

void ImportProfileTests::
    defaultProfileUsesCurrentSchema()
{
    const ImportProfile profile;

    QCOMPARE(
        profile.schemaVersion,
        ImportProfile::CurrentSchemaVersion
        );

    QCOMPARE(
        profile.importerId,
        QStringLiteral("json-lines")
        );

    QVERIFY(profile.preserveUnmappedFields);
}

void ImportProfileTests::
    defaultProfileMatchesJsonLinesMappings()
{
    const ImportProfile profile;

    QCOMPARE(
        profile.canonicalFields.timestampPath,
        QStringLiteral("timestamp")
        );

    QCOMPARE(
        profile.canonicalFields.severityPath,
        QStringLiteral("level")
        );

    QCOMPARE(
        profile.canonicalFields.subsystemPath,
        QStringLiteral("subsystem")
        );

    QCOMPARE(
        profile.canonicalFields.eventCodePath,
        QStringLiteral("eventCode")
        );

    QCOMPARE(
        profile.canonicalFields.entityIdPath,
        QStringLiteral("entityId")
        );

    QCOMPARE(
        profile.canonicalFields.messagePath,
        QStringLiteral("message")
        );
}

void ImportProfileTests::
    profileStoresCustomFieldMappings()
{
    ImportProfile profile;

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral("context.requestId")
    });

    QCOMPARE(profile.customFields.size(), 1);

    QCOMPARE(
        profile.customFields.first().name,
        QStringLiteral("Request ID")
        );

    QCOMPARE(
        profile.customFields.first().sourcePath,
        QStringLiteral("context.requestId")
        );
}

void ImportProfileTests::
    profileStoresSeverityAliases()
{
    ImportProfile profile;

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    QVERIFY(
        profile.severityAliases.contains(
            QStringLiteral("NOTICE")
            )
        );

    QVERIFY(
        profile.severityAliases.value(
            QStringLiteral("NOTICE")
            )
        == RecordSeverity::Info
        );
}

void ImportProfileTests::
    profileStoresTimestampRules()
{
    ImportProfile profile;

    QCOMPARE(
        profile.timestampRules.size(),
        1
        );

    QVERIFY(
        profile.timestampRules.first().type
        == TimestampRuleType::Iso8601
        );

    TimestampRule customRule;

    customRule.type =
        TimestampRuleType::QtFormat;

    customRule.format =
        QStringLiteral(
            "yyyy-MM-dd HH:mm:ss.zzz"
            );

    profile.timestampRules.append(
        customRule
        );

    QCOMPARE(
        profile.timestampRules.size(),
        2
        );

    QVERIFY(
        profile.timestampRules.at(1).type
        == TimestampRuleType::QtFormat
        );

    QCOMPARE(
        profile.timestampRules.at(1).format,
        QStringLiteral(
            "yyyy-MM-dd HH:mm:ss.zzz"
            )
        );
}

QTEST_MAIN(ImportProfileTests)

#include "ImportProfileTests.moc"