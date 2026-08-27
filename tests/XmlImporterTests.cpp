#include <QtTest/QtTest>

#include <QTemporaryFile>
#include <QFileInfo>
#include <QVector>

#include "../src/importing/XmlImporter.h"

namespace
{
const ImportDiagnostic *findDiagnostic(
    const ImportResult &result,
    const QString &code
    )
{
    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        if (diagnostic.code == code) {
            return &diagnostic;
        }
    }

    return nullptr;
}

QString writeTemporaryContent(
    QTemporaryFile &file,
    const QByteArray &content
    )
{
    if (!file.open()) {
        return {};
    }

    if (file.write(content)
        != content.size()) {
        return {};
    }

    file.flush();

    const QString path =
        file.fileName();

    file.close();

    return path;
}

ImportProfile xmlProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "Structured XML Test"
            );

    profile.importerId =
        QStringLiteral("xml");

    return profile;
}
}

class XmlImporterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void hasStableImporterIdentity();
    void importsDocumentRoot();
    void importsRecordsAtConfiguredPath();
    void mapsNestedCanonicalFields();
    void mapsAttributes();
    void mapsCanonicalTextFromElementWithAttributes();
    void preservesUnmappedFields();
    void convertsRepeatedElementsToArray();
    void preservesMixedElementText();
    void reportsMissingRecordPath();
    void reportsMalformedXml();
    void honorsRecordLimit();
    void importFilePreservesSourceMetadata();
    void importFileReportsOpenFailure();
    void importFileReportsProgress();
    void importFileCanBeCancelled();
    void convertsNamedDataElementsToFields();
    void preservesRepeatedNamedDataValues();
};

void
    XmlImporterTests::
    hasStableImporterIdentity()
{
    XmlImporter importer;

    QCOMPARE(
        importer.id(),
        QStringLiteral("xml")
        );

    QCOMPARE(
        importer.displayName(),
        QStringLiteral(
            "Structured XML"
            )
        );
}

void
    XmlImporterTests::
    importsDocumentRoot()
{
    ImportProfile profile =
        xmlProfile();

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<event>"
                "<message>System started</message>"
                "</event>"
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

    QVERIFY(
        result.records.first()
            .message.has_value()
        );

    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral(
            "System started"
            )
        );

    QVERIFY(
        result.records.first()
            .rawSource.contains(
                QStringLiteral("<event>")
                )
        );
}

void
    XmlImporterTests::
    importsRecordsAtConfiguredPath()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.events.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<events>"
                "<event>"
                "<message>First</message>"
                "</event>"
                "<event>"
                "<message>Second</message>"
                "</event>"
                "</events>"
                "</session>"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .message.value(),
        QStringLiteral("First")
        );

    QCOMPARE(
        result.records.at(1)
            .message.value(),
        QStringLiteral("Second")
        );
}

void
    XmlImporterTests::
    mapsNestedCanonicalFields()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.timestampPath =
        QStringLiteral(
            "metadata.timestamp"
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

    profile.canonicalFields.entityIdPath =
        QStringLiteral(
            "context.deviceId"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "details.message"
            );

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<metadata>"
                "<timestamp>"
                "2026-08-13T08:15:00Z"
                "</timestamp>"
                "<level>WARN</level>"
                "<component>Tracking</component>"
                "<code>TRACK_DELAY</code>"
                "</metadata>"
                "<context>"
                "<deviceId>SENSOR-42</deviceId>"
                "</context>"
                "<details>"
                "<message>"
                "Tracking update delayed"
                "</message>"
                "</details>"
                "</event>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QVERIFY(
        record.timestamp.has_value()
        );

    QCOMPARE(
        record.severity.value(),
        RecordSeverity::Warning
        );

    QCOMPARE(
        record.subsystem.value(),
        QStringLiteral("Tracking")
        );

    QCOMPARE(
        record.eventCode.value(),
        QStringLiteral("TRACK_DELAY")
        );

    QCOMPARE(
        record.entityId.value(),
        QStringLiteral("SENSOR-42")
        );

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Tracking update delayed"
            )
        );
}

void
    XmlImporterTests::
    mapsAttributes()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "message.#text"
            );

    profile.customFields.append({
        QStringLiteral("Host"),
        QStringLiteral(
            "message.@host"
            )
    });

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message host=\"api-02\">"
                "Request completed"
                "</message>"
                "</event>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.message.value(),
        QStringLiteral(
            "Request completed"
            )
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("Host")
                )
            .toString(),
        QStringLiteral("api-02")
        );
}

