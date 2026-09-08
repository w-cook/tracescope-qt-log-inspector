#include <QtTest/QtTest>

#include "../tools/live-log-generator/renderers/RegexTextRenderer.h"

class RegexTextRendererTests : public QObject
{
    Q_OBJECT

private slots:
    void initialContentIsEmpty();
    void canonicalFieldsAndTimestampOffsetAreRendered();
    void attributesUseConfiguredOrder();
    void missingAndScalarAttributesAreRendered();
    void recordEndsWithExactlyOneNewline();
};

void RegexTextRendererTests::
    initialContentIsEmpty()
{
    const RegexTextRenderer renderer{
        QStringList()
    };

    QVERIFY(renderer.initialContent().isEmpty());
}

void RegexTextRendererTests::
    canonicalFieldsAndTimestampOffsetAreRendered()
{
    LiveLogRecord record;

    record.timestampOffsetMs = 3750;
    record.severity =
        QStringLiteral("WARN");
    record.subsystem =
        QStringLiteral("Transport");
    record.eventCode =
        QStringLiteral("UPSTREAM_LATENCY");
    record.entityId =
        QStringLiteral("gateway-17");
    record.message =
        QStringLiteral(
            "Latency exceeded expected range."
            );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const RegexTextRenderer renderer{
        QStringList()
    };

    QCOMPARE(
        renderer.renderRecord(
            record,
            scenarioStart
            ),
        QByteArray(
            "2026-09-08T12:00:03.750Z "
            "[WARN] "
            "[Transport] "
            "[UPSTREAM_LATENCY] "
            "[gateway-17] "
            "Latency exceeded expected range.\n"
            )
        );
}

void RegexTextRendererTests::
    attributesUseConfiguredOrder()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("site"),
        QStringLiteral("north-yard")
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    const RegexTextRenderer renderer(
        QStringList({
            QStringLiteral("latencyMs"),
            QStringLiteral("site")
        })
        );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart
            );

    const qsizetype latencyPosition =
        rendered.indexOf(
            "[latencyMs=1840]"
            );

    const qsizetype sitePosition =
        rendered.indexOf(
            "[site=north-yard]"
            );

    QVERIFY(latencyPosition >= 0);
    QVERIFY(sitePosition >= 0);

    QVERIFY(
        latencyPosition
        < sitePosition
        );
}

void RegexTextRendererTests::
    missingAndScalarAttributesAreRendered()
{
    LiveLogRecord record;

    record.attributes.insert(
        QStringLiteral("endpoint"),
        QStringLiteral("collector-a")
        );

    record.attributes.insert(
        QStringLiteral("latencyMs"),
        1840
        );

    record.attributes.insert(
        QStringLiteral("retrying"),
        true
        );

    record.attributes.insert(
        QStringLiteral("optionalValue"),
        QVariant()
        );

    const RegexTextRenderer renderer(
        QStringList({
            QStringLiteral("attempt"),
            QStringLiteral("endpoint"),
            QStringLiteral("latencyMs"),
            QStringLiteral("optionalValue"),
            QStringLiteral("retrying")
        })
        );

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart
            );

    QVERIFY(
        rendered.contains(
            "[attempt=]"
            )
        );

    QVERIFY(
        rendered.contains(
            "[endpoint=collector-a]"
            )
        );

    QVERIFY(
        rendered.contains(
            "[latencyMs=1840]"
            )
        );

    QVERIFY(
        rendered.contains(
            "[optionalValue=]"
            )
        );

    QVERIFY(
        rendered.contains(
            "[retrying=true]"
            )
        );
}

void RegexTextRendererTests::
    recordEndsWithExactlyOneNewline()
{
    LiveLogRecord record;

    record.message =
        QStringLiteral("Test message.");

    const RegexTextRenderer renderer{
        QStringList()
    };

    const QDateTime scenarioStart =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-08T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    const QByteArray rendered =
        renderer.renderRecord(
            record,
            scenarioStart
            );

    QVERIFY(rendered.endsWith('\n'));

    QCOMPARE(
        rendered.count('\n'),
        1
        );
}

QTEST_MAIN(RegexTextRendererTests)

#include "RegexTextRendererTests.moc"