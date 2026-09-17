#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/live/LiveLineImportAdapter.h"

class LiveLineImportAdapterTests
    : public QObject
{
    Q_OBJECT

private slots:
    void recognizesSupportedImporterKinds();
    void importsJsonLinesWithLiveSourcePosition();
    void resetsDelimitedStateForNewGeneration();
};

void LiveLineImportAdapterTests::
    recognizesSupportedImporterKinds()
{
    struct TestCase
    {
        QString importerId;
        LiveLineImporterKind expectedKind;
    };

    const QVector<TestCase> cases {
        {
            QStringLiteral("json-lines"),
            LiveLineImporterKind::JsonLines
        },
        {
            QStringLiteral("regex-text"),
            LiveLineImporterKind::RegexText
        },
        {
            QStringLiteral("key-value"),
            LiveLineImporterKind::KeyValue
        },
        {
            QStringLiteral("syslog"),
            LiveLineImporterKind::Syslog
        },
        {
            QStringLiteral("csv"),
            LiveLineImporterKind::Csv
        },
        {
            QStringLiteral("tsv"),
            LiveLineImporterKind::Tsv
        },
        {
            QStringLiteral("iis-w3c"),
            LiveLineImporterKind::IisW3c
        }
    };

    for (const TestCase &testCase : cases) {
        ImportProfile profile;

        profile.importerId =
            testCase.importerId;

        LiveLineImportAdapter adapter(
            profile
            );

        QVERIFY(
            adapter.isSupported()
            );

        QCOMPARE(
            adapter.kind(),
            testCase.expectedKind
            );
    }

    ImportProfile structuredProfile;

    structuredProfile.importerId =
        QStringLiteral(
            "structured-json"
            );

    LiveLineImportAdapter unsupported(
        structuredProfile
        );

    QVERIFY(
        !unsupported.isSupported()
        );

    QCOMPARE(
        unsupported.kind(),
        LiveLineImporterKind::
        Unsupported
        );
}

void LiveLineImportAdapterTests::
    importsJsonLinesWithLiveSourcePosition()
{
    ImportProfile profile;

    profile.importerId =
        QStringLiteral(
            "json-lines"
            );

    LiveLineImportAdapter adapter(
        profile
        );

    LiveFramedLineBatch batch;

    batch.firstPhysicalLineNumber = 14;
    batch.sourceGeneration = 3;

    batch.lines.append(
        QByteArray(
            R"({"timestamp":"2026-09-10T15:00:00Z","level":"WARN","message":"Live record"})"
            )
        );

    const ImportResult result =
        adapter.importBatch(
            batch,
            QStringLiteral(
                "session.jsonl"
                )
            );

    QCOMPARE(
        result.records.size(),
        1
        );

    QCOMPARE(
        result.records.first()
            .source.recordNumber,
        qint64(14)
        );

    QCOMPARE(
        result.records.first()
            .source.sourceGeneration,
        quint64(3)
        );

    QCOMPARE(
        result.records.first().message,
        std::optional<QString>(
            QStringLiteral(
                "Live record"
                )
            )
        );
}

void LiveLineImportAdapterTests::
    resetsDelimitedStateForNewGeneration()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "session.csv"
                )
            );

    QFile file(
        sourcePath
        );

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            )
        );

    const QByteArray existing(
        "timestamp,message\n"
        "2026-09-10T15:00:00Z,Existing\n"
        );

    QCOMPARE(
        file.write(
            existing
            ),
        qint64(existing.size())
        );

    file.close();

    ImportProfile profile;

    profile.importerId =
        QStringLiteral("csv");

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    LiveLineImportAdapter adapter(
        profile
        );

    const qint64 baselineByteCount =
        QFileInfo(
            sourcePath
            ).size();

    const auto initialization =
        adapter.initializeFromExistingFile(
            sourcePath,
            baselineByteCount
            );

    QVERIFY(
        initialization.succeeded
        );

    LiveFramedLineBatch continuation;

    continuation.firstPhysicalLineNumber = 3;
    continuation.sourceGeneration = 0;
    continuation.lines = {
        QByteArray(
            "2026-09-10T15:00:01Z,Continued"
            )
    };

    const ImportResult continuedResult =
        adapter.importBatch(
            continuation,
            sourcePath
            );

    QCOMPARE(
        continuedResult.records.size(),
        1
        );

    QCOMPARE(
        continuedResult.records.first().message,
        std::optional<QString>(
            QStringLiteral("Continued")
            )
        );

    /*
     * A replacement/rotation starts a new physical
     * source. The old CSV header must no longer be
     * carried into that generation.
     */
    adapter.beginSourceGeneration();

    LiveFramedLineBatch replacement;

    replacement.firstPhysicalLineNumber = 1;
    replacement.sourceGeneration = 1;

    replacement.lines = {
        QByteArray(
            "timestamp,message"
            ),
        QByteArray(
            "2026-09-10T15:01:00Z,Replacement"
            )
    };

    const ImportResult replacementResult =
        adapter.importBatch(
            replacement,
            sourcePath
            );

    QCOMPARE(
        replacementResult.records.size(),
        1
        );

    QCOMPARE(
        replacementResult.records.first()
            .source.recordNumber,
        qint64(2)
        );

    QCOMPARE(
        replacementResult.records.first()
            .source.sourceGeneration,
        quint64(1)
        );

    QCOMPARE(
        replacementResult.records.first().message,
        std::optional<QString>(
            QStringLiteral("Replacement")
            )
        );
}

QTEST_MAIN(LiveLineImportAdapterTests)

#include "LiveLineImportAdapterTests.moc"