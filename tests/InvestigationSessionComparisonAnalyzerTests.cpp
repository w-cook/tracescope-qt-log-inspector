#include <QtTest/QtTest>

#include "../src/analysis/InvestigationSessionComparisonAnalyzer.h"

class InvestigationSessionComparisonAnalyzerTests
    : public QObject
{
    Q_OBJECT

private slots:
    void totalRecordsAndTimingAreCompared();
    void timingIgnoresMissingAndInvalidTimestamps();

    void severityDifferencesIncludeZeroToNonzeroChanges();
    void severityIsUnavailableWhenOneSessionLacksData();

    void eventCodesShowAppearingAndDisappearingValues();
    void eventCodesAreUnavailableWhenOneSessionLacksData();

    void healthyBaselineWithZeroElevatedEventsIsComparable();
    void elevatedSubsystemDifferencesAreCompared();
    void elevatedEntityDifferencesAreCompared();
    void elevatedDimensionsAreUnavailableWithoutRequiredData();

    void categoricalCustomFieldsShowAppearingAndDisappearingValues();
    void categoricalFrequencyOnlyChangesAreIgnored();
    void numericCustomFieldsSummarizeNumericStrings();
    void unchangedNumericCustomFieldsAreIgnored();
    void highCardinalityCategoricalFieldsAreIgnored();
    void mixedNumericAndTextValuesRemainCategorical();
    void structuredCustomValuesAreIgnored();
    void customFieldsMustExistInBothSessions();
};

static InvestigationRecord
makeComparisonRecord(
    const QString &timestamp = QString(),
    const std::optional<RecordSeverity> &severity =
    std::nullopt,
    const std::optional<QString> &subsystem =
    std::nullopt,
    const std::optional<QString> &eventCode =
    std::nullopt,
    const std::optional<QString> &entityId =
    std::nullopt
    )
{
    InvestigationRecord record;

    if (!timestamp.isEmpty()) {
        record.timestamp =
            QDateTime::fromString(
                timestamp,
                Qt::ISODateWithMs
                );
    }

    record.severity = severity;
    record.subsystem = subsystem;
    record.eventCode = eventCode;
    record.entityId = entityId;

    return record;
}

static InvestigationRecord
makeCustomComparisonRecord(
    const QString &fieldName,
    const QVariant &value
    )
{
    InvestigationRecord record =
        makeComparisonRecord();

    record.customAttributes.insert(
        fieldName,
        value
        );

    return record;
}

