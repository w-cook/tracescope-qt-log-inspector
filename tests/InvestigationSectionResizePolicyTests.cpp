#include <QtTest>

#include "../src/ui/workspace/InvestigationSectionResizePolicy.h"

class InvestigationSectionResizePolicyTests
    : public QObject
{
    Q_OBJECT

private:
    using Policy =
        InvestigationSectionResizePolicy;

    using Section =
        Policy::Section;

    const Policy::ResizeLimits limits{
        220, // Timeline preferred
        160, // Timeline minimum
        150, // Events minimum
        240, // Lower preferred
        200  // Lower minimum
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

private slots:
    void allSectionsOpen();
    void timelineAndEventsOpen();
    void eventsAndLowerOpen();
    void timelineAndLowerOpen();

    void singleOpenSectionOwnsSurplus();

    void automaticOpenOrderRespectsManualCollapse();

    void allOpenGrowthUsesLowerThenTimelineThenEvents();
    void allOpenShrinkUsesEventsThenTimelineThenLower();

    void timelineAndEventsResizeCorrectly();
    void eventsAndLowerResizeCorrectly();
    void timelineAndLowerResizeCorrectly();

    void shrinkReturnsUnconsumedDeficitAtMinimums();

    void automaticCollapseTransfersOnlyReleasedHeight();
    void automaticCollapseWithoutEventsUsesLower();
    void automaticLastCollapseLeavesSpaceWithEventContainer();

    void automaticLowerReopenTakesFromEvents();
    void automaticTimelineReopenUsesEventsThenLower();
    void automaticTimelineReopenWithoutEventsTakesFromLower();

    void automaticFirstEventOpenKeepsAvailableSurplus();
    void automaticFirstLowerOpenClaimsUnownedEventSpace();

    void automaticOpenFailsRatherThanViolatingMinimums();

    void fullAutomaticShrinkCollapsesInCorrectOrder();
    void fullAutomaticGrowthRestoresAllSections();
    void automaticGrowthWithTimelineManuallyCollapsed();
    void automaticGrowthWithLowerManuallyCollapsed();
    void automaticGrowthWithEventsManuallyCollapsed();
    void timelineLowerAutomaticResizeWithoutEvents();
    void allManuallyCollapsedKeepsGrowthInEventContainer();

    void onePixelPastTimelineMinimumCollapsesTimeline();
    void reversingOnePixelReopensTimelineAtExactBoundary();

    void lowerCollapseBoundaryIsReversible();
    void finalEventCollapseBoundaryIsReversible();

    void fullShrinkGrowRoundTripRestoresOriginalGeometry();
};

void InvestigationSectionResizePolicyTests::
    allSectionsOpen()
{
    const Policy::OpenSections open{
        true,
        true,
        true
    };

    QCOMPARE(
        Policy::growthRecoveryOrder(open),
        QList<Section>({
            Section::LowerDetails,
            Section::Timeline
        })
        );

    QCOMPARE(
        Policy::growthSurplusOwner(open),
        std::optional<Section>(
            Section::Events
            )
        );

    QCOMPARE(
        Policy::shrinkOrder(open),
        QList<Section>({
            Section::Events,
            Section::Timeline,
            Section::LowerDetails
        })
        );

    QCOMPARE(
        Policy::automaticCollapseOrder(open),
        QList<Section>({
            Section::Timeline,
            Section::LowerDetails,
            Section::Events
        })
        );
}

void InvestigationSectionResizePolicyTests::
    timelineAndEventsOpen()
{
    const Policy::OpenSections open{
        true,
        true,
        false
    };

    QCOMPARE(
        Policy::growthRecoveryOrder(open),
        QList<Section>({
            Section::Timeline
        })
        );

    QCOMPARE(
        Policy::growthSurplusOwner(open),
        std::optional<Section>(
            Section::Events
            )
        );

    QCOMPARE(
        Policy::shrinkOrder(open),
        QList<Section>({
            Section::Events,
            Section::Timeline
        })
        );

    QCOMPARE(
        Policy::automaticCollapseOrder(open),
        QList<Section>({
            Section::Timeline,
            Section::Events
        })
        );
}

void InvestigationSectionResizePolicyTests::
    eventsAndLowerOpen()
{
    const Policy::OpenSections open{
        false,
        true,
        true
    };

    QCOMPARE(
        Policy::growthRecoveryOrder(open),
        QList<Section>({
            Section::LowerDetails
        })
        );

    QCOMPARE(
        Policy::growthSurplusOwner(open),
        std::optional<Section>(
            Section::Events
            )
        );

    QCOMPARE(
        Policy::shrinkOrder(open),
        QList<Section>({
            Section::Events,
            Section::LowerDetails
        })
        );

    QCOMPARE(
        Policy::automaticCollapseOrder(open),
        QList<Section>({
            Section::LowerDetails,
            Section::Events
        })
        );
}

void InvestigationSectionResizePolicyTests::
    timelineAndLowerOpen()
{
    const Policy::OpenSections open{
        true,
        false,
        true
    };

    QCOMPARE(
        Policy::growthRecoveryOrder(open),
        QList<Section>({
            Section::LowerDetails,
            Section::Timeline
        })
        );

    QCOMPARE(
        Policy::growthSurplusOwner(open),
        std::optional<Section>(
            Section::LowerDetails
            )
        );

    QCOMPARE(
        Policy::shrinkOrder(open),
        QList<Section>({
            Section::Timeline,
            Section::LowerDetails
        })
        );

    QCOMPARE(
        Policy::automaticCollapseOrder(open),
        QList<Section>({
            Section::Timeline,
            Section::LowerDetails
        })
        );
}

void InvestigationSectionResizePolicyTests::
    singleOpenSectionOwnsSurplus()
{
    const QList<
        QPair<
            Policy::OpenSections,
            Section
            >
        > cases{
            {
                {
                    true,
                    false,
                    false
                },
                Section::Timeline
            },
            {
                {
                    false,
                    true,
                    false
                },
                Section::Events
            },
            {
                {
                    false,
                    false,
                    true
                },
                Section::LowerDetails
            }
        };

    for (const auto &testCase : cases) {
        QCOMPARE(
            Policy::growthSurplusOwner(
                testCase.first
                ),
            std::optional<Section>(
                testCase.second
                )
            );
    }
}

void InvestigationSectionResizePolicyTests::
    automaticOpenOrderRespectsManualCollapse()
{
    QCOMPARE(
        Policy::automaticOpenOrder(
            false,
            false,
            false
            ),
        QList<Section>({
            Section::Events,
            Section::LowerDetails,
            Section::Timeline
        })
        );

    /*
     * Timeline manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            true,
            false,
            false
            ),
        QList<Section>({
            Section::Events,
            Section::LowerDetails
        })
        );

    /*
     * Lower manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            false,
            false,
            true
            ),
        QList<Section>({
            Section::Events,
            Section::Timeline
        })
        );

    /*
     * Events manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            false,
            true,
            false
            ),
        QList<Section>({
            Section::LowerDetails,
            Section::Timeline
        })
        );

    /*
     * Timeline + Events manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            true,
            true,
            false
            ),
        QList<Section>({
            Section::LowerDetails
        })
        );

    /*
     * Lower + Events manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            false,
            true,
            true
            ),
        QList<Section>({
            Section::Timeline
        })
        );

    /*
     * Timeline + Lower manually collapsed.
     */
    QCOMPARE(
        Policy::automaticOpenOrder(
            true,
            false,
            true
            ),
        QList<Section>({
            Section::Events
        })
        );
}

void InvestigationSectionResizePolicyTests::
    allOpenGrowthUsesLowerThenTimelineThenEvents()
{
    const Policy::OpenSections open{
        true,
        true,
        true
    };

    const Policy::SectionHeights current{
        180,
        170,
        210
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::ResizeResult result =
        Policy::resizeOpenSections(
            open,
            current,
            limits,
            100
            );

    /*
     * +30 Lower -> preferred 240
     * +40 Timeline -> preferred 220
     * +30 Events
     */
    QCOMPARE(
        result.heights.timeline,
        220
        );

    QCOMPARE(
        result.heights.events,
        200
        );

    QCOMPARE(
        result.heights.lowerDetails,
        240
        );

    QCOMPARE(
        result.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    allOpenShrinkUsesEventsThenTimelineThenLower()
{
    const Policy::OpenSections open{
        true,
        true,
        true
    };

    const Policy::SectionHeights current{
        200,
        190,
        220
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::ResizeResult result =
        Policy::resizeOpenSections(
            open,
            current,
            limits,
            -100
            );

    /*
     * -40 Events -> minimum 150
     * -40 Timeline -> minimum 160
     * -20 Lower -> minimum 200
     */
    QCOMPARE(
        result.heights.timeline,
        160
        );

    QCOMPARE(
        result.heights.events,
        150
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );

    QCOMPARE(
        result.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    timelineAndEventsResizeCorrectly()
{
    const Policy::OpenSections open{
        true,
        true,
        false
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    180,
                    170,
                    0
                },
                limits,
                100
                );

        /*
         * Timeline receives 40 to preferred.
         * Events receives remaining 60.
         */
        QCOMPARE(
            result.heights.timeline,
            220
            );

        QCOMPARE(
            result.heights.events,
            230
            );
    }

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    200,
                    190,
                    0
                },
                limits,
                -80
                );

        /*
         * Events loses 40 to minimum.
         * Timeline loses remaining 40 to minimum.
         */
        QCOMPARE(
            result.heights.timeline,
            160
            );

        QCOMPARE(
            result.heights.events,
            150
            );

        QCOMPARE(
            result.unconsumedDelta,
            0
            );
    }
}

void InvestigationSectionResizePolicyTests::
    eventsAndLowerResizeCorrectly()
{
    const Policy::OpenSections open{
        false,
        true,
        true
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    0,
                    170,
                    210
                },
                limits,
                100
                );

        /*
         * Lower receives 30 to preferred.
         * Events receives remaining 70.
         */
        QCOMPARE(
            result.heights.events,
            240
            );

        QCOMPARE(
            result.heights.lowerDetails,
            240
            );
    }

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    0,
                    190,
                    230
                },
                limits,
                -70
                );

        /*
         * Events loses 40.
         * Lower loses remaining 30.
         */
        QCOMPARE(
            result.heights.events,
            150
            );

        QCOMPARE(
            result.heights.lowerDetails,
            200
            );
    }
}

