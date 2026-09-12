#include <QtTest>

#include <optional>

#include "../src/live/LiveStructuredJsonImportAdapter.h"

namespace
{
ImportProfile testProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "structured-json"
            );

    profile.canonicalFields.severityPath =
        QStringLiteral("level");

    profile.canonicalFields.subsystemPath =
        QStringLiteral("subsystem");

    profile.canonicalFields.eventCodePath =
        QStringLiteral("eventCode");

    profile.canonicalFields.entityIdPath =
        QStringLiteral("entityId");

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    profile.preserveUnmappedFields =
        true;

    return profile;
}
}

class LiveStructuredJsonImportAdapterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void recognizesStructuredJsonProfile();
    void importsBatchWithLiveSourcePosition();
    void mapsMultipleRecordsInOneBatch();
    void malformedRecordDoesNotDiscardValidBatchRecords();
    void normalizesRawSourceLikeStaticImporter();
};

void LiveStructuredJsonImportAdapterTests::
    recognizesStructuredJsonProfile()
{
    LiveStructuredJsonImportAdapter supported(
        testProfile()
        );

    QVERIFY(
        supported.isSupported()
        );

    ImportProfile unsupportedProfile;

    unsupportedProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    LiveStructuredJsonImportAdapter unsupported(
        unsupportedProfile
        );

    QVERIFY(
        !unsupported.isSupported()
        );
}

void LiveStructuredJsonImportAdapterTests::
    importsBatchWithLiveSourcePosition()
{
    LiveStructuredJsonImportAdapter adapter(
        testProfile()
        );

    LiveStructuredJsonRecordBatch batch;

    batch.firstRecordNumber = 14;
    batch.sourceGeneration = 3;

    batch.records.append(
        QByteArray(
            "{"
            "\"level\":\"WARN\","
            "\"subsystem\":\"Payments\","
            "\"eventCode\":\"PAYMENT_SLOW\","
            "\"entityId\":\"checkout-17\","
            "\"message\":\"Live record\","
            "\"latencyMs\":1840"
            "}"
            )
        );

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "session.json"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.source.recordNumber,
        qint64(14)
        );

    QCOMPARE(
        record.source.sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        record.source.sourceName,
        QStringLiteral(
            "session.json"
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral(
                "Live record"
                )
            )
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral(
                "Payments"
                )
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "latencyMs"
                    )
                )
            .toInt(),
        1840
        );
}

void LiveStructuredJsonImportAdapterTests::
    mapsMultipleRecordsInOneBatch()
{
    LiveStructuredJsonImportAdapter adapter(
        testProfile()
        );

    LiveStructuredJsonRecordBatch batch;

    batch.firstRecordNumber = 21;
    batch.sourceGeneration = 2;

    batch.records = {
        QByteArray(
            "{"
            "\"message\":\"first\","
            "\"subsystem\":\"Checkout\""
            "}"
            ),
        QByteArray(
            "{"
            "\"message\":\"second\","
            "\"subsystem\":\"Payments\""
            "}"
            ),
        QByteArray(
            "{"
            "\"message\":\"third\","
            "\"subsystem\":\"Inventory\""
            "}"
            )
    };

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "live.json"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result.records.size(),
        3
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(21)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(22)
        );

    QCOMPARE(
        result.records.at(2)
            .source.recordNumber,
        qint64(23)
        );

    QCOMPARE(
        result.records.at(0).message,
        std::optional<QString>(
            QStringLiteral("first")
            )
        );

    QCOMPARE(
        result.records.at(1).message,
        std::optional<QString>(
            QStringLiteral("second")
            )
        );

    QCOMPARE(
        result.records.at(2).message,
        std::optional<QString>(
            QStringLiteral("third")
            )
        );
}

void LiveStructuredJsonImportAdapterTests::
    malformedRecordDoesNotDiscardValidBatchRecords()
{
    LiveStructuredJsonImportAdapter adapter(
        testProfile()
        );

    LiveStructuredJsonRecordBatch batch;

    batch.firstRecordNumber = 7;
    batch.sourceGeneration = 4;

    batch.records = {
        QByteArray(
            "{\"message\":\"before\"}"
            ),
        QByteArray(
            "{"
            "\"message\": invalid"
            "}"
            ),
        QByteArray(
            "{\"message\":\"after\"}"
            )
    };

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "live.json"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .source.recordNumber,
        qint64(7)
        );

    QCOMPARE(
        result.records.at(1)
            .source.recordNumber,
        qint64(9)
        );

    QCOMPARE(
        result.records.at(0).message,
        std::optional<QString>(
            QStringLiteral("before")
            )
        );

    QCOMPARE(
        result.records.at(1).message,
        std::optional<QString>(
            QStringLiteral("after")
            )
        );

    QCOMPARE(
        result.diagnostics.size(),
        1
        );

    QCOMPARE(
        result.diagnostics.first().code,
        QStringLiteral(
            "MALFORMED_STRUCTURED_JSON_LIVE_RECORD"
            )
        );

    QVERIFY(
        result.diagnostics
            .first()
            .source
            .has_value()
        );

    QCOMPARE(
        result.diagnostics
            .first()
            .source
            ->recordNumber,
        qint64(8)
        );

    QCOMPARE(
        result.diagnostics
            .first()
            .source
            ->sourceGeneration,
        quint64(4)
        );
}

void LiveStructuredJsonImportAdapterTests::
    normalizesRawSourceLikeStaticImporter()
{
    LiveStructuredJsonImportAdapter adapter(
        testProfile()
        );

    LiveStructuredJsonRecordBatch batch;

    batch.records.append(
        QByteArray(
            "{\n"
            "    \"message\" : \"normalized\",\n"
            "    \"value\" : 42\n"
            "}"
            )
        );

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "live.json"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first().rawSource,
        QStringLiteral(
            "{\"message\":\"normalized\",\"value\":42}"
            )
        );
}

QTEST_MAIN(
    LiveStructuredJsonImportAdapterTests
    )

#include "LiveStructuredJsonImportAdapterTests.moc"