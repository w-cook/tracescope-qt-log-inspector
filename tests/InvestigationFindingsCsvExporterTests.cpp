#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryFile>
#include <QVariantList>
#include <QVariantMap>

#include "../src/exporting/InvestigationFindingsCsvExporter.h"

namespace
{
QString readFile(
    const QString &filePath
    )
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            | QIODevice::Text
            )) {
        return {};
    }

    return QString::fromUtf8(
        file.readAll()
        );
}

InvestigationFindingExport makeFinding(
    const QString &recordId,
    FindingStatus status,
    qint64 recordNumber
    )
{
    InvestigationFindingExport finding;

    finding.status = status;

    finding.record.recordId =
        recordId;

    finding.record.source.sourceName =
        QStringLiteral("session.jsonl");

    finding.record.source.sourcePath =
        QStringLiteral(
            "C:/logs/session.jsonl"
            );

    finding.record.source.recordNumber =
        recordNumber;

    return finding;
}
}

class InvestigationFindingsCsvExporterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void writesFindingAndSupportingRecordFields();
    void escapesNotesMessagesAndRawSource();
    void includesDeterministicCustomAttributeColumns();
    void preservesSnapshotOrder();
    void writesStableHeaderForEmptySnapshot();
};

void
    InvestigationFindingsCsvExporterTests::
    writesFindingAndSupportingRecordFields()
{
    QTemporaryFile file;

    QVERIFY(file.open());
    file.close();

    InvestigationFindingExport finding =
        makeFinding(
            QStringLiteral("record-42"),
            FindingStatus::Open,
            42
            );

    finding.note =
        QStringLiteral(
            "Investigate provider timeout"
            );

    finding.bookmarked = true;

    finding.record.timestamp =
        QDateTime::fromString(
            "2026-09-02T12:30:04.125Z",
            Qt::ISODateWithMs
            );

    finding.record.severity =
        RecordSeverity::Error;

    finding.record.subsystem =
        QStringLiteral("Payments");

    finding.record.eventCode =
        QStringLiteral("DB_TIMEOUT");

    finding.record.entityId =
        QStringLiteral("order-1842");

    finding.record.message =
        QStringLiteral(
            "Database request timed out"
            );

    InvestigationFindingsCsvExporter
        exporter;

    QVERIFY(
        exporter.exportToFile(
            {finding},
            file.fileName()
            )
        );

    const QString content =
        readFile(file.fileName());

    QVERIFY(
        content.startsWith(
            "Finding Status,Analyst Note,"
            "Bookmarked,Record ID,Timestamp,"
            "Severity,Subsystem,Event Code,"
            "Entity ID,Message,Source Name,"
            "Source Path,Source Record"
            )
        );

    QVERIFY(
        content.contains(
            "OPEN,"
            "Investigate provider timeout,"
            "true,"
            "record-42,"
            "2026-09-02T12:30:04.125Z,"
            "ERROR,"
            "Payments,"
            "DB_TIMEOUT,"
            "order-1842,"
            "Database request timed out,"
            "session.jsonl,"
            "C:/logs/session.jsonl,"
            "42"
            )
        );
}

void
    InvestigationFindingsCsvExporterTests::
    escapesNotesMessagesAndRawSource()
{
    QTemporaryFile file;

    QVERIFY(file.open());
    file.close();

    InvestigationFindingExport finding =
        makeFinding(
            QStringLiteral("escaped"),
            FindingStatus::Resolved,
            7
            );

    finding.note =
        QStringLiteral(
            "Confirmed, \"retry succeeded\"\n"
            "No further action"
            );

    finding.record.message =
        QStringLiteral(
            "Timeout, then recovery"
            );

    finding.record.rawSource =
        QStringLiteral(
            "{\"message\":\"timeout, "
            "then recovery\"}\n"
            "{\"message\":\"healthy\"}"
            );

    InvestigationFindingsCsvExporter
        exporter;

    QVERIFY(
        exporter.exportToFile(
            {finding},
            file.fileName()
            )
        );

    const QString content =
        readFile(file.fileName());

    QVERIFY(
        content.contains(
            "\"Confirmed, \"\"retry succeeded\"\"\n"
            "No further action\""
            )
        );

    QVERIFY(
        content.contains(
            "\"Timeout, then recovery\""
            )
        );

    QVERIFY(
        content.contains(
            "\"{\"\"message\"\":\"\"timeout, "
            "then recovery\"\"}\n"
            "{\"\"message\"\":\"\"healthy\"\"}\""
            )
        );
}

