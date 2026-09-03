#include <QtTest/QtTest>

#include "../src/exporting/InvestigationReportHtmlRenderer.h"

namespace
{

QDateTime fixedTimestamp(
    int secondsFromStart = 0
    )
{
    return QDateTime::fromString(
               QStringLiteral(
                   "2026-09-03T15:00:00.000Z"
                   ),
               Qt::ISODateWithMs
               )
        .addSecs(secondsFromStart);
}

InvestigationReportSessionSnapshot makeSession(
    const QString &sessionId,
    const QString &documentTitle,
    const QString &sourceName
    )
{
    InvestigationReportSessionSnapshot session;

    session.source.sessionId =
        sessionId;

    session.source.documentTitle =
        documentTitle;

    session.source.sourceName =
        sourceName;

    session.source.importerId =
        QStringLiteral("json-lines");

    session.source.processedRecordCount = 3;
    session.source.importedRecordCount = 3;

    session.recordContext.totalRecordCount = 3;
    session.recordContext.visibleRecordCount = 2;
    session.recordContext.analysisRecordCount = 3;

    session.visibleSeveritySummary.warningCount = 1;
    session.visibleSeveritySummary.errorCount = 1;

    session.analysisSeveritySummary.infoCount = 1;
    session.analysisSeveritySummary.warningCount = 1;
    session.analysisSeveritySummary.errorCount = 1;

    return session;
}

InvestigationReportComparisonSnapshot makeComparison()
{
    InvestigationReportComparisonSnapshot comparison;

    comparison.comparisonId =
        QStringLiteral("comparison-1");

    comparison.documentTitle =
        QStringLiteral("Known Good → Degraded");

    comparison.baselineSessionId =
        QStringLiteral("baseline");

    comparison.comparisonSessionId =
        QStringLiteral("degraded");

    comparison.baselineSourceName =
        QStringLiteral("known-good.jsonl");

    comparison.comparisonSourceName =
        QStringLiteral("degraded.jsonl");

    comparison.analysis.totalRecords.baselineCount =
        100;

    comparison.analysis.totalRecords.comparisonCount =
        130;

    return comparison;
}

}

class InvestigationReportHtmlRendererTests
    : public QObject
{
    Q_OBJECT

private slots:
    void rendersSelfContainedHtmlShell();
    void escapesUntrustedText();
    void omitsLocalSourcePaths();
    void preservesReportSectionOrder();
    void rendersImmutableComparisonProvenance();
    void rendersCrossSourceClockCaveat();
    void rendersTimelineAndDeterministicAnalytics();
    void rendersCadenceBurstsAndEvidence();
    void neverRendersEvidenceSourcePath();
    void rendersTechnicalImportAppendix();
    void omitsTechnicalProfileWhenDisabled();
};

void InvestigationReportHtmlRendererTests::
    rendersSelfContainedHtmlShell()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Test Investigation");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.sessions.append(
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            )
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.startsWith(
            QStringLiteral("<!DOCTYPE html>")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "<meta charset=\"utf-8\">"
                )
            )
        );

    /*
     * The report shell and persistent navigation are
     * embedded directly into the generated document.
     */
    QVERIFY(
        html.contains(
            QStringLiteral(
                "class=\"report-nav\""
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "position: sticky"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "@media print"
                )
            )
        );

    /*
     * No external stylesheet, script, font, CDN, or
     * companion resource should be required.
     */
    QVERIFY(
        !html.contains(
            QStringLiteral("<link"),
            Qt::CaseInsensitive
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral("<script src="),
            Qt::CaseInsensitive
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral("http://"),
            Qt::CaseInsensitive
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral("https://"),
            Qt::CaseInsensitive
            )
        );
}

