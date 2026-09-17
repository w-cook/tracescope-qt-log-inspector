#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/domain/RecordIdentity.h"
#include "../src/importing/JsonLinesImporter.h"
#include "../src/sources/SourcePhysicalIdentity.h"
#include "../src/workspace/HybridInvestigationReconstructionService.h"

namespace
{
ImportProfile jsonLinesProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "message"
            );

    profile.preserveUnmappedFields =
        true;

    return profile;
}

void writeFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}

void appendFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}

void rebaseRecords(
    QVector<InvestigationRecord> &records,
    quint64 sourceGeneration
    )
{
    for (InvestigationRecord &record
         : records) {
        record.source.sourceGeneration =
            sourceGeneration;

        record.recordId =
            createStableRecordIdentity(
                record.source,
                record.rawSource
                );
    }
}

InvestigationExternalSourceBinding
makeExternalBinding(
    const QString &sourcePath,
    quint64 sourceGeneration
    )
{
    InvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration =
        sourceGeneration;

    const SourcePhysicalIdentityCaptureResult
        identityResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    if (identityResult.succeeded) {
        binding.sourceIdentity =
            identityResult.identity;
    }

    return binding;
}
}

class HybridInvestigationReconstructionServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sameGenerationReplayPreservesBaselineAndAddsGrowth();
    void replacementAdvancesGenerationAndPreservesHistory();
    void missingExternalSourceRestoresSnapshotOnly();
    void importerMismatchIsRejected();
};

void HybridInvestigationReconstructionServiceTests::
    sameGenerationReplayPreservesBaselineAndAddsGrowth()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    const QByteArray baseline(
        "{\"message\":\"one\"}\n"
        "{\"message\":\"two\"}\n"
        );

    writeFile(
        sourcePath,
        baseline
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        2
        );

    constexpr quint64
        SavedGeneration = 2;

    rebaseRecords(
        initialResult.records,
        SavedGeneration
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        initialResult.records;

    snapshot.diagnostics =
        initialResult.diagnostics;

    snapshot.processedRecordCount =
        initialResult.processedRecordCount;

    snapshot.sourceTruncated =
        initialResult.sourceTruncated;

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            SavedGeneration
            );

    QVERIFY(
        binding
            .sourceIdentity
            .has_value()
        );

    appendFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"three\"}\n"
            )
        );

    HybridInvestigationReconstructionService
        service;

    const HybridInvestigationReconstructionResult
        result =
        service.reconstruct(
            snapshot,
            binding,
            importer
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.externalSourceAvailable
        );

    QVERIFY(
        !result.sourceGenerationChanged
        );

    QCOMPARE(
        result
            .externalSourceBinding
            .sourceGeneration,
        SavedGeneration
        );

    QVERIFY(
        result
            .externalSourceBinding
            .sourceIdentity
            .has_value()
        );

    QCOMPARE(
        result.duplicateReplayRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.appendedReplayRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importResult.records.size(),
        3
        );

    QCOMPARE(
        result.importResult.processedRecordCount,
        qint64(3)
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(0)
            .message,
        std::optional<QString>(
            QStringLiteral("one")
            )
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(1)
            .message,
        std::optional<QString>(
            QStringLiteral("two")
            )
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(2)
            .message,
        std::optional<QString>(
            QStringLiteral("three")
            )
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(2)
            .source
            .sourceGeneration,
        SavedGeneration
        );
}

