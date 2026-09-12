#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/importing/StructuredJsonImporter.h"
#include "../src/live/LiveSessionFollowCoordinator.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{
ImportProfile structuredJsonProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "structured-json"
            );

    profile.recordPath =
        QStringLiteral(
            "data.records"
            );

    profile.canonicalFields.subsystemPath =
        QStringLiteral(
            "subsystem"
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
}

class LiveSessionFollowCoordinatorTests
    : public QObject
{
    Q_OBJECT

private slots:
    void supportsStructuredJsonProfile();
    void structuredJsonBaselineAndGrowthUseIncrementalSessionPath();
    void structuredJsonReplacementStartsNewSourceGeneration();
};

void LiveSessionFollowCoordinatorTests::
    supportsStructuredJsonProfile()
{
    QVERIFY(
        LiveSessionFollowCoordinator::
        supportsProfile(
            structuredJsonProfile()
            )
        );

    ImportProfile unsupported;

    unsupported.importerId =
        QStringLiteral(
            "xml"
            );

    QVERIFY(
        !LiveSessionFollowCoordinator::
        supportsProfile(
            unsupported
            )
        );
}

void LiveSessionFollowCoordinatorTests::
    structuredJsonBaselineAndGrowthUseIncrementalSessionPath()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.json"
                )
            );

    /*
     * The outer JSON document is intentionally open.
     *
     * The first object is complete and must be
     * imported immediately by StructuredJsonImporter.
     *
     * The second object is only partially written and
     * must remain pending until live following receives
     * its remaining bytes.
     */
    const QByteArray baseline(
        "{"
        "\"data\":{"
        "\"records\":["
        "{"
        "\"message\":\"first\","
        "\"subsystem\":\"Payments\""
        "},"
        "{"
        "\"message\":\"sec"
        );

    writeFile(
        sourcePath,
        baseline
        );

    const ImportProfile profile =
        structuredJsonProfile();

    StructuredJsonImportConfig config;

    config.recordPath =
        profile.recordPath;

    StructuredJsonImporter importer(
        config,
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    /*
     * Initial import now understands an actively
     * open structured record array.
     */
    QCOMPARE(
        initialResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    QCOMPARE(
        initialResult.records.first()
            .message,
        std::optional<QString>(
            QStringLiteral(
                "first"
                )
            )
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    LiveSessionFollowCoordinator coordinator(
        session
        );

    QVERIFY(
        coordinator.isSupported()
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator.start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    QCOMPARE(
        startResult.baselineByteCount,
        qint64(
            baseline.size()
            )
        );

    /*
     * Starting live following must reconstruct
     * framing state only.
     *
     * It must NOT process the already-imported
     * baseline record a second time.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session
            .investigationController()
            ->allRecords()
            .size(),
        1
        );

    /*
     * Establish an active filter before later live
     * records arrive.
     */
    session
        .investigationController()
        ->setFilters(
            QString(),
            QStringLiteral(
                "Payments"
                ),
            QString()
            );

    QCOMPARE(
        session
            .investigationController()
            ->visibleRecords()
            .size(),
        1
        );

    /*
     * Finish the partial second record, then provide
     * a third complete record.
     */
    const QByteArray appended(
        "ond\","
        "\"subsystem\":\"Checkout\""
        "},"
        "{"
        "\"message\":\"third\","
        "\"subsystem\":\"Payments\""
        "},"
        );

    appendFile(
        sourcePath,
        appended
        );

    const LiveSessionFollowPollResult
        pollResult =
        coordinator.pollOnce();

    QVERIFY2(
        pollResult.succeeded,
        qPrintable(
            pollResult.errorMessage
            )
        );

    /*
     * Both newly completed objects belong to one
     * semantic live-import batch.
     */
    QCOMPARE(
        pollResult.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(3)
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(3)
        );

    /*
     * Record numbering continues from the baseline
     * rather than restarting when live follow begins.
     */
    const QVector<InvestigationRecord>
        allRecords =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        allRecords.size(),
        3
        );

    QCOMPARE(
        allRecords.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        allRecords.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        allRecords.at(2)
            .source.recordNumber,
        qint64(3)
        );

    QCOMPARE(
        allRecords.at(0)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        allRecords.at(1)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        allRecords.at(2)
            .source.sourceGeneration,
        quint64(0)
        );

    /*
     * The active Payments filter survives the live
     * append. Checkout stays hidden and the new
     * Payments record enters the proxy automatically.
     */
    const QVector<InvestigationRecord>
        visibleRecords =
        session
            .investigationController()
            ->visibleRecords();

    QCOMPARE(
        visibleRecords.size(),
        2
        );

    QCOMPARE(
        visibleRecords.at(0)
            .message,
        std::optional<QString>(
            QStringLiteral(
                "first"
                )
            )
        );

    QCOMPARE(
        visibleRecords.at(1)
            .message,
        std::optional<QString>(
            QStringLiteral(
                "third"
                )
            )
        );

    QVERIFY(
        coordinator.stop()
        );
}

void LiveSessionFollowCoordinatorTests::
    structuredJsonReplacementStartsNewSourceGeneration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.json"
                )
            );

    /*
     * Make generation zero deliberately larger than
     * its replacement so truncation is an
     * unambiguous source-reset signal.
     *
     * It contains one complete record and one
     * incomplete trailing record.
     */
    const QByteArray original(
        "{"
        "\"data\":{"
        "\"records\":["
        "{"
        "\"message\":"
        "\"Original generation record with "
        "enough text to keep this file larger "
        "than the replacement\","
        "\"subsystem\":\"Original\""
        "},"
        "{"
        "\"message\":\"old-partial"
        );

    writeFile(
        sourcePath,
        original
        );

    const ImportProfile profile =
        structuredJsonProfile();

    StructuredJsonImportConfig config;

    config.recordPath =
        profile.recordPath;

    StructuredJsonImporter importer(
        config,
        profile
        );

    /*
     * Initial session creation follows the actual
     * application path: the static structured
     * importer recovers complete records from the
     * still-open record array.
     */
    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    QCOMPARE(
        initialResult.records.first()
            .message,
        std::optional<QString>(
            QStringLiteral(
                "Original generation record with "
                "enough text to keep this file larger "
                "than the replacement"
                )
            )
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    LiveSessionFollowCoordinator coordinator(
        session
        );

    const LiveSessionFollowStartResult
        startResult =
        coordinator.start();

    QVERIFY2(
        startResult.succeeded,
        qPrintable(
            startResult.errorMessage
            )
        );

    /*
     * Starting live following reconstructs the
     * structured framing state but does not import
     * generation-zero records a second time.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    const QVector<InvestigationRecord>
        originalRecords =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        originalRecords.size(),
        1
        );

    QCOMPARE(
        originalRecords.first()
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        originalRecords.first()
            .source.sourceGeneration,
        quint64(0)
        );

    /*
     * Truncate/rewrite the same physical path with
     * a fresh open structured document.
     *
     * This represents generation one.
     */
    const QByteArray replacement(
        "{"
        "\"data\":{"
        "\"records\":["
        "{"
        "\"message\":\"Replacement record\","
        "\"subsystem\":\"Replacement\""
        "},"
        );

    QVERIFY(
        replacement.size()
        < original.size()
        );

    writeFile(
        sourcePath,
        replacement
        );

    const LiveSessionFollowPollResult
        pollResult =
        coordinator.pollOnce();

    QVERIFY2(
        pollResult.succeeded,
        qPrintable(
            pollResult.errorMessage
            )
        );

    QVERIFY(
        pollResult.sourceGenerationChanged
        );

    QCOMPARE(
        pollResult.observation.kind,
        LiveFileObservationKind::
        SourceReset
        );

    QCOMPARE(
        pollResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(1)
        );

    /*
     * Historical generation-zero evidence remains.
     * The replacement is appended as generation one.
     */
    QCOMPARE(
        session.processedRecordCount(),
        qint64(2)
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(2)
        );

    const QVector<InvestigationRecord>
        records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        2
        );

    QCOMPARE(
        records.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        records.at(0)
            .source.sourceGeneration,
        quint64(0)
        );

    /*
     * Logical source numbering restarts at one after
     * a physical source-generation reset.
     */
    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(1)
        );

    QCOMPARE(
        records.at(1)
            .message,
        std::optional<QString>(
            QStringLiteral(
                "Replacement record"
                )
            )
        );

    /*
     * The partial object retained from generation
     * zero must have been discarded when the new
     * physical generation began.
     */
    QVERIFY(
        !records.at(1)
             .rawSource
             .contains(
                 QStringLiteral(
                     "old-partial"
                     )
                 )
        );

    QVERIFY(
        coordinator.stop()
        );
}

QTEST_MAIN(
    LiveSessionFollowCoordinatorTests
    )

#include "LiveSessionFollowCoordinatorTests.moc"