void InvestigationReportHtmlRendererTests::
    escapesUntrustedText()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral(
            "<script>alert('title')</script>"
            );

    snapshot.context =
        QStringLiteral(
            "Context <b>must not render as markup</b> & evidence"
            );

    snapshot.generatedAtUtc =
        fixedTimestamp();

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("source"),
            QStringLiteral(
                "<img src=x onerror=alert(1)>"
                ),
            QStringLiteral(
                "source<&>.jsonl"
                )
            );

    session.filters.searchText =
        QStringLiteral(
            "<script>search()</script>"
            );

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "<script>alert('title')</script>"
                )
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "<b>must not render as markup</b>"
                )
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "<img src=x onerror=alert(1)>"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("&lt;script&gt;alert(")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("&lt;/script&gt;")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Context &lt;b&gt;must not render as markup&lt;/b&gt; &amp; evidence"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "source&lt;&amp;&gt;.jsonl"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    omitsLocalSourcePaths()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Privacy Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            );

    InvestigationReportEvidenceRecord evidence;

    evidence.record.recordId =
        QStringLiteral("record-1");

    evidence.record.source.sourceName =
        QStringLiteral("gateway.jsonl");

    evidence.record.source.sourcePath =
        QStringLiteral(
            "C:/Users/William/private/customer-data/gateway.jsonl"
            );

    evidence.record.rawSource =
        QStringLiteral(
            "{\"message\":\"safe raw evidence\"}"
            );

    evidence.state.note =
        QStringLiteral("Important evidence");

    session.evidenceRecords.append(
        evidence
        );

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "C:/Users/William/private/customer-data"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("gateway.jsonl")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Local workstation source paths are intentionally"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    preservesReportSectionOrder()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Ordering Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.comparisons.append(
        makeComparison()
        );

    snapshot.sessions.append(
        makeSession(
            QStringLiteral("scheduler"),
            QStringLiteral("Scheduler Source"),
            QStringLiteral("scheduler.csv")
            )
        );

    snapshot.sessions.append(
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway Source"),
            QStringLiteral("gateway.jsonl")
            )
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    const qsizetype comparisonPosition =
        html.indexOf(
            QStringLiteral(
                "id=\"comparison-1\""
                )
            );

    const qsizetype firstSessionPosition =
        html.indexOf(
            QStringLiteral(
                "id=\"session-1\""
                )
            );

    const qsizetype secondSessionPosition =
        html.indexOf(
            QStringLiteral(
                "id=\"session-2\""
                )
            );

    QVERIFY(
        comparisonPosition >= 0
        );

    QVERIFY(
        firstSessionPosition
        > comparisonPosition
        );

    QVERIFY(
        secondSessionPosition
        > firstSessionPosition
        );

    /*
     * Session snapshot order remains report order.
     */
    const qsizetype schedulerPosition =
        html.indexOf(
            QStringLiteral("Scheduler Source")
            );

    const qsizetype gatewayPosition =
        html.indexOf(
            QStringLiteral("Gateway Source")
            );

    QVERIFY(
        schedulerPosition >= 0
        );

    QVERIFY(
        gatewayPosition
        > schedulerPosition
        );
}

