#include <QtTest/QtTest>

#include "../src/exporting/InvestigationReportSelectionModel.h"

namespace
{

QVector<InvestigationReportSessionSelection>
sessions()
{
    return {
        {
            QStringLiteral("gateway"),
            QStringLiteral("gateway.jsonl"),
            false
        },
        {
            QStringLiteral("scheduler"),
            QStringLiteral("scheduler.csv"),
            false
        },
        {
            QStringLiteral("controller"),
            QStringLiteral("controller.xml"),
            false
        }
    };
}

InvestigationReportComparisonSelection
comparison(
    const QString &comparisonId =
    QStringLiteral("gateway-comparison"),
    const QString &baselineSessionId =
    QStringLiteral("gateway"),
    const QString &comparisonSessionId =
    QStringLiteral("controller")
    )
{
    InvestigationReportComparisonSelection result;

    result.comparisonId =
        comparisonId;

    result.documentTitle =
        QStringLiteral(
            "Gateway → Controller"
            );

    result.baselineSessionId =
        baselineSessionId;

    result.comparisonSessionId =
        comparisonSessionId;

    return result;
}

}

class InvestigationReportSelectionModelTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sessionOriginSelectsOnlyOriginSession();
    void comparisonOriginSelectsComparisonAndOpenDependencies();
    void comparisonRemainsSelectableWithClosedDependencies();
    void selectingComparisonAddsDependencies();
    void deselectingComparisonRetainsSessions();
    void selectedIdsPreserveWorkspaceOrder();
    void duplicateDescriptorsAreNormalized();
    void unknownSelectionsDoNotMutateModel();
};

void InvestigationReportSelectionModelTests::
    sessionOriginSelectsOnlyOriginSession()
{
    InvestigationReportSelectionOrigin origin;

    origin.type =
        InvestigationReportSelectionOriginType::Session;

    origin.documentId =
        QStringLiteral("scheduler");

    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison()
        },
        origin
        );

    QCOMPARE(
        model.selectedSessionIds(),
        QStringList {
            QStringLiteral("scheduler")
        }
        );

    QVERIFY(
        model.selectedComparisonIds().isEmpty()
        );

    QVERIFY(
        model.hasSelection()
        );
}

void InvestigationReportSelectionModelTests::
    comparisonOriginSelectsComparisonAndOpenDependencies()
{
    InvestigationReportSelectionOrigin origin;

    origin.type =
        InvestigationReportSelectionOriginType::Comparison;

    origin.documentId =
        QStringLiteral("gateway-comparison");

    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison()
        },
        origin
        );

    QCOMPARE(
        model.selectedComparisonIds(),
        QStringList {
            QStringLiteral("gateway-comparison")
        }
        );

    /*
     * Output follows workspace/session order rather
     * than comparison baseline/comparison order.
     */
    const QStringList expectedSessionIds {
        QStringLiteral("gateway"),
        QStringLiteral("controller")
    };

    QCOMPARE(
        model.selectedSessionIds(),
        expectedSessionIds
        );
}

void InvestigationReportSelectionModelTests::
    comparisonRemainsSelectableWithClosedDependencies()
{
    InvestigationReportSelectionOrigin origin;

    origin.type =
        InvestigationReportSelectionOriginType::Comparison;

    origin.documentId =
        QStringLiteral("archived-comparison");

    InvestigationReportSelectionModel model(
        {
            {
                QStringLiteral("gateway"),
                QStringLiteral("gateway.jsonl"),
                false
            }
        },
        {
            comparison(
                QStringLiteral("archived-comparison"),
                QStringLiteral("closed-baseline"),
                QStringLiteral("closed-comparison")
                )
        },
        origin
        );

    QCOMPARE(
        model.selectedComparisonIds(),
        QStringList {
            QStringLiteral("archived-comparison")
        }
        );

    /*
     * Neither source session is currently open, but
     * the immutable comparison document remains a
     * valid report selection on its own.
     */
    QVERIFY(
        model.selectedSessionIds().isEmpty()
        );

    QVERIFY(
        model.hasSelection()
        );
}

void InvestigationReportSelectionModelTests::
    selectingComparisonAddsDependencies()
{
    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison()
        },
        {}
        );

    QVERIFY(
        !model.hasSelection()
        );

    QVERIFY(
        model.setSessionSelected(
            QStringLiteral("scheduler"),
            true
            )
        );

    QVERIFY(
        model.setComparisonSelected(
            QStringLiteral("gateway-comparison"),
            true
            )
        );

    const QStringList expectedSessionIds {
        QStringLiteral("gateway"),
        QStringLiteral("scheduler"),
        QStringLiteral("controller")
    };

    QCOMPARE(
        model.selectedSessionIds(),
        expectedSessionIds
        );

    QCOMPARE(
        model.selectedComparisonIds(),
        QStringList {
            QStringLiteral("gateway-comparison")
        }
        );
}

