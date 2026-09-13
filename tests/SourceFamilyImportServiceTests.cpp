#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/importing/JsonLinesImporter.h"
#include "../src/importing/SourceFamilyImportService.h"

namespace
{
bool writeRecords(
    const QString &filePath,
    const QString &prefix,
    int count
    )
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )) {
        return false;
    }

    for (int index = 1;
         index <= count;
         ++index) {
        const QByteArray line =
            QStringLiteral(
                "{\"message\":\"%1-%2\"}\n"
                )
                .arg(
                    prefix,
                    QString::number(index)
                    )
                .toUtf8();

        if (file.write(line)
            != line.size()) {
            return false;
        }
    }

    return true;
}
}

class SourceFamilyImportServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void importsSourcesInProvidedOrder();
    void preservesPhysicalSourceIdentity();
    void distributedLimitSamplesAcrossSources();
    void aggregatesDiagnosticsAcrossSources();
};

void SourceFamilyImportServiceTests::
    importsSourcesInProvidedOrder()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString oldestPath =
        directory.filePath(
            QStringLiteral(
                "service.log.2"
                )
            );

    const QString newestPath =
        directory.filePath(
            QStringLiteral(
                "service.log.1"
                )
            );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeRecords(
            oldestPath,
            QStringLiteral("oldest"),
            2
            )
        );

    QVERIFY(
        writeRecords(
            newestPath,
            QStringLiteral("newest"),
            2
            )
        );

    QVERIFY(
        writeRecords(
            activePath,
            QStringLiteral("active"),
            2
            )
        );

    JsonLinesImporter importer;

    SourceFamilyImportService service;

    const ImportResult result =
        service.importFiles(
            {
                oldestPath,
                newestPath,
                activePath
            },
            importer
            );

    QCOMPARE(
        result.records.size(),
        6
        );

    QCOMPARE(
        result.records.at(0)
            .message
            .value_or(QString()),
        QStringLiteral(
            "oldest-1"
            )
        );

    QCOMPARE(
        result.records.at(2)
            .message
            .value_or(QString()),
        QStringLiteral(
            "newest-1"
            )
        );

    QCOMPARE(
        result.records.at(4)
            .message
            .value_or(QString()),
        QStringLiteral(
            "active-1"
            )
        );
}

void SourceFamilyImportServiceTests::
    preservesPhysicalSourceIdentity()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString rotatedPath =
        directory.filePath(
            QStringLiteral(
                "service.log.1"
                )
            );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeRecords(
            rotatedPath,
            QStringLiteral("rotated"),
            2
            )
        );

    QVERIFY(
        writeRecords(
            activePath,
            QStringLiteral("active"),
            2
            )
        );

    JsonLinesImporter importer;

    SourceFamilyImportService service;

    const ImportResult result =
        service.importFiles(
            {
                rotatedPath,
                activePath
            },
            importer
            );

    QCOMPARE(
        result.records.size(),
        4
        );

    QCOMPARE(
        result.records.at(0)
            .source
            .sourceName,
        QStringLiteral(
            "service.log.1"
            )
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

    QCOMPARE(
        result.records.at(2)
            .source
            .sourceName,
        QStringLiteral(
            "service.log"
            )
        );

    /*
     * Each physical file retains its own record
     * numbering. A rotated sibling is not represented
     * as a live source generation.
     */
    QCOMPARE(
        result.records.at(2)
            .source
            .recordNumber,
        qint64(1)
        );

    QCOMPARE(
        result.records.at(2)
            .source
            .sourceGeneration,
        quint64(0)
        );
}

void SourceFamilyImportServiceTests::
    distributedLimitSamplesAcrossSources()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString oldestPath =
        directory.filePath(
            QStringLiteral(
                "service.log.2"
                )
            );

    const QString newestPath =
        directory.filePath(
            QStringLiteral(
                "service.log.1"
                )
            );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeRecords(
            oldestPath,
            QStringLiteral("oldest"),
            10
            )
        );

    QVERIFY(
        writeRecords(
            newestPath,
            QStringLiteral("newest"),
            10
            )
        );

    QVERIFY(
        writeRecords(
            activePath,
            QStringLiteral("active"),
            10
            )
        );

    SourceFamilyImportOptions options;

    options.maxProcessedRecords = 6;

    options.recordLimitMode =
        SourceFamilyRecordLimitMode::
        DistributedAcrossSources;

    JsonLinesImporter importer;

    SourceFamilyImportService service;

    const ImportResult result =
        service.importFiles(
            {
                oldestPath,
                newestPath,
                activePath
            },
            importer,
            options
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(6)
        );

    QCOMPARE(
        result.records.size(),
        6
        );

    QCOMPARE(
        result.records.at(0)
            .source
            .sourceName,
        QStringLiteral(
            "service.log.2"
            )
        );

    QCOMPARE(
        result.records.at(1)
            .source
            .sourceName,
        QStringLiteral(
            "service.log.2"
            )
        );

    QCOMPARE(
        result.records.at(2)
            .source
            .sourceName,
        QStringLiteral(
            "service.log.1"
            )
        );

    QCOMPARE(
        result.records.at(3)
            .source
            .sourceName,
        QStringLiteral(
            "service.log.1"
            )
        );

    QCOMPARE(
        result.records.at(4)
            .source
            .sourceName,
        QStringLiteral(
            "service.log"
            )
        );

    QCOMPARE(
        result.records.at(5)
            .source
            .sourceName,
        QStringLiteral(
            "service.log"
            )
        );

    QVERIFY(
        result.sourceTruncated
        );
}

void SourceFamilyImportServiceTests::
    aggregatesDiagnosticsAcrossSources()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString firstPath =
        directory.filePath(
            QStringLiteral(
                "service.log.1"
                )
            );

    const QString secondPath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeRecords(
            firstPath,
            QStringLiteral("valid"),
            1
            )
        );

    QFile malformedFile(
        secondPath
        );

    QVERIFY(
        malformedFile.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QVERIFY(
        malformedFile.write(
            QByteArrayLiteral(
                "{not-json}\n"
                )
            )
        > 0
        );

    malformedFile.close();

    JsonLinesImporter importer;

    SourceFamilyImportService service;

    const ImportResult result =
        service.importFiles(
            {
                firstPath,
                secondPath
            },
            importer
            );

    QCOMPARE(
        result.processedRecordCount,
        qint64(2)
        );

    QCOMPARE(
        result.records.size(),
        1
        );

    QVERIFY(
        !result.diagnostics.isEmpty()
        );

    bool foundActiveSourceDiagnostic =
        false;

    for (const ImportDiagnostic &diagnostic
         : result.diagnostics) {
        if (!diagnostic.source.has_value()) {
            continue;
        }

        if (diagnostic
                .source
                ->sourceName
            == QStringLiteral(
                "service.log"
                )) {
            foundActiveSourceDiagnostic =
                true;

            break;
        }
    }

    QVERIFY(
        foundActiveSourceDiagnostic
        );
}

QTEST_MAIN(
    SourceFamilyImportServiceTests
    )

#include "SourceFamilyImportServiceTests.moc"