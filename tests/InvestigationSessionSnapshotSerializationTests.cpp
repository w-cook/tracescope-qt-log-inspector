#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>

#include "../src/persistence/InvestigationSessionSnapshotSerialization.h"

class InvestigationSessionSnapshotSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsNormalizedSnapshot();
    void rejectsUnsupportedSchemaVersion();
    void rejectsInvalidRecord();
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

QTEST_MAIN(
    InvestigationSessionSnapshotSerializationTests
    )

#include "InvestigationSessionSnapshotSerializationTests.moc"