void InvestigationSectionResizePolicyTests::
    timelineAndLowerResizeCorrectly()
{
    const Policy::OpenSections open{
        true,
        false,
        true
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    180,
                    0,
                    210
                },
                limits,
                100
                );

        /*
         * Lower receives 30 to preferred.
         * Timeline receives 40 to preferred.
         * With Events unavailable, Lower owns the
         * remaining 30 surplus.
         */
        QCOMPARE(
            result.heights.timeline,
            220
            );

        QCOMPARE(
            result.heights.lowerDetails,
            270
            );
    }

    {
        const Policy::ResizeResult result =
            Policy::resizeOpenSections(
                open,
                {
                    200,
                    0,
                    230
                },
                limits,
                -70
                );

        /*
         * Timeline loses 40 first.
         * Lower loses remaining 30.
         */
        QCOMPARE(
            result.heights.timeline,
            160
            );

        QCOMPARE(
            result.heights.lowerDetails,
            200
            );

        QCOMPARE(
            result.unconsumedDelta,
            0
            );
    }
}

void InvestigationSectionResizePolicyTests::
    shrinkReturnsUnconsumedDeficitAtMinimums()
{
    const Policy::OpenSections open{
        true,
        true,
        true
    };

    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::ResizeResult result =
        Policy::resizeOpenSections(
            open,
            {
                160,
                150,
                200
            },
            limits,
            -25
            );

    QCOMPARE(
        result.heights.timeline,
        160
        );

    QCOMPARE(
        result.heights.events,
        150
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );

    QCOMPARE(
        result.unconsumedDelta,
        -25
        );
}