void InvestigationReportHtmlRendererTests::
    rendersImmutableComparisonProvenance()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Comparison Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.comparisons.append(
        makeComparison()
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.contains(
            QStringLiteral("known-good.jsonl")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("degraded.jsonl")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(">100<")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(">130<")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("+30")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "immutable analysis captured by the TraceScope comparison document"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "It was not recalculated from current session state"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    rendersCrossSourceClockCaveat()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Cross Source Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    InvestigationReportSourceTimeCoverage coverage;

    coverage.sessionId =
        QStringLiteral("gateway");

    coverage.sourceName =
        QStringLiteral("gateway.jsonl");

    coverage.timestampedRecordCount =
        5;

    coverage.firstTimestamp =
        fixedTimestamp();

    coverage.lastTimestamp =
        fixedTimestamp(10);

    snapshot.sourceTimeCoverage.append(
        coverage
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Cross-source chronology reflects timestamps as imported by TraceScope."
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Clock synchronization or clock skew between independent sources is not inferred."
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    rendersTimelineAndDeterministicAnalytics()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Analytics Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("controller"),
            QStringLiteral("Controller"),
            QStringLiteral("controller.xml")
            );

    session.timeline.available =
        true;

    session.timeline.intervalMilliseconds =
        1000;

    EventCountBucket firstBucket;

    firstBucket.label =
        QStringLiteral("15:00:00");

    firstBucket.infoCount = 2;
    firstBucket.warningCount = 1;

    EventCountBucket secondBucket;

    secondBucket.label =
        QStringLiteral(
            "<15:00:01>"
            );

    secondBucket.errorCount = 1;
    secondBucket.criticalCount = 1;

    session.timeline.buckets = {
        firstBucket,
        secondBucket
    };

    TelemetryIssueGroup issueGroup;

    issueGroup.subsystem =
        QStringLiteral("Network");

    issueGroup.warningCount = 2;
    issueGroup.errorCount = 3;

    session.elevatedIssueGroups.append(
        issueGroup
        );

    InvestigationValueFrequency eventCode;

    eventCode.value =
        QStringLiteral("NET_TIMEOUT");

    eventCode.count = 4;

    session.eventCodeFrequencies.append(
        eventCode
        );

    InvestigationValueFrequency entity;

    entity.value =
        QStringLiteral("device-17");

    entity.count = 3;

    session.entityFrequencies.append(
        entity
        );

    InvestigationValueFrequency subsystem;

    subsystem.value =
        QStringLiteral("Network");

    subsystem.count = 5;

    session.subsystemFrequencies.append(
        subsystem
        );

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Event Activity Timeline"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "aria-label=\"Event activity timeline\""
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("<svg")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("1.0 s buckets")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("15:00:00")
            )
        );

    /*
     * SVG labels remain escaped just like ordinary
     * report text.
     */
    QVERIFY(
        !html.contains(
            QStringLiteral(
                "<15:00:01>"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "&lt;15:00:01&gt;"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Subsystem Issue Groups"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("Network")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("NET_TIMEOUT")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("device-17")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Deterministic Analytics"
                )
            )
        );
}
void InvestigationReportHtmlRendererTests::
    rendersCadenceBurstsAndEvidence()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral(
            "Evidence Test"
            );

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.supportingEvidenceIncluded =
        true;

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            );

    session.burstAnalysis.available =
        true;

    session.burstAnalysis.timingMode =
        InvestigationReportBurstTimingMode::Auto;

    session
        .burstAnalysis
        .cadence
        .timestampCount = 20;

    session
        .burstAnalysis
        .cadence
        .positiveGapCount = 18;

    session
        .burstAnalysis
        .cadence
        .zeroGapCount = 1;

    session
        .burstAnalysis
        .cadence
        .medianPositiveGapMilliseconds =
        500.0;

    InvestigationBurst burst;

    burst.startTimestamp =
        fixedTimestamp(5);

    burst.endTimestamp =
        fixedTimestamp(8);

    burst.warningCount = 2;
    burst.errorCount = 1;

    burst.triggeredByElevatedThreshold =
        true;

    session
        .burstAnalysis
        .bursts
        .append(
            burst
            );

    InvestigationReportEvidenceRecord evidence;

    evidence.record.recordId =
        QStringLiteral("record-17");

    evidence.record.timestamp =
        fixedTimestamp(6);

    evidence.record.severity =
        RecordSeverity::Error;

    evidence.record.message =
        QStringLiteral(
            "Gateway request failed"
            );

    evidence.record.source.sourceName =
        QStringLiteral(
            "gateway.jsonl"
            );

    evidence.record.source.recordNumber =
        17;

    evidence.record.customAttributes.insert(
        QStringLiteral("attempt"),
        3
        );

    evidence.record.customAttributes.insert(
        QStringLiteral("payload"),
        QVariantMap {
            {
                QStringLiteral("safe"),
                true
            }
        }
        );

    evidence.record.rawSource =
        QStringLiteral(
            "<raw>& data"
            );

    evidence.state.bookmarked =
        true;

    evidence.state.findingStatus =
        FindingStatus::Open;

    evidence.state.note =
        QStringLiteral(
            "Reproduced during QA run."
            );

    evidence.burstEvidence =
        true;

    session.evidenceRecords.append(
        evidence
        );

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Timestamp Cadence"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Burst Analysis"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Detected Bursts"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Gateway request failed"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Finding: Open"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Reproduced during QA run."
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Source record"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "gateway.jsonl"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "&lt;raw&gt;&amp; data"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "{&quot;safe&quot;:true}"
                )
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "{\"safe\":true}"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    neverRendersEvidenceSourcePath()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Path Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            );

    InvestigationReportEvidenceRecord evidence;

    evidence.record.recordId =
        QStringLiteral("record");

    /*
     * Deliberately violate the normal sanitized
     * snapshot invariant. The renderer should still
     * refuse to expose the path.
     */
    evidence.record.source.sourcePath =
        QStringLiteral(
            "C:/private/customer/internal/gateway.jsonl"
            );

    evidence.record.source.sourceName =
        QStringLiteral("gateway.jsonl");

    evidence.state.note =
        QStringLiteral("Evidence");

    session.evidenceRecords.append(
        evidence
        );

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "C:/private/customer/internal"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "gateway.jsonl"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    rendersTechnicalImportAppendix()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("Import Context Test");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.technicalAppendixIncluded =
        true;

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            );

    session.source.sourceSizeBytes =
        2048;

    session.source.importProfileName =
        QStringLiteral("Gateway Profile");

    ImportDiagnostic diagnostic;

    diagnostic.severity =
        ImportDiagnosticSeverity::Warning;

    diagnostic.code =
        QStringLiteral("MISSING_FIELD");

    diagnostic.message =
        QStringLiteral(
            "Optional field <entity> was unavailable."
            );

    RecordSourceMetadata diagnosticSource;

    diagnosticSource.sourcePath =
        QStringLiteral(
            "C:/private/customer/gateway.jsonl"
            );

    diagnosticSource.sourceName =
        QStringLiteral("gateway.jsonl");

    diagnosticSource.recordNumber =
        12;

    diagnostic.source =
        diagnosticSource;

    session.source.diagnostics.append(
        diagnostic
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral("Gateway Profile");

    profile.importerId =
        QStringLiteral("json-lines");

    profile.recordPath =
        QStringLiteral("$.events[*]");

    profile
        .canonicalFields
        .timestampPath =
        QStringLiteral("$.time");

    profile.customFields.append(
        {
            QStringLiteral("queueDepth"),
            QStringLiteral("$.metrics.queueDepth")
        }
        );

    profile.severityAliases.insert(
        QStringLiteral("fatal"),
        RecordSeverity::Critical
        );

    TimestampRule timestampRule;

    timestampRule.type =
        TimestampRuleType::QtFormat;

    timestampRule.format =
        QStringLiteral(
            "yyyy-MM-dd HH:mm:ss.zzz"
            );

    profile.timestampRules = {
        timestampRule
    };

    session.source.technicalImportProfile =
        profile;

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Import and Data-Quality Context"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Import Diagnostics (1)"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "MISSING_FIELD"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Optional field &lt;entity&gt; was unavailable."
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Complete Import Profile"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "$.events[*]"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "$.metrics.queueDepth"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("fatal")
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral("Critical")
            )
        );

    /*
     * Diagnostic source paths also stay private.
     */
    QVERIFY(
        !html.contains(
            QStringLiteral(
                "C:/private/customer"
                )
            )
        );
}

