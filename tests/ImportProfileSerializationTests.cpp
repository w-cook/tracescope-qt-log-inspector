#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>

#include "../src/importing/ImportProfileSerialization.h"

class ImportProfileSerializationTests :
                                        public QObject
{
    Q_OBJECT

private slots:
    void serializationProducesVersionedJson();
    void profileRoundTripsThroughJson();
    void malformedJsonIsRejected();
    void nonObjectRootIsRejected();
    void missingRequiredFieldIsRejected();
    void invalidSeverityTargetIsRejected();
    void unknownTimestampRuleTypeIsRejected();
    void unsupportedSchemaVersionCanBeDeserialized();
    void recordPathRoundTripsThroughJson();
    void missingRecordPathDefaultsToEmpty();
    void nonStringRecordPathIsRejected();
    void regexPatternRoundTripsThroughJson();
    void missingRegexPatternDefaultsToEmpty();
    void nonStringRegexPatternIsRejected();
};

ImportProfile populatedProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Application JSON Lines"
            );

    profile.canonicalFields.timestampPath =
        QStringLiteral("time");

    profile.canonicalFields.severityPath =
        QStringLiteral("severity");

    profile.canonicalFields.subsystemPath =
        QStringLiteral("service");

    profile.canonicalFields.eventCodePath =
        QStringLiteral("event.code");

    profile.canonicalFields.entityIdPath =
        QStringLiteral("resource.id");

    profile.canonicalFields.messagePath =
        QStringLiteral("payload.message");

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral(
            "context.requestId"
            )
    });

    profile.customFields.append({
        QStringLiteral("Duration"),
        QStringLiteral(
            "metrics.durationMs"
            )
    });

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    profile.severityAliases.insert(
        QStringLiteral("SEVERE"),
        RecordSeverity::Critical
        );

    profile.timestampRules.clear();

    profile.timestampRules.append({
        TimestampRuleType::Iso8601,
        QString()
    });

    profile.timestampRules.append({
        TimestampRuleType::QtFormat,
        QStringLiteral(
            "yyyy-MM-dd HH:mm:ss.zzz"
            )
    });

    profile.preserveUnmappedFields = false;

    return profile;
}

void ImportProfileSerializationTests::
    serializationProducesVersionedJson()
{
    const ImportProfile profile =
        populatedProfile();

    const QByteArray json =
        ImportProfileSerializer().serialize(
            profile
            );

    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json,
            &error
            );

    QCOMPARE(
        error.error,
        QJsonParseError::NoError
        );

    QVERIFY(document.isObject());

    const QJsonObject root =
        document.object();

    QCOMPARE(
        root.value(
                QStringLiteral("schemaVersion")
                ).toInt(),
        ImportProfile::CurrentSchemaVersion
        );

    QCOMPARE(
        root.value(
                QStringLiteral("name")
                ).toString(),
        QStringLiteral(
            "Application JSON Lines"
            )
        );

    QCOMPARE(
        root.value(
                QStringLiteral("importerId")
                ).toString(),
        QStringLiteral("json-lines")
        );

    QVERIFY(
        root.value(
                QStringLiteral(
                    "canonicalFields"
                    )
                ).isObject()
        );

    QVERIFY(
        root.value(
                QStringLiteral("customFields")
                ).isArray()
        );

    QVERIFY(
        root.value(
                QStringLiteral(
                    "severityAliases"
                    )
                ).isObject()
        );

    QVERIFY(
        root.value(
                QStringLiteral(
                    "timestampRules"
                    )
                ).isArray()
        );

    QCOMPARE(
        root.value(
                QStringLiteral(
                    "preserveUnmappedFields"
                    )
                ).toBool(),
        false
        );
}

