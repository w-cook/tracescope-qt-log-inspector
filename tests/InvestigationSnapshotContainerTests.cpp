#include <QtTest/QtTest>

#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/persistence/InvestigationSessionSnapshotFile.h"
#include "../src/persistence/InvestigationSnapshotContainer.h"

class InvestigationSnapshotContainerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsCompressedPayload();
    void rejectsInvalidMagic();
    void rejectsUnsupportedContainerVersion();
    void rejectsChecksumMismatch();
    void savesAndLoadsSnapshotFile();
};

void InvestigationSnapshotContainerTests::
    roundTripsCompressedPayload()
{
    QByteArray payload;

    for (int index = 0;
         index < 500;
         ++index) {
        payload.append(
            R"({"recordId":"record","message":"Repeated telemetry payload"})"
            );
    }

    InvestigationSnapshotContainer codec;

    const QByteArray container =
        codec.encode(
            payload
            );

    QVERIFY(!container.isEmpty());

    /*
     * The deliberately repetitive test payload should
     * compress substantially even after the small
     * container header is included.
     */
    QVERIFY(
        container.size()
        < payload.size()
        );

    const InvestigationSnapshotContainerDecodeResult
        result =
        codec.decode(
            container
            );

    QVERIFY(result.isSuccess());

    QCOMPARE(
        *result.payload,
        payload
        );
}

void InvestigationSnapshotContainerTests::
    rejectsInvalidMagic()
{
    InvestigationSnapshotContainer codec;

    QByteArray container =
        codec.encode(
            QByteArrayLiteral(
                "{\"schemaVersion\":1}"
                )
            );

    QVERIFY(
        container.size() >= 5
        );

    container[0] = 'X';

    const InvestigationSnapshotContainerDecodeResult
        result =
        codec.decode(
            container
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_CONTAINER_MAGIC"
            )
        );
}

void InvestigationSnapshotContainerTests::
    rejectsUnsupportedContainerVersion()
{
    InvestigationSnapshotContainer codec;

    QByteArray container =
        codec.encode(
            QByteArrayLiteral(
                "{\"schemaVersion\":1}"
                )
            );

    /*
     * Layout:
     * bytes 0-4 = "TSINV"
     * bytes 5-6 = big-endian quint16 container version
     */
    QVERIFY(
        container.size() >= 7
        );

    container[5] = 0;
    container[6] = 99;

    const InvestigationSnapshotContainerDecodeResult
        result =
        codec.decode(
            container
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNSUPPORTED_CONTAINER_VERSION"
            )
        );
}

void InvestigationSnapshotContainerTests::
    rejectsChecksumMismatch()
{
    InvestigationSnapshotContainer codec;

    QByteArray container =
        codec.encode(
            QByteArrayLiteral(
                "{\"schemaVersion\":1,"
                "\"records\":[1,2,3]}"
                )
            );

    /*
     * Header before the checksum:
     *
     * 5 bytes  magic
     * 2 bytes  container version
     * 1 byte   compression method
     * 8 bytes  uncompressed payload size
     *
     * The checksum therefore begins at byte 16.
     */
    constexpr qsizetype checksumOffset = 16;

    QVERIFY(
        container.size()
        > checksumOffset
        );

    container[checksumOffset] =
        static_cast<char>(
            container.at(
                checksumOffset
                )
            ^ 0x01
            );

    const InvestigationSnapshotContainerDecodeResult
        result =
        codec.decode(
            container
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "CHECKSUM_MISMATCH"
            )
        );
}

void InvestigationSnapshotContainerTests::
    savesAndLoadsSnapshotFile()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "gateway.tsinv"
                )
            );

    InvestigationSessionSnapshot original;

    original.importProfile.name =
        QStringLiteral(
            "Gateway Profile"
            );

    original.importProfile.importerId =
        QStringLiteral(
            "json-lines"
            );

    InvestigationRecord record;

    record.recordId =
        QStringLiteral(
            "record-77"
            );

    record.message =
        QStringLiteral(
            "Persisted evidence"
            );

    record.rawSource =
        QStringLiteral(
            R"({"message":"Persisted evidence"})"
            );

    record.source.sourcePath =
        QStringLiteral(
            "C:/logs/gateway.jsonl"
            );

    record.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    record.source.recordNumber = 11;
    record.source.sourceGeneration = 3;

    original.records.append(
        record
        );

    original.processedRecordCount = 1;

    InvestigationSessionSnapshotFile snapshotFile;

    const InvestigationSessionSnapshotSaveResult
        saveResult =
        snapshotFile.save(
            filePath,
            original
            );

    QVERIFY2(
        saveResult.isSuccess(),
        qPrintable(
            saveResult.errorMessage
            )
        );

    QVERIFY(
        QFileInfo::exists(
            filePath
            )
        );

    const InvestigationSessionSnapshotLoadResult
        loadResult =
        snapshotFile.load(
            filePath
            );

    QVERIFY2(
        loadResult.isSuccess(),
        qPrintable(
            loadResult.errorMessage
            )
        );

    const InvestigationSessionSnapshot
        &restored =
        *loadResult.snapshot;

    QCOMPARE(
        restored.importProfile.name,
        original.importProfile.name
        );

    QCOMPARE(
        restored.records.size(),
        1
        );

    QCOMPARE(
        restored.records.at(0).recordId,
        QStringLiteral(
            "record-77"
            )
        );

    QCOMPARE(
        restored
            .records
            .at(0)
            .source
            .sourceGeneration,
        quint64(3)
        );
}

QTEST_MAIN(
    InvestigationSnapshotContainerTests
    )

#include "InvestigationSnapshotContainerTests.moc"