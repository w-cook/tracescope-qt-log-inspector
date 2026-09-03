#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/exporting/InvestigationReportHtmlExporter.h"

namespace
{

InvestigationReportSnapshot makeSnapshot()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral(
            "Exporter Test"
            );

    snapshot.context =
        QStringLiteral(
            "UTF-8 evidence: café"
            );

    snapshot.generatedAtUtc =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-03T15:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    return snapshot;
}

}

class InvestigationReportHtmlExporterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void writesSelfContainedUtf8Html();
    void replacesExistingFile();
    void rejectsEmptyPath();
    void failsForInvalidDestination();
};

void InvestigationReportHtmlExporterTests::
    writesSelfContainedUtf8Html()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "report.html"
                )
            );

    const InvestigationReportHtmlExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            makeSnapshot(),
            filePath
            )
        );

    QFile file(
        filePath
        );

    QVERIFY(
        file.open(
            QIODevice::ReadOnly
            )
        );

    const QByteArray bytes =
        file.readAll();

    QVERIFY(
        bytes.startsWith(
            QByteArrayLiteral(
                "<!DOCTYPE html>"
                )
            )
        );

    const QString html =
        QString::fromUtf8(
            bytes
            );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Exporter Test"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "UTF-8 evidence: café"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "<meta charset=\"utf-8\">"
                )
            )
        );
}

void InvestigationReportHtmlExporterTests::
    replacesExistingFile()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "existing.html"
                )
            );

    {
        QFile existing(
            filePath
            );

        QVERIFY(
            existing.open(
                QIODevice::WriteOnly
                )
            );

        QCOMPARE(
            existing.write(
                QByteArrayLiteral(
                    "old report content"
                    )
                ),
            static_cast<qint64>(
                QByteArrayLiteral(
                    "old report content"
                    )
                    .size()
                )
            );
    }

    const InvestigationReportHtmlExporter exporter;

    QVERIFY(
        exporter.exportToFile(
            makeSnapshot(),
            filePath
            )
        );

    QFile file(
        filePath
        );

    QVERIFY(
        file.open(
            QIODevice::ReadOnly
            )
        );

    const QByteArray bytes =
        file.readAll();

    QVERIFY(
        !bytes.contains(
            QByteArrayLiteral(
                "old report content"
                )
            )
        );

    QVERIFY(
        bytes.startsWith(
            QByteArrayLiteral(
                "<!DOCTYPE html>"
                )
            )
        );
}

void InvestigationReportHtmlExporterTests::
    rejectsEmptyPath()
{
    const InvestigationReportHtmlExporter exporter;

    QVERIFY(
        !exporter.exportToFile(
            makeSnapshot(),
            QString()
            )
        );

    QVERIFY(
        !exporter.exportToFile(
            makeSnapshot(),
            QStringLiteral("   ")
            )
        );
}

void InvestigationReportHtmlExporterTests::
    failsForInvalidDestination()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "missing/subdirectory/report.html"
                )
            );

    const InvestigationReportHtmlExporter exporter;

    QVERIFY(
        !exporter.exportToFile(
            makeSnapshot(),
            filePath
            )
        );
}

QTEST_APPLESS_MAIN(
    InvestigationReportHtmlExporterTests
    )

#include "InvestigationReportHtmlExporterTests.moc"