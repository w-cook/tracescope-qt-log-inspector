#include "InvestigationSessionBackingPersistenceSerialization.h"

#include <cmath>
#include <limits>

#include <QJsonDocument>
#include <QJsonValue>

#include "../importing/ImportProfileSerialization.h"

namespace
{
InvestigationSessionBackingPersistenceDeserializationResult
failure(
    const QString &code,
    const QString &message
    )
{
    InvestigationSessionBackingPersistenceDeserializationResult
        result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

QString backingModeToString(
    PersistedInvestigationSessionBackingMode mode
    )
{
    switch (mode) {
    case PersistedInvestigationSessionBackingMode::
        SourceBacked:
        return QStringLiteral("sourceBacked");

    case PersistedInvestigationSessionBackingMode::
        SnapshotBacked:
        return QStringLiteral("snapshotBacked");

    case PersistedInvestigationSessionBackingMode::
        Hybrid:
        return QStringLiteral("hybrid");
    }

    return QStringLiteral("sourceBacked");
}

std::optional<
    PersistedInvestigationSessionBackingMode>
backingModeFromString(
    const QString &value
    )
{
    if (value
        == QStringLiteral("sourceBacked")) {
        return PersistedInvestigationSessionBackingMode::
            SourceBacked;
    }

    if (value
        == QStringLiteral("snapshotBacked")) {
        return PersistedInvestigationSessionBackingMode::
            SnapshotBacked;
    }

    if (value
        == QStringLiteral("hybrid")) {
        return PersistedInvestigationSessionBackingMode::
            Hybrid;
    }

    return std::nullopt;
}

QString namingSchemeToString(
    RotatedSourceNamingScheme scheme
    )
{
    switch (scheme) {
    case RotatedSourceNamingScheme::Disabled:
        return QStringLiteral("disabled");

    case RotatedSourceNamingScheme::NumericSuffix:
        return QStringLiteral("numericSuffix");

    case RotatedSourceNamingScheme::
        NumericBeforeExtension:
        return QStringLiteral(
            "numericBeforeExtension"
            );

    case RotatedSourceNamingScheme::
        IsoDateTimeBeforeExtension:
        return QStringLiteral(
            "isoDateTimeBeforeExtension"
            );

    case RotatedSourceNamingScheme::CustomRegex:
        return QStringLiteral("customRegex");
    }

    return QStringLiteral("disabled");
}

std::optional<RotatedSourceNamingScheme>
namingSchemeFromString(
    const QString &value
    )
{
    if (value == QStringLiteral("disabled")) {
        return RotatedSourceNamingScheme::Disabled;
    }

    if (value
        == QStringLiteral("numericSuffix")) {
        return RotatedSourceNamingScheme::
            NumericSuffix;
    }

    if (value
        == QStringLiteral(
            "numericBeforeExtension"
            )) {
        return RotatedSourceNamingScheme::
            NumericBeforeExtension;
    }

    if (value
        == QStringLiteral(
            "isoDateTimeBeforeExtension"
            )) {
        return RotatedSourceNamingScheme::
            IsoDateTimeBeforeExtension;
    }

    if (value
        == QStringLiteral("customRegex")) {
        return RotatedSourceNamingScheme::
            CustomRegex;
    }

    return std::nullopt;
}

QString orderDirectionToString(
    RotatedSourceOrderDirection direction
    )
{
    switch (direction) {
    case RotatedSourceOrderDirection::Ascending:
        return QStringLiteral("ascending");

    case RotatedSourceOrderDirection::Descending:
        return QStringLiteral("descending");
    }

    return QStringLiteral("ascending");
}

std::optional<RotatedSourceOrderDirection>
orderDirectionFromString(
    const QString &value
    )
{
    if (value == QStringLiteral("ascending")) {
        return RotatedSourceOrderDirection::
            Ascending;
    }

    if (value == QStringLiteral("descending")) {
        return RotatedSourceOrderDirection::
            Descending;
    }

    return std::nullopt;
}

QString orderValueTypeToString(
    RotatedSourceOrderValueType valueType
    )
{
    switch (valueType) {
    case RotatedSourceOrderValueType::Numeric:
        return QStringLiteral("numeric");

    case RotatedSourceOrderValueType::
        Lexicographic:
        return QStringLiteral("lexicographic");

    case RotatedSourceOrderValueType::DateTime:
        return QStringLiteral("dateTime");
    }

    return QStringLiteral("lexicographic");
}

std::optional<RotatedSourceOrderValueType>
orderValueTypeFromString(
    const QString &value
    )
{
    if (value == QStringLiteral("numeric")) {
        return RotatedSourceOrderValueType::
            Numeric;
    }

    if (value
        == QStringLiteral("lexicographic")) {
        return RotatedSourceOrderValueType::
            Lexicographic;
    }

    if (value == QStringLiteral("dateTime")) {
        return RotatedSourceOrderValueType::
            DateTime;
    }

    return std::nullopt;
}

QJsonObject sourceFamilyToJson(
    const SourceFamilyConfiguration &configuration
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("includeRotatedSources"),
        configuration.includeRotatedSources
        );

    const RotatedSourceRule &rule =
        configuration.rotationRule;

    QJsonObject ruleObject;

    ruleObject.insert(
        QStringLiteral("namingScheme"),
        namingSchemeToString(
            rule.namingScheme
            )
        );

    ruleObject.insert(
        QStringLiteral("numericOrderDirection"),
        orderDirectionToString(
            rule.numericOrderDirection
            )
        );

    ruleObject.insert(
        QStringLiteral(
            "customRegularExpression"
            ),
        rule.customRegularExpression
        );

    ruleObject.insert(
        QStringLiteral(
            "customOrderCaptureGroup"
            ),
        rule.customOrderCaptureGroup
        );

    ruleObject.insert(
        QStringLiteral("customOrderValueType"),
        orderValueTypeToString(
            rule.customOrderValueType
            )
        );

    ruleObject.insert(
        QStringLiteral("customOrderDirection"),
        orderDirectionToString(
            rule.customOrderDirection
            )
        );

    ruleObject.insert(
        QStringLiteral("customDateTimeFormat"),
        rule.customDateTimeFormat
        );

    object.insert(
        QStringLiteral("rotationRule"),
        ruleObject
        );

    return object;
}

bool sourceFamilyFromJson(
    const QJsonObject &object,
    SourceFamilyConfiguration &configuration
    )
{
    const QJsonValue includeValue =
        object.value(
            QStringLiteral(
                "includeRotatedSources"
                )
            );

    const QJsonValue ruleValue =
        object.value(
            QStringLiteral("rotationRule")
            );

    if (!includeValue.isBool()
        || !ruleValue.isObject()) {
        return false;
    }

    const QJsonObject ruleObject =
        ruleValue.toObject();

    const QJsonValue namingValue =
        ruleObject.value(
            QStringLiteral("namingScheme")
            );

    const QJsonValue numericDirectionValue =
        ruleObject.value(
            QStringLiteral(
                "numericOrderDirection"
                )
            );

    const QJsonValue regularExpressionValue =
        ruleObject.value(
            QStringLiteral(
                "customRegularExpression"
                )
            );

    const QJsonValue captureGroupValue =
        ruleObject.value(
            QStringLiteral(
                "customOrderCaptureGroup"
                )
            );

    const QJsonValue orderTypeValue =
        ruleObject.value(
            QStringLiteral(
                "customOrderValueType"
                )
            );

    const QJsonValue orderDirectionValue =
        ruleObject.value(
            QStringLiteral(
                "customOrderDirection"
                )
            );

    const QJsonValue dateTimeFormatValue =
        ruleObject.value(
            QStringLiteral(
                "customDateTimeFormat"
                )
            );

    if (!namingValue.isString()
        || !numericDirectionValue.isString()
        || !regularExpressionValue.isString()
        || !captureGroupValue.isDouble()
        || !orderTypeValue.isString()
        || !orderDirectionValue.isString()
        || !dateTimeFormatValue.isString()) {
        return false;
    }

    const double captureGroupNumber =
        captureGroupValue.toDouble();

    if (captureGroupNumber < 1
        || captureGroupNumber
               > std::numeric_limits<int>::max()
        || std::floor(captureGroupNumber)
               != captureGroupNumber) {
        return false;
    }

    const auto namingScheme =
        namingSchemeFromString(
            namingValue.toString()
            );

    const auto numericDirection =
        orderDirectionFromString(
            numericDirectionValue.toString()
            );

    const auto orderType =
        orderValueTypeFromString(
            orderTypeValue.toString()
            );

    const auto orderDirection =
        orderDirectionFromString(
            orderDirectionValue.toString()
            );

    if (!namingScheme.has_value()
        || !numericDirection.has_value()
        || !orderType.has_value()
        || !orderDirection.has_value()) {
        return false;
    }

    configuration.includeRotatedSources =
        includeValue.toBool();

    configuration.rotationRule.namingScheme =
        *namingScheme;

    configuration
        .rotationRule
        .numericOrderDirection =
        *numericDirection;

    configuration
        .rotationRule
        .customRegularExpression =
        regularExpressionValue.toString();

    configuration
        .rotationRule
        .customOrderCaptureGroup =
        static_cast<int>(
            captureGroupNumber
            );

    configuration
        .rotationRule
        .customOrderValueType =
        *orderType;

    configuration
        .rotationRule
        .customOrderDirection =
        *orderDirection;

    configuration
        .rotationRule
        .customDateTimeFormat =
        dateTimeFormatValue.toString();

    /*
     * Physical members are rediscovered when the
     * workspace is opened. They are never persisted.
     */
    configuration.rotatedSourcePaths.clear();

    return true;
}

QJsonObject sourceIdentityToJson(
    const SourcePhysicalIdentity &identity
    )
{
    QJsonObject object;

    if (identity.birthTime.isValid()) {
        object.insert(
            QStringLiteral("birthTime"),
            identity.birthTime.toString(
                Qt::ISODateWithMs
                )
            );
    } else {
        object.insert(
            QStringLiteral("birthTime"),
            QJsonValue(QJsonValue::Null)
            );
    }

    object.insert(
        QStringLiteral("fingerprintLength"),
        QString::number(
            identity.fingerprintLength
            )
        );

    object.insert(
        QStringLiteral("prefixFingerprint"),
        QString::fromLatin1(
            identity
                .prefixFingerprint
                .toHex()
            )
        );

    object.insert(
        QStringLiteral("observedSizeBytes"),
        QString::number(
            identity.observedSizeBytes
            )
        );

    return object;
}

bool isHexText(
    const QByteArray &value
    )
{
    for (const char character : value) {
        const bool digit =
            character >= '0'
            && character <= '9';

        const bool lower =
            character >= 'a'
            && character <= 'f';

        const bool upper =
            character >= 'A'
            && character <= 'F';

        if (!digit && !lower && !upper) {
            return false;
        }
    }

    return true;
}

bool sourceIdentityFromJson(
    const QJsonObject &object,
    SourcePhysicalIdentity &identity
    )
{
    const QJsonValue birthTimeValue =
        object.value(
            QStringLiteral("birthTime")
            );

    if (!birthTimeValue.isNull()
        && !birthTimeValue.isString()) {
        return false;
    }

    if (birthTimeValue.isString()) {
        identity.birthTime =
            QDateTime::fromString(
                birthTimeValue.toString(),
                Qt::ISODateWithMs
                );

        if (!identity.birthTime.isValid()) {
            return false;
        }
    }

    const QJsonValue fingerprintLengthValue =
        object.value(
            QStringLiteral(
                "fingerprintLength"
                )
            );

    const QJsonValue fingerprintValue =
        object.value(
            QStringLiteral(
                "prefixFingerprint"
                )
            );

    const QJsonValue observedSizeValue =
        object.value(
            QStringLiteral(
                "observedSizeBytes"
                )
            );

    if (!fingerprintLengthValue.isString()
        || !fingerprintValue.isString()
        || !observedSizeValue.isString()) {
        return false;
    }

    bool fingerprintLengthOk = false;
    bool observedSizeOk = false;

    const qint64 fingerprintLength =
        fingerprintLengthValue
            .toString()
            .toLongLong(
                &fingerprintLengthOk
                );

    const qint64 observedSize =
        observedSizeValue
            .toString()
            .toLongLong(
                &observedSizeOk
                );

    if (!fingerprintLengthOk
        || !observedSizeOk
        || fingerprintLength < 0
        || observedSize < 0
        || fingerprintLength
               > observedSize) {
        return false;
    }

    const QByteArray fingerprintHex =
        fingerprintValue
            .toString()
            .toLatin1();

    if (!isHexText(fingerprintHex)
        || fingerprintHex.size() % 2 != 0) {
        return false;
    }

    const QByteArray fingerprint =
        QByteArray::fromHex(
            fingerprintHex
            );

    /*
     * Current physical identity uses SHA-256 whenever
     * a non-empty prefix can be fingerprinted.
     */
    if (fingerprintLength == 0) {
        if (!fingerprint.isEmpty()) {
            return false;
        }
    } else if (fingerprint.size() != 32) {
        return false;
    }

    identity.fingerprintLength =
        fingerprintLength;

    identity.prefixFingerprint =
        fingerprint;

    identity.observedSizeBytes =
        observedSize;

    return identity.isValid();
}

QJsonObject externalSourceToJson(
    const PersistedInvestigationExternalSourceBinding
        &binding
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("sourcePath"),
        binding.sourcePath
        );

