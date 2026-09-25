
#include <QtTest>

#include <algorithm>
#include <optional>
#include <utility>

#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QScrollBar>
#include <QTableWidget>

#include "../src/ui/InterfaceScale.h"
#include "../src/ui/workspace/InvestigationComparisonDocument.h"

class InvestigationComparisonDocumentTests
    : public QObject
{
    Q_OBJECT

private slots:
    void typographyAndTableHeightsTrackInterfaceScale();
};

void InvestigationComparisonDocumentTests::
    typographyAndTableHeightsTrackInterfaceScale()
{
    auto *application =
        qobject_cast<QApplication *>(
            QCoreApplication::instance()
            );

    QVERIFY(application != nullptr);

    if (application == nullptr) {
        return;
    }

    InterfaceScale::initializeApplication(application);
    InterfaceScale::applyUserFactor(application, 1.0);

    InvestigationComparisonSourceSnapshot baseline;
    baseline.sessionId = QStringLiteral("baseline");
    baseline.sourceMetadata.sourceName =
        QStringLiteral("baseline.jsonl");

    InvestigationComparisonSourceSnapshot comparison;
    comparison.sessionId = QStringLiteral("comparison");
    comparison.sourceMetadata.sourceName =
        QStringLiteral("comparison.jsonl");

    InvestigationSessionComparison analysis;
    analysis.totalRecords.baselineCount = 10;
    analysis.totalRecords.comparisonCount = 15;

    InvestigationComparisonDocument document(
        InvestigationComparisonSnapshot(
            QStringLiteral("comparison-scale-test"),
            std::move(baseline),
            std::move(comparison),
            std::nullopt,
            std::move(analysis)
            )
        );

    document.resize(1000, 800);
    document.show();
    QCoreApplication::processEvents();

    QLabel *heading = nullptr;
    QList<QLabel *> boldLabels;

    for (QLabel *label
         : document.findChildren<QLabel *>()) {

        const QString role =
            label->property(
                     "comparisonFontRole"
                     ).toString();

        if (role == QStringLiteral("heading")
            && label->text()
                   == QStringLiteral("Key Differences")) {
            heading = label;
        }

        if (role == QStringLiteral("bold")) {
            boldLabels.append(label);
        }
    }

    QVERIFY(heading != nullptr);
    QVERIFY(!boldLabels.isEmpty());

    if (heading == nullptr || boldLabels.isEmpty()) {
        return;
    }

    QTableWidget *table = nullptr;

    for (QTableWidget *candidate
         : document.findChildren<QTableWidget *>()) {
        if (candidate->rowCount() > 0) {
            table = candidate;
            break;
        }
    }

    QVERIFY(table != nullptr);

    if (table == nullptr) {
        return;
    }

    /*
     * Check against the current font and geometry,
     * not hardcoded pixel measurements that could
     * vary between operating systems.
     */
    const auto presentationIsCorrect = [&]() {
        QFont expectedHeading =
            QApplication::font(heading);

        expectedHeading.setBold(true);

        if (expectedHeading.pointSizeF() > 0.0) {
            expectedHeading.setPointSizeF(
                expectedHeading.pointSizeF() + 2.0
                );
        }

        if (heading->font() != expectedHeading) {
            return false;
        }

        for (QLabel *label : boldLabels) {
            QFont expected =
                QApplication::font(label);

            expected.setBold(true);

            if (label->font() != expected) {
                return false;
            }
        }

        const int expectedRowHeight =
            std::max(
                InterfaceScale::pixels(24, table),
                table->fontMetrics().height()
                    + InterfaceScale::pixels(4, table)
                );

        if (table->verticalHeader()
                ->defaultSectionSize()
            != expectedRowHeight) {
            return false;
        }

        const int visibleRows =
            std::min(table->rowCount(), 10);

        const int expectedTableHeight =
            table->horizontalHeader()
                ->sizeHint().height()
            + visibleRows * expectedRowHeight
            + table->frameWidth() * 2
            + table->horizontalScrollBar()
                  ->sizeHint().height();

        return table->minimumHeight()
                   == expectedTableHeight
               && table->maximumHeight()
                      == expectedTableHeight;
    };

    document.refreshInterfaceScale();
    QTRY_VERIFY(presentationIsCorrect());

    const int originalRowHeight =
        table->verticalHeader()->defaultSectionSize();

    const QFont originalHeadingFont =
        heading->font();

    /*
     * Increase scale and verify that both typography
     * and table geometry update.
     */
    InterfaceScale::applyUserFactor(
        application,
        2.0
        );

    document.refreshInterfaceScale();

    QTRY_VERIFY(presentationIsCorrect());

    QVERIFY(
        table->verticalHeader()->defaultSectionSize()
        > originalRowHeight
        );

    /*
     * Return to the original scale. Geometry and
     * fonts must return to their original values
     * without accumulating scaling changes.
     */
    InterfaceScale::applyUserFactor(
        application,
        1.0
        );

    document.refreshInterfaceScale();

    QTRY_VERIFY(presentationIsCorrect());

    QCOMPARE(
        table->verticalHeader()->defaultSectionSize(),
        originalRowHeight
        );

    QCOMPARE(
        heading->font(),
        originalHeadingFont
        );
}

QTEST_MAIN(InvestigationComparisonDocumentTests)

#include "InvestigationComparisonDocumentTests.moc"