void
    XmlImporterTests::
    mapsCanonicalTextFromElementWithAttributes()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "Events.Event"
            );

    profile.canonicalFields.eventCodePath =
        QStringLiteral(
            "System.EventID"
            );

    profile.canonicalFields.messagePath.clear();

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<Events>"
                "<Event>"
                "<System>"
                "<EventID Qualifiers=\"16384\">"
                "255"
                "</EventID>"
                "</System>"
                "</Event>"
                "</Events>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        result.records.first()
            .eventCode.has_value()
        );

    QCOMPARE(
        result.records.first()
            .eventCode.value(),
        QStringLiteral("255")
        );
}

void
    XmlImporterTests::
    preservesUnmappedFields()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    profile.preserveUnmappedFields =
        true;

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message>Processed</message>"
                "<host>worker-03</host>"
                "<latencyMs>482</latencyMs>"
                "</event>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("host")
                )
            .toString(),
        QStringLiteral("worker-03")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral("latencyMs")
                )
            .toString(),
        QStringLiteral("482")
        );
}

void
    XmlImporterTests::
    convertsRepeatedElementsToArray()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    profile.preserveUnmappedFields =
        true;

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message>Request processed</message>"
                "<tag>network</tag>"
                "<tag>latency</tag>"
                "<tag>production</tag>"
                "</event>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const QVariant tags =
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral("tag")
                );

    QVERIFY(
        tags.canConvert<QVariantList>()
        );

    const QVariantList tagValues =
        tags.toList();

    QCOMPARE(
        tagValues.size(),
        3
        );

    QCOMPARE(
        tagValues.at(0).toString(),
        QStringLiteral("network")
        );

    QCOMPARE(
        tagValues.at(1).toString(),
        QStringLiteral("latency")
        );

    QCOMPARE(
        tagValues.at(2).toString(),
        QStringLiteral("production")
        );
}

void
    XmlImporterTests::
    preservesMixedElementText()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral(
            "message.#text"
            );

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message category=\"network\">"
                "Connection "
                "<detail>timed out</detail>"
                "</message>"
                "</event>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    /*
     * #text preserves the direct text belonging
     * to the element. Child content remains
     * available separately under its element.
     */
    QCOMPARE(
        result.records.first()
            .message.value(),
        QStringLiteral("Connection")
        );

    QCOMPARE(
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral(
                    "message.detail"
                    )
                )
            .toString(),
        QStringLiteral("timed out")
        );
}

void
    XmlImporterTests::
    reportsMissingRecordPath()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.records.record"
            );

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<events>"
                "<event />"
                "</events>"
                "</session>"
                )
            );

    QCOMPARE(
        result.records.size(),
        0
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "XML_RECORD_PATH_NOT_FOUND"
                )
            )
        != nullptr
        );
}

void
    XmlImporterTests::
    reportsMalformedXml()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message>Broken</message>"
                "</session>"
                )
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(1)
        );

    QCOMPARE(
        result.importedRecordCount(),
        qint64(0)
        );

    QCOMPARE(
        result.skippedRecordCount(),
        qint64(1)
        );

    QVERIFY(
        result.hasErrors()
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "XML_PARSE_ERROR"
                )
            )
        != nullptr
        );
}

void
    XmlImporterTests::
    honorsRecordLimit()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<session>"
                "<event><message>One</message></event>"
                "<event><message>Two</message></event>"
                "<event><message>Three</message></event>"
                "</session>"
                ),
            QString(),
            2
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QVERIFY(
        result.sourceTruncated
        );

    QCOMPARE(
        result.records.at(1)
            .message.value(),
        QStringLiteral("Two")
        );
}

void
    XmlImporterTests::
    importFilePreservesSourceMetadata()
{
    QTemporaryFile file;

    const QString filePath =
        writeTemporaryContent(
            file,
            QByteArrayLiteral(
                "<session>"
                "<event>"
                "<message>First</message>"
                "</event>"
                "<event>"
                "<message>Second</message>"
                "</event>"
                "</session>"
                )
            );

    QVERIFY(
        !filePath.isEmpty()
        );

    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importFile(
            filePath
            );

    QCOMPARE(
        result.records.size(),
        2
        );

    QCOMPARE(
        result.records.at(0)
            .source
            .sourcePath,
        filePath
        );

    QCOMPARE(
        result.records.at(0)
            .source
            .recordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.records.at(1)
            .source
            .recordNumber,
        qint64(2)
        );

    QVERIFY(
        !result.records.at(0)
             .recordId
             .isEmpty()
        );
}

void
    XmlImporterTests::
    importFileReportsOpenFailure()
{
    ImportProfile profile =
        xmlProfile();

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importFile(
            QStringLiteral(
                "missing-xml-source-file.xml"
                )
            );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                )
            )
        != nullptr
        );
}

