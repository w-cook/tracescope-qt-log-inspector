#include <QtTest/QtTest>

#include "../src/importing/ImportProfileValidator.h"

class ImportProfileValidatorTests : public QObject
{
    Q_OBJECT

private slots:
    void validProfilePassesValidation();
    void unsupportedSchemaVersionFailsValidation();
    void profileNameIsRequired();
    void importerIdIsRequired();
    void emptyCanonicalPathIsAllowed();
    void malformedCanonicalPathFailsValidation();
    void customFieldRequiresNameAndPath();
    void duplicateCustomFieldNamesFailValidation();
    void canonicalCustomFieldNameFailsValidation();
    void duplicateSeverityAliasesFailValidation();
    void timestampMappingRequiresRule();
    void qtTimestampRuleRequiresFormat();
    void isoTimestampFormatProducesWarning();
    void emptyRecordPathIsAllowed();
    void validRecordPathPassesValidation();
    void malformedRecordPathFailsValidation();
};

ImportProfile validProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Default JSON Lines");

    return profile;
}

void ImportProfileValidatorTests::
    validProfilePassesValidation()
{
    const ImportProfileValidator validator;

    const ProfileValidationResult result =
        validator.validate(validProfile());

    QVERIFY(result.isValid());
    QVERIFY(result.issues.isEmpty());
}

void ImportProfileValidatorTests::
    unsupportedSchemaVersionFailsValidation()
{
    ImportProfile profile = validProfile();

    profile.schemaVersion =
        ImportProfile::CurrentSchemaVersion + 1;

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());

    QCOMPARE(
        result.issues.first().code,
        QStringLiteral(
            "UNSUPPORTED_SCHEMA_VERSION"
            )
        );
}

void ImportProfileValidatorTests::
    profileNameIsRequired()
{
    ImportProfile profile = validProfile();

    profile.name.clear();

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    importerIdIsRequired()
{
    ImportProfile profile = validProfile();

    profile.importerId.clear();

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    emptyCanonicalPathIsAllowed()
{
    ImportProfile profile = validProfile();

    profile.canonicalFields.eventCodePath.clear();

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(result.isValid());
}

void ImportProfileValidatorTests::
    malformedCanonicalPathFailsValidation()
{
    ImportProfile profile = validProfile();

    profile.canonicalFields.messagePath =
        QStringLiteral("payload..message");

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    customFieldRequiresNameAndPath()
{
    ImportProfile profile = validProfile();

    profile.customFields.append({
        QString(),
        QString()
    });

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
    QCOMPARE(result.issues.size(), 2);
}

void ImportProfileValidatorTests::
    duplicateCustomFieldNamesFailValidation()
{
    ImportProfile profile = validProfile();

    profile.customFields.append({
        QStringLiteral("Request ID"),
        QStringLiteral("request.id")
    });

    profile.customFields.append({
        QStringLiteral("request id"),
        QStringLiteral("context.requestId")
    });

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    canonicalCustomFieldNameFailsValidation()
{
    ImportProfile profile = validProfile();

    profile.customFields.append({
        QStringLiteral("Message"),
        QStringLiteral("payload.text")
    });

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    duplicateSeverityAliasesFailValidation()
{
    ImportProfile profile = validProfile();

    profile.severityAliases.insert(
        QStringLiteral("NOTICE"),
        RecordSeverity::Info
        );

    profile.severityAliases.insert(
        QStringLiteral("notice"),
        RecordSeverity::Warning
        );

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    timestampMappingRequiresRule()
{
    ImportProfile profile = validProfile();

    profile.timestampRules.clear();

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    qtTimestampRuleRequiresFormat()
{
    ImportProfile profile = validProfile();

    profile.timestampRules = {
        TimestampRule {
            TimestampRuleType::QtFormat,
            QString()
        }
    };

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(!result.isValid());
}

void ImportProfileValidatorTests::
    isoTimestampFormatProducesWarning()
{
    ImportProfile profile = validProfile();

    profile.timestampRules.first().format =
        QStringLiteral("ignored");

    const ProfileValidationResult result =
        ImportProfileValidator().validate(profile);

    QVERIFY(result.isValid());
    QVERIFY(result.hasWarnings());

    QCOMPARE(result.issues.size(), 1);

    QCOMPARE(
        result.issues.first().code,
        QStringLiteral(
            "ISO_TIMESTAMP_FORMAT_IGNORED"
            )
        );
}

void ImportProfileValidatorTests::
    emptyRecordPathIsAllowed()
{
    ImportProfile profile =
        validProfile();

    profile.recordPath.clear();

    const ProfileValidationResult result =
        ImportProfileValidator()
            .validate(profile);

    QVERIFY(result.isValid());
}

void ImportProfileValidatorTests::
    validRecordPathPassesValidation()
{
    ImportProfile profile =
        validProfile();

    profile.recordPath =
        QStringLiteral(
            "payload.events"
            );

    const ProfileValidationResult result =
        ImportProfileValidator()
            .validate(profile);

    QVERIFY(result.isValid());
}

void ImportProfileValidatorTests::
    malformedRecordPathFailsValidation()
{
    ImportProfile profile =
        validProfile();

    profile.recordPath =
        QStringLiteral(
            "payload..events"
            );

    const ProfileValidationResult result =
        ImportProfileValidator()
            .validate(profile);

    QVERIFY(!result.isValid());

    bool foundIssue = false;

    for (const ProfileValidationIssue &issue
         : result.issues) {
        if (issue.code ==
            QStringLiteral(
                "INVALID_RECORD_PATH"
                )) {
            foundIssue = true;
            break;
        }
    }

    QVERIFY(foundIssue);
}

QTEST_MAIN(ImportProfileValidatorTests)

#include "ImportProfileValidatorTests.moc"