    const QString logicalSourceKey =
        binding.logicalSourceKey.trimmed().isEmpty()
            ? binding.sourcePath
            : binding.logicalSourceKey;

    object.insert(
        QStringLiteral("logicalSourceKey"),
        logicalSourceKey
        );

    /*
     * Keep quint64 exact instead of relying on the
     * limited integer precision of a JSON double.
     */
    object.insert(
        QStringLiteral("sourceGeneration"),
        QString::number(
            binding.sourceGeneration
            )
        );

    object.insert(
        QStringLiteral(
            "sourceFamilyConfiguration"
            ),
        sourceFamilyToJson(
            binding.sourceFamilyConfiguration
            )
        );

    if (binding.sourceIdentity.has_value()) {
        object.insert(
            QStringLiteral("sourceIdentity"),
            sourceIdentityToJson(
                *binding.sourceIdentity
                )
            );
    }

    return object;
}

bool externalSourceFromJson(
    const QJsonObject &object,
    PersistedInvestigationExternalSourceBinding
        &binding
    )
{
    const QJsonValue sourcePathValue =
        object.value(
            QStringLiteral("sourcePath")
            );

    const QJsonValue generationValue =
        object.value(
            QStringLiteral("sourceGeneration")
            );

    const QJsonValue familyValue =
        object.value(
            QStringLiteral(
                "sourceFamilyConfiguration"
                )
            );

    if (!sourcePathValue.isString()
        || sourcePathValue
               .toString()
               .trimmed()
               .isEmpty()
        || !generationValue.isString()
        || !familyValue.isObject()) {
        return false;
    }

    bool generationOk = false;

    const quint64 generation =
        generationValue
            .toString()
            .toULongLong(
                &generationOk
                );

    if (!generationOk) {
        return false;
    }

    SourceFamilyConfiguration family;

    if (!sourceFamilyFromJson(
            familyValue.toObject(),
            family
            )) {
        return false;
    }

    binding.sourcePath =
        sourcePathValue.toString();

    const QJsonValue logicalSourceKeyValue =
        object.value(
            QStringLiteral(
                "logicalSourceKey"
                )
            );

    if (logicalSourceKeyValue.isUndefined()
        || logicalSourceKeyValue.isNull()) {
        /*
         * Backward compatibility for existing persisted
         * bindings created before logical/physical source
         * identity were separated.
         */
        binding.logicalSourceKey =
            binding.sourcePath;
    } else {
        if (!logicalSourceKeyValue.isString()
            || logicalSourceKeyValue
                   .toString()
                   .trimmed()
                   .isEmpty()) {
            return false;
        }

        binding.logicalSourceKey =
            logicalSourceKeyValue.toString();
    }

    binding.sourceGeneration =
        generation;

    binding.sourceFamilyConfiguration =
        std::move(family);

    const QJsonValue identityValue =
        object.value(
            QStringLiteral("sourceIdentity")
            );

    if (!identityValue.isUndefined()) {
        if (!identityValue.isObject()) {
            return false;
        }

        SourcePhysicalIdentity identity;

        if (!sourceIdentityFromJson(
                identityValue.toObject(),
                identity
                )) {
            return false;
        }

        binding.sourceIdentity =
            std::move(identity);
    }

    return true;
}

