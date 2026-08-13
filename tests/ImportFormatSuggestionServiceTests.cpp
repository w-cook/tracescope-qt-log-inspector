#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/importing/BuiltInImportProfilePresets.h"
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
    void xmlExtensionSuggestsStructuredXml();
    void malformedContentHasNoSuggestion();
    void jsonExtensionSuggestsStructuredJson();
    void logfmtContentSuggestsKeyValue();
    void isolatedAssignmentDoesNotSuggestKeyValue();
    void missingFileHasNoSuggestion();
    void syslogExtensionSuggestsSyslog();
    void rfc5424ContentSuggestsSyslog();
    void rfc3164ContentSuggestsSyslog();
    void invalidSyslogPriorityHasNoSuggestion();
    void apacheCommonContentSuggestsPreset();
    void combinedAccessContentSuggestsPreset();
    void iisW3cContentSuggestsPreset();
    void singleWindowsEventXmlSuggestsPreset();
    void windowsEventXmlCollectionSuggestsPreset();
    void ordinaryXmlDoesNotSuggestWindowsEvent();

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
    xmlExtensionSuggestsStructuredXml()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "session.xml"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<session />"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(
            filePath
            );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("xml")
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Structured XML"
            )
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
    jsonExtensionSuggestsStructuredJson()
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

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(filePath);

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "structured-json"
            )
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Structured JSON"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    logfmtContentSuggestsKeyValue()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "application.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "timestamp=2026-08-12T08:30:00Z "
                "level=INFO "
                "subsystem=Orders "
                "message=\"Order accepted\"\n"

                "timestamp=2026-08-12T08:31:00Z "
                "level=WARN "
                "subsystem=Inventory "
                "message=\"Stock is low\"\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(
            filePath
            );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "key-value"
            )
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Key-Value / logfmt"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    isolatedAssignmentDoesNotSuggestKeyValue()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "application.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "Request failed after retryCount=4\n"
                "Connection closed by peer\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    QVERIFY(
        !service.suggestForFile(
                    filePath
                    )
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

void
    ImportFormatSuggestionServiceTests::
    syslogExtensionSuggestsSyslog()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "service.syslog"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "content need not be inspected\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(
            filePath
            );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "syslog"
            )
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Syslog (RFC 5424 / RFC 3164)"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    rfc5424ContentSuggestsSyslog()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<165>1 "
                "2026-08-12T08:20:18.427Z "
                "api-01 orders-service 4242 "
                "ORDER_RECEIVED - "
                "Order received\n"

                "<132>1 "
                "2026-08-12T08:21:04.812Z "
                "worker-02 supplier-gateway 8301 "
                "SUPPLIER_DELAY - "
                "Supplier delayed\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(
            filePath
            );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "syslog"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    rfc3164ContentSuggestsSyslog()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "legacy.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<34>Aug 12 08:24:16 "
                "worker-04 telemetry: "
                "Queue unavailable\n"

                "<11>Aug 12 08:25:01 "
                "db-01 postgres[7214]: "
                "Connection timed out\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    const ImportFormatSuggestion suggestion =
        service.suggestForFile(
            filePath
            );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "syslog"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    invalidSyslogPriorityHasNoSuggestion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "invalid.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<999>1 "
                "2026-08-12T08:20:18Z "
                "host app - - - Invalid\n"

                "<999>Aug 12 08:21:00 "
                "host app: Invalid\n"
                )
            )
        );

    ImportFormatSuggestionService service;

    QVERIFY(
        !service.suggestForFile(
                    filePath
                    )
             .hasSuggestion()
        );
}

