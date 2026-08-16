#include <QtTest/QtTest>

#include <algorithm>

#include "../src/analysis/TelemetryIssueAnalyzer.h"
#include "../src/domain/RecordSeverity.h"

class IssueAnalyzerTests : public QObject
{
    Q_OBJECT

private slots:
    void groupWarningsAndErrorsBySubsystemIgnoresInfoEvents();
    void groupWarningsAndErrorsBySubsystemCountsWarningsAndErrors();
    void groupWarningsAndErrorsBySubsystemSortsByTotalDescending();
    void groupWarningsAndErrorsBySubsystemCountsCriticalAsError();
};

static InvestigationRecord makeRecord(
    RecordSeverity severity,
    const QString &subsystem
    )
{
    InvestigationRecord record;
    record.severity = severity;
    record.subsystem = subsystem;

    return record;
}

static QVector<InvestigationRecord> sampleRecords()
{
    return {
        makeRecord(
            RecordSeverity::Info,
            QStringLiteral("Startup")
            ),
        makeRecord(
            RecordSeverity::Warning,
            QStringLiteral("Tracking")
            ),
        makeRecord(
            RecordSeverity::Error,
            QStringLiteral("Comms")
            ),
        makeRecord(
            RecordSeverity::Warning,
            QStringLiteral("Comms")
            )
    };
}

void IssueAnalyzerTests::
    groupWarningsAndErrorsBySubsystemIgnoresInfoEvents()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups =
        analyzer.groupWarningsAndErrorsBySubsystem(
            sampleRecords()
            );

    QCOMPARE(groups.size(), 2);
}

void IssueAnalyzerTests::
    groupWarningsAndErrorsBySubsystemCountsWarningsAndErrors()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups =
        analyzer.groupWarningsAndErrorsBySubsystem(
            sampleRecords()
            );

    const auto commsGroup =
        std::find_if(
            groups.begin(),
            groups.end(),
            [](const TelemetryIssueGroup &group) {
                return group.subsystem
                       == QStringLiteral("Comms");
            }
            );

    QVERIFY(commsGroup != groups.end());

    QCOMPARE(
        commsGroup->warningCount,
        1
        );

    QCOMPARE(
        commsGroup->errorCount,
        1
        );

    QCOMPARE(
        commsGroup->totalCount(),
        2
        );
}

void IssueAnalyzerTests::
    groupWarningsAndErrorsBySubsystemSortsByTotalDescending()
{
    TelemetryIssueAnalyzer analyzer;

    const auto groups =
        analyzer.groupWarningsAndErrorsBySubsystem(
            sampleRecords()
            );

    QCOMPARE(groups.size(), 2);

    QCOMPARE(
        groups.at(0).subsystem,
        QStringLiteral("Comms")
        );

    QCOMPARE(
        groups.at(0).totalCount(),
        2
        );

    QCOMPARE(
        groups.at(1).subsystem,
        QStringLiteral("Tracking")
        );

    QCOMPARE(
        groups.at(1).totalCount(),
        1
        );
}

void IssueAnalyzerTests::
    groupWarningsAndErrorsBySubsystemCountsCriticalAsError()
{
    const QVector<InvestigationRecord> records = {
        makeRecord(
            RecordSeverity::Critical,
            QStringLiteral("TelemetryPipeline")
            )
    };

    TelemetryIssueAnalyzer analyzer;

    const auto groups =
        analyzer.groupWarningsAndErrorsBySubsystem(
            records
            );

    QCOMPARE(groups.size(), 1);

    QCOMPARE(
        groups.first().subsystem,
        QStringLiteral("TelemetryPipeline")
        );

    QCOMPARE(
        groups.first().warningCount,
        0
        );

    QCOMPARE(
        groups.first().errorCount,
        1
        );

    QCOMPARE(
        groups.first().totalCount(),
        1
        );
}

QTEST_MAIN(IssueAnalyzerTests)

#include "IssueAnalyzerTests.moc"