void
    XmlImporterTests::
    importFileReportsProgress()
{
    QTemporaryFile file;

    const QString filePath =
        writeTemporaryContent(
            file,
            QByteArrayLiteral(
                "<session>"
                "<event><message>First</message></event>"
                "<event><message>Second</message></event>"
                "</session>"
                )
            );

    QVERIFY(
        !filePath.isEmpty()
        );

    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    QVector<ImportProgress> progress;

    ImportExecutionContext context;

    context.reportProgress =
        [&progress](
            const ImportProgress &value
            ) {
            progress.append(value);
        };

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importFile(
            filePath,
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        2
        );

    QVERIFY(
        !result.cancelled
        );

    QVERIFY(
        !progress.isEmpty()
        );

    QCOMPARE(
        progress.first().bytesProcessed,
        qint64(0)
        );

    QCOMPARE(
        progress.last().bytesProcessed,
        QFileInfo(filePath).size()
        );

    QCOMPARE(
        progress.last().totalBytes,
        QFileInfo(filePath).size()
        );

    QCOMPARE(
        progress.last().processedRecordCount,
        qint64(2)
        );
}

void
    XmlImporterTests::
    importFileCanBeCancelled()
{
    QTemporaryFile file;

    const QString filePath =
        writeTemporaryContent(
            file,
            QByteArrayLiteral(
                "<session>"
                "<event><message>First</message></event>"
                "<event><message>Second</message></event>"
                "</session>"
                )
            );

    QVERIFY(
        !filePath.isEmpty()
        );

    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "session.event"
            );

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    ImportExecutionContext context;

    context.isCancellationRequested =
        []() {
            return true;
        };

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importFile(
            filePath,
            ILogImporter::UnlimitedRecordLimit,
            context
            );

    QVERIFY(
        result.cancelled
        );

    QCOMPARE(
        result.processedRecordCount,
        qint64(0)
        );

    QCOMPARE(
        result.records.size(),
        0
        );

    QVERIFY(
        !result.sourceTruncated
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "XML_RECORD_PATH_NOT_FOUND"
                )
            )
        == nullptr
        );

    QVERIFY(
        findDiagnostic(
            result,
            QStringLiteral(
                "XML_DOCUMENT_EMPTY"
                )
            )
        == nullptr
        );
}

void
    XmlImporterTests::
    convertsNamedDataElementsToFields()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "Events.Event"
            );

    profile.canonicalFields.messagePath.clear();

    profile.preserveUnmappedFields =
        true;

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<Events>"
                "<Event>"
                "<EventData>"
                "<Data Name=\"ProcessName\">"
                "worker.exe"
                "</Data>"
                "<Data Name=\"ProcessId\">"
                "4812"
                "</Data>"
                "<Data Name=\"Result\">"
                "Timeout"
                "</Data>"
                "</EventData>"
                "</Event>"
                "</Events>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const InvestigationRecord &record =
        result.records.first();

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.ProcessName"
                    )
                )
            .toString(),
        QStringLiteral("worker.exe")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.ProcessId"
                    )
                )
            .toString(),
        QStringLiteral("4812")
        );

    QCOMPARE(
        record.customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.Result"
                    )
                )
            .toString(),
        QStringLiteral("Timeout")
        );
}

void
    XmlImporterTests::
    preservesRepeatedNamedDataValues()
{
    ImportProfile profile =
        xmlProfile();

    profile.recordPath =
        QStringLiteral(
            "Events.Event"
            );

    profile.canonicalFields.messagePath.clear();

    profile.preserveUnmappedFields =
        true;

    XmlImporter importer(profile);

    const ImportResult result =
        importer.importContent(
            QByteArrayLiteral(
                "<Events>"
                "<Event>"
                "<EventData>"
                "<Data Name=\"Tag\">network</Data>"
                "<Data Name=\"Tag\">latency</Data>"
                "</EventData>"
                "</Event>"
                "</Events>"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    const QVariant value =
        result.records.first()
            .customAttributes
            .value(
                QStringLiteral(
                    "EventData.NamedData.Tag"
                    )
                );

    QVERIFY(
        value.canConvert<QVariantList>()
        );

    const QVariantList values =
        value.toList();

    QCOMPARE(
        values.size(),
        2
        );

    QCOMPARE(
        values.at(0).toString(),
        QStringLiteral("network")
        );

    QCOMPARE(
        values.at(1).toString(),
        QStringLiteral("latency")
        );
}

QTEST_MAIN(XmlImporterTests)

#include "XmlImporterTests.moc"