void
    InvestigationFindingsCsvExporterTests::
    includesDeterministicCustomAttributeColumns()
{
    QTemporaryFile file;

    QVERIFY(file.open());
    file.close();

    InvestigationFindingExport finding =
        makeFinding(
            QStringLiteral("custom"),
            FindingStatus::Open,
            3
            );

    finding.record.customAttributes.insert(
        QStringLiteral("zeta"),
        QStringLiteral("last")
        );

    finding.record.customAttributes.insert(
        QStringLiteral("alpha"),
        QStringLiteral("second")
        );

    finding.record.customAttributes.insert(
        QStringLiteral("Alpha"),
        QStringLiteral("first")
        );

    finding.record.customAttributes.insert(
        QStringLiteral("middle"),
        QStringLiteral("center")
        );

    InvestigationFindingsCsvExporter
        exporter;

    QVERIFY(
        exporter.exportToFile(
            {finding},
            file.fileName()
            )
        );

    QFile output(
        file.fileName()
        );

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

    const qsizetype alphaUpper =
        header.indexOf(",Alpha,");

    const qsizetype alphaLower =
        header.indexOf(",alpha,");

    const qsizetype middle =
        header.indexOf(",middle,");

    const qsizetype zeta =
        header.indexOf(",zeta,");

    QVERIFY(alphaUpper >= 0);
    QVERIFY(alphaLower > alphaUpper);
    QVERIFY(middle > alphaLower);
    QVERIFY(zeta > middle);

    QVERIFY(
        header.endsWith(
            ",Raw Source"
            )
        );
}

void
    InvestigationFindingsCsvExporterTests::
    preservesSnapshotOrder()
{
    QTemporaryFile file;

    QVERIFY(file.open());
    file.close();

    InvestigationFindingExport first =
        makeFinding(
            QStringLiteral("first-record"),
            FindingStatus::Resolved,
            10
            );

    InvestigationFindingExport second =
        makeFinding(
            QStringLiteral("second-record"),
            FindingStatus::Open,
            20
            );

    InvestigationFindingExport third =
        makeFinding(
            QStringLiteral("third-record"),
            FindingStatus::Dismissed,
            30
            );

    InvestigationFindingsCsvExporter
        exporter;

    QVERIFY(
        exporter.exportToFile(
            {
                first,
                second,
                third
            },
            file.fileName()
            )
        );

    const QString content =
        readFile(file.fileName());

    const qsizetype firstIndex =
        content.indexOf(
            "first-record"
            );

    const qsizetype secondIndex =
        content.indexOf(
            "second-record"
            );

    const qsizetype thirdIndex =
        content.indexOf(
            "third-record"
            );

    QVERIFY(firstIndex >= 0);
    QVERIFY(secondIndex > firstIndex);
    QVERIFY(thirdIndex > secondIndex);
}

void
    InvestigationFindingsCsvExporterTests::
    writesStableHeaderForEmptySnapshot()
{
    QTemporaryFile file;

    QVERIFY(file.open());
    file.close();

    InvestigationFindingsCsvExporter
        exporter;

    QVERIFY(
        exporter.exportToFile(
            {},
            file.fileName()
            )
        );

    const QString content =
        readFile(file.fileName());

    QCOMPARE(
        content,
        QString(
            "Finding Status,Analyst Note,"
            "Bookmarked,Record ID,Timestamp,"
            "Severity,Subsystem,Event Code,"
            "Entity ID,Message,Source Name,"
            "Source Path,Source Record,"
            "Raw Source\n"
            )
        );
}

QTEST_MAIN(
    InvestigationFindingsCsvExporterTests
    )

#include "InvestigationFindingsCsvExporterTests.moc"