void ImportProfileSerializationTests::
    profileRoundTripsThroughJson()
{
    const ImportProfile original =
        populatedProfile();

    const ImportProfileSerializer serializer;

    const QByteArray json =
        serializer.serialize(original);

    const ProfileDeserializationResult
        result =
        serializer.deserialize(json);

    QVERIFY(result.isSuccess());
    QVERIFY(result.profile.has_value());

    const ImportProfile &restored =
        *result.profile;

    QCOMPARE(
        restored.schemaVersion,
        original.schemaVersion
        );

    QCOMPARE(
        restored.name,
        original.name
        );

    QCOMPARE(
        restored.importerId,
        original.importerId
        );

    QCOMPARE(
        restored.canonicalFields.timestampPath,
        original.canonicalFields.timestampPath
        );

    QCOMPARE(
        restored.canonicalFields.severityPath,
        original.canonicalFields.severityPath
        );

    QCOMPARE(
        restored.canonicalFields.subsystemPath,
        original.canonicalFields.subsystemPath
        );

    QCOMPARE(
        restored.canonicalFields.eventCodePath,
        original.canonicalFields.eventCodePath
        );

    QCOMPARE(
        restored.canonicalFields.entityIdPath,
        original.canonicalFields.entityIdPath
        );

    QCOMPARE(
        restored.canonicalFields.messagePath,
        original.canonicalFields.messagePath
        );

    QCOMPARE(
        restored.customFields.size(),
        original.customFields.size()
        );

    for (qsizetype index = 0;
         index < original.customFields.size();
         ++index) {
        QCOMPARE(
            restored.customFields.at(index).name,
            original.customFields.at(index).name
            );

        QCOMPARE(
            restored.customFields.at(index)
                .sourcePath,
            original.customFields.at(index)
                .sourcePath
            );
    }

    QCOMPARE(
        restored.severityAliases,
        original.severityAliases
        );

    QCOMPARE(
        restored.timestampRules.size(),
        original.timestampRules.size()
        );

    for (qsizetype index = 0;
         index < original.timestampRules.size();
         ++index) {
        QCOMPARE(
            restored.timestampRules.at(index).type,
            original.timestampRules.at(index).type
            );

        QCOMPARE(
            restored.timestampRules.at(index).format,
            original.timestampRules.at(index).format
            );
    }

    QCOMPARE(
        restored.preserveUnmappedFields,
        original.preserveUnmappedFields
        );

    QCOMPARE(
        restored.recordPath,
        original.recordPath
        );

    QCOMPARE(
        restored.regexPattern,
        original.regexPattern
        );
}

void ImportProfileSerializationTests::
    malformedJsonIsRejected()
{
    const auto result =
        ImportProfileSerializer().deserialize(
            QByteArray(
                R"({"schemaVersion":1,)"
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral("INVALID_JSON")
        );
}

void ImportProfileSerializationTests::
    nonObjectRootIsRejected()
{
    const auto result =
        ImportProfileSerializer().deserialize(
            QByteArray(R"([])")
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "PROFILE_ROOT_NOT_OBJECT"
            )
        );
}

void ImportProfileSerializationTests::
    missingRequiredFieldIsRejected()
{
    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        1
        );

    const auto result =
        ImportProfileSerializer().deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_PROFILE_NAME"
            )
        );
}

void ImportProfileSerializationTests::
    invalidSeverityTargetIsRejected()
{
    ImportProfile profile =
        populatedProfile();

    const QByteArray json =
        ImportProfileSerializer().serialize(
            profile
            );

    QJsonDocument document =
        QJsonDocument::fromJson(json);

    QJsonObject root =
        document.object();

    QJsonObject aliases =
        root.value(
                QStringLiteral(
                    "severityAliases"
                    )
                ).toObject();

    aliases.insert(
        QStringLiteral("NOTICE"),
        QStringLiteral("BANANA")
        );

    root.insert(
        QStringLiteral(
            "severityAliases"
            ),
        aliases
        );

    const auto result =
        ImportProfileSerializer().deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SEVERITY_ALIAS_TARGET"
            )
        );
}