void InvestigationReportSelectionModelTests::
    deselectingComparisonRetainsSessions()
{
    InvestigationReportSelectionOrigin origin;

    origin.type =
        InvestigationReportSelectionOriginType::Comparison;

    origin.documentId =
        QStringLiteral("gateway-comparison");

    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison()
        },
        origin
        );

    QVERIFY(
        model.setComparisonSelected(
            QStringLiteral("gateway-comparison"),
            false
            )
        );

    QVERIFY(
        model.selectedComparisonIds().isEmpty()
        );

    /*
     * Comparison deselection must not guess why its
     * sessions are selected. They remain available as
     * independent report sources.
     */
    const QStringList expectedSessionIds {
        QStringLiteral("gateway"),
        QStringLiteral("controller")
    };

    QCOMPARE(
        model.selectedSessionIds(),
        expectedSessionIds
        );

    QVERIFY(
        model.hasSelection()
        );
}

void InvestigationReportSelectionModelTests::
    selectedIdsPreserveWorkspaceOrder()
{
    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison(
                QStringLiteral("comparison-b"),
                QStringLiteral("scheduler"),
                QStringLiteral("controller")
                ),
            comparison(
                QStringLiteral("comparison-a"),
                QStringLiteral("gateway"),
                QStringLiteral("scheduler")
                )
        },
        {}
        );

    /*
     * Select in deliberately different order.
     */
    QVERIFY(
        model.setSessionSelected(
            QStringLiteral("controller"),
            true
            )
        );

    QVERIFY(
        model.setSessionSelected(
            QStringLiteral("gateway"),
            true
            )
        );

    QVERIFY(
        model.setComparisonSelected(
            QStringLiteral("comparison-a"),
            true
            )
        );

    QVERIFY(
        model.setComparisonSelected(
            QStringLiteral("comparison-b"),
            true
            )
        );

    const QStringList expectedSessionIds {
        QStringLiteral("gateway"),
        QStringLiteral("scheduler"),
        QStringLiteral("controller")
    };

    QCOMPARE(
        model.selectedSessionIds(),
        expectedSessionIds
        );

    const QStringList expectedComparisonIds {
        QStringLiteral("comparison-b"),
        QStringLiteral("comparison-a")
    };

    QCOMPARE(
        model.selectedComparisonIds(),
        expectedComparisonIds
        );
}

void InvestigationReportSelectionModelTests::
    duplicateDescriptorsAreNormalized()
{
    QVector<InvestigationReportSessionSelection>
        sessionSelections {
            {
                QStringLiteral("gateway"),
                QStringLiteral("Gateway First"),
                true
            },
            {
                QStringLiteral(" gateway "),
                QStringLiteral("Gateway Duplicate"),
                true
            },
            {
                QString(),
                QStringLiteral("Invalid"),
                true
            },
            {
                QStringLiteral("controller"),
                QStringLiteral("Controller"),
                true
            }
        };

    QVector<InvestigationReportComparisonSelection>
        comparisonSelections {
            comparison(
                QStringLiteral("comparison")
                ),
            comparison(
                QStringLiteral(" comparison ")
                ),
            comparison(
                QString()
                )
        };

    InvestigationReportSelectionModel model(
        std::move(sessionSelections),
        std::move(comparisonSelections),
        {}
        );

    QCOMPARE(
        model.sessions().size(),
        2
        );

    QCOMPARE(
        model.sessions().at(0).sessionId,
        QStringLiteral("gateway")
        );

    QCOMPARE(
        model.sessions().at(0).documentTitle,
        QStringLiteral("Gateway First")
        );

    QCOMPARE(
        model.sessions().at(1).sessionId,
        QStringLiteral("controller")
        );

    QCOMPARE(
        model.comparisons().size(),
        1
        );

    QCOMPARE(
        model.comparisons().front().comparisonId,
        QStringLiteral("comparison")
        );

    /*
     * Caller-provided selected flags are deliberately
     * discarded. Initial selection comes only from the
     * explicit origin.
     */
    QVERIFY(
        !model.hasSelection()
        );
}

void InvestigationReportSelectionModelTests::
    unknownSelectionsDoNotMutateModel()
{
    InvestigationReportSelectionModel model(
        sessions(),
        {
            comparison()
        },
        {}
        );

    QVERIFY(
        !model.setSessionSelected(
            QStringLiteral("missing-session"),
            true
            )
        );

    QVERIFY(
        !model.setComparisonSelected(
            QStringLiteral("missing-comparison"),
            true
            )
        );

    QVERIFY(
        !model.hasSelection()
        );

    QVERIFY(
        model.selectedSessionIds().isEmpty()
        );

    QVERIFY(
        model.selectedComparisonIds().isEmpty()
        );
}

QTEST_APPLESS_MAIN(
    InvestigationReportSelectionModelTests
    )

#include "InvestigationReportSelectionModelTests.moc"