void InvestigationSectionResizePolicyTests::
    automaticCollapseTransfersOnlyReleasedHeight()
{
    const Policy::TransitionResult result =
        Policy::automaticCollapseSection(
            {
                true,
                true,
                true
            },
            {
                160,
                150,
                200
            },
            {
                28,
                30,
                44
            },
            Section::Timeline
            );

    QVERIFY(result.completed);

    QCOMPARE(
        result.heights.timeline,
        28
        );

    QCOMPARE(
        result.heights.events,
        282
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );
}

void InvestigationSectionResizePolicyTests::
    automaticCollapseWithoutEventsUsesLower()
{
    const Policy::TransitionResult result =
        Policy::automaticCollapseSection(
            {
                true,
                false,
                true
            },
            {
                160,
                30,
                200
            },
            {
                28,
                30,
                44
            },
            Section::Timeline
            );

    QVERIFY(result.completed);

    QCOMPARE(
        result.heights.timeline,
        28
        );

    QCOMPARE(
        result.heights.lowerDetails,
        332
        );
}

void InvestigationSectionResizePolicyTests::
    automaticLastCollapseLeavesSpaceWithEventContainer()
{
    const Policy::TransitionResult result =
        Policy::automaticCollapseSection(
            {
                false,
                true,
                false
            },
            {
                28,
                150,
                44
            },
            {
                28,
                30,
                44
            },
            Section::Events
            );

    QVERIFY(result.completed);

    QVERIFY(!result.openSections.timeline);
    QVERIFY(!result.openSections.events);
    QVERIFY(!result.openSections.lowerDetails);

    /*
     * Event becomes visually collapsed but its container
     * keeps the available vertical space.
     */
    QCOMPARE(
        result.heights.events,
        150
        );
}

