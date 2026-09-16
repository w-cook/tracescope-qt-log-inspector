#include "InvestigationSessionSnapshotSerialization.h"

#include <cmath>
#include <limits>
#include <utility>

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

    if (!sourcePath.isString()
        || !sourceName.isString()) {
        return false;
    }

    source.sourcePath =
        sourcePath.toString();

    source.sourceName =
        sourceName.toString();

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

    if (schemaVersion
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