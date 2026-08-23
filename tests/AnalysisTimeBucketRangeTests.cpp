#include <QtTest/QtTest>

#include "../src/analysis/AnalysisTimeBucketRange.h"

class AnalysisTimeBucketRangeTests
    : public QObject
{
    Q_OBJECT

private slots:
    void createRejectsInvalidInputs();
    void bucketCountUsesAlignedBoundaries();
    void bucketIndexUsesAlignedIntervals();
    void bucketIndexRejectsOutOfRangeTimestamps();
    void bucketLabelUsesMillisecondResolution();
    void bucketLabelIncludesDateAcrossMidnight();
};

void AnalysisTimeBucketRangeTests::
    createRejectsInvalidInputs()
{
    const QDateTime valid =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    QVERIFY(
        !AnalysisTimeBucketRange::create(
             {},
             valid,
             1000
             )
             .has_value()
        );

    QVERIFY(
        !AnalysisTimeBucketRange::create(
             valid,
             {},
             1000
             )
             .has_value()
        );

    QVERIFY(
        !AnalysisTimeBucketRange::create(
             valid,
             valid,
             0
             )
             .has_value()
        );

    QVERIFY(
        !AnalysisTimeBucketRange::create(
             valid.addSecs(1),
             valid,
             1000
             )
             .has_value()
        );
}

void AnalysisTimeBucketRangeTests::
    bucketCountUsesAlignedBoundaries()
{
    const auto range =
        AnalysisTimeBucketRange::create(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:01:15.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:09:59.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            5 * 60 * 1000
            );

    QVERIFY(range.has_value());

    QCOMPARE(
        range->bucketCount(),
        2
        );

    QCOMPARE(
        range->bucketTimestamp(0),
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:00:00.000Z"
                ),
            Qt::ISODateWithMs
            )
        );

    QCOMPARE(
        range->bucketTimestamp(1),
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-22T10:05:00.000Z"
                ),
            Qt::ISODateWithMs
            )
        );
}

void AnalysisTimeBucketRangeTests::
    bucketIndexUsesAlignedIntervals()
{
    const auto range =
        AnalysisTimeBucketRange::create(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.299Z"
                    ),
                Qt::ISODateWithMs
                ),
            100
            );

    QVERIFY(range.has_value());

    const auto firstIndex =
        range->bucketIndexForTimestamp(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.099Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    const auto secondIndex =
        range->bucketIndexForTimestamp(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.100Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    const auto thirdIndex =
        range->bucketIndexForTimestamp(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.299Z"
                    ),
                Qt::ISODateWithMs
                )
            );

    QVERIFY(firstIndex.has_value());
    QVERIFY(secondIndex.has_value());
    QVERIFY(thirdIndex.has_value());

    QCOMPARE(*firstIndex, 0);
    QCOMPARE(*secondIndex, 1);
    QCOMPARE(*thirdIndex, 2);
}

void AnalysisTimeBucketRangeTests::
    bucketIndexRejectsOutOfRangeTimestamps()
{
    const auto range =
        AnalysisTimeBucketRange::create(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:02.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            1000
            );

    QVERIFY(range.has_value());

    QVERIFY(
        !range->bucketIndexForTimestamp(
                  QDateTime::fromString(
                      QStringLiteral(
                          "2026-08-22T09:59:59.000Z"
                          ),
                      Qt::ISODateWithMs
                      )
                  )
             .has_value()
        );

    QVERIFY(
        !range->bucketIndexForTimestamp(
                  QDateTime::fromString(
                      QStringLiteral(
                          "2026-08-22T10:00:03.000Z"
                          ),
                      Qt::ISODateWithMs
                      )
                  )
             .has_value()
        );
}

void AnalysisTimeBucketRangeTests::
    bucketLabelUsesMillisecondResolution()
{
    const auto range =
        AnalysisTimeBucketRange::create(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T10:00:00.199Z"
                    ),
                Qt::ISODateWithMs
                ),
            100
            );

    QVERIFY(range.has_value());

    QCOMPARE(
        range->bucketLabel(0),
        QStringLiteral(
            "10:00:00.000"
            )
        );

    QCOMPARE(
        range->bucketLabel(1),
        QStringLiteral(
            "10:00:00.100"
            )
        );
}

void AnalysisTimeBucketRangeTests::
    bucketLabelIncludesDateAcrossMidnight()
{
    const auto range =
        AnalysisTimeBucketRange::create(
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-22T23:59:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            QDateTime::fromString(
                QStringLiteral(
                    "2026-08-23T00:01:00.000Z"
                    ),
                Qt::ISODateWithMs
                ),
            60 * 1000
            );

    QVERIFY(range.has_value());

    QCOMPARE(
        range->bucketLabel(0),
        QStringLiteral(
            "2026-08-22 23:59"
            )
        );

    QCOMPARE(
        range->bucketLabel(1),
        QStringLiteral(
            "2026-08-23 00:00"
            )
        );

    QCOMPARE(
        range->bucketLabel(2),
        QStringLiteral(
            "2026-08-23 00:01"
            )
        );
}

QTEST_MAIN(AnalysisTimeBucketRangeTests)

#include "AnalysisTimeBucketRangeTests.moc"