void InvestigationSectionResizePolicyTests::
    automaticLowerReopenTakesFromEvents()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                true,
                false
            },
            {
                28,
                306,
                44
            },
            limits,
            compact,
            Section::LowerDetails
            );

    QVERIFY(result.completed);

    QCOMPARE(
        result.heights.events,
        150
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );
}

void InvestigationSectionResizePolicyTests::
    automaticTimelineReopenUsesEventsThenLower()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                true,
                true
            },
            {
                28,
                200,
                282
            },
            limits,
            compact,
            Section::Timeline
            );

    QVERIFY(result.completed);

    /*
     * Timeline needs 132.
     *
     * Events contributes 50 down to minimum.
     * Lower contributes remaining 82 down to minimum.
     */
    QCOMPARE(
        result.heights.timeline,
        160
        );

    QCOMPARE(
        result.heights.events,
        150
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );
}

void InvestigationSectionResizePolicyTests::
    automaticTimelineReopenWithoutEventsTakesFromLower()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                false,
                true
            },
            {
                28,
                30,
                360
            },
            limits,
            compact,
            Section::Timeline
            );

    QVERIFY(result.completed);

    QCOMPARE(
        result.heights.timeline,
        160
        );

    QCOMPARE(
        result.heights.lowerDetails,
        228
        );
}

void InvestigationSectionResizePolicyTests::
    automaticFirstEventOpenKeepsAvailableSurplus()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                false,
                false
            },
            {
                28,
                250,
                44
            },
            limits,
            compact,
            Section::Events
            );

    QVERIFY(result.completed);

    /*
     * Its container was already holding the available
     * all-collapsed surplus, so opening Events does not
     * require any redistribution.
     */
    QCOMPARE(
        result.heights.events,
        250
        );
}

void InvestigationSectionResizePolicyTests::
    automaticFirstLowerOpenClaimsUnownedEventSpace()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                false,
                false
            },
            {
                28,
                300,
                44
            },
            limits,
            compact,
            Section::LowerDetails
            );

    QVERIFY(result.completed);

    QCOMPARE(
        result.heights.events,
        30
        );

    /*
     * 156 reaches Lower's minimum.
     * The remaining 114 is surplus and Lower is now the
     * sole open section, so it keeps that too.
     */
    QCOMPARE(
        result.heights.lowerDetails,
        314
        );
}

void InvestigationSectionResizePolicyTests::
    automaticOpenFailsRatherThanViolatingMinimums()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::SectionHeights original{
        28,
        170,
        210
    };

    const Policy::TransitionResult result =
        Policy::automaticOpenSection(
            {
                false,
                true,
                true
            },
            original,
            limits,
            compact,
            Section::Timeline
            );

    QVERIFY(!result.completed);

    QCOMPARE(
        result.heights.timeline,
        original.timeline
        );

    QCOMPARE(
        result.heights.events,
        original.events
        );

    QCOMPARE(
        result.heights.lowerDetails,
        original.lowerDetails
        );
}