void
    ImportFormatSuggestionServiceTests::
    apacheCommonContentSuggestsPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "access.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "192.0.2.10 - - "
                "[12/Aug/2026:08:20:02 -0400] "
                "\"GET / HTTP/1.1\" 200 1256\n"

                "192.0.2.44 - - "
                "[12/Aug/2026:08:21:06 -0400] "
                "\"GET /missing HTTP/1.1\" "
                "404 212\n"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("regex-text")
        );

    QCOMPARE(
        suggestion.profilePresetId,
        BuiltInImportProfilePresetIds::
        ApacheCommon
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Apache Common Access Log"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    combinedAccessContentSuggestsPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "access.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "198.51.100.10 - - "
                "[12/Aug/2026:10:10:02 -0400] "
                "\"GET / HTTP/1.1\" 200 1834 "
                "\"-\" \"Mozilla/5.0\"\n"

                "203.0.113.55 - - "
                "[12/Aug/2026:10:15:02 -0400] "
                "\"DELETE /api/orders/5812 HTTP/1.1\" "
                "403 176 "
                "\"https://app.example.test/orders/5812\" "
                "\"Mozilla/5.0\"\n"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("regex-text")
        );

    QCOMPARE(
        suggestion.profilePresetId,
        BuiltInImportProfilePresetIds::
        ApacheNginxCombined
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Apache/Nginx Combined Access Log"
            )
        );
}

void
    ImportFormatSuggestionServiceTests::
    iisW3cContentSuggestsPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "u_ex260812.log"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "#Software: Microsoft Internet "
                "Information Services 10.0\n"
                "#Version: 1.0\n"
                "#Date: 2026-08-12 15:04:03\n"
                "#Fields: date time s-ip "
                "cs-method cs-uri-stem "
                "cs-uri-query c-ip "
                "sc-status time-taken\n"
                "2026-08-12 15:04:03 "
                "192.0.2.10 GET /health - "
                "198.51.100.24 200 42\n"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral(
            "iis-w3c"
            )
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "IIS W3C Extended Log"
            )
        );

    QCOMPARE(
        suggestion.profilePresetId,
        BuiltInImportProfilePresetIds::
        IisW3c
        );
}

void
    ImportFormatSuggestionServiceTests::
    singleWindowsEventXmlSuggestsPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "windows-event.xml"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<?xml version=\"1.0\"?>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<EventID>1001</EventID>"
                "</System>"
                "</Event>"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("xml")
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Windows Event XML"
            )
        );

    QCOMPARE(
        suggestion.profilePresetId,
        BuiltInImportProfilePresetIds::
        WindowsEventXml
        );
}

void
    ImportFormatSuggestionServiceTests::
    windowsEventXmlCollectionSuggestsPreset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "windows-events.xml"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<?xml version=\"1.0\"?>"
                "<Events>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<EventID>1001</EventID>"
                "</System>"
                "</Event>"
                "<Event "
                "xmlns=\"http://schemas.microsoft.com/"
                "win/2004/08/events/event\">"
                "<System>"
                "<EventID>1002</EventID>"
                "</System>"
                "</Event>"
                "</Events>"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("xml")
        );

    QCOMPARE(
        suggestion.profilePresetId,
        BuiltInImportProfilePresetIds::
        WindowsEventXmlCollection
        );
}

void
    ImportFormatSuggestionServiceTests::
    ordinaryXmlDoesNotSuggestWindowsEvent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath =
        directory.filePath(
            QStringLiteral(
                "engineering-events.xml"
                )
            );

    QVERIFY(
        writeFile(
            filePath,
            QByteArrayLiteral(
                "<Events>"
                "<Event>"
                "<message>Test event</message>"
                "</Event>"
                "</Events>"
                )
            )
        );

    const ImportFormatSuggestion suggestion =
        ImportFormatSuggestionService()
            .suggestForFile(
                filePath
                );

    QVERIFY(
        suggestion.hasSuggestion()
        );

    QCOMPARE(
        suggestion.importerId,
        QStringLiteral("xml")
        );

    QCOMPARE(
        suggestion.displayName,
        QStringLiteral(
            "Structured XML"
            )
        );

    QVERIFY(
        suggestion.profilePresetId.isEmpty()
        );
}

QTEST_MAIN(
    ImportFormatSuggestionServiceTests
    )

#include "ImportFormatSuggestionServiceTests.moc"