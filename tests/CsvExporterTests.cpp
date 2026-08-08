#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryFile>

#include "../src/exporting/InvestigationCsvExporter.h"

class CsvExporterTests : public QObject
{
    Q_OBJECT

private slots:
    void exportToFileWritesCanonicalFields();
    void exportToFileIncludesDynamicCustomAttributes();
    void exportToFileUsesDeterministicCustomColumnOrder();
    void exportToFileEscapesCanonicalAndCustomValues();
    void exportToFileSerializesStructuredCustomValues();
};

void CsvExporterTests::exportToFileWritesCanonicalFields()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    InvestigationRecord record;
    record.timestamp =
        QDateTime::fromString(
            "2026-07-07T10:14:22.381Z",
            Qt::ISODateWithMs
            );
    record.severity = RecordSeverity::Warning;
    record.subsystem = QStringLiteral("Tracking");
    record.eventCode = QStringLiteral("TRACK_LOST");
    record.entityId = QStringLiteral("TRK-402");
    record.message =
        QStringLiteral(
            "Track 402 lost for 1200ms"
            );

    const QVector<InvestigationRecord> records {
        record
    };

    InvestigationCsvExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            records,
            file.fileName()
            )
        );

    QFile output(file.fileName());

    QVERIFY(
        output.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )
        );

    const QString content =
        QString::fromUtf8(
            output.readAll()
            );

    QVERIFY(
        content.contains(
            "timestamp,level,subsystem,"
            "eventCode,entityId,message"
            )
        );

    QVERIFY(
        content.contains(
            "2026-07-07T10:14:22.381Z,"
            "WARN,"
            "Tracking,"
            "TRACK_LOST,"
            "TRK-402,"
            "Track 402 lost for 1200ms"
            )
        );
}

void CsvExporterTests::
    exportToFileIncludesDynamicCustomAttributes()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    InvestigationRecord firstRecord;
    firstRecord.timestamp =
        QDateTime::fromString(
            "2026-08-08T14:00:00.000Z",
            Qt::ISODateWithMs
            );
    firstRecord.severity =
        RecordSeverity::Info;
    firstRecord.subsystem =
        QStringLiteral("Gateway");
    firstRecord.eventCode =
        QStringLiteral("START");
    firstRecord.message =
        QStringLiteral("Gateway started");

    firstRecord.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("edge-gw-01")
        );

    firstRecord.customAttributes.insert(
        QStringLiteral("environment"),
        QStringLiteral("staging")
        );

    InvestigationRecord secondRecord;
    secondRecord.timestamp =
        QDateTime::fromString(
            "2026-08-08T14:01:00.000Z",
            Qt::ISODateWithMs
            );
    secondRecord.severity =
        RecordSeverity::Error;
    secondRecord.subsystem =
        QStringLiteral("Payments");
    secondRecord.eventCode =
        QStringLiteral("TIMEOUT");
    secondRecord.message =
        QStringLiteral("Provider timed out");

    secondRecord.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("api-03")
        );

    secondRecord.customAttributes.insert(
        QStringLiteral("latencyMs"),
        5032
        );

    secondRecord.customAttributes.insert(
        QStringLiteral("provider"),
        QStringLiteral("sandbox-payments")
        );

    const QVector<InvestigationRecord> records {
        firstRecord,
        secondRecord
    };

    InvestigationCsvExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            records,
            file.fileName()
            )
        );

    QFile output(file.fileName());

    QVERIFY(
        output.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )
        );

    const QString content =
        QString::fromUtf8(
            output.readAll()
            );

    const QStringList lines =
        content.split(
            '\n',
            Qt::SkipEmptyParts
            );

    QCOMPARE(
        lines.size(),
        3
        );

    QCOMPARE(
        lines[0],
        QString(
            "timestamp,level,subsystem,"
            "eventCode,entityId,message,"
            "environment,host,latencyMs,provider"
            )
        );

    QCOMPARE(
        lines[1],
        QString(
            "2026-08-08T14:00:00.000Z,"
            "INFO,"
            "Gateway,"
            "START,,"
            "Gateway started,"
            "staging,"
            "edge-gw-01,,"
            )
        );

    QCOMPARE(
        lines[2],
        QString(
            "2026-08-08T14:01:00.000Z,"
            "ERROR,"
            "Payments,"
            "TIMEOUT,,"
            "Provider timed out,,"
            "api-03,"
            "5032,"
            "sandbox-payments"
            )
        );
}

void CsvExporterTests::
    exportToFileUsesDeterministicCustomColumnOrder()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    InvestigationRecord record;

    record.customAttributes.insert(
        QStringLiteral("zeta"),
        QStringLiteral("last")
        );

    record.customAttributes.insert(
        QStringLiteral("Alpha"),
        QStringLiteral("first")
        );

    record.customAttributes.insert(
        QStringLiteral("middle"),
        QStringLiteral("center")
        );

    const QVector<InvestigationRecord> records {
        record
    };

    InvestigationCsvExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            records,
            file.fileName()
            )
        );

    QFile output(file.fileName());

    QVERIFY(
        output.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )
        );

    const QString header =
        QString::fromUtf8(
            output.readLine()
            ).trimmed();

    QCOMPARE(
        header,
        QString(
            "timestamp,level,subsystem,"
            "eventCode,entityId,message,"
            "Alpha,middle,zeta"
            )
        );
}

void CsvExporterTests::
    exportToFileEscapesCanonicalAndCustomValues()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    InvestigationRecord record;
    record.severity =
        RecordSeverity::Error;
    record.message =
        QStringLiteral(
            "Packet loss exceeded threshold, "
            "\"critical\""
            );

    record.customAttributes.insert(
        QStringLiteral("note"),
        QStringLiteral(
            "Retry, \"manual review\""
            )
        );

    const QVector<InvestigationRecord> records {
        record
    };

    InvestigationCsvExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            records,
            file.fileName()
            )
        );

    QFile output(file.fileName());

    QVERIFY(
        output.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )
        );

    const QString content =
        QString::fromUtf8(
            output.readAll()
            );

    QVERIFY(
        content.contains(
            "\"Packet loss exceeded threshold, "
            "\"\"critical\"\"\""
            )
        );

    QVERIFY(
        content.contains(
            "\"Retry, \"\"manual review\"\"\""
            )
        );
}

void CsvExporterTests::
    exportToFileSerializesStructuredCustomValues()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    InvestigationRecord record;

    QVariantMap details;
    details.insert(
        QStringLiteral("attempt"),
        2
        );
    details.insert(
        QStringLiteral("status"),
        QStringLiteral("retry")
        );

    record.customAttributes.insert(
        QStringLiteral("details"),
        details
        );

    QVariantList tags;
    tags.append(
        QStringLiteral("network")
        );
    tags.append(
        QStringLiteral("timeout")
        );

    record.customAttributes.insert(
        QStringLiteral("tags"),
        tags
        );

    const QVector<InvestigationRecord> records {
        record
    };

    InvestigationCsvExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            records,
            file.fileName()
            )
        );

    QFile output(file.fileName());

    QVERIFY(
        output.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )
        );

    const QString content =
        QString::fromUtf8(
            output.readAll()
            );

    QVERIFY(
        content.contains(
            "\"{\"\"attempt\"\":2,"
            "\"\"status\"\":\"\"retry\"\"}\""
            )
        );

    QVERIFY(
        content.contains(
            "\"[\"\"network\"\","
            "\"\"timeout\"\"]\""
            )
        );
}

QTEST_MAIN(CsvExporterTests)

#include "CsvExporterTests.moc"