void InvestigationSectionResizePolicyTests::
    fullAutomaticShrinkCollapsesInCorrectOrder()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                true,
                true,
                true
            },
            {
                220,
                250,
                240
            },
            limits,
            compact,
            false,
            false,
            false,
            -489
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::Timeline,
            Section::LowerDetails,
            Section::Events
        })
        );

    QVERIFY(!result.openSections.timeline);
    QVERIFY(!result.openSections.events);
    QVERIFY(!result.openSections.lowerDetails);

    QCOMPARE(
        result.heights.timeline,
        28
        );

    QCOMPARE(
        result.heights.lowerDetails,
        44
        );

    /*
     * After Events collapses, its container keeps the
     * released area and then absorbs the final remaining
     * window loss.
     */
    QCOMPARE(
        result.heights.events,
        149
        );

    QCOMPARE(
        result.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    fullAutomaticGrowthRestoresAllSections()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                false,
                false,
                false
            },
            {
                28,
                100,
                44
            },
            limits,
            compact,
            false,
            false,
            false,
            600
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::Events,
            Section::LowerDetails,
            Section::Timeline
        })
        );

    QVERIFY(result.openSections.timeline);
    QVERIFY(result.openSections.events);
    QVERIFY(result.openSections.lowerDetails);

    /*
     * After all three sections have reopened:
     *
     * Lower recovers to preferred.
     * Timeline recovers to preferred.
     * Events owns the remaining surplus.
     */
    QCOMPARE(
        result.heights.timeline,
        220
        );

    QCOMPARE(
        result.heights.lowerDetails,
        240
        );

    QCOMPARE(
        result.heights.events,
        312
        );

    QCOMPARE(
        result.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    automaticGrowthWithTimelineManuallyCollapsed()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                false,
                false,
                false
            },
            {
                28,
                100,
                44
            },
            limits,
            compact,
            true,
            false,
            false,
            500
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::Events,
            Section::LowerDetails
        })
        );

    QVERIFY(!result.openSections.timeline);
    QVERIFY(result.openSections.events);
    QVERIFY(result.openSections.lowerDetails);

    QCOMPARE(
        result.heights.lowerDetails,
        240
        );

    QVERIFY(
        result.heights.events
        > limits.eventsMinimum
        );
}

void InvestigationSectionResizePolicyTests::
    automaticGrowthWithLowerManuallyCollapsed()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                false,
                false,
                false
            },
            {
                28,
                100,
                44
            },
            limits,
            compact,
            false,
            false,
            true,
            500
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::Events,
            Section::Timeline
        })
        );

    QVERIFY(result.openSections.timeline);
    QVERIFY(result.openSections.events);
    QVERIFY(!result.openSections.lowerDetails);

    QCOMPARE(
        result.heights.timeline,
        220
        );

    QVERIFY(
        result.heights.events
        > limits.eventsMinimum
        );
}

void InvestigationSectionResizePolicyTests::
    automaticGrowthWithEventsManuallyCollapsed()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                false,
                false,
                false
            },
            {
                28,
                300,
                44
            },
            limits,
            compact,
            false,
            true,
            false,
            400
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::LowerDetails,
            Section::Timeline
        })
        );

    QVERIFY(result.openSections.timeline);
    QVERIFY(!result.openSections.events);
    QVERIFY(result.openSections.lowerDetails);

    QCOMPARE(
        result.heights.timeline,
        220
        );

    /*
     * With Events unavailable, Lower owns surplus after
     * both preferred targets are recovered.
     */
    QVERIFY(
        result.heights.lowerDetails
        >= limits.lowerPreferred
        );
}

void InvestigationSectionResizePolicyTests::
    timelineLowerAutomaticResizeWithoutEvents()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                true,
                false,
                true
            },
            {
                200,
                30,
                230
            },
            limits,
            compact,
            false,
            true,
            false,
            -70
            );

    /*
     * Timeline loses 40 first.
     * Lower loses 30 second.
     */
    QCOMPARE(
        result.heights.timeline,
        160
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );

    QVERIFY(result.transitions.isEmpty());
}

