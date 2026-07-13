#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>

#include "../src/exporting/TelemetryCsvExporter.h"

class CsvExporterTests : public QObject
{
    Q_OBJECT

private slots:
    void exportToFileWritesHeaderAndEvents();
    void exportToFileEscapesCommasAndQuotes();
};

void CsvExporterTests::exportToFileWritesHeaderAndEvents()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    QVector<TelemetryEvent> events = {
        {
            "2026-07-07T10:14:22.381Z",
            "WARN",
            "Tracking",
            "TRACK_LOST",
            "Track 402 lost for 1200ms",
            "TRK-402"
        }
    };

    TelemetryCsvExporter exporter;

    QVERIFY(exporter.exportToFile(events, file.fileName()));

    QFile output(file.fileName());
    QVERIFY(output.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString content = QString::fromUtf8(output.readAll());

    QVERIFY(content.contains("timestamp,level,subsystem,eventCode,entityId,message"));
    QVERIFY(content.contains("2026-07-07T10:14:22.381Z,WARN,Tracking,TRACK_LOST,TRK-402,Track 402 lost for 1200ms"));
}

void CsvExporterTests::exportToFileEscapesCommasAndQuotes()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();

    QVector<TelemetryEvent> events = {
        {
            "2026-07-07T10:14:24.219Z",
            "ERROR",
            "Comms",
            "PACKET_DROP",
            "Packet loss exceeded threshold, \"critical\"",
            "LINK-A"
        }
    };

    TelemetryCsvExporter exporter;

    QVERIFY(exporter.exportToFile(events, file.fileName()));

    QFile output(file.fileName());
    QVERIFY(output.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString content = QString::fromUtf8(output.readAll());

    QVERIFY(content.contains("\"Packet loss exceeded threshold, \"\"critical\"\"\""));
}

QTEST_MAIN(CsvExporterTests)

#include "CsvExporterTests.moc"