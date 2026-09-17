#include <QtTest/QtTest>

#include <QByteArray>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "../src/persistence/InvestigationSessionSnapshotSerialization.h"

class InvestigationSessionSnapshotSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsNormalizedSnapshot();
    void readsLegacyDiagnosticLogicalSourceKey();
    void rejectsUnsupportedSchemaVersion();
    void rejectsInvalidRecord();

    void roundTripsSourceContinuity();
    void readsVersion1WithoutSourceContinuity();
    void rejectsInvalidSourceContinuity();
};

void InvestigationSessionSnapshotSerializationTests::
    roundTripsNormalizedSnapshot()
{
    InvestigationSessionSnapshot original;

    original.sourceFidelity =
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly;

    original.importProfile.name =
        QStringLiteral(
            "Gateway JSONL"
            );

    original.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    original.importProfile
        .canonicalFields
        .timestampPath =
        QStringLiteral("time");

    original.importProfile
        .canonicalFields
        .severityPath =
        QStringLiteral("severity");

    InvestigationRecord first;

    first.recordId =
        QStringLiteral("record-1");

    first.timestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-15T18:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    first.severity =
        RecordSeverity::Info;

    first.message =
        QStringLiteral("Started");

    first.rawSource =
        QStringLiteral(
            R"({"severity":"info","message":"Started"})"
            );

    first.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    first.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    first.source.recordNumber = 1;

    InvestigationRecord second;

    second.recordId =
        QStringLiteral("record-2");

    second.severity =
        RecordSeverity::Error;

    second.message =
        QStringLiteral("Failed");

    second.rawSource =
        QStringLiteral(
            R"({"severity":"error","message":"Failed"})"
            );

    second.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    second.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    second.source.recordNumber = 2;
    second.source.sourceGeneration = 3;

    original.records = {
        first,
        second
    };

    original.processedRecordCount = 3;
    original.sourceTruncated = true;

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral(
            "TEST_WARNING"
            );

    diagnostic.message =
        QStringLiteral(
            "A test record was skipped."
            );

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    RecordSourceMetadata diagnosticSource;

    diagnosticSource.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    diagnosticSource.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    diagnosticSource.recordNumber = 3;
    diagnosticSource.sourceGeneration = 3;

    diagnostic.source =
        diagnosticSource;

    original.diagnostics.append(
        diagnostic
        );

    InvestigationSessionSnapshotSerializer
        serializer;

    const QByteArray serialized =
        serializer.serialize(
            original
            );

    /*
     * The logical snapshot payload is compact JSON.
     * Compression belongs to the outer .tsinv
     * container layer.
     */
    QVERIFY(
        !serialized.contains(
            QByteArray("\n    ")
            )
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            serialized
            );

    QVERIFY(result.isSuccess());

    const InvestigationSessionSnapshot
        &restored =
        *result.snapshot;

    QCOMPARE(
        restored.schemaVersion,
        InvestigationSessionSnapshot::
        CurrentSchemaVersion
        );

    QCOMPARE(
        restored.sourceFidelity,
        InvestigationSnapshotSourceFidelity::
        NormalizedOnly
        );

    QCOMPARE(
        restored.importProfile.name,
        QStringLiteral(
            "Gateway JSONL"
            )
        );

    QCOMPARE(
        restored.importProfile.importerId,
        QStringLiteral(
            "json-lines"
            )
        );

    QCOMPARE(
        restored
            .importProfile
            .canonicalFields
            .timestampPath,
        QStringLiteral("time")
        );

    QCOMPARE(
        restored
            .importProfile
            .canonicalFields
            .severityPath,
        QStringLiteral("severity")
        );

    QCOMPARE(
        restored.records.size(),
        2
        );

    QCOMPARE(
        restored.records.at(0).recordId,
        QStringLiteral("record-1")
        );

    QCOMPARE(
        restored.records.at(1).recordId,
        QStringLiteral("record-2")
        );

    QCOMPARE(
        restored
            .records
            .at(1)
            .source
            .sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        restored.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        restored.sourceTruncated,
        true
        );

    QCOMPARE(
        restored.diagnostics.size(),
        1
        );

    QCOMPARE(
        restored.diagnostics.at(0).code,
        QStringLiteral(
            "TEST_WARNING"
            )
        );

    QCOMPARE(
        restored
            .diagnostics
            .at(0)
            .severity,
        ImportDiagnosticSeverity::Warning
        );

    QVERIFY(
        restored
            .diagnostics
            .at(0)
            .source
            .has_value()
        );

    QCOMPARE(
        restored
            .diagnostics
            .at(0)
            .source
            ->sourceGeneration,
        quint64(3)
        );
}