void InvestigationSectionResizePolicyTests::
    allManuallyCollapsedKeepsGrowthInEventContainer()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                false,
                false,
                false
            },
            {
                28,
                100,
                44
            },
            limits,
            compact,
            true,
            true,
            true,
            200
            );

    QVERIFY(result.transitions.isEmpty());

    QVERIFY(!result.openSections.timeline);
    QVERIFY(!result.openSections.events);
    QVERIFY(!result.openSections.lowerDetails);

    QCOMPARE(
        result.heights.timeline,
        28
        );

    QCOMPARE(
        result.heights.events,
        300
        );

    QCOMPARE(
        result.heights.lowerDetails,
        44
        );
}

void InvestigationSectionResizePolicyTests::
    onePixelPastTimelineMinimumCollapsesTimeline()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    /*
     * Every open section is already at its useful minimum.
     *
     * One additional lost pixel therefore crosses the
     * first automatic-collapse boundary.
     */
    const Policy::AutomaticResizeResult result =
        Policy::resizeAutomatically(
            {
                true,
                true,
                true
            },
            {
                160,
                150,
                200
            },
            limits,
            compact,
            false,
            false,
            false,
            -1
            );

    QCOMPARE(
        result.transitions,
        QList<Section>({
            Section::Timeline
        })
        );

    QVERIFY(!result.openSections.timeline);
    QVERIFY(result.openSections.events);
    QVERIFY(result.openSections.lowerDetails);

    /*
     * Timeline releases:
     *
     *   160 - 28 = 132
     *
     * Events receives that space, then absorbs the one
     * pixel of actual outer-window loss:
     *
     *   150 + 132 - 1 = 281
     */
    QCOMPARE(
        result.heights.timeline,
        28
        );

    QCOMPARE(
        result.heights.events,
        281
        );

    QCOMPARE(
        result.heights.lowerDetails,
        200
        );

    QCOMPARE(
        result.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    reversingOnePixelReopensTimelineAtExactBoundary()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult shrunk =
        Policy::resizeAutomatically(
            {
                true,
                true,
                true
            },
            {
                160,
                150,
                200
            },
            limits,
            compact,
            false,
            false,
            false,
            -1
            );

    QVERIFY(!shrunk.openSections.timeline);

    const Policy::AutomaticResizeResult restored =
        Policy::resizeAutomatically(
            shrunk.openSections,
            shrunk.heights,
            limits,
            compact,
            false,
            false,
            false,
            1
            );

    QCOMPARE(
        restored.transitions,
        QList<Section>({
            Section::Timeline
        })
        );

    QVERIFY(restored.openSections.timeline);
    QVERIFY(restored.openSections.events);
    QVERIFY(restored.openSections.lowerDetails);

    /*
     * The exact inverse transition returns us to the exact
     * useful-minimum geometry we started with.
     */
    QCOMPARE(
        restored.heights.timeline,
        160
        );

    QCOMPARE(
        restored.heights.events,
        150
        );

    QCOMPARE(
        restored.heights.lowerDetails,
        200
        );

    QCOMPARE(
        restored.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    lowerCollapseBoundaryIsReversible()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    /*
     * Timeline is already automatically collapsed.
     * Events and Lower are both at minimum.
     */
    const Policy::AutomaticResizeResult shrunk =
        Policy::resizeAutomatically(
            {
                false,
                true,
                true
            },
            {
                28,
                150,
                200
            },
            limits,
            compact,
            false,
            false,
            false,
            -1
            );

    QCOMPARE(
        shrunk.transitions,
        QList<Section>({
            Section::LowerDetails
        })
        );

    QVERIFY(!shrunk.openSections.timeline);
    QVERIFY(shrunk.openSections.events);
    QVERIFY(!shrunk.openSections.lowerDetails);

    QCOMPARE(
        shrunk.heights.timeline,
        28
        );

    QCOMPARE(
        shrunk.heights.events,
        305
        );

    QCOMPARE(
        shrunk.heights.lowerDetails,
        44
        );

    /*
     * Reverse by exactly the pixel that crossed the
     * threshold.
     */
    const Policy::AutomaticResizeResult restored =
        Policy::resizeAutomatically(
            shrunk.openSections,
            shrunk.heights,
            limits,
            compact,
            false,
            false,
            false,
            1
            );

    QCOMPARE(
        restored.transitions,
        QList<Section>({
            Section::LowerDetails
        })
        );

    QVERIFY(restored.openSections.events);
    QVERIFY(restored.openSections.lowerDetails);

    QCOMPARE(
        restored.heights.events,
        150
        );

    QCOMPARE(
        restored.heights.lowerDetails,
        200
        );
}

void InvestigationSectionResizePolicyTests::
    finalEventCollapseBoundaryIsReversible()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::AutomaticResizeResult shrunk =
        Policy::resizeAutomatically(
            {
                false,
                true,
                false
            },
            {
                28,
                150,
                44
            },
            limits,
            compact,
            false,
            false,
            false,
            -1
            );

    QCOMPARE(
        shrunk.transitions,
        QList<Section>({
            Section::Events
        })
        );

    QVERIFY(!shrunk.openSections.timeline);
    QVERIFY(!shrunk.openSections.events);
    QVERIFY(!shrunk.openSections.lowerDetails);

    /*
     * Events is visually collapsed, but its middle
     * container owns the spare all-collapsed space.
     */
    QCOMPARE(
        shrunk.heights.timeline,
        28
        );

    QCOMPARE(
        shrunk.heights.events,
        149
        );

    QCOMPARE(
        shrunk.heights.lowerDetails,
        44
        );

    const Policy::AutomaticResizeResult restored =
        Policy::resizeAutomatically(
            shrunk.openSections,
            shrunk.heights,
            limits,
            compact,
            false,
            false,
            false,
            1
            );

    QCOMPARE(
        restored.transitions,
        QList<Section>({
            Section::Events
        })
        );

    QVERIFY(!restored.openSections.timeline);
    QVERIFY(restored.openSections.events);
    QVERIFY(!restored.openSections.lowerDetails);

    QCOMPARE(
        restored.heights.events,
        150
        );

    QCOMPARE(
        restored.unconsumedDelta,
        0
        );
}