void HybridInvestigationReconstructionServiceTests::
    replacementAdvancesGenerationAndPreservesHistory()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    /*
     * Keep the saved generation deliberately larger
     * than the replacement so size regression gives a
     * deterministic replacement signal on every
     * supported filesystem.
     */
    const QByteArray original(
        "{\"message\":\"old one with a deliberately long payload\"}\n"
        "{\"message\":\"old two with another deliberately long payload\"}\n"
        );

    writeFile(
        sourcePath,
        original
        );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    constexpr quint64
        SavedGeneration = 4;

    rebaseRecords(
        initialResult.records,
        SavedGeneration
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records =
        initialResult.records;

    snapshot.processedRecordCount =
        initialResult.processedRecordCount;

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            SavedGeneration
            );

    QVERIFY(
        binding
            .sourceIdentity
            .has_value()
        );

    const QByteArray replacement(
        "{\"message\":\"new\"}\n"
        );

    QVERIFY(
        replacement.size()
        < original.size()
        );

    writeFile(
        sourcePath,
        replacement
        );

    HybridInvestigationReconstructionService
        service;

    const HybridInvestigationReconstructionResult
        result =
        service.reconstruct(
            snapshot,
            binding,
            importer
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.externalSourceAvailable
        );

    QVERIFY(
        result.sourceGenerationChanged
        );

    QCOMPARE(
        result
            .externalSourceBinding
            .sourceGeneration,
        quint64(
            SavedGeneration + 1
            )
        );

    QCOMPARE(
        result.duplicateReplayRecordCount,
        qint64(0)
        );

    QCOMPARE(
        result.appendedReplayRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importResult.records.size(),
        3
        );

    /*
     * Both durable historical records remain first.
     */
    QCOMPARE(
        result
            .importResult
            .records
            .at(0)
            .source
            .sourceGeneration,
        SavedGeneration
        );

    QCOMPARE(
        result
            .importResult
            .records
            .at(1)
            .source
            .sourceGeneration,
        SavedGeneration
        );

    const InvestigationRecord &newRecord =
        result
            .importResult
            .records
            .at(2);

    QCOMPARE(
        newRecord.message,
        std::optional<QString>(
            QStringLiteral("new")
            )
        );

    QCOMPARE(
        newRecord
            .source
            .sourceGeneration,
        quint64(
            SavedGeneration + 1
            )
        );
}

void HybridInvestigationReconstructionServiceTests::
    missingExternalSourceRestoresSnapshotOnly()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "missing.jsonl"
                )
            );

    const ImportProfile profile =
        jsonLinesProfile();

    JsonLinesImporter importer(
        profile
        );

    InvestigationRecord record;

    record.source.sourcePath =
        sourcePath;

    record.source.sourceName =
        QStringLiteral(
            "missing.jsonl"
            );

    record.source.recordNumber = 1;
    record.source.sourceGeneration = 3;

    record.rawSource =
        QStringLiteral(
            "{\"message\":\"saved\"}"
            );

    record.message =
        QStringLiteral(
            "saved"
            );

    record.recordId =
        createStableRecordIdentity(
            record.source,
            record.rawSource
            );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        profile;

    snapshot.records.append(
        record
        );

    snapshot.processedRecordCount = 1;

    InvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        sourcePath;

    binding.sourceGeneration = 3;

    HybridInvestigationReconstructionService
        service;

    const HybridInvestigationReconstructionResult
        result =
        service.reconstruct(
            snapshot,
            binding,
            importer
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        !result.externalSourceAvailable
        );

    QVERIFY(
        !result.sourceGenerationChanged
        );

    QCOMPARE(
        result
            .externalSourceBinding
            .sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        result.importResult.records.size(),
        1
        );

    QCOMPARE(
        result
            .importResult
            .records
            .first()
            .recordId,
        record.recordId
        );

    QCOMPARE(
        result.importResult.processedRecordCount,
        qint64(1)
        );
}

void HybridInvestigationReconstructionServiceTests::
    importerMismatchIsRejected()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.jsonl"
                )
            );

    writeFile(
        sourcePath,
        QByteArray(
            "{\"message\":\"record\"}\n"
            )
        );

    InvestigationSessionSnapshot
        snapshot;

    snapshot.importProfile =
        jsonLinesProfile();

    /*
     * Make the saved snapshot claim a different
     * importer than the concrete replay importer.
     */
    snapshot.importProfile.importerId =
        QStringLiteral(
            "csv"
            );

    JsonLinesImporter importer(
        jsonLinesProfile()
        );

    InvestigationExternalSourceBinding
        binding =
        makeExternalBinding(
            sourcePath,
            0
            );

    HybridInvestigationReconstructionService
        service;

    const HybridInvestigationReconstructionResult
        result =
        service.reconstruct(
            snapshot,
            binding,
            importer
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );

    QVERIFY(
        result.importResult.records.isEmpty()
        );
}

QTEST_MAIN(
    HybridInvestigationReconstructionServiceTests
    )

#include "HybridInvestigationReconstructionServiceTests.moc"