void InvestigationSessionSnapshotSerializationTests::
    readsLegacyDiagnosticLogicalSourceKey()
{
    InvestigationSessionSnapshot snapshot;

    snapshot.importProfile.name =
        QStringLiteral("Legacy diagnostic test");

    snapshot.importProfile.importerId =
        QStringLiteral("json-lines");

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral("LEGACY_WARNING");

    diagnostic.message =
        QStringLiteral(
            "Legacy diagnostic metadata test."
            );

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    RecordSourceMetadata source;

    source.sourcePath =
        QStringLiteral(
            "C:/logs/legacy-service.log"
            );

    source.sourceName =
        QStringLiteral(
            "legacy-service.log"
            );

    source.recordNumber = 7;
    source.sourceGeneration = 2;

    diagnostic.source =
        source;

    snapshot.diagnostics.append(
        diagnostic
        );

    InvestigationSessionSnapshotSerializer
        serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(
                snapshot
                )
            );

    QJsonObject root =
        document.object();

    QJsonArray diagnostics =
        root.value(
                QStringLiteral("diagnostics")
                ).toArray();

    QVERIFY(!diagnostics.isEmpty());

    QJsonObject serializedDiagnostic =
        diagnostics.at(0).toObject();

    QJsonObject serializedSource =
        serializedDiagnostic
            .value(
                QStringLiteral("source")
                )
            .toObject();

    /*
     * Simulate a snapshot written before explicit
     * logical source identity was persisted in
     * diagnostic metadata.
     */
    serializedSource.remove(
        QStringLiteral("logicalSourceKey")
        );

    serializedDiagnostic.insert(
        QStringLiteral("source"),
        serializedSource
        );

    diagnostics[0] =
        serializedDiagnostic;

    root.insert(
        QStringLiteral("diagnostics"),
        diagnostics
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(result.isSuccess());

    QVERIFY(
        result.snapshot
            ->diagnostics
            .at(0)
            .source
            .has_value()
        );

    const RecordSourceMetadata &restoredSource =
        *result.snapshot
             ->diagnostics
             .at(0)
             .source;

    QCOMPARE(
        restoredSource.sourcePath,
        QStringLiteral(
            "C:/logs/legacy-service.log"
            )
        );

    QCOMPARE(
        restoredSource.logicalSourceKey,
        QStringLiteral(
            "C:/logs/legacy-service.log"
            )
        );

    QCOMPARE(
        restoredSource.sourceGeneration,
        quint64(2)
        );
}

void InvestigationSessionSnapshotSerializationTests::
    rejectsUnsupportedSchemaVersion()
{
    InvestigationSessionSnapshot snapshot;

    InvestigationSessionSnapshotSerializer
        serializer;

    const QByteArray serialized =
        serializer.serialize(
            snapshot
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            serialized
            );

    QJsonObject root =
        document.object();

    root.insert(
        QStringLiteral("schemaVersion"),
        999
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNSUPPORTED_SCHEMA_VERSION"
            )
        );
}

void InvestigationSessionSnapshotSerializationTests::
    rejectsInvalidRecord()
{
    InvestigationSessionSnapshot snapshot;

    InvestigationRecord record;

    record.recordId =
        QStringLiteral("record-1");

    record.rawSource =
        QStringLiteral("test");

    snapshot.records.append(
        record
        );

    snapshot.processedRecordCount = 1;

    InvestigationSessionSnapshotSerializer
        serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(
                snapshot
                )
            );

    QJsonObject root =
        document.object();

    QJsonArray records =
        root.value(
                QStringLiteral("records")
                ).toArray();

    QJsonObject invalidRecord =
        records.at(0).toObject();

    QJsonObject source =
        invalidRecord.value(
                         QStringLiteral("source")
                         ).toObject();

    source.insert(
        QStringLiteral("sourceGeneration"),
        -1
        );

    invalidRecord.insert(
        QStringLiteral("source"),
        source
        );

    records[0] =
        invalidRecord;

    root.insert(
        QStringLiteral("records"),
        records
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral("INVALID_RECORD")
        );
}