void InvestigationSectionResizePolicyTests::
    fullShrinkGrowRoundTripRestoresOriginalGeometry()
{
    const Policy::ResizeLimits limits{
        220,
        160,
        150,
        240,
        200
    };

    const Policy::CompactHeights compact{
        28,
        30,
        44
    };

    const Policy::OpenSections originalOpen{
        true,
        true,
        true
    };

    const Policy::SectionHeights originalHeights{
        220,
        300,
        240
    };

    /*
     * This is enough shrink to:
     *
     * Events -> minimum
     * Timeline -> minimum
     * Lower -> minimum
     * Timeline collapses
     * Events shrinks again
     * Lower collapses
     *
     * Events remains open with the remaining space.
     */
    const Policy::AutomaticResizeResult shrunk =
        Policy::resizeAutomatically(
            originalOpen,
            originalHeights,
            limits,
            compact,
            false,
            false,
            false,
            -400
            );

    QCOMPARE(
        shrunk.transitions,
        QList<Section>({
            Section::Timeline,
            Section::LowerDetails
        })
        );

    QVERIFY(!shrunk.openSections.timeline);
    QVERIFY(shrunk.openSections.events);
    QVERIFY(!shrunk.openSections.lowerDetails);

    /*
     * Now apply the exact inverse outer-window resize.
     */
    const Policy::AutomaticResizeResult restored =
        Policy::resizeAutomatically(
            shrunk.openSections,
            shrunk.heights,
            limits,
            compact,
            false,
            false,
            false,
            400
            );

    QCOMPARE(
        restored.transitions,
        QList<Section>({
            Section::LowerDetails,
            Section::Timeline
        })
        );

    QVERIFY(restored.openSections.timeline);
    QVERIFY(restored.openSections.events);
    QVERIFY(restored.openSections.lowerDetails);

    /*
     * No preference drift.
     * No accumulated rounding drift.
     * No stale recovery state.
     */
    QCOMPARE(
        restored.heights.timeline,
        originalHeights.timeline
        );

    QCOMPARE(
        restored.heights.events,
        originalHeights.events
        );

    QCOMPARE(
        restored.heights.lowerDetails,
        originalHeights.lowerDetails
        );

    QCOMPARE(
        restored.unconsumedDelta,
        0
        );
}

QTEST_APPLESS_MAIN(
    InvestigationSectionResizePolicyTests
    )

#include "InvestigationSectionResizePolicyTests.moc"