void ImportProfileSerializationTests::
    unknownTimestampRuleTypeIsRejected()
{
    ImportProfile profile =
        populatedProfile();

    QJsonDocument document =
        QJsonDocument::fromJson(
            ImportProfileSerializer().serialize(
                profile
                )
            );

    QJsonObject root =
        document.object();

    QJsonArray rules;

    rules.append(
        QJsonObject {
            {
                QStringLiteral("type"),
                QStringLiteral("unix-millis")
            }
        }
        );

    root.insert(
        QStringLiteral("timestampRules"),
        rules
        );

    const auto result =
        ImportProfileSerializer().deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNKNOWN_TIMESTAMP_RULE_TYPE"
            )
        );
}

void ImportProfileSerializationTests::
    unsupportedSchemaVersionCanBeDeserialized()
{
    ImportProfile profile =
        populatedProfile();

    profile.schemaVersion =
        ImportProfile::CurrentSchemaVersion + 1;

    const ImportProfileSerializer serializer;

    const auto result =
        serializer.deserialize(
            serializer.serialize(profile)
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        result.profile->schemaVersion,
        ImportProfile::CurrentSchemaVersion + 1
        );
}

void ImportProfileSerializationTests::
    recordPathRoundTripsThroughJson()
{
    ImportProfile profile =
        populatedProfile();

    profile.importerId =
        QStringLiteral(
            "structured-json"
            );

    profile.recordPath =
        QStringLiteral(
            "payload.events"
            );

    const ImportProfileSerializer serializer;

    const QByteArray json =
        serializer.serialize(profile);

    const ProfileDeserializationResult result =
        serializer.deserialize(json);

    QVERIFY(result.isSuccess());
    QVERIFY(result.profile.has_value());

    QCOMPARE(
        result.profile->recordPath,
        QStringLiteral(
            "payload.events"
            )
        );
}

void ImportProfileSerializationTests::
    missingRecordPathDefaultsToEmpty()
{
    ImportProfile profile =
        populatedProfile();

    const ImportProfileSerializer serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(profile)
            );

    QJsonObject root =
        document.object();

    root.remove(
        QStringLiteral(
            "recordPath"
            )
        );

    const ProfileDeserializationResult result =
        serializer.deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.profile.has_value());

    QVERIFY(
        result.profile
            ->recordPath
            .isEmpty()
        );
}

void ImportProfileSerializationTests::
    nonStringRecordPathIsRejected()
{
    ImportProfile profile =
        populatedProfile();

    const ImportProfileSerializer serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(profile)
            );

    QJsonObject root =
        document.object();

    root.insert(
        QStringLiteral(
            "recordPath"
            ),
        42
        );

    const ProfileDeserializationResult result =
        serializer.deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_RECORD_PATH"
            )
        );
}

void ImportProfileSerializationTests::
    regexPatternRoundTripsThroughJson()
{
    ImportProfile profile =
        populatedProfile();

    profile.importerId =
        QStringLiteral("regex-text");

    profile.regexPattern =
        QStringLiteral(
            R"(^(?<timestamp>\S+)\s+(?<message>.*)$)"
            );

    const ImportProfileSerializer serializer;

    const auto result =
        serializer.deserialize(
            serializer.serialize(profile)
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.profile.has_value());

    QCOMPARE(
        result.profile->regexPattern,
        profile.regexPattern
        );
}

void ImportProfileSerializationTests::
    missingRegexPatternDefaultsToEmpty()
{
    ImportProfile profile =
        populatedProfile();

    const ImportProfileSerializer serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(profile)
            );

    QJsonObject root =
        document.object();

    root.remove(
        QStringLiteral("regexPattern")
        );

    const auto result =
        serializer.deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.profile.has_value());

    QVERIFY(
        result.profile
            ->regexPattern
            .isEmpty()
        );
}

void ImportProfileSerializationTests::
    nonStringRegexPatternIsRejected()
{
    ImportProfile profile =
        populatedProfile();

    const ImportProfileSerializer serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(profile)
            );

    QJsonObject root =
        document.object();

    root.insert(
        QStringLiteral("regexPattern"),
        42
        );

    const auto result =
        serializer.deserialize(
            QJsonDocument(root).toJson()
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_REGEX_PATTERN_TYPE"
            )
        );
}

QTEST_MAIN(ImportProfileSerializationTests)

#include "ImportProfileSerializationTests.moc"