void InvestigationReportHtmlRendererTests::
    omitsTechnicalProfileWhenDisabled()
{
    InvestigationReportSnapshot snapshot;

    snapshot.title =
        QStringLiteral("No Appendix");

    snapshot.generatedAtUtc =
        fixedTimestamp();

    snapshot.technicalAppendixIncluded =
        false;

    InvestigationReportSessionSnapshot session =
        makeSession(
            QStringLiteral("gateway"),
            QStringLiteral("Gateway"),
            QStringLiteral("gateway.jsonl")
            );

    ImportDiagnostic diagnostic;

    diagnostic.code =
        QStringLiteral("VISIBLE_DIAGNOSTIC");

    diagnostic.message =
        QStringLiteral(
            "Diagnostics remain report content."
            );

    session.source.diagnostics.append(
        diagnostic
        );

    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "SHOULD_NOT_BE_RENDERED"
            );

    session.source.technicalImportProfile =
        profile;

    snapshot.sessions.append(
        session
        );

    InvestigationReportHtmlRenderer renderer;

    const QString html =
        renderer.render(snapshot);

    /*
     * Data-quality diagnostics remain visible.
     */
    QVERIFY(
        html.contains(
            QStringLiteral(
                "VISIBLE_DIAGNOSTIC"
                )
            )
        );

    QVERIFY(
        html.contains(
            QStringLiteral(
                "Diagnostics remain report content."
                )
            )
        );

    /*
     * Optional technical profile does not.
     */
    QVERIFY(
        !html.contains(
            QStringLiteral(
                "SHOULD_NOT_BE_RENDERED"
                )
            )
        );

    QVERIFY(
        !html.contains(
            QStringLiteral(
                "Complete Import Profile"
                )
            )
        );
}

QTEST_APPLESS_MAIN(
    InvestigationReportHtmlRendererTests
    )

#include "InvestigationReportHtmlRendererTests.moc"