bool validateBacking(
    const PersistedInvestigationSessionBacking
        &backing
    )
{
    const bool hasSnapshot =
        backing.snapshotReference.has_value()
        && !backing
                .snapshotReference
                ->trimmed()
                .isEmpty();

    const bool hasExternal =
        backing.externalSourceBinding.has_value()
        && !backing
                .externalSourceBinding
                ->sourcePath
                .trimmed()
                .isEmpty();

    const bool hasSourceProfile =
        backing.sourceImportProfile.has_value();

    switch (backing.mode) {
    case PersistedInvestigationSessionBackingMode::
        SourceBacked:
        return hasExternal
               && hasSourceProfile;

    case PersistedInvestigationSessionBackingMode::
        SnapshotBacked:
        /*
         * SnapshotBacked requires durable snapshot
         * evidence and must not carry an active
         * SourceBacked import profile.
         *
         * An external-source binding is optional here.
         * When present, it is a dormant reconnect hint,
         * not an active source dependency.
         */
        return hasSnapshot
               && !hasSourceProfile;

    case PersistedInvestigationSessionBackingMode::
        Hybrid:
        return hasSnapshot
               && hasExternal
               && !hasSourceProfile;
    }

    return false;
}
}

QJsonObject
    InvestigationSessionBackingPersistenceSerializer::
    serialize(
        const PersistedInvestigationSessionBacking
            &backing
        ) const
{
    QJsonObject object;

    object.insert(
        QStringLiteral("mode"),
        backingModeToString(
            backing.mode
            )
        );

    if (backing.snapshotReference.has_value()) {
        object.insert(
            QStringLiteral("snapshotReference"),
            *backing.snapshotReference
            );
    }

    if (backing
            .externalSourceBinding
            .has_value()) {
        object.insert(
            QStringLiteral(
                "externalSourceBinding"
                ),
            externalSourceToJson(
                *backing.externalSourceBinding
                )
            );
    }

    if (backing.sourceImportProfile.has_value()) {
        const ImportProfileSerializer
            profileSerializer;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                profileSerializer.serialize(
                    *backing.sourceImportProfile
                    )
                );

        object.insert(
            QStringLiteral("sourceImportProfile"),
            document.object()
            );
    }

    return object;
}

