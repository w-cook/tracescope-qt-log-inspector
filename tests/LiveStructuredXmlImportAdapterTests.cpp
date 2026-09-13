#include <QtTest>

#include <optional>

#include "../src/live/LiveStructuredXmlImportAdapter.h"

namespace
{
ImportProfile structuredXmlProfile()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral("xml");

    profile.recordPath =
        QStringLiteral(
            "session.events.event"
            );

    profile.canonicalFields.severityPath =
        QStringLiteral(
            "metadata.level"
            );

    profile.canonicalFields.subsystemPath =
        QStringLiteral(
            "metadata.component"
            );

    profile.canonicalFields.eventCodePath =
        QStringLiteral(
            "metadata.code"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "details.message"
            );

    profile.preserveUnmappedFields =
        true;

    return profile;
}

ImportProfile windowsEventProfile()
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

    return profile;
}
}

class LiveStructuredXmlImportAdapterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void recognizesXmlProfile();
    void importsStructuredXmlWithLiveSourcePosition();
    void importsWindowsEventRecord();
};

void LiveStructuredXmlImportAdapterTests::
    recognizesXmlProfile()
{
    LiveStructuredXmlImportAdapter supported(
        structuredXmlProfile()
        );

    QVERIFY(
        supported.isSupported()
        );

    ImportProfile unsupported;

    unsupported.importerId =
        QStringLiteral(
            "structured-json"
            );

    unsupported.recordPath =
        QStringLiteral(
            "data.records"
            );

    LiveStructuredXmlImportAdapter other(
        unsupported
        );

    QVERIFY(
        !other.isSupported()
        );
}

void LiveStructuredXmlImportAdapterTests::
    importsStructuredXmlWithLiveSourcePosition()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "session.events.event"
            )
        );

    const auto parsed =
        parser.appendBytes(
            QByteArray(
                "<session>"
                "<events>"
                "<event>"
                "<metadata>"
                "<level>WARN</level>"
                "<component>Payments</component>"
                "<code>PAYMENT_SLOW</code>"
                "</metadata>"
                "<details>"
                "<message>Live XML record</message>"
                "</details>"
                "<latencyMs>1840</latencyMs>"
                "</event>"
                )
            );

    QVERIFY(parsed.succeeded);

    QCOMPARE(
        parsed.records.size(),
        1
        );

    LiveStructuredXmlRecordBatch batch;

    batch.firstRecordNumber = 14;
    batch.sourceGeneration = 3;
    batch.records = parsed.records;

    LiveStructuredXmlImportAdapter adapter(
        structuredXmlProfile()
        );

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "session.xml"
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
            "session.xml"
            )
        );

    QCOMPARE(
        record.message,
        std::optional<QString>(
            QStringLiteral(
                "Live XML record"
                )
            )
        );

    QCOMPARE(
        record.subsystem,
        std::optional<QString>(
            QStringLiteral("Payments")
            )
        );
}

void LiveStructuredXmlImportAdapterTests::
    importsWindowsEventRecord()
{
    StructuredXmlRecordStreamParser parser(
        QStringLiteral(
            "Events.Event"
            )
        );

    const auto parsed =
        parser.appendBytes(
            QByteArray(
                "<Events>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<Provider Name=\"CheckoutApi\"/>"
                "</System>"
                "<EventData>"
                "<Data Name=\"EventCode\">FAILURE</Data>"
                "<Data Name=\"DeviceId\">node-07</Data>"
                "</EventData>"
                "<RenderingInfo>"
                "<Message>Operation failed</Message>"
                "</RenderingInfo>"
                "</Event>"
                )
            );

    QVERIFY(parsed.succeeded);

    QCOMPARE(
        parsed.records.size(),
        1
        );

    LiveStructuredXmlRecordBatch batch;

    batch.firstRecordNumber = 7;
    batch.sourceGeneration = 2;
    batch.records = parsed.records;

    LiveStructuredXmlImportAdapter adapter(
        windowsEventProfile()
        );

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "events.xml"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.source.recordNumber,
        qint64(7)
        );

    QCOMPARE(
        record.source.sourceGeneration,
        quint64(2)
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral(
            "CheckoutApi"
            )
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral(
            "FAILURE"
            )
        );

    QCOMPARE(
        record.entityId.value(),
        QStringLiteral(
            "node-07"
            )
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Operation failed"
            )
        );
}

QTEST_MAIN(
    LiveStructuredXmlImportAdapterTests
    )

#include "LiveStructuredXmlImportAdapterTests.moc"