#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/importing/JsonLinesImporter.h"
#include "../src/importing/StructuredJsonImporter.h"
#include "../src/importing/XmlImporter.h"
#include "../src/live/LiveSessionFollowCoordinator.h"
#include "../src/workspace/InvestigationSession.h"

namespace
{
ImportProfile jsonLinesProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "json-lines"
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

ImportProfile structuredXmlProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral("xml");

    profile.recordPath =
        QStringLiteral(
            "session.events.event"
            );

    profile.canonicalFields.subsystemPath =
        QStringLiteral(
            "metadata.component"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "details.message"
            );

    profile.preserveUnmappedFields =
        true;

    return profile;
}

ImportProfile windowsEventXmlProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral("xml");

    profile.recordPath =
        QStringLiteral(
            "Events.Event"
            );

    profile.canonicalFields.subsystemPath =
        QStringLiteral(
            "System.Provider.@Name"
            );

    profile.canonicalFields.eventCodePath =
        QStringLiteral(
            "EventData.NamedData.EventCode"
            );

    profile.canonicalFields.entityIdPath =
        QStringLiteral(
            "EventData.NamedData.DeviceId"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "RenderingInfo.Message"
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
    void supportsStructuredXmlProfile();

    void structuredJsonBaselineAndGrowthUseIncrementalSessionPath();
    void structuredJsonReplacementStartsNewSourceGeneration();

    void structuredXmlBaselineAndGrowthUseIncrementalSessionPath();
    void windowsEventXmlBaselineAndGrowthUseIncrementalSessionPath();
    void structuredXmlReplacementStartsNewSourceGeneration();
    void structuredXmlFirstStartConsumesPostImportGrowth();

    void stopStartPreservesPartialLineContext();
    void capturesLineInitialBoundaryAtCompleteLine();
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
            "unsupported-importer"
            );

    QVERIFY(
        !LiveSessionFollowCoordinator::
        supportsProfile(
            unsupported
            )
        );
}