void InvestigationSessionSnapshotSerializationTests::
    roundTripsSourceContinuity()
{
    InvestigationSessionSnapshot original;

    InvestigationSnapshotSourceContinuity
        continuity;

    continuity.sourcePath =
        QStringLiteral(
            "C:/logs/current/service.log"
            );

    continuity.logicalSourceKey =
        QStringLiteral(
            "C:/logs/original/service.log"
            );

    continuity.sourceGeneration = 7;

    continuity
        .sourceFamilyConfiguration
        .includeRotatedSources = true;

    continuity
        .sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericBeforeExtension;

    continuity
        .sourceFamilyConfiguration
        .rotationRule
        .numericOrderDirection =
        RotatedSourceOrderDirection::
        Descending;

    /*
     * Physical discovery results must not survive
     * persistence.
     */
    continuity
        .sourceFamilyConfiguration
        .rotatedSourcePaths = {
        QStringLiteral(
            "C:/logs/current/service.2.log"
            ),
        QStringLiteral(
            "C:/logs/current/service.1.log"
            )
    };

    SourcePhysicalIdentity identity;

    identity.birthTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-16T14:30:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    identity.fingerprintLength = 16;

    identity.prefixFingerprint =
        QByteArray(
            32,
            static_cast<char>(0x5a)
            );

    identity.observedSizeBytes = 4096;

    continuity.sourceIdentity =
        identity;

    original.sourceContinuity =
        continuity;

    InvestigationSessionSnapshotSerializer
        serializer;

    const QByteArray serialized =
        serializer.serialize(
            original
            );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            serialized
            );

    QVERIFY(result.isSuccess());

    const InvestigationSessionSnapshot
        &restored =
        *result.snapshot;

    QCOMPARE(
        restored.schemaVersion,
        2
        );

    QVERIFY(
        restored.sourceContinuity.has_value()
        );

    const InvestigationSnapshotSourceContinuity
        &restoredContinuity =
        *restored.sourceContinuity;

    QCOMPARE(
        restoredContinuity.sourcePath,
        QStringLiteral(
            "C:/logs/current/service.log"
            )
        );

    QCOMPARE(
        restoredContinuity.logicalSourceKey,
        QStringLiteral(
            "C:/logs/original/service.log"
            )
        );

    QCOMPARE(
        restoredContinuity.sourceGeneration,
        quint64(7)
        );

    QCOMPARE(
        restoredContinuity
            .sourceFamilyConfiguration
            .includeRotatedSources,
        true
        );

    QCOMPARE(
        restoredContinuity
            .sourceFamilyConfiguration
            .rotationRule
            .namingScheme,
        RotatedSourceNamingScheme::
        NumericBeforeExtension
        );

    QCOMPARE(
        restoredContinuity
            .sourceFamilyConfiguration
            .rotationRule
            .numericOrderDirection,
        RotatedSourceOrderDirection::
        Descending
        );

    /*
     * These are transient discovery results, not
     * durable source-family configuration.
     */
    QVERIFY(
        restoredContinuity
            .sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(
        restoredContinuity
            .sourceIdentity
            .has_value()
        );

    QCOMPARE(
        restoredContinuity
            .sourceIdentity
            ->birthTime,
        identity.birthTime
        );

    QCOMPARE(
        restoredContinuity
            .sourceIdentity
            ->fingerprintLength,
        qint64(16)
        );

    QCOMPARE(
        restoredContinuity
            .sourceIdentity
            ->prefixFingerprint,
        identity.prefixFingerprint
        );

    QCOMPARE(
        restoredContinuity
            .sourceIdentity
            ->observedSizeBytes,
        qint64(4096)
        );
}

void InvestigationSessionSnapshotSerializationTests::
    readsVersion1WithoutSourceContinuity()
{
    InvestigationSessionSnapshot original;

    original.schemaVersion = 1;

    InvestigationSnapshotSourceContinuity
        continuity;

    continuity.sourcePath =
        QStringLiteral(
            "C:/logs/service.log"
            );

    continuity.logicalSourceKey =
        continuity.sourcePath;

    /*
     * Even if an in-memory object contains continuity,
     * schema version 1 must not write the v2 field.
     */
    original.sourceContinuity =
        continuity;

    InvestigationSessionSnapshotSerializer
        serializer;

    const QByteArray serialized =
        serializer.serialize(
            original
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            serialized
            );

    QVERIFY(document.isObject());

    QVERIFY(
        !document
             .object()
             .contains(
                 QStringLiteral(
                     "sourceContinuity"
                     )
                 )
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            serialized
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        result.snapshot->schemaVersion,
        1
        );

    QVERIFY(
        !result
             .snapshot
             ->sourceContinuity
             .has_value()
        );
}

void InvestigationSessionSnapshotSerializationTests::
    rejectsInvalidSourceContinuity()
{
    InvestigationSessionSnapshot snapshot;

    InvestigationSessionSnapshotSerializer
        serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(
                snapshot
                )
            );

    QJsonObject root =
        document.object();

    QJsonObject invalidContinuity;

    /*
     * A v2 continuity object must contain the complete
     * durable continuity contract, not merely a path.
     */
    invalidContinuity.insert(
        QStringLiteral("sourcePath"),
        QStringLiteral(
            "C:/logs/service.log"
            )
        );

    root.insert(
        QStringLiteral("sourceContinuity"),
        invalidContinuity
        );

    const InvestigationSessionSnapshotDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SOURCE_CONTINUITY"
            )
        );
}

QTEST_MAIN(
    InvestigationSessionSnapshotSerializationTests
    )

#include "InvestigationSessionSnapshotSerializationTests.moc"