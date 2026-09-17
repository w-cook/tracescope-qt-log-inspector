#include "InvestigationSessionSnapshotSerialization.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "InvestigationRecordSerialization.h"

#include "../importing/ImportProfileSerialization.h"

namespace
{
InvestigationSessionSnapshotDeserializationResult
failure(
    const QString &code,
    const QString &message
    )
{
    InvestigationSessionSnapshotDeserializationResult
        result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

QString sourceFidelityToJson(
    InvestigationSnapshotSourceFidelity fidelity
    )
{
    switch (fidelity) {
    case InvestigationSnapshotSourceFidelity::
        NormalizedOnly:
        return QStringLiteral(
            "normalizedOnly"
            );

    case InvestigationSnapshotSourceFidelity::
        CompleteSource:
        return QStringLiteral(
            "completeSource"
            );
    }

    return QStringLiteral(
        "normalizedOnly"
        );
}

std::optional<InvestigationSnapshotSourceFidelity>
sourceFidelityFromJson(
    const QString &value
    )
{
    if (value
        == QStringLiteral(
            "normalizedOnly"
            )) {
        return InvestigationSnapshotSourceFidelity::
            NormalizedOnly;
    }

    if (value
        == QStringLiteral(
            "completeSource"
            )) {
        return InvestigationSnapshotSourceFidelity::
            CompleteSource;
    }

    return std::nullopt;
}

QString diagnosticSeverityToJson(
    ImportDiagnosticSeverity severity
    )
{
    switch (severity) {
    case ImportDiagnosticSeverity::Information:
        return QStringLiteral("information");

    case ImportDiagnosticSeverity::Warning:
        return QStringLiteral("warning");

    case ImportDiagnosticSeverity::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("information");
}

std::optional<ImportDiagnosticSeverity>
diagnosticSeverityFromJson(
    const QString &value
    )
{
    if (value
        == QStringLiteral("information")) {
        return ImportDiagnosticSeverity::
            Information;
    }

    if (value
        == QStringLiteral("warning")) {
        return ImportDiagnosticSeverity::
            Warning;
    }

    if (value
        == QStringLiteral("error")) {
        return ImportDiagnosticSeverity::
            Error;
    }

    return std::nullopt;
}

bool readInteger(
    const QJsonValue &value,
    qint64 &result
    )
{
    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number
               < static_cast<double>(
                   std::numeric_limits<qint64>::min()
                   )
        || number
               > static_cast<double>(
                   std::numeric_limits<qint64>::max()
                   )) {
        return false;
    }

    result =
        static_cast<qint64>(number);

    return true;
}

QString namingSchemeToJson(
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
namingSchemeFromJson(
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

QString orderDirectionToJson(
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
orderDirectionFromJson(
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

QString orderValueTypeToJson(
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
orderValueTypeFromJson(
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
        namingSchemeToJson(
            rule.namingScheme
            )
        );

    ruleObject.insert(
        QStringLiteral("numericOrderDirection"),
        orderDirectionToJson(
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
        orderValueTypeToJson(
            rule.customOrderValueType
            )
        );

    ruleObject.insert(
        QStringLiteral("customOrderDirection"),
        orderDirectionToJson(
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

    /*
     * rotatedSourcePaths is deliberately omitted.
     * Those paths are discovery results and are
     * rediscovered from the durable rule.
     */
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

    if (!std::isfinite(captureGroupNumber)
        || captureGroupNumber < 1
        || captureGroupNumber
               > std::numeric_limits<int>::max()
        || std::floor(captureGroupNumber)
               != captureGroupNumber) {
        return false;
    }

    const auto namingScheme =
        namingSchemeFromJson(
            namingValue.toString()
            );

    const auto numericDirection =
        orderDirectionFromJson(
            numericDirectionValue.toString()
            );

    const auto orderType =
        orderValueTypeFromJson(
            orderTypeValue.toString()
            );

    const auto orderDirection =
        orderDirectionFromJson(
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

QJsonObject sourceContinuityToJson(
    const InvestigationSnapshotSourceContinuity
        &continuity
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("sourcePath"),
        continuity.sourcePath
        );

    const QString logicalSourceKey =
        continuity.logicalSourceKey
                .trimmed()
                .isEmpty()
            ? continuity.sourcePath
            : continuity.logicalSourceKey;

    object.insert(
        QStringLiteral("logicalSourceKey"),
        logicalSourceKey
        );

    object.insert(
        QStringLiteral("sourceGeneration"),
        QString::number(
            continuity.sourceGeneration
            )
        );

    object.insert(
        QStringLiteral(
            "sourceFamilyConfiguration"
            ),
        sourceFamilyToJson(
            continuity.sourceFamilyConfiguration
            )
        );

    if (continuity.sourceIdentity.has_value()) {
        object.insert(
            QStringLiteral("sourceIdentity"),
            sourceIdentityToJson(
                *continuity.sourceIdentity
                )
            );
    }

    return object;
}

bool sourceContinuityFromJson(
    const QJsonObject &object,
    InvestigationSnapshotSourceContinuity
        &continuity
    )
{
    const QJsonValue sourcePathValue =
        object.value(
            QStringLiteral("sourcePath")
            );

    const QJsonValue logicalSourceKeyValue =
        object.value(
            QStringLiteral("logicalSourceKey")
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
        || !logicalSourceKeyValue.isString()
        || logicalSourceKeyValue
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

    continuity.sourcePath =
        sourcePathValue.toString();

    continuity.logicalSourceKey =
        logicalSourceKeyValue.toString();

    continuity.sourceGeneration =
        generation;

    continuity.sourceFamilyConfiguration =
        std::move(family);

    const QJsonValue identityValue =
        object.value(
            QStringLiteral("sourceIdentity")
            );

    if (!identityValue.isUndefined()
        && !identityValue.isNull()) {
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

        continuity.sourceIdentity =
            std::move(identity);
    }

    return true;
}

QJsonObject sourceMetadataToJson(
    const RecordSourceMetadata &source
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("sourcePath"),
        source.sourcePath
        );

    const QString logicalSourceKey =
        source.logicalSourceKey
                .trimmed()
                .isEmpty()
            ? (
                  source.sourcePath
                          .trimmed()
                          .isEmpty()
                      ? source.sourceName
                      : source.sourcePath
                  )
            : source.logicalSourceKey;

    object.insert(
        QStringLiteral("logicalSourceKey"),
        logicalSourceKey
        );

    object.insert(
        QStringLiteral("sourceName"),
        source.sourceName
        );

    object.insert(
        QStringLiteral("recordNumber"),
        source.recordNumber
        );

    object.insert(
        QStringLiteral("sourceGeneration"),
        static_cast<qint64>(
            source.sourceGeneration
            )
        );

    return object;
}

bool sourceMetadataFromJson(
    const QJsonObject &object,
    RecordSourceMetadata &source
    )
{
    const QJsonValue sourcePath =
        object.value(
            QStringLiteral("sourcePath")
            );

    const QJsonValue sourceName =
        object.value(
            QStringLiteral("sourceName")
            );

    const QJsonValue logicalSourceKey =
        object.value(
            QStringLiteral(
                "logicalSourceKey"
                )
            );

    if (!sourcePath.isString()
        || !sourceName.isString()) {
        return false;
    }

    source.sourcePath =
        sourcePath.toString();

    source.sourceName =
        sourceName.toString();

    if (logicalSourceKey.isUndefined()
        || logicalSourceKey.isNull()) {
        source.logicalSourceKey =
            source.sourcePath
                    .trimmed()
                    .isEmpty()
                ? source.sourceName
                : source.sourcePath;
    } else {
        if (!logicalSourceKey.isString()) {
            return false;
        }

        source.logicalSourceKey =
            logicalSourceKey.toString();
    }

    if (!readInteger(
            object.value(
                QStringLiteral("recordNumber")
                ),
            source.recordNumber
            )) {
        return false;
    }

    qint64 sourceGeneration = 0;

    if (!readInteger(
            object.value(
                QStringLiteral(
                    "sourceGeneration"
                    )
                ),
            sourceGeneration
            )
        || sourceGeneration < 0) {
        return false;
    }

    source.sourceGeneration =
        static_cast<quint64>(
            sourceGeneration
            );

    return true;
}

QJsonObject diagnosticToJson(
    const ImportDiagnostic &diagnostic
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("code"),
        diagnostic.code
        );

    object.insert(
        QStringLiteral("message"),
        diagnostic.message
        );

    object.insert(
        QStringLiteral("severity"),
        diagnosticSeverityToJson(
            diagnostic.severity
            )
        );

    if (diagnostic.source.has_value()) {
        object.insert(
            QStringLiteral("source"),
            sourceMetadataToJson(
                *diagnostic.source
                )
            );
    }

    return object;
}

std::optional<ImportDiagnostic>
diagnosticFromJson(
    const QJsonObject &object
    )
{
    const QJsonValue code =
        object.value(
            QStringLiteral("code")
            );

    const QJsonValue message =
        object.value(
            QStringLiteral("message")
            );

    const QJsonValue severityValue =
        object.value(
            QStringLiteral("severity")
            );

    if (!code.isString()
        || !message.isString()
        || !severityValue.isString()) {
        return std::nullopt;
    }

    const std::optional<ImportDiagnosticSeverity>
        severity =
        diagnosticSeverityFromJson(
            severityValue.toString()
            );

    if (!severity.has_value()) {
        return std::nullopt;
    }

    ImportDiagnostic diagnostic;

    diagnostic.code =
        code.toString();

    diagnostic.message =
        message.toString();

    diagnostic.severity =
        *severity;

    const QJsonValue sourceValue =
        object.value(
            QStringLiteral("source")
            );

    if (!sourceValue.isUndefined()
        && !sourceValue.isNull()) {
        if (!sourceValue.isObject()) {
            return std::nullopt;
        }

        RecordSourceMetadata source;

        if (!sourceMetadataFromJson(
                sourceValue.toObject(),
                source
                )) {
            return std::nullopt;
        }

        diagnostic.source =
            std::move(source);
    }

    return diagnostic;
}
}

QByteArray
InvestigationSessionSnapshotSerializer::serialize(
    const InvestigationSessionSnapshot &snapshot
    ) const
{
    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        snapshot.schemaVersion
        );

    root.insert(
        QStringLiteral("sourceFidelity"),
        sourceFidelityToJson(
            snapshot.sourceFidelity
            )
        );

    if (snapshot.schemaVersion >= 2
        && snapshot.sourceContinuity.has_value()) {
        root.insert(
            QStringLiteral("sourceContinuity"),
            sourceContinuityToJson(
                *snapshot.sourceContinuity
                )
            );
    }

    const ImportProfileSerializer
        profileSerializer;

    const QJsonDocument profileDocument =
        QJsonDocument::fromJson(
            profileSerializer.serialize(
                snapshot.importProfile
                )
            );

    root.insert(
        QStringLiteral("importProfile"),
        profileDocument.object()
        );

    root.insert(
        QStringLiteral("processedRecordCount"),
        snapshot.processedRecordCount
        );

    root.insert(
        QStringLiteral("sourceTruncated"),
        snapshot.sourceTruncated
        );

    QJsonArray records;

    const InvestigationRecordSerializer
        recordSerializer;

    for (const InvestigationRecord &record
         : snapshot.records) {
        records.append(
            recordSerializer.serialize(
                record
                )
            );
    }

    root.insert(
        QStringLiteral("records"),
        records
        );

    QJsonArray diagnostics;

    for (const ImportDiagnostic &diagnostic
         : snapshot.diagnostics) {
        diagnostics.append(
            diagnosticToJson(
                diagnostic
                )
            );
    }

    root.insert(
        QStringLiteral("diagnostics"),
        diagnostics
        );

    /*
     * The .tsinv storage envelope will provide
     * compression, so the logical JSON payload is
     * deliberately compact.
     */
    return QJsonDocument(root).toJson(
        QJsonDocument::Compact
        );
}

InvestigationSessionSnapshotDeserializationResult
InvestigationSessionSnapshotSerializer::deserialize(
    const QByteArray &json
    ) const
{
    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json,
            &parseError
            );

    if (parseError.error
        != QJsonParseError::NoError) {
        return failure(
            QStringLiteral("INVALID_JSON"),
            QStringLiteral(
                "The investigation snapshot "
                "payload is not valid JSON: %1"
                )
                .arg(
                    parseError.errorString()
                    )
            );
    }

    if (!document.isObject()) {
        return failure(
            QStringLiteral(
                "SNAPSHOT_ROOT_NOT_OBJECT"
                ),
            QStringLiteral(
                "The investigation snapshot root "
                "must be a JSON object."
                )
            );
    }

    const QJsonObject root =
        document.object();

    qint64 schemaVersion = 0;

    if (!readInteger(
            root.value(
                QStringLiteral("schemaVersion")
                ),
            schemaVersion
            )) {
        return failure(
            QStringLiteral(
                "INVALID_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "schemaVersion must be an integer."
                )
            );
    }

    if (schemaVersion != 1
        && schemaVersion
               != InvestigationSessionSnapshot::
               CurrentSchemaVersion) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "Investigation snapshot schema "
                "version %1 is not supported."
                )
                .arg(schemaVersion)
            );
    }

    const QJsonValue fidelityValue =
        root.value(
            QStringLiteral("sourceFidelity")
            );

    if (!fidelityValue.isString()) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_FIDELITY"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "sourceFidelity must be a string."
                )
            );
    }