void LiveSessionFollowCoordinatorTests::
    supportsStructuredXmlProfile()
{
    QVERIFY(
        LiveSessionFollowCoordinator::
        supportsProfile(
            structuredXmlProfile()
            )
        );

    QVERIFY(
        LiveSessionFollowCoordinator::
        supportsProfile(
            windowsEventXmlProfile()
            )
        );

    /*
     * XML document-root import remains valid for
     * ordinary static import, but live following
     * requires a repeating record path/container.
     */
    ImportProfile rootXml;

    rootXml.importerId =
        QStringLiteral("xml");

    QVERIFY(
        !LiveSessionFollowCoordinator::
        supportsProfile(
            rootXml
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

    /*
 * The session's external-source binding must track
 * the same runtime generation and physical identity
 * established by the live follower.
 *
 * This persisted-friendly state is what a future
 * Hybrid workspace restore will use to distinguish
 * same-generation replay from source replacement.
 */
    const InvestigationExternalSourceBinding
        *externalSource =
        session
            .backing()
            .externalSource();

    QVERIFY(
        externalSource != nullptr
        );

    if (!externalSource) {
        return;
    }

    QCOMPARE(
        externalSource->sourceGeneration,
        coordinator
            .state()
            .sourceGeneration()
        );

    QCOMPARE(
        externalSource->sourceGeneration,
        quint64(1)
        );

    QVERIFY(
        externalSource
            ->sourceIdentity
            .has_value()
        );

    if (!externalSource
             ->sourceIdentity
             .has_value()) {
        return;
    }

    QCOMPARE(
        externalSource
            ->sourceIdentity
            ->observedSizeBytes,
        QFileInfo(
            sourcePath
            ).size()
        );

    QVERIFY(
        !externalSource
             ->sourceIdentity
             ->prefixFingerprint
             .isEmpty()
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

void LiveSessionFollowCoordinatorTests::
    structuredXmlBaselineAndGrowthUseIncrementalSessionPath()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.xml"
                )
            );

    /*
     * One complete baseline record plus one
     * incomplete record. The outer XML record
     * container remains intentionally open.
     */
    const QByteArray baseline(
        "<?xml version=\"1.0\"?>"
        "<session>"
        "<events>"
        "<event>"
        "<metadata>"
        "<component>Payments</component>"
        "</metadata>"
        "<details>"
        "<message>first</message>"
        "</details>"
        "</event>"
        "<event>"
        "<metadata>"
        "<component>Checkout</component>"
        "</metadata>"
        "<details>"
        "<message>sec"
        );

    writeFile(
        sourcePath,
        baseline
        );

    const ImportProfile profile =
        structuredXmlProfile();

    XmlImporter importer(
        profile
        );

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
            QStringLiteral("first")
            )
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
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
     * Baseline reconstruction must not duplicate the
     * already-imported complete record.
     */
    QCOMPARE(
        session.processedRecordCount(),
        qint64(1)
        );

    QCOMPARE(
        session.importedRecordCount(),
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
     * Establish a filter before live growth. The
     * proxy must remain active during incremental
     * record insertion.
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

    const QByteArray appended(
        "ond</message>"
        "</details>"
        "</event>"
        "<event>"
        "<metadata>"
        "<component>Payments</component>"
        "</metadata>"
        "<details>"
        "<message>third</message>"
        "</details>"
        "</event>"
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

    const QVector<InvestigationRecord>
        records =
        session
            .investigationController()
            ->allRecords();

    QCOMPARE(
        records.size(),
        3
        );

    QCOMPARE(
        records.at(0)
            .source.recordNumber,
        qint64(1)
        );

    QCOMPARE(
        records.at(1)
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        records.at(2)
            .source.recordNumber,
        qint64(3)
        );

    QCOMPARE(
        records.at(0)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        records.at(1)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        records.at(2)
            .source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral("second")
            )
        );

    QCOMPARE(
        records.at(2).message,
        std::optional<QString>(
            QStringLiteral("third")
            )
        );

    /*
     * Checkout stays hidden. The newly appended
     * Payments record enters the existing proxy
     * automatically.
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
        visibleRecords.at(0).message,
        std::optional<QString>(
            QStringLiteral("first")
            )
        );

    QCOMPARE(
        visibleRecords.at(1).message,
        std::optional<QString>(
            QStringLiteral("third")
            )
        );

    QVERIFY(
        coordinator.stop()
        );
}

void LiveSessionFollowCoordinatorTests::
    windowsEventXmlBaselineAndGrowthUseIncrementalSessionPath()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "windows-events.xml"
                )
            );

    const QByteArray baseline(
        "<?xml version=\"1.0\"?>"
        "<Events>"
        "<Event "
        "xmlns=\"http://schemas.microsoft.com/"
        "win/2004/08/events/event\">"
        "<System>"
        "<Provider Name=\"CheckoutApi\"/>"
        "<EventID>4101</EventID>"
        "<Level>4</Level>"
        "</System>"
        "<EventData>"
        "<Data Name=\"EventCode\">STARTED</Data>"
        "<Data Name=\"DeviceId\">node-01</Data>"
        "</EventData>"
        "<RenderingInfo>"
        "<Message>Service started</Message>"
        "</RenderingInfo>"
        "</Event>"
        );

    writeFile(
        sourcePath,
        baseline
        );

    const ImportProfile profile =
        windowsEventXmlProfile();

    XmlImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
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
     * Starting follow must only reconstruct the open
     * <Events> container.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    const QByteArray appended(
        "<Event "
        "xmlns=\"http://schemas.microsoft.com/"
        "win/2004/08/events/event\">"
        "<System>"
        "<Provider Name=\"PaymentsWorker\"/>"
        "<EventID>4102</EventID>"
        "<Level>2</Level>"
        "</System>"
        "<EventData>"
        "<Data Name=\"EventCode\">PAYMENT_FAILURE</Data>"
        "<Data Name=\"DeviceId\">node-07</Data>"
        "</EventData>"
        "<RenderingInfo>"
        "<Message>Payment processing failed</Message>"
        "</RenderingInfo>"
        "</Event>"
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

    QCOMPARE(
        pollResult.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        pollResult.importedRecordCount,
        qint64(1)
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

    const InvestigationRecord &liveRecord =
        records.at(1);

    QCOMPARE(
        liveRecord.source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        liveRecord.source.sourceGeneration,
        quint64(0)
        );

    QCOMPARE(
        liveRecord.subsystem,
        std::optional<QString>(
            QStringLiteral(
                "PaymentsWorker"
                )
            )
        );

    QCOMPARE(
        liveRecord.eventCode,
        std::optional<QString>(
            QStringLiteral(
                "PAYMENT_FAILURE"
                )
            )
        );

    QCOMPARE(
        liveRecord.entityId,
        std::optional<QString>(
            QStringLiteral(
                "node-07"
                )
            )
        );

    QCOMPARE(
        liveRecord.message,
        std::optional<QString>(
            QStringLiteral(
                "Payment processing failed"
                )
            )
        );

    QVERIFY(
        coordinator.stop()
        );
}

void LiveSessionFollowCoordinatorTests::
    structuredXmlReplacementStartsNewSourceGeneration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.xml"
                )
            );

    /*
     * Deliberately make generation zero larger than
     * the replacement so truncation is unambiguous.
     */
    const QByteArray original(
        "<session>"
        "<events>"
        "<event>"
        "<details>"
        "<message>"
        "Original generation record with enough "
        "text to keep this source larger than its "
        "replacement"
        "</message>"
        "</details>"
        "</event>"
        "<event>"
        "<details>"
        "<message>old-partial"
        );

    writeFile(
        sourcePath,
        original
        );

    const ImportProfile profile =
        structuredXmlProfile();

    XmlImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
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

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    const QByteArray replacement(
        "<session>"
        "<events>"
        "<event>"
        "<details>"
        "<message>Replacement record</message>"
        "</details>"
        "</event>"
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
     * Replacement generation starts source-local
     * numbering again at one while preserving the
     * historical generation-zero evidence.
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
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "Replacement record"
                )
            )
        );

    /*
     * Parser state retained from generation zero
     * must not contaminate the replacement.
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

void LiveSessionFollowCoordinatorTests::
    structuredXmlFirstStartConsumesPostImportGrowth()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.xml"
                )
            );

    const QByteArray initialContent(
        "<?xml version=\"1.0\"?>\n"
        "<session>\n"
        "<events>\n"
        "<event>"
        "<metadata>"
        "<component>Gateway</component>"
        "</metadata>"
        "<details>"
        "<message>first</message>"
        "</details>"
        "</event>\n"
        );

    writeFile(
        sourcePath,
        initialContent
        );

    const ImportProfile profile =
        structuredXmlProfile();

    /*
     * Capture the application handoff boundary before
     * the real import begins.
     */
    const qint64 importBoundary =
        LiveSessionFollowCoordinator::
        captureInitialReadOffset(
            sourcePath,
            profile
            );

    QCOMPARE(
        importBoundary,
        qint64(
            initialContent.size()
            )
        );

    XmlImporter importer(
        profile
        );

    ImportResult initialResult =
        importer.importFile(
            sourcePath
            );

    QCOMPARE(
        initialResult.records.size(),
        1
        );

    /*
     * This record arrives after static import has
     * completed but before the user starts live
     * following. It must not disappear into the
     * handoff gap.
     */
    const QByteArray growthBeforeStart(
        "<event>"
        "<metadata>"
        "<component>Gateway</component>"
        "</metadata>"
        "<details>"
        "<message>second</message>"
        "</details>"
        "</event>\n"
        );

    appendFile(
        sourcePath,
        growthBeforeStart
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    session.setInitialLiveFollowByteOffset(
        importBoundary
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

    QCOMPARE(
        startResult.baselineByteCount,
        importBoundary
        );

    /*
     * Starting itself reconstructs parser state but
     * does not yet consume the unread overlap/growth.
     */
    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
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
        records.at(0).message,
        std::optional<QString>(
            QStringLiteral("first")
            )
        );

    QCOMPARE(
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral("second")
            )
        );

    QVERIFY(
        coordinator.stop()
        );
}

