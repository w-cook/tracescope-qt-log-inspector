#include <QtTest/QtTest>

#include <variant>

#include "../tools/live-log-generator/LiveLogScenarioLoader.h"

class LiveLogScenarioLoaderTests : public QObject
{
    Q_OBJECT

private slots:
    void validScenarioIsLoaded();
    void allStepTypesAreLoaded();
    void partialWriteOptionsAreLoaded();
    void scalarCustomAttributesAreLoaded();

    void malformedJsonIsRejected();
    void unsupportedSchemaVersionIsRejected();
    void unknownStepTypeIsRejected();
    void negativeTimingValuesAreRejected();
    void invalidPartialSplitIsRejected();
    void nestedCustomAttributesAreRejected();
};

void LiveLogScenarioLoaderTests::
    validScenarioIsLoaded()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Gateway Incident",
  "description": "Simple deterministic incident.",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 1250,
        "severity": "WARN",
        "subsystem": "Transport",
        "eventCode": "LATENCY_HIGH",
        "entityId": "gateway-17",
        "message": "Latency exceeded expected range."
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(result.isSuccess());
    QVERIFY(result.scenario.has_value());

    const LiveLogScenario &scenario =
        *result.scenario;

    QCOMPARE(
        scenario.schemaVersion,
        1
        );

    QCOMPARE(
        scenario.name,
        QStringLiteral("Gateway Incident")
        );

    QCOMPARE(
        scenario.description,
        QStringLiteral(
            "Simple deterministic incident."
            )
        );

    QCOMPARE(
        scenario.steps.size(),
        1
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogRecordStep
            >(scenario.steps.at(0))
        );

    const LiveLogRecordStep &step =
        std::get<LiveLogRecordStep>(
            scenario.steps.at(0)
            );

    QCOMPARE(
        step.record.timestampOffsetMs,
        qint64(1250)
        );

    QCOMPARE(
        step.record.severity,
        QStringLiteral("WARN")
        );

    QCOMPARE(
        step.record.subsystem,
        QStringLiteral("Transport")
        );

    QCOMPARE(
        step.record.eventCode,
        QStringLiteral("LATENCY_HIGH")
        );

    QCOMPARE(
        step.record.entityId,
        QStringLiteral("gateway-17")
        );

    QCOMPARE(
        step.record.message,
        QStringLiteral(
            "Latency exceeded expected range."
            )
        );

    QCOMPARE(
        step.write.mode,
        LiveLogWriteMode::Normal
        );
}

void LiveLogScenarioLoaderTests::
    allStepTypesAreLoaded()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "All Steps",
  "description": "Exercises every scenario step type.",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 0,
        "severity": "INFO",
        "subsystem": "Gateway",
        "eventCode": "START",
        "entityId": "gateway-17",
        "message": "Started."
      }
    },
    {
      "type": "wait",
      "durationMs": 500
    },
    {
      "type": "truncate"
    },
    {
      "type": "replace"
    },
    {
      "type": "rotate"
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(result.isSuccess());

    const LiveLogScenario &scenario =
        *result.scenario;

    QCOMPARE(
        scenario.steps.size(),
        5
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogRecordStep
            >(scenario.steps.at(0))
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogWaitStep
            >(scenario.steps.at(1))
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogTruncateStep
            >(scenario.steps.at(2))
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogReplaceStep
            >(scenario.steps.at(3))
        );

    QVERIFY(
        std::holds_alternative<
            LiveLogRotateStep
            >(scenario.steps.at(4))
        );

    QCOMPARE(
        std::get<LiveLogWaitStep>(
            scenario.steps.at(1)
            ).durationMs,
        qint64(500)
        );
}

