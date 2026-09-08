#include <QtTest/QtTest>

#include "../tools/live-log-generator/renderers/IisW3cRenderer.h"

class IisW3cRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void initialContentDefinesExpectedFields();
    void rendersTypicalAccessRecord();
    void missingValuesUseHyphen();
    void timestampOffsetIsAppliedAsUtc();
};

void IisW3cRendererTests::
    initialContentDefinesExpectedFields()
{
    const IisW3cRenderer renderer;

    QCOMPARE(
        renderer.initialContent(),
        QByteArray(
            "#Software: Microsoft Internet Information Services 10.0\n"
            "#Version: 1.0\n"
            "#Fields: date time s-ip cs-method "
            "cs-uri-stem cs-uri-query s-port "
            "cs-username c-ip cs(User-Agent) "
            "cs(Referer) sc-status sc-substatus "
            "sc-win32-status time-taken\n"
            )
        );
}

void IisW3cRendererTests::
    rendersTypicalAccessRecord()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("serverIp"),
        QStringLiteral("192.0.2.10")
        );

    record.attributes.insert(
        QStringLiteral("method"),
        QStringLiteral("GET")
        );

    record.attributes.insert(
        QStringLiteral("path"),
        QStringLiteral("/api/orders/5812")
        );

    record.attributes.insert(
        QStringLiteral("serverPort"),
        443
        );

    record.attributes.insert(
        QStringLiteral("username"),
        QStringLiteral("analyst")
        );

    record.attributes.insert(
        QStringLiteral("clientIp"),
        QStringLiteral("198.51.100.31")
        );

    record.attributes.insert(
        QStringLiteral("userAgent"),
        QStringLiteral("Mozilla/5.0")
        );

    record.attributes.insert(
        QStringLiteral("referer"),
        QStringLiteral(
            "https://portal.example.test/orders"
            )
        );

    record.attributes.insert(
        QStringLiteral("status"),
        200
        );

    record.attributes.insert(
        QStringLiteral("substatus"),
        0
        );

    record.attributes.insert(
        QStringLiteral("win32Status"),
        0
        );

    record.attributes.insert(
        QStringLiteral("timeTakenMs"),
        57
        );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QCOMPARE(
        IisW3cRenderer().renderRecord(
            record,
            scenarioStart
            ),
        QByteArray(
            "2026-09-08 12:00:00 "
            "192.0.2.10 "
            "GET "
            "/api/orders/5812 "
            "- "
            "443 "
            "analyst "
            "198.51.100.31 "
            "Mozilla/5.0 "
            "https://portal.example.test/orders "
            "200 0 0 57\n"
            )
        );
}

void IisW3cRendererTests::
    missingValuesUseHyphen()
{
    LiveLogRecord record;

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

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QCOMPARE(
        IisW3cRenderer().renderRecord(
            record,
            scenarioStart
            ),
        QByteArray(
            "2026-09-08 12:00:00 "
            "- GET /health - - - - - - "
            "200 - - -\n"
            )
        );
}

void IisW3cRendererTests::
    timestampOffsetIsAppliedAsUtc()
{
    LiveLogRecord record;

    record.timestampOffsetMs =
        65000;

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T23:59:30.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        IisW3cRenderer().renderRecord(
            record,
            scenarioStart
            );

    QVERIFY(
        rendered.startsWith(
            "2026-09-09 00:00:35 "
            )
        );
}

QTEST_MAIN(IisW3cRendererTests)

#include "IisW3cRendererTests.moc"