void InvestigationSessionComparisonAnalyzerTests::
    totalRecordsAndTimingAreCompared()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-20T10:00:00.000Z"
                    )
                ),
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-20T10:00:30.000Z"
                    )
                ),
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-20T10:01:00.000Z"
                    )
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-21T10:00:00.000Z"
                    )
                ),
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-21T10:00:40.000Z"
                    )
                ),
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-21T10:01:20.000Z"
                    )
                ),
            makeComparisonRecord(
                QStringLiteral(
                    "2026-08-21T10:02:00.000Z"
                    )
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QCOMPARE(
        result.totalRecords.baselineCount,
        3
        );

    QCOMPARE(
        result.totalRecords.comparisonCount,
        4
        );

    QCOMPARE(
        result.totalRecords.delta(),
        1
        );

    QCOMPARE(
        result.baselineTiming
            .timestampedRecordCount,
        3
        );

    QCOMPARE(
        result.comparisonTiming
            .timestampedRecordCount,
        4
        );

    QVERIFY(
        result.baselineTiming.available()
        );

    QVERIFY(
        result.comparisonTiming.available()
        );

    QCOMPARE(
        result.baselineTiming
            .durationMilliseconds,
        60'000
        );

    QCOMPARE(
        result.comparisonTiming
            .durationMilliseconds,
        120'000
        );

    QVERIFY(
        result.baselineTiming.rateAvailable()
        );

    QVERIFY(
        result.comparisonTiming.rateAvailable()
        );

    QVERIFY(
        qAbs(
            result.baselineTiming
                .recordsPerMinute
                .value()
            - 3.0
            )
        < 0.000001
        );

    QVERIFY(
        qAbs(
            result.comparisonTiming
                .recordsPerMinute
                .value()
            - 2.0
            )
        < 0.000001
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    timingIgnoresMissingAndInvalidTimestamps()
{
    QVector<InvestigationRecord>
        baselineRecords;

    baselineRecords.append(
        makeComparisonRecord()
        );

    baselineRecords.append(
        makeComparisonRecord(
            QStringLiteral(
                "not-a-timestamp"
                )
            )
        );

    baselineRecords.append(
        makeComparisonRecord(
            QStringLiteral(
                "2026-08-20T10:00:00.000Z"
                )
            )
        );

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord()
};

InvestigationSessionComparisonAnalyzer
    analyzer;

const InvestigationSessionComparison result =
    analyzer.compare(
        baselineRecords,
        comparisonRecords
        );

QCOMPARE(
    result.baselineTiming
        .timestampedRecordCount,
    1
    );

QVERIFY(
    result.baselineTiming.available()
    );

QCOMPARE(
    result.baselineTiming
        .durationMilliseconds,
    0
    );

QVERIFY(
    !result.baselineTiming.rateAvailable()
    );

QCOMPARE(
    result.comparisonTiming
        .timestampedRecordCount,
    0
    );

QVERIFY(
    !result.comparisonTiming.available()
    );

QVERIFY(
    !result.comparisonTiming.rateAvailable()
    );
}

void InvestigationSessionComparisonAnalyzerTests::
    severityDifferencesIncludeZeroToNonzeroChanges()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.severity.comparable()
        );

    QCOMPARE(
        result.severity
            .baselinePopulatedRecordCount,
        2
        );

    QCOMPARE(
        result.severity
            .comparisonPopulatedRecordCount,
        5
        );

    /*
     * Info remained 2 -> 2 and therefore should
     * not consume comparison space.
     */
    QCOMPARE(
        result.severity.differences.size(),
        2
        );

    QCOMPARE(
        result.severity
            .differences.at(0).severity,
        RecordSeverity::Warning
        );

    QCOMPARE(
        result.severity
            .differences.at(0).baselineCount,
        0
        );

    QCOMPARE(
        result.severity
            .differences.at(0).comparisonCount,
        2
        );

    QCOMPARE(
        result.severity
            .differences.at(0).delta(),
        2
        );

    QCOMPARE(
        result.severity
            .differences.at(1).severity,
        RecordSeverity::Error
        );

    QCOMPARE(
        result.severity
            .differences.at(1).baselineCount,
        0
        );

    QCOMPARE(
        result.severity
            .differences.at(1).comparisonCount,
        1
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    severityIsUnavailableWhenOneSessionLacksData()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(),
            makeComparisonRecord()
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QCOMPARE(
        result.severity
            .baselinePopulatedRecordCount,
        0
        );

    QCOMPARE(
        result.severity
            .comparisonPopulatedRecordCount,
        2
        );

    QVERIFY(
        !result.severity.comparable()
        );

    QVERIFY(
        result.severity.differences.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    eventCodesShowAppearingAndDisappearingValues()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("NORMAL")
                ),
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("REMOVED")
                ),
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("REMOVED")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("NORMAL")
                ),
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("NEW_FAILURE")
                ),
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("NEW_FAILURE")
                ),
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("NEW_FAILURE")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.eventCodes.comparable()
        );

    /*
     * NORMAL is unchanged and omitted.
     *
     * Deterministic QMap ordering leaves the two
     * changed values alphabetically ordered.
     */
    QCOMPARE(
        result.eventCodes.differences.size(),
        2
        );

    const InvestigationValueDifference
        &newFailure =
        result.eventCodes
            .differences.at(0);

    QCOMPARE(
        newFailure.value,
        QStringLiteral("NEW_FAILURE")
        );

    QCOMPARE(
        newFailure.baselineCount,
        0
        );

    QCOMPARE(
        newFailure.comparisonCount,
        3
        );

    QVERIFY(
        newFailure.appearsOnlyInComparison()
        );

    QVERIFY(
        !newFailure.appearsOnlyInBaseline()
        );

    const InvestigationValueDifference
        &removed =
        result.eventCodes
            .differences.at(1);

    QCOMPARE(
        removed.value,
        QStringLiteral("REMOVED")
        );

    QCOMPARE(
        removed.baselineCount,
        2
        );

    QCOMPARE(
        removed.comparisonCount,
        0
        );

    QVERIFY(
        removed.appearsOnlyInBaseline()
        );

    QVERIFY(
        !removed.appearsOnlyInComparison()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    eventCodesAreUnavailableWhenOneSessionLacksData()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(),
            makeComparisonRecord()
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                std::nullopt,
                std::nullopt,
                QStringLiteral("LINK_DROP")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QCOMPARE(
        result.eventCodes
            .baselinePopulatedRecordCount,
        0
        );

    QCOMPARE(
        result.eventCodes
            .comparisonPopulatedRecordCount,
        1
        );

    QVERIFY(
        !result.eventCodes.comparable()
        );

    QVERIFY(
        result.eventCodes
            .differences.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    healthyBaselineWithZeroElevatedEventsIsComparable()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Network")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Modbus")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Network")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Modbus")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error,
                QStringLiteral("Network")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    /*
     * The baseline has severity and subsystem data,
     * so zero elevated Network events is meaningful
     * rather than unavailable.
     */
    QVERIFY(
        result.elevatedSubsystems
            .comparable()
        );

    QCOMPARE(
        result.elevatedSubsystems
            .baselinePopulatedRecordCount,
        2
        );

    QCOMPARE(
        result.elevatedSubsystems
            .comparisonPopulatedRecordCount,
        3
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.size(),
        1
        );

    const InvestigationValueDifference
        &difference =
        result.elevatedSubsystems
            .differences.first();

    QCOMPARE(
        difference.value,
        QStringLiteral("Network")
        );

    QCOMPARE(
        difference.baselineCount,
        0
        );

    QCOMPARE(
        difference.comparisonCount,
        1
        );

    QVERIFY(
        difference.appearsOnlyInComparison()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    elevatedSubsystemDifferencesAreCompared()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning,
                QStringLiteral("Network")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Storage")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning,
                QStringLiteral("Network")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error,
                QStringLiteral("Network")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Critical,
                QStringLiteral("Modbus")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                QStringLiteral("Storage")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.elevatedSubsystems
            .comparable()
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.size(),
        2
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(0).value,
        QStringLiteral("Modbus")
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(0).baselineCount,
        0
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(0).comparisonCount,
        1
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(1).value,
        QStringLiteral("Network")
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(1).baselineCount,
        1
        );

    QCOMPARE(
        result.elevatedSubsystems
            .differences.at(1).comparisonCount,
        2
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    elevatedEntityDifferencesAreCompared()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                std::nullopt,
                std::nullopt,
                QStringLiteral("PLC-03")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning,
                std::nullopt,
                std::nullopt,
                QStringLiteral("RTU-07")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info,
                std::nullopt,
                std::nullopt,
                QStringLiteral("PLC-03")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error,
                std::nullopt,
                std::nullopt,
                QStringLiteral("PLC-03")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error,
                std::nullopt,
                std::nullopt,
                QStringLiteral("PLC-03")
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning,
                std::nullopt,
                std::nullopt,
                QStringLiteral("RTU-07")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.elevatedEntities.comparable()
        );

    /*
     * RTU-07 remains 1 -> 1 and is omitted.
     */
    QCOMPARE(
        result.elevatedEntities
            .differences.size(),
        1
        );

    const InvestigationValueDifference
        &difference =
        result.elevatedEntities
            .differences.first();

    QCOMPARE(
        difference.value,
        QStringLiteral("PLC-03")
        );

    QCOMPARE(
        difference.baselineCount,
        0
        );

    QCOMPARE(
        difference.comparisonCount,
        2
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    elevatedDimensionsAreUnavailableWithoutRequiredData()
{
    /*
     * Baseline records have severity but neither
     * subsystem nor entity data.
     */
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Info
                ),
            makeComparisonRecord(
                QString(),
                RecordSeverity::Warning
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeComparisonRecord(
                QString(),
                RecordSeverity::Error,
                QStringLiteral("Network"),
                std::nullopt,
                QStringLiteral("PLC-03")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        !result.elevatedSubsystems
             .comparable()
        );

    QVERIFY(
        result.elevatedSubsystems
            .differences.isEmpty()
        );

    QVERIFY(
        !result.elevatedEntities
             .comparable()
        );

    QVERIFY(
        result.elevatedEntities
            .differences.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    categoricalCustomFieldsShowAppearingAndDisappearingValues()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Firmware"),
                QStringLiteral("3.14.7")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Firmware"),
                QStringLiteral("3.14.7")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Firmware"),
                QStringLiteral("3.14.8-rc1")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Firmware"),
                QStringLiteral("3.14.8-rc1")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QCOMPARE(
        result.customFields
            .categoricalFields.size(),
        1
        );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );

    const InvestigationCategoricalCustomFieldComparison
        &firmware =
        result.customFields
            .categoricalFields.first();

    QCOMPARE(
        firmware.fieldName,
        QStringLiteral("Firmware")
        );

    QCOMPARE(
        firmware.changedValues.size(),
        2
        );

    QCOMPARE(
        firmware.changedValues.at(0).value,
        QStringLiteral("3.14.7")
        );

    QCOMPARE(
        firmware.changedValues.at(0)
            .baselineCount,
        2
        );

    QCOMPARE(
        firmware.changedValues.at(0)
            .comparisonCount,
        0
        );

    QVERIFY(
        firmware.changedValues.at(0)
            .appearsOnlyInBaseline()
        );

    QCOMPARE(
        firmware.changedValues.at(1).value,
        QStringLiteral("3.14.8-rc1")
        );

    QCOMPARE(
        firmware.changedValues.at(1)
            .baselineCount,
        0
        );

    QCOMPARE(
        firmware.changedValues.at(1)
            .comparisonCount,
        2
        );

    QVERIFY(
        firmware.changedValues.at(1)
            .appearsOnlyInComparison()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    categoricalFrequencyOnlyChangesAreIgnored()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Interface"),
                QStringLiteral("cell0")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Interface"),
                QStringLiteral("rs485-1")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Interface"),
                QStringLiteral("cell0")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Interface"),
                QStringLiteral("cell0")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Interface"),
                QStringLiteral("rs485-1")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    /*
     * Both sessions contain exactly the same set
     * of interface values. The changed occurrence
     * count of cell0 is intentionally not treated
     * as a custom-field investigation signal.
     */
    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    numericCustomFieldsSummarizeNumericStrings()
{
    /*
     * Key-value/logfmt imports preserve values such
     * as latencyMs as QString-backed QVariant values,
     * so numeric classification must not depend on
     * the QVariant storage type.
     */
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("18")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("34")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("50")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("18")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("38")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Latency (ms)"),
                QStringLiteral("850")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );

    QCOMPARE(
        result.customFields
            .numericFields.size(),
        1
        );

    const InvestigationNumericCustomFieldComparison
        &latency =
        result.customFields
            .numericFields.first();

    QCOMPARE(
        latency.fieldName,
        QStringLiteral("Latency (ms)")
        );

    QCOMPARE(
        latency.baseline
            .populatedRecordCount,
        3
        );

    QCOMPARE(
        latency.comparison
            .populatedRecordCount,
        3
        );

    QCOMPARE(
        latency.baseline.minimum,
        18.0
        );

    QCOMPARE(
        latency.baseline.median,
        34.0
        );

    QCOMPARE(
        latency.baseline.maximum,
        50.0
        );

    QCOMPARE(
        latency.comparison.minimum,
        18.0
        );

    QCOMPARE(
        latency.comparison.median,
        38.0
        );

    QCOMPARE(
        latency.comparison.maximum,
        850.0
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    unchangedNumericCustomFieldsAreIgnored()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                2
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                4
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                6
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                2
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                4
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Queue Depth"),
                6
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );

    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    highCardinalityCategoricalFieldsAreIgnored()
{
    QVector<InvestigationRecord>
        baselineRecords;

    QVector<InvestigationRecord>
        comparisonRecords;

    for (int index = 0;
         index < 11;
         ++index) {
        baselineRecords.append(
            makeCustomComparisonRecord(
                QStringLiteral("Request ID"),
                QStringLiteral("baseline-%1")
                    .arg(index)
                )
            );
    }

    for (int index = 0;
         index < 10;
         ++index) {
        comparisonRecords.append(
            makeCustomComparisonRecord(
                QStringLiteral("Request ID"),
                QStringLiteral("comparison-%1")
                    .arg(index)
                )
            );
    }

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    /*
     * Twenty-one distinct categorical values exceed
     * the automatic comparison cardinality limit.
     */
    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    mixedNumericAndTextValuesRemainCategorical()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Build"),
                QStringLiteral("100")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Build"),
                QStringLiteral("101")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Build"),
                QStringLiteral("100")
                ),
            makeCustomComparisonRecord(
                QStringLiteral("Build"),
                QStringLiteral("release-candidate")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );

    QCOMPARE(
        result.customFields
            .categoricalFields.size(),
        1
        );

    const InvestigationCategoricalCustomFieldComparison
        &build =
        result.customFields
            .categoricalFields.first();

    QCOMPARE(
        build.fieldName,
        QStringLiteral("Build")
        );

    QCOMPARE(
        build.changedValues.size(),
        2
        );

    QCOMPARE(
        build.changedValues.at(0).value,
        QStringLiteral("101")
        );

    QVERIFY(
        build.changedValues.at(0)
            .appearsOnlyInBaseline()
        );

    QCOMPARE(
        build.changedValues.at(1).value,
        QStringLiteral("release-candidate")
        );

    QVERIFY(
        build.changedValues.at(1)
            .appearsOnlyInComparison()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    structuredCustomValuesAreIgnored()
{
    InvestigationRecord baselineRecord =
        makeComparisonRecord();

    InvestigationRecord comparisonRecord =
        makeComparisonRecord();

    QVariantMap baselineDetails;

    baselineDetails.insert(
        QStringLiteral("mode"),
        QStringLiteral("normal")
        );

    QVariantMap comparisonDetails;

    comparisonDetails.insert(
        QStringLiteral("mode"),
        QStringLiteral("degraded")
        );

    baselineRecord.customAttributes.insert(
        QStringLiteral("Details"),
        baselineDetails
        );

    comparisonRecord.customAttributes.insert(
        QStringLiteral("Details"),
        comparisonDetails
        );

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            {baselineRecord},
            {comparisonRecord}
            );

    /*
     * Objects and arrays remain preserved source
     * data, but are not automatically reduced to
     * scalar comparison semantics.
     */
    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );
}

void InvestigationSessionComparisonAnalyzerTests::
    customFieldsMustExistInBothSessions()
{
    const QVector<InvestigationRecord>
        baselineRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Firmware"),
                QStringLiteral("3.14.7")
                )
        };

    const QVector<InvestigationRecord>
        comparisonRecords = {
            makeCustomComparisonRecord(
                QStringLiteral("Environment"),
                QStringLiteral("Field")
                )
        };

    InvestigationSessionComparisonAnalyzer
        analyzer;

    const InvestigationSessionComparison result =
        analyzer.compare(
            baselineRecords,
            comparisonRecords
            );

    /*
     * Differently named fields are not assumed to
     * represent equivalent concepts.
     */
    QVERIFY(
        result.customFields
            .categoricalFields.isEmpty()
        );

    QVERIFY(
        result.customFields
            .numericFields.isEmpty()
        );
}
QTEST_MAIN(
    InvestigationSessionComparisonAnalyzerTests
    )

#include "InvestigationSessionComparisonAnalyzerTests.moc"