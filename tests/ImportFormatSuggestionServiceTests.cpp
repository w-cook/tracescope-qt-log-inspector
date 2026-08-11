#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/importing/ImportFormatSuggestionService.h"

class ImportFormatSuggestionServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void jsonlExtensionSuggestsJsonLines();
    void ndjsonExtensionSuggestsJsonLines();
    void jsonObjectLinesSuggestJsonLines();
    void csvExtensionSuggestsCsv();
    void tsvExtensionSuggestsTsv();
    void malformedContentHasNoSuggestion();
    void jsonArrayHasNoSuggestion();
    void missingFileHasNoSuggestion();

private:
    static bool writeFile(
        const QString &filePath,
        const QByteArray &contents
        );
};

bool
ImportFormatSuggestionServiceTests::writeFile(
    const QString &filePath,
    const QByteArray &contents
    )
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Text
            )) {
        return false;
    }

    return file.write(contents)
           == contents.size();
}

void
    ImportFormatSuggestionServiceTests::
    jsonlExtensionSuggestsJsonLines()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.jsonl")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral("not inspected\n")
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(filePath);

    QVERIFY(suggestion.hasSuggestion());
    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("json-lines")
        );
    QCOMPARE(
        suggestion.displayName,
        QStringLiteral("JSON Lines")
        );
}

void
    ImportFormatSuggestionServiceTests::
    ndjsonExtensionSuggestsJsonLines()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.ndjson")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral("{}\n")
            )
        );

    ImportFormatSuggestionService service;

    QCOMPARE(
        service.suggestForFile(filePath)
            .importerId,
        QStringLiteral("json-lines")
        );
}

void
    ImportFormatSuggestionServiceTests::
    jsonObjectLinesSuggestJsonLines()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.log")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "{\"level\":\"INFO\"}\n"
                "\n"
                "{\"level\":\"WARN\"}\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(filePath);

    QVERIFY(suggestion.hasSuggestion());
    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("json-lines")
        );
}

void
    ImportFormatSuggestionServiceTests::
    csvExtensionSuggestsCsv()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.csv")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "timestamp,level,message\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(filePath);

    QVERIFY(suggestion.hasSuggestion());

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("csv")
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral("CSV")
        );
}

void
    ImportFormatSuggestionServiceTests::
    tsvExtensionSuggestsTsv()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.tsv")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "timestamp\tlevel\tmessage\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(filePath);

    QVERIFY(suggestion.hasSuggestion());

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("tsv")
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral("TSV")
        );
}

void
    ImportFormatSuggestionServiceTests::
    malformedContentHasNoSuggestion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.log")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "{\"level\":\"INFO\"}\n"
                "not json\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    QVERIFY(
        !service.suggestForFile(filePath)
             .hasSuggestion()
        );
}

void
    ImportFormatSuggestionServiceTests::
    jsonArrayHasNoSuggestion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral("sample.json")
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "[{\"level\":\"INFO\"}]"
                )
            )
        );

    ImportFormatSuggestionService service;

    QVERIFY(
        !service.suggestForFile(filePath)
             .hasSuggestion()
        );
}

void
    ImportFormatSuggestionServiceTests::
    missingFileHasNoSuggestion()
{
    ImportFormatSuggestionService service;

    QVERIFY(
        !service.suggestForFile(
                    QStringLiteral(
                        "file-that-does-not-exist.jsonl"
                        )
                    )
             .hasSuggestion()
        );
}

QTEST_MAIN(
    ImportFormatSuggestionServiceTests
    )

#include "ImportFormatSuggestionServiceTests.moc"