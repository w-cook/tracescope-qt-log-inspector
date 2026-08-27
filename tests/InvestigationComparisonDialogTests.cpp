#include <memory>

#include <QtTest/QtTest>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimeZone>

#include "../src/ui/InvestigationComparisonDialog.h"
#include "../src/workspace/InvestigationWorkspace.h"

namespace
{

InvestigationRecord makeRecord(
    const QString &recordId,
    qint64 timestampMilliseconds
    )
{
    InvestigationRecord record;

    record.recordId =
        recordId;

    record.timestamp =
        QDateTime::fromMSecsSinceEpoch(
            timestampMilliseconds,
            QTimeZone::UTC
            );

    record.severity =
        RecordSeverity::Warning;

    return record;
}

ImportResult makeCadenceResult(
    const QString &prefix,
    qint64 gapMilliseconds
    )
{
    ImportResult result;

    /*
     * Eleven records produce ten positive gaps,
     * which is enough for the adaptive cadence
     * recommendation rather than the sparse-data
     * fallback.
     */
    for (int index = 0;
         index < 11;
         ++index) {
        result.records.append(
            makeRecord(
                QStringLiteral("%1-%2")
                    .arg(prefix)
                    .arg(index),
                static_cast<qint64>(index)
                    * gapMilliseconds
                )
            );
    }

    result.processedRecordCount =
        result.records.size();

    return result;
}

ImportResult makeSimpleResult(
    const QString &recordId
    )
{
    ImportResult result;

    InvestigationRecord record;

    record.recordId =
        recordId;

    record.severity =
        RecordSeverity::Info;

    result.records.append(
        record
        );

    result.processedRecordCount =
        result.records.size();

    return result;
}

QString addSession(
    InvestigationWorkspace &workspace,
    const QString &path,
    ImportResult result
    )
{
    ImportProfile profile;

    auto session =
        std::make_unique<InvestigationSession>(
            path,
            profile,
            std::move(result)
            );

    const QString id =
        session->id();

    workspace.addSession(
        std::move(session)
        );

    return id;
}

}

class InvestigationComparisonDialogTests
    : public QObject
{
    Q_OBJECT

private slots:
    void activeSessionDefaultsToComparison();
    void preferredInactiveSessionDefaultsToBaseline();
    void swapReversesOrientation();
    void sameSessionSelectionDisablesCreation();
    void sharedBurstDefaultsUseBothCompleteSessions();
};

void InvestigationComparisonDialogTests::
    activeSessionDefaultsToComparison()
{
    InvestigationWorkspace workspace;

    const QString baselineId =
        addSession(
            workspace,
            QStringLiteral("known-good.jsonl"),
            makeSimpleResult(
                QStringLiteral("baseline-1")
                )
            );

    const QString activeId =
        addSession(
            workspace,
            QStringLiteral("degraded.jsonl"),
            makeSimpleResult(
                QStringLiteral("comparison-1")
                )
            );

    QCOMPARE(
        workspace.activeSession()->id(),
        activeId
        );

    InvestigationComparisonDialog dialog(
        &workspace,
        QString(),
        activeId
        );

    QCOMPARE(
        dialog.comparisonSessionId(),
        activeId
        );

    QCOMPARE(
        dialog.baselineSessionId(),
        baselineId
        );
}

void InvestigationComparisonDialogTests::
    preferredInactiveSessionDefaultsToBaseline()
{
    InvestigationWorkspace workspace;

    const QString knownGoodId =
        addSession(
            workspace,
            QStringLiteral("known-good.jsonl"),
            makeSimpleResult(
                QStringLiteral("baseline-1")
                )
            );

    const QString degradedId =
        addSession(
            workspace,
            QStringLiteral("degraded.jsonl"),
            makeSimpleResult(
                QStringLiteral("comparison-1")
                )
            );

    QCOMPARE(
        workspace.activeSession()->id(),
        degradedId
        );

    /*
     * This is the orientation supplied by the tab
     * context-menu workflow:
     *
     * clicked inactive session -> Baseline
     * active session           -> Comparison
     */
    InvestigationComparisonDialog dialog(
        &workspace,
        knownGoodId,
        degradedId
        );

    QCOMPARE(
        dialog.baselineSessionId(),
        knownGoodId
        );

    QCOMPARE(
        dialog.comparisonSessionId(),
        degradedId
        );
}