void LiveLogScenarioLoaderTests::
    partialWriteOptionsAreLoaded()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Partial Write",
  "description": "Exercises a partial record write.",
  "steps": [
    {
      "type": "record",
      "write": {
        "mode": "partial",
        "splitFraction": 0.6,
        "holdMs": 750
      },
      "record": {
        "timestampOffsetMs": 2000,
        "severity": "ERROR",
        "subsystem": "Transport",
        "eventCode": "TIMEOUT",
        "entityId": "gateway-17",
        "message": "Request timed out."
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(result.isSuccess());

    const LiveLogRecordStep &step =
        std::get<LiveLogRecordStep>(
            result.scenario->steps.at(0)
            );

    QCOMPARE(
        step.write.mode,
        LiveLogWriteMode::Partial
        );

    QCOMPARE(
        step.write.splitFraction,
        0.6
        );

    QCOMPARE(
        step.write.holdMs,
        qint64(750)
        );
}

void LiveLogScenarioLoaderTests::
    scalarCustomAttributesAreLoaded()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Attributes",
  "description": "Exercises supported custom attributes.",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 0,
        "severity": "INFO",
        "subsystem": "Gateway",
        "eventCode": "REQUEST",
        "entityId": "gateway-17",
        "message": "Request processed.",
        "attributes": {
          "endpoint": "collector-a",
          "latencyMs": 1840,
          "retrying": true,
          "correlationId": null
        }
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(result.isSuccess());

    const LiveLogRecordStep &step =
        std::get<LiveLogRecordStep>(
            result.scenario->steps.at(0)
            );

    QCOMPARE(
        step.record.attributes.value(
                                  QStringLiteral("endpoint")
                                  ).toString(),
        QStringLiteral("collector-a")
        );

    QCOMPARE(
        step.record.attributes.value(
                                  QStringLiteral("latencyMs")
                                  ).toDouble(),
        1840.0
        );

    QCOMPARE(
        step.record.attributes.value(
                                  QStringLiteral("retrying")
                                  ).toBool(),
        true
        );

    QVERIFY(
        step.record.attributes.contains(
            QStringLiteral("correlationId")
            )
        );

    QVERIFY(
        step.record.attributes.value(
                                  QStringLiteral("correlationId")
                                  ).isNull()
        );
}

void LiveLogScenarioLoaderTests::
    malformedJsonIsRejected()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Broken",
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral("INVALID_JSON")
        );
}

void LiveLogScenarioLoaderTests::
    unsupportedSchemaVersionIsRejected()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 2,
  "name": "Future Scenario",
  "description": "",
  "steps": [
    {
      "type": "wait",
      "durationMs": 100
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNSUPPORTED_SCHEMA_VERSION"
            )
        );
}

void LiveLogScenarioLoaderTests::
    unknownStepTypeIsRejected()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Unknown Step",
  "description": "",
  "steps": [
    {
      "type": "explode"
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral("UNKNOWN_STEP_TYPE")
        );
}

void LiveLogScenarioLoaderTests::
    negativeTimingValuesAreRejected()
{
    const QByteArray negativeWait = R"JSON(
{
  "schemaVersion": 1,
  "name": "Negative Wait",
  "description": "",
  "steps": [
    {
      "type": "wait",
      "durationMs": -1
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult waitResult =
        LiveLogScenarioLoader().deserialize(
            negativeWait
            );

    QVERIFY(!waitResult.isSuccess());

    QCOMPARE(
        waitResult.errorCode,
        QStringLiteral(
            "INVALID_WAIT_DURATION"
            )
        );

    const QByteArray negativeTimestamp = R"JSON(
{
  "schemaVersion": 1,
  "name": "Negative Timestamp",
  "description": "",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": -1,
        "severity": "INFO",
        "subsystem": "Gateway",
        "eventCode": "START",
        "entityId": "gateway-17",
        "message": "Started."
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult timestampResult =
        LiveLogScenarioLoader().deserialize(
            negativeTimestamp
            );

    QVERIFY(!timestampResult.isSuccess());

    QCOMPARE(
        timestampResult.errorCode,
        QStringLiteral(
            "INVALID_TIMESTAMP_OFFSET"
            )
        );
}

void LiveLogScenarioLoaderTests::
    invalidPartialSplitIsRejected()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Invalid Partial",
  "description": "",
  "steps": [
    {
      "type": "record",
      "write": {
        "mode": "partial",
        "splitFraction": 1.0,
        "holdMs": 500
      },
      "record": {
        "timestampOffsetMs": 0,
        "severity": "ERROR",
        "subsystem": "Gateway",
        "eventCode": "FAIL",
        "entityId": "gateway-17",
        "message": "Failure."
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SPLIT_FRACTION"
            )
        );
}

void LiveLogScenarioLoaderTests::
    nestedCustomAttributesAreRejected()
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "name": "Nested Attributes",
  "description": "",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 0,
        "severity": "INFO",
        "subsystem": "Gateway",
        "eventCode": "REQUEST",
        "entityId": "gateway-17",
        "message": "Request processed.",
        "attributes": {
          "context": {
            "requestId": "abc-123"
          }
        }
      }
    }
  ]
}
)JSON";

    const LiveLogScenarioLoadResult result =
        LiveLogScenarioLoader().deserialize(json);

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_ATTRIBUTE_VALUE"
            )
        );
}

QTEST_MAIN(LiveLogScenarioLoaderTests)

#include "LiveLogScenarioLoaderTests.moc"