    const std::optional<
        InvestigationSnapshotSourceFidelity>
        sourceFidelity =
        sourceFidelityFromJson(
            fidelityValue.toString()
            );

    if (!sourceFidelity.has_value()) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_FIDELITY"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "sourceFidelity is not recognized."
                )
            );
    }

    const QJsonValue profileValue =
        root.value(
            QStringLiteral("importProfile")
            );

    if (!profileValue.isObject()) {
        return failure(
            QStringLiteral(
                "INVALID_IMPORT_PROFILE"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "importProfile must be an object."
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
                "INVALID_IMPORT_PROFILE"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "import profile is invalid: %1"
                )
                .arg(
                    profileResult.errorMessage
                    )
            );
    }

    qint64 processedRecordCount = 0;

    if (!readInteger(
            root.value(
                QStringLiteral(
                    "processedRecordCount"
                    )
                ),
            processedRecordCount
            )
        || processedRecordCount < 0) {
        return failure(
            QStringLiteral(
                "INVALID_PROCESSED_RECORD_COUNT"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "processedRecordCount must be a "
                "non-negative integer."
                )
            );
    }

    const QJsonValue sourceTruncatedValue =
        root.value(
            QStringLiteral("sourceTruncated")
            );

    if (!sourceTruncatedValue.isBool()) {
        return failure(
            QStringLiteral(
                "INVALID_SOURCE_TRUNCATED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "sourceTruncated value must be "
                "a boolean."
                )
            );
    }

    const QJsonValue recordsValue =
        root.value(
            QStringLiteral("records")
            );

    if (!recordsValue.isArray()) {
        return failure(
            QStringLiteral(
                "INVALID_RECORDS"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "records value must be an array."
                )
            );
    }

    InvestigationSessionSnapshot snapshot;

    snapshot.schemaVersion =
        static_cast<int>(
            schemaVersion
            );

    snapshot.sourceFidelity =
        *sourceFidelity;

    if (schemaVersion >= 2) {
        const QJsonValue continuityValue =
            root.value(
                QStringLiteral(
                    "sourceContinuity"
                    )
                );

        if (!continuityValue.isUndefined()
            && !continuityValue.isNull()) {
            if (!continuityValue.isObject()) {
                return failure(
                    QStringLiteral(
                        "INVALID_SOURCE_CONTINUITY"
                        ),
                    QStringLiteral(
                        "The investigation snapshot "
                        "sourceContinuity value must "
                        "be an object when present."
                        )
                    );
            }

            InvestigationSnapshotSourceContinuity
                continuity;

            if (!sourceContinuityFromJson(
                    continuityValue.toObject(),
                    continuity
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_SOURCE_CONTINUITY"
                        ),
                    QStringLiteral(
                        "The investigation snapshot "
                        "sourceContinuity value is "
                        "invalid."
                        )
                    );
            }

            snapshot.sourceContinuity =
                std::move(continuity);
        }
    }

    snapshot.importProfile =
        std::move(
            *profileResult.profile
            );

    snapshot.processedRecordCount =
        processedRecordCount;

    snapshot.sourceTruncated =
        sourceTruncatedValue.toBool();

    const InvestigationRecordSerializer
        recordSerializer;

    const QJsonArray recordArray =
        recordsValue.toArray();

    snapshot.records.reserve(
        recordArray.size()
        );

    for (int index = 0;
         index < recordArray.size();
         ++index) {
        const QJsonValue recordValue =
            recordArray.at(index);

        if (!recordValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_RECORD"
                    ),
                QStringLiteral(
                    "Investigation snapshot "
                    "record %1 must be an object."
                    )
                    .arg(index)
                );
        }

        InvestigationRecordDeserializationResult
            recordResult =
            recordSerializer.deserialize(
                recordValue.toObject()
                );

        if (!recordResult.isSuccess()) {
            return failure(
                QStringLiteral(
                    "INVALID_RECORD"
                    ),
                QStringLiteral(
                    "Investigation snapshot "
                    "record %1 is invalid: %2"
                    )
                    .arg(index)
                    .arg(
                        recordResult.errorMessage
                        )
                );
        }

        snapshot.records.append(
            std::move(
                *recordResult.record
                )
            );
    }

    const QJsonValue diagnosticsValue =
        root.value(
            QStringLiteral("diagnostics")
            );

    if (!diagnosticsValue.isArray()) {
        return failure(
            QStringLiteral(
                "INVALID_DIAGNOSTICS"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "diagnostics value must be an array."
                )
            );
    }

    const QJsonArray diagnosticArray =
        diagnosticsValue.toArray();

    snapshot.diagnostics.reserve(
        diagnosticArray.size()
        );

    for (int index = 0;
         index < diagnosticArray.size();
         ++index) {
        const QJsonValue diagnosticValue =
            diagnosticArray.at(index);

        if (!diagnosticValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_DIAGNOSTIC"
                    ),
                QStringLiteral(
                    "Investigation snapshot "
                    "diagnostic %1 must be an object."
                    )
                    .arg(index)
                );
        }

        std::optional<ImportDiagnostic>
            diagnostic =
            diagnosticFromJson(
                diagnosticValue.toObject()
                );

        if (!diagnostic.has_value()) {
            return failure(
                QStringLiteral(
                    "INVALID_DIAGNOSTIC"
                    ),
                QStringLiteral(
                    "Investigation snapshot "
                    "diagnostic %1 is invalid."
                    )
                    .arg(index)
                );
        }

        snapshot.diagnostics.append(
            std::move(*diagnostic)
            );
    }

    InvestigationSessionSnapshotDeserializationResult
        result;

    result.snapshot =
        std::move(snapshot);

    return result;
}