void InvestigationComparisonDialogTests::
    swapReversesOrientation()
{
    InvestigationWorkspace workspace;

    const QString baselineId =
        addSession(
            workspace,
            QStringLiteral("known-good.jsonl"),
            makeSimpleResult(
                QStringLiteral("baseline-1")
                )
            );

    const QString comparisonId =
        addSession(
            workspace,
            QStringLiteral("degraded.jsonl"),
            makeSimpleResult(
                QStringLiteral("comparison-1")
                )
            );

    InvestigationComparisonDialog dialog(
        &workspace,
        baselineId,
        comparisonId
        );

    QPushButton *swapButton =
        dialog.findChild<QPushButton *>(
            QStringLiteral(
                "swapSessionsButton"
                )
            );

    QVERIFY(
        swapButton != nullptr
        );

    swapButton->click();

    QCOMPARE(
        dialog.baselineSessionId(),
        comparisonId
        );

    QCOMPARE(
        dialog.comparisonSessionId(),
        baselineId
        );

    swapButton->click();

    QCOMPARE(
        dialog.baselineSessionId(),
        baselineId
        );

    QCOMPARE(
        dialog.comparisonSessionId(),
        comparisonId
        );
}

void InvestigationComparisonDialogTests::
    sameSessionSelectionDisablesCreation()
{
    InvestigationWorkspace workspace;

    const QString baselineId =
        addSession(
            workspace,
            QStringLiteral("known-good.jsonl"),
            makeSimpleResult(
                QStringLiteral("baseline-1")
                )
            );

    const QString comparisonId =
        addSession(
            workspace,
            QStringLiteral("degraded.jsonl"),
            makeSimpleResult(
                QStringLiteral("comparison-1")
                )
            );

    InvestigationComparisonDialog dialog(
        &workspace,
        baselineId,
        comparisonId
        );

    QComboBox *baselineCombo =
        dialog.findChild<QComboBox *>(
            QStringLiteral(
                "baselineSessionCombo"
                )
            );

    QComboBox *comparisonCombo =
        dialog.findChild<QComboBox *>(
            QStringLiteral(
                "comparisonSessionCombo"
                )
            );

    QDialogButtonBox *buttons =
        dialog.findChild<QDialogButtonBox *>(
            QStringLiteral(
                "comparisonDialogButtons"
                )
            );

    QVERIFY(
        baselineCombo != nullptr
        );

    QVERIFY(
        comparisonCombo != nullptr
        );

    QVERIFY(
        buttons != nullptr
        );

    QPushButton *okButton =
        buttons->button(
            QDialogButtonBox::Ok
            );

    QVERIFY(
        okButton != nullptr
        );

    QVERIFY(
        okButton->isEnabled()
        );

    const int sameIndex =
        baselineCombo->findData(
            comparisonId
            );

    QVERIFY(
        sameIndex >= 0
        );

    baselineCombo->setCurrentIndex(
        sameIndex
        );

    QCOMPARE(
        dialog.baselineSessionId(),
        dialog.comparisonSessionId()
        );

    QVERIFY(
        !okButton->isEnabled()
        );
}

void InvestigationComparisonDialogTests::
    sharedBurstDefaultsUseBothCompleteSessions()
{
    InvestigationWorkspace workspace;

    /*
     * Fast cadence:
     *
     * 100 ms gaps over a 1-second span.
     * The cadence analyzer constrains the resulting
     * recommendation to a 200 ms window and 50 ms
     * merge gap.
     */
    const QString fastId =
        addSession(
            workspace,
            QStringLiteral("fast.jsonl"),
            makeCadenceResult(
                QStringLiteral("fast"),
                100
                )
            );

    /*
     * Slow cadence:
     *
     * 1-second gaps over a 10-second span.
     * The recommendation is a 2-second window and
     * 500 ms merge gap.
     *
     * The comparison dialog should therefore choose
     * the larger shared values:
     *
     * window    = 2000 ms
     * merge gap = 500 ms
     */
    const QString slowId =
        addSession(
            workspace,
            QStringLiteral("slow.jsonl"),
            makeCadenceResult(
                QStringLiteral("slow"),
                1000
                )
            );

    InvestigationSession *slowSession =
        workspace.sessionAt(
            workspace.indexOfSession(
                slowId
                )
            );

    QVERIFY(
        slowSession != nullptr
        );

    /*
     * Make the visible investigation much smaller.
     * Shared automatic defaults must still use the
     * complete imported session rather than the
     * filtered view.
     */
    slowSession
        ->investigationController()
        ->setFilters(
            QString(),
            QString(),
            QStringLiteral(
                "does-not-match"
                )
            );

    QCOMPARE(
        slowSession
            ->investigationController()
            ->recordsForAnalysis()
            .size(),
        0
        );

    QCOMPARE(
        slowSession
            ->investigationController()
            ->allRecords()
            .size(),
        11
        );

    InvestigationComparisonDialog dialog(
        &workspace,
        fastId,
        slowId
        );

    const std::optional<BurstDetectionSettings>
        settings =
        dialog.burstSettings();

    QVERIFY(
        settings.has_value()
        );

    QCOMPARE(
        settings->windowMilliseconds,
        static_cast<qint64>(2000)
        );

    QCOMPARE(
        settings->mergeGapMilliseconds,
        static_cast<qint64>(500)
        );
}

QTEST_MAIN(
    InvestigationComparisonDialogTests
    )

#include "InvestigationComparisonDialogTests.moc"