#include <QtTest/QtTest>

#include "../tools/live-log-generator/renderers/AccessLogRenderer.h"

class AccessLogRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void initialContentIsEmpty();
    void apacheCommonRecordIsRendered();
    void apacheCombinedAddsReferrerAndUserAgent();
    void nginxCombinedUsesCombinedGrammar();
    void queryIsIncludedInRequestTarget();
    void missingOptionalValuesUseHyphen();
};

namespace
{
QDateTime scenarioStart()
{
    return QDateTime::fromString(
        QStringLiteral(
            "2026-09-08T12:00:00.000Z"
            ),
        Qt::ISODateWithMs
        );
}

LiveLogRecord typicalRecord()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("clientIp"),
        QStringLiteral("198.51.100.24")
        );

    record.attributes.insert(
        QStringLiteral("method"),
        QStringLiteral("GET")
        );

    record.attributes.insert(
        QStringLiteral("path"),
        QStringLiteral(
            "/api/orders/5812"
            )
        );

    record.attributes.insert(
        QStringLiteral("username"),
        QStringLiteral("analyst")
        );

    record.attributes.insert(
        QStringLiteral("status"),
        200
        );

    record.attributes.insert(
        QStringLiteral("responseBytes"),
        842
        );

    record.attributes.insert(
        QStringLiteral("referer"),
        QStringLiteral(
            "https://portal.example.test/orders"
            )
        );

    record.attributes.insert(
        QStringLiteral("userAgent"),
        QStringLiteral("Mozilla/5.0")
        );

    return record;
}
}

void AccessLogRendererTests::
    initialContentIsEmpty()
{
    const AccessLogRenderer renderer(
        AccessLogFormat::ApacheCommon
        );

    QVERIFY(
        renderer.initialContent().isEmpty()
        );
}

void AccessLogRendererTests::
    apacheCommonRecordIsRendered()
{
    const LiveLogRecord record =
        typicalRecord();

    const AccessLogRenderer renderer(
        AccessLogFormat::ApacheCommon
        );

    QCOMPARE(
        renderer.renderRecord(
            record,
            scenarioStart()
            ),
        QByteArray(
            "198.51.100.24 - analyst "
            "[08/Sep/2026:12:00:00 +0000] "
            "\"GET /api/orders/5812 HTTP/1.1\" "
            "200 842\n"
            )
        );
}

void AccessLogRendererTests::
    apacheCombinedAddsReferrerAndUserAgent()
{
    const LiveLogRecord record =
        typicalRecord();

    const AccessLogRenderer renderer(
        AccessLogFormat::ApacheCombined
        );

    QCOMPARE(
        renderer.renderRecord(
            record,
            scenarioStart()
            ),
        QByteArray(
            "198.51.100.24 - analyst "
            "[08/Sep/2026:12:00:00 +0000] "
            "\"GET /api/orders/5812 HTTP/1.1\" "
            "200 842 "
            "\"https://portal.example.test/orders\" "
            "\"Mozilla/5.0\"\n"
            )
        );
}

void AccessLogRendererTests::
    nginxCombinedUsesCombinedGrammar()
{
    const LiveLogRecord record =
        typicalRecord();

    const AccessLogRenderer apache(
        AccessLogFormat::ApacheCombined
        );

    const AccessLogRenderer nginx(
        AccessLogFormat::NginxCombined
        );

    QCOMPARE(
        nginx.renderRecord(
            record,
            scenarioStart()
            ),
        apache.renderRecord(
            record,
            scenarioStart()
            )
        );
}

void AccessLogRendererTests::
    queryIsIncludedInRequestTarget()
{
    LiveLogRecord record =
        typicalRecord();

    record.attributes.insert(
        QStringLiteral("path"),
        QStringLiteral("/search")
        );

    record.attributes.insert(
        QStringLiteral("query"),
        QStringLiteral(
            "q=widget&page=2"
            )
        );

    const AccessLogRenderer renderer(
        AccessLogFormat::ApacheCommon
        );

    QVERIFY(
        renderer.renderRecord(
                    record,
                    scenarioStart()
                    )
            .contains(
                "\"GET /search?q=widget&page=2 "
                "HTTP/1.1\""
                )
        );
}

void AccessLogRendererTests::
    missingOptionalValuesUseHyphen()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("clientIp"),
        QStringLiteral("203.0.113.77")
        );

    record.attributes.insert(
        QStringLiteral("method"),
        QStringLiteral("GET")
        );

    record.attributes.insert(
        QStringLiteral("path"),
        QStringLiteral("/health")
        );

    record.attributes.insert(
        QStringLiteral("status"),
        200
        );

    const AccessLogRenderer renderer(
        AccessLogFormat::ApacheCombined
        );

    QCOMPARE(
        renderer.renderRecord(
            record,
            scenarioStart()
            ),
        QByteArray(
            "203.0.113.77 - - "
            "[08/Sep/2026:12:00:00 +0000] "
            "\"GET /health HTTP/1.1\" "
            "200 - \"-\" \"-\"\n"
            )
        );
}

QTEST_MAIN(AccessLogRendererTests)

#include "AccessLogRendererTests.moc"