void LiveSessionFollowCoordinatorTests::
    stopStartPreservesPartialLineContext()
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
        "{\"message\":\"baseline\","
        "\"subsystem\":\"API\"}\n"
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
        1
        );

    InvestigationSession session(
        sourcePath,
        profile,
        std::move(
            initialResult
            )
        );

    LiveSessionFollowCoordinator coordinator(
        session
        );

    const LiveSessionFollowStartResult
        firstStart =
        coordinator.start();

    QVERIFY2(
        firstStart.succeeded,
        qPrintable(
            firstStart.errorMessage
            )
        );

    /*
     * Consume only the beginning of a new physical
     * line. The follower cursor advances, but the
     * line framer deliberately retains the incomplete
     * record.
     */
    const QByteArray partialRecord(
        "{\"message\":\"arrived "
        "across stop"
        );

    appendFile(
        sourcePath,
        partialRecord
        );

    const LiveSessionFollowPollResult
        partialPoll =
        coordinator.pollOnce();

    QVERIFY2(
        partialPoll.succeeded,
        qPrintable(
            partialPoll.errorMessage
            )
        );

    QCOMPARE(
        partialPoll.processedRecordCount,
        qint64(0)
        );

    QCOMPARE(
        session.importedRecordCount(),
        qint64(1)
        );

    QVERIFY(
        coordinator.stop()
        );

    /*
     * Complete that same physical line while live
     * following is stopped.
     */
    const QByteArray stoppedGrowth(
        "\",\"subsystem\":\"API\"}\n"
        );

    appendFile(
        sourcePath,
        stoppedGrowth
        );

    const LiveSessionFollowStartResult
        restartResult =
        coordinator.start();

    QVERIFY2(
        restartResult.succeeded,
        qPrintable(
            restartResult.errorMessage
            )
        );

    QVERIFY(
        !restartResult.sourceGenerationChanged
        );

    /*
     * Start must not rescan/reject the retained
     * partial-line boundary. The existing framer is
     * still authoritative for this generation.
     */
    const LiveSessionFollowPollResult
        catchUpPoll =
        coordinator.pollOnce();

    QVERIFY2(
        catchUpPoll.succeeded,
        qPrintable(
            catchUpPoll.errorMessage
            )
        );

    QCOMPARE(
        catchUpPoll.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        catchUpPoll.importedRecordCount,
        qint64(1)
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
        records.at(1).message,
        std::optional<QString>(
            QStringLiteral(
                "arrived across stop"
                )
            )
        );

    QCOMPARE(
        records.at(1)
            .source
            .sourceGeneration,
        quint64(0)
        );

    QVERIFY(
        coordinator.stop()
        );
}

void LiveSessionFollowCoordinatorTests::
    capturesLineInitialBoundaryAtCompleteLine()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "partial.jsonl"
                )
            );

    const QByteArray content(
        "{\"message\":\"first\"}\n"
        "{\"message\":\"part"
        );

    writeFile(
        sourcePath,
        content
        );

    const qint64 boundary =
        LiveSessionFollowCoordinator::
        captureInitialReadOffset(
            sourcePath,
            jsonLinesProfile()
            );

    QCOMPARE(
        boundary,
        qint64(
            QByteArray(
                "{\"message\":\"first\"}\n"
                ).size()
            )
        );
}

QTEST_MAIN(
    LiveSessionFollowCoordinatorTests
    )

#include "LiveSessionFollowCoordinatorTests.moc"