InvestigationSessionBackingPersistenceDeserializationResult
    InvestigationSessionBackingPersistenceSerializer::
    deserialize(
        const QJsonObject &object
        ) const
{
    const QJsonValue modeValue =
        object.value(
            QStringLiteral("mode")
            );

    if (!modeValue.isString()) {
        return failure(
            QStringLiteral("INVALID_BACKING_MODE"),
            QStringLiteral(
                "The persisted investigation backing "
                "mode must be a string."
                )
            );
    }

    const auto mode =
        backingModeFromString(
            modeValue.toString()
            );

    if (!mode.has_value()) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_BACKING_MODE"
                ),
            QStringLiteral(
                "The persisted investigation backing "
                "mode is not supported."
                )
            );
    }

    PersistedInvestigationSessionBacking
        backing;

    backing.mode = *mode;

    const QJsonValue snapshotValue =
        object.value(
            QStringLiteral("snapshotReference")
            );

    if (!snapshotValue.isUndefined()) {
        if (!snapshotValue.isString()
            || snapshotValue
                   .toString()
                   .trimmed()
                   .isEmpty()) {
            return failure(
                QStringLiteral(
                    "INVALID_SNAPSHOT_REFERENCE"
                    ),
                QStringLiteral(
                    "The persisted snapshot reference "
                    "must be a non-empty string."
                    )
                );
        }

        backing.snapshotReference =
            snapshotValue.toString();
    }

    const QJsonValue externalValue =
        object.value(
            QStringLiteral(
                "externalSourceBinding"
                )
            );

    if (!externalValue.isUndefined()) {
        if (!externalValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_EXTERNAL_SOURCE_BINDING"
                    ),
                QStringLiteral(
                    "The persisted external source "
                    "binding must be an object."
                    )
                );
        }

        PersistedInvestigationExternalSourceBinding
            external;

        if (!externalSourceFromJson(
                externalValue.toObject(),
                external
                )) {
            return failure(
                QStringLiteral(
                    "INVALID_EXTERNAL_SOURCE_BINDING"
                    ),
                QStringLiteral(
                    "The persisted external source "
                    "binding is invalid."
                    )
                );
        }

        backing.externalSourceBinding =
            std::move(external);
    }

    const QJsonValue profileValue =
        object.value(
            QStringLiteral(
                "sourceImportProfile"
                )
            );

    if (!profileValue.isUndefined()) {
        if (!profileValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_SOURCE_IMPORT_PROFILE"
                    ),
                QStringLiteral(
                    "The persisted source import "
                    "profile must be an object."
                    )
                );
        }

        const ImportProfileSerializer
            profileSerializer;

        const ProfileDeserializationResult
            profileResult =
            profileSerializer.deserialize(
                QJsonDocument(
                    profileValue.toObject()
                    ).toJson(
                        QJsonDocument::Compact
                        )
                );

        if (!profileResult.isSuccess()) {
            return failure(
                QStringLiteral(
                    "INVALID_SOURCE_IMPORT_PROFILE"
                    ),
                profileResult.errorMessage
                );
        }

        backing.sourceImportProfile =
            *profileResult.profile;
    }

    if (!validateBacking(backing)) {
        return failure(
            QStringLiteral(
                "INVALID_BACKING_CONFIGURATION"
                ),
            QStringLiteral(
                "The persisted investigation backing "
                "contains an invalid combination of "
                "snapshot, external-source, and import-"
                "profile state."
                )
            );
    }

    InvestigationSessionBackingPersistenceDeserializationResult
        result;

    result.backing =
        std::move(backing);

    return result;
}