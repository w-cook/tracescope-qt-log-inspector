#include "InvestigationSectionResizePolicy.h"

#include <algorithm>

namespace
{
using Policy =
    InvestigationSectionResizePolicy;

using Section =
    Policy::Section;

bool isOpen(
    Section section,
    const Policy::OpenSections &openSections
    )
{
    switch (section) {
    case Section::Timeline:
        return openSections.timeline;

    case Section::Events:
        return openSections.events;

    case Section::LowerDetails:
        return openSections.lowerDetails;
    }

    return false;
}

QList<Section> openSectionsInOrder(
    const Policy::OpenSections &openSections,
    const QList<Section> &priority
    )
{
    QList<Section> result;

    for (const Section section : priority) {
        if (isOpen(
                section,
                openSections
                )) {
            result.append(
                section
                );
        }
    }

    return result;
}

int &heightForSection(
    Policy::SectionHeights &heights,
    Section section
    )
{
    switch (section) {
    case Section::Timeline:
        return heights.timeline;

    case Section::Events:
        return heights.events;

    case Section::LowerDetails:
        return heights.lowerDetails;
    }

    return heights.events;
}

int minimumForSection(
    const Policy::ResizeLimits &limits,
    Section section
    )
{
    switch (section) {
    case Section::Timeline:
        return limits.timelineMinimum;

    case Section::Events:
        return limits.eventsMinimum;

    case Section::LowerDetails:
        return limits.lowerMinimum;
    }

    return 0;
}

int preferredForSection(
    const Policy::ResizeLimits &limits,
    Section section
    )
{
    switch (section) {
    case Section::Timeline:
        return limits.timelinePreferred;

    case Section::LowerDetails:
        return limits.lowerPreferred;

    case Section::Events:
        /*
         * Events has no preferred recovery height.
         */
        return 0;
    }

    return 0;
}

void setOpen(
    Policy::OpenSections &openSections,
    Section section,
    bool open
    )
{
    switch (section) {
    case Section::Timeline:
        openSections.timeline =
            open;
        break;

    case Section::Events:
        openSections.events =
            open;
        break;

    case Section::LowerDetails:
        openSections.lowerDetails =
            open;
        break;
    }
}

int compactForSection(
    const Policy::CompactHeights &compactHeights,
    Section section
    )
{
    switch (section) {
    case Section::Timeline:
        return compactHeights.timeline;

    case Section::Events:
        return compactHeights.events;

    case Section::LowerDetails:
        return compactHeights.lowerDetails;
    }

    return 0;
}

QList<Section> automaticOpenDonorOrder(
    Section candidate
    )
{
    switch (candidate) {
    case Section::Timeline:
        /*
         * Timeline reopening:
         * Events first, then Lower.
         */
        return {
            Section::Events,
            Section::LowerDetails
        };

    case Section::Events:
        /*
         * Events is highest retention priority.
         */
        return {
            Section::Timeline,
            Section::LowerDetails
        };

    case Section::LowerDetails:
        /*
         * Lower reopening:
         * Events first, then Timeline.
         */
        return {
            Section::Events,
            Section::Timeline
        };
    }

    return {};
}

bool anySectionOpen(
    const Policy::OpenSections &openSections
    )
{
    return
        openSections.timeline
        || openSections.events
        || openSections.lowerDetails;
}

std::optional<Section>
nextAutomaticOpenCandidate(
    const Policy::OpenSections &openSections,
    bool timelinePreferredCollapsed,
    bool eventsPreferredCollapsed,
    bool lowerPreferredCollapsed
    )
{
    const QList<Section> order =
        Policy::automaticOpenOrder(
            timelinePreferredCollapsed,
            eventsPreferredCollapsed,
            lowerPreferredCollapsed
            );

    for (const Section section : order) {
        if (!isOpen(
                section,
                openSections
                )) {
            return section;
        }
    }

    return std::nullopt;
}

Policy::SectionHeights
applyGrowthWithoutTransitions(
    const Policy::OpenSections &openSections,
    const Policy::SectionHeights &currentHeights,
    const Policy::ResizeLimits &limits,
    int growth
    )
{
    Policy::SectionHeights heights =
        currentHeights;

    if (growth <= 0) {
        return heights;
    }

    if (!anySectionOpen(openSections)) {
        /*
         * In the all-collapsed presentation, the Event
         * container owns spare vertical space.
         */
        heights.events +=
            growth;

        return heights;
    }

    return Policy::resizeOpenSections(
               openSections,
               currentHeights,
               limits,
               growth
               )
        .heights;
}
}

QList<InvestigationSectionResizePolicy::Section>
    InvestigationSectionResizePolicy::
    growthRecoveryOrder(
        const OpenSections &openSections
        )
{
    /*
     * Preferred-height recovery:
     *
     *   Lower Details
     *   Timeline
     *
     * Events has no preferred-height recovery target.
     */
    return openSectionsInOrder(
        openSections,
        {
            Section::LowerDetails,
            Section::Timeline
        }
        );
}

std::optional<
    InvestigationSectionResizePolicy::Section>
    InvestigationSectionResizePolicy::
    growthSurplusOwner(
        const OpenSections &openSections
        )
{
    /*
     * Events is normally elastic.
     *
     * When Events is unavailable, Lower Details owns
     * surplus. Timeline owns it only when it is the sole
     * remaining open section.
     */
    if (openSections.events) {
        return Section::Events;
    }

    if (openSections.lowerDetails) {
        return Section::LowerDetails;
    }

    if (openSections.timeline) {
        return Section::Timeline;
    }

    return std::nullopt;
}

QList<InvestigationSectionResizePolicy::Section>
    InvestigationSectionResizePolicy::
    shrinkOrder(
        const OpenSections &openSections
        )
{
    /*
     * Height is surrendered toward minimum useful sizes:
     *
     *   Events
     *   Timeline
     *   Lower Details
     */
    return openSectionsInOrder(
        openSections,
        {
            Section::Events,
            Section::Timeline,
            Section::LowerDetails
        }
        );
}

QList<InvestigationSectionResizePolicy::Section>
    InvestigationSectionResizePolicy::
    automaticCollapseOrder(
        const OpenSections &openSections
        )
{
    /*
     * Once open sections are all at their useful floors,
     * retention priority is:
     *
     *   Events
     *   Lower Details
     *   Timeline
     *
     * Therefore collapse happens in reverse:
     *
     *   Timeline
     *   Lower Details
     *   Events
     */
    return openSectionsInOrder(
        openSections,
        {
            Section::Timeline,
            Section::LowerDetails,
            Section::Events
        }
        );
}

QList<InvestigationSectionResizePolicy::Section>
    InvestigationSectionResizePolicy::
    automaticOpenOrder(
        bool timelinePreferredCollapsed,
        bool eventsPreferredCollapsed,
        bool lowerPreferredCollapsed
        )
{
    QList<Section> result;

    if (!eventsPreferredCollapsed) {
        result.append(
            Section::Events
            );
    }

    if (!lowerPreferredCollapsed) {
        result.append(
            Section::LowerDetails
            );
    }

    if (!timelinePreferredCollapsed) {
        result.append(
            Section::Timeline
            );
    }

    return result;
}

InvestigationSectionResizePolicy::ResizeResult
    InvestigationSectionResizePolicy::
    resizeOpenSections(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        int delta
        )
{
    ResizeResult result;

    result.heights =
        currentHeights;

    if (delta == 0) {
        return result;
    }

    /*
     * ---------------------------------------------------------
     * Growth
     * ---------------------------------------------------------
     */
    if (delta > 0) {
        int remainingGrowth =
            delta;

        /*
         * Restore preferred auxiliary heights first.
         */
        const QList<Section> recoveryOrder =
            growthRecoveryOrder(
                openSections
                );

        for (
            const Section section
            : recoveryOrder
            ) {
            if (remainingGrowth <= 0) {
                break;
            }

            int &currentHeight =
                heightForSection(
                    result.heights,
                    section
                    );

            const int preferredHeight =
                preferredForSection(
                    limits,
                    section
                    );

            const int needed =
                std::max(
                    0,
                    preferredHeight
                        - currentHeight
                    );

            const int amount =
                std::min(
                    remainingGrowth,
                    needed
                    );

            currentHeight +=
                amount;

            remainingGrowth -=
                amount;
        }

        /*
         * Once every applicable preference is recovered,
         * the state's elastic/surplus owner receives
         * everything else.
         */
        if (remainingGrowth > 0) {
            const std::optional<Section> owner =
                growthSurplusOwner(
                    openSections
                    );

            if (owner.has_value()) {
                heightForSection(
                    result.heights,
                    owner.value()
                    ) += remainingGrowth;

                remainingGrowth = 0;
            }
        }

        result.unconsumedDelta =
            remainingGrowth;

        return result;
    }

    /*
     * ---------------------------------------------------------
     * Shrink
     * ---------------------------------------------------------
     */
    int remainingLoss =
        -delta;

    const QList<Section> lossOrder =
        shrinkOrder(
            openSections
            );

    for (
        const Section section
        : lossOrder
        ) {
        if (remainingLoss <= 0) {
            break;
        }

        int &currentHeight =
            heightForSection(
                result.heights,
                section
                );

        const int minimumHeight =
            minimumForSection(
                limits,
                section
                );

        const int available =
            std::max(
                0,
                currentHeight
                    - minimumHeight
                );

        const int amount =
            std::min(
                remainingLoss,
                available
                );

        currentHeight -=
            amount;

        remainingLoss -=
            amount;
    }

    /*
     * The unresolved negative delta is deliberately
     * returned rather than forcing any open section below
     * its useful minimum.
     *
     * Automatic collapse is a separate discrete
     * transition and does not belong in this function.
     */
    result.unconsumedDelta =
        -remainingLoss;

    return result;
}

InvestigationSectionResizePolicy::TransitionResult
    InvestigationSectionResizePolicy::
    automaticCollapseSection(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const CompactHeights &compactHeights,
        Section candidate
        )
{
    TransitionResult result;

    result.openSections =
        openSections;

    result.heights =
        currentHeights;

    if (!isOpen(
            candidate,
            openSections
            )) {
        return result;
    }

    int &candidateHeight =
        heightForSection(
            result.heights,
            candidate
            );

    const int compactHeight =
        compactForSection(
            compactHeights,
            candidate
            );

    const int releasedHeight =
        std::max(
            0,
            candidateHeight
                - compactHeight
            );

    candidateHeight =
        compactHeight;

    setOpen(
        result.openSections,
        candidate,
        false
        );

    const std::optional<Section> recipient =
        growthSurplusOwner(
            result.openSections
            );

    if (recipient.has_value()) {
        heightForSection(
            result.heights,
            recipient.value()
            ) += releasedHeight;
    } else {
        /*
         * All sections are now collapsed.
         *
         * The middle Event container owns the otherwise
         * unassigned vertical space so its collapsed title
         * strip can remain centered.
         */
        result.heights.events +=
            releasedHeight;
    }

    result.completed =
        true;

    return result;
}

InvestigationSectionResizePolicy::TransitionResult
    InvestigationSectionResizePolicy::
    automaticOpenSection(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        const CompactHeights &compactHeights,
        Section candidate
        )
{
    TransitionResult result;

    result.openSections =
        openSections;

    result.heights =
        currentHeights;

    if (isOpen(
            candidate,
            openSections
            )) {
        return result;
    }

    /*
     * Work on a prospective open state. We only return it
     * as completed if the candidate can reach its minimum.
     */
    setOpen(
        result.openSections,
        candidate,
        true
        );

    int freeSpace = 0;

    /*
     * Once at least one section is open, collapsed sections
     * no longer own arbitrary vertical surplus.
     *
     * Any collapsed slot currently larger than its compact
     * height represents unowned presentation space.
     *
     * The common example is the centered Event container in
     * the all-collapsed presentation.
     */
    for (
        const Section section
        : {
            Section::Timeline,
            Section::Events,
            Section::LowerDetails
        }
        ) {
        if (
            section == candidate
            || isOpen(
                section,
                openSections
                )
            ) {
            continue;
        }

        int &currentHeight =
            heightForSection(
                result.heights,
                section
                );

        const int compactHeight =
            compactForSection(
                compactHeights,
                section
                );

        if (currentHeight > compactHeight) {
            freeSpace +=
                currentHeight
                - compactHeight;

            currentHeight =
                compactHeight;
        }
    }

    int &candidateHeight =
        heightForSection(
            result.heights,
            candidate
            );

    const int candidateMinimum =
        minimumForSection(
            limits,
            candidate
            );

    int remainingNeeded =
        std::max(
            0,
            candidateMinimum
                - candidateHeight
            );

    /*
     * Unowned presentation space is always consumed before
     * shrinking a real open section.
     */
    if (
        remainingNeeded > 0
        && freeSpace > 0
        ) {
        const int amount =
            std::min(
                remainingNeeded,
                freeSpace
                );

        candidateHeight +=
            amount;

        remainingNeeded -=
            amount;

        freeSpace -=
            amount;
    }

    /*
     * Then perform the discrete reopen transfer from the
     * currently open donor sections.
     *
     * Donors may shrink to, but never below, their minimum
     * useful heights.
     */
    const QList<Section> donors =
        automaticOpenDonorOrder(
            candidate
            );

    for (
        const Section donor
        : donors
        ) {
        if (
            remainingNeeded <= 0
            || !isOpen(
                donor,
                openSections
                )
            ) {
            continue;
        }

        int &donorHeight =
            heightForSection(
                result.heights,
                donor
                );

        const int donorMinimum =
            minimumForSection(
                limits,
                donor
                );

        const int available =
            std::max(
                0,
                donorHeight
                    - donorMinimum
                );

        const int amount =
            std::min(
                remainingNeeded,
                available
                );

        donorHeight -=
            amount;

        candidateHeight +=
            amount;

        remainingNeeded -=
            amount;
    }

    if (remainingNeeded > 0) {
        /*
         * The capacity threshold was reached too early.
         *
         * Return the original state unchanged rather than
         * producing a partially opened section.
         */
        result.openSections =
            openSections;

        result.heights =
            currentHeights;

        result.completed =
            false;

        return result;
    }

    /*
     * Any genuinely unowned space left after opening the
     * section belongs to the normal surplus owner of the
     * resulting state.
     */
    if (freeSpace > 0) {
        const std::optional<Section> recipient =
            growthSurplusOwner(
                result.openSections
                );

        if (recipient.has_value()) {
            heightForSection(
                result.heights,
                recipient.value()
                ) += freeSpace;
        }
    }

    result.completed =
        true;

    return result;
}

InvestigationSectionResizePolicy::
    AutomaticResizeResult
        InvestigationSectionResizePolicy::
    resizeAutomatically(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        const CompactHeights &compactHeights,
        bool timelinePreferredCollapsed,
        bool eventsPreferredCollapsed,
        bool lowerPreferredCollapsed,
        int delta
        )
{
    AutomaticResizeResult result;

    result.openSections =
        openSections;

    result.heights =
        currentHeights;

    if (delta == 0) {
        return result;
    }

    /*
     * =========================================================
     * SHRINK
     * =========================================================
     */
    if (delta < 0) {
        int remainingLoss =
            -delta;

        while (remainingLoss > 0) {
            /*
             * Once everything is collapsed, only the
             * deliberately oversized Event container can
             * absorb further loss.
             */
            if (!anySectionOpen(
                    result.openSections
                    )) {
                const int available =
                    std::max(
                        0,
                        result.heights.events
                            - compactHeights.events
                        );

                const int amount =
                    std::min(
                        remainingLoss,
                        available
                        );

                result.heights.events -=
                    amount;

                remainingLoss -=
                    amount;

                break;
            }

            /*
             * First consume ordinary continuous shrink
             * according to the current open-section state.
             */
            const ResizeResult continuous =
                resizeOpenSections(
                    result.openSections,
                    result.heights,
                    limits,
                    -remainingLoss
                    );

            result.heights =
                continuous.heights;

            if (continuous.unconsumedDelta == 0) {
                remainingLoss = 0;
                break;
            }

            remainingLoss =
                -continuous.unconsumedDelta;

            /*
             * Every currently open section has now reached
             * the appropriate useful floor.
             *
             * The next pixel of requested shrink causes the
             * next automatic collapse.
             */
            const QList<Section> collapseOrder =
                automaticCollapseOrder(
                    result.openSections
                    );

            if (collapseOrder.isEmpty()) {
                break;
            }

            const Section candidate =
                collapseOrder.first();

            const TransitionResult transition =
                automaticCollapseSection(
                    result.openSections,
                    result.heights,
                    compactHeights,
                    candidate
                    );

            if (!transition.completed) {
                break;
            }

            result.openSections =
                transition.openSections;

            result.heights =
                transition.heights;

            result.transitions.append(
                candidate
                );

            /*
             * The collapse itself does not consume the
             * outstanding outer-window loss.
             *
             * It releases space into a surviving section
             * (or the Event container when everything is
             * closed), and the loop then continues
             * consuming the same remaining loss.
             */
        }

        result.unconsumedDelta =
            -remainingLoss;

        return result;
    }

    /*
     * =========================================================
     * GROWTH
     * =========================================================
     */
    int remainingGrowth =
        delta;

    while (remainingGrowth > 0) {
        const std::optional<Section> candidate =
            nextAutomaticOpenCandidate(
                result.openSections,
                timelinePreferredCollapsed,
                eventsPreferredCollapsed,
                lowerPreferredCollapsed
                );

        /*
         * Every user-preferred section is already open.
         *
         * Ordinary continuous growth owns the rest.
         */
        if (!candidate.has_value()) {
            result.heights =
                applyGrowthWithoutTransitions(
                    result.openSections,
                    result.heights,
                    limits,
                    remainingGrowth
                    );

            remainingGrowth = 0;

            break;
        }

        /*
         * The current geometry may already contain enough
         * room to open the next section immediately.
         */
        const TransitionResult immediateTransition =
            automaticOpenSection(
                result.openSections,
                result.heights,
                limits,
                compactHeights,
                candidate.value()
                );

        if (immediateTransition.completed) {
            result.openSections =
                immediateTransition.openSections;

            result.heights =
                immediateTransition.heights;

            result.transitions.append(
                candidate.value()
                );

            continue;
        }

        /*
         * Determine whether this resize contains enough
         * growth to reach the next opening threshold.
         *
         * Opening feasibility is monotonic as growth is
         * added, so find the first growth amount at which
         * the transition becomes possible.
         */
        const SectionHeights maximumGrowthHeights =
            applyGrowthWithoutTransitions(
                result.openSections,
                result.heights,
                limits,
                remainingGrowth
                );

        const TransitionResult maximumTransition =
            automaticOpenSection(
                result.openSections,
                maximumGrowthHeights,
                limits,
                compactHeights,
                candidate.value()
                );

        if (!maximumTransition.completed) {
            /*
             * The threshold is not crossed during this
             * resize. Apply all growth continuously.
             */
            result.heights =
                maximumGrowthHeights;

            remainingGrowth = 0;

            break;
        }

        /*
         * Binary-search the exact first pixel at which the
         * section can open.
         */
        int low = 1;
        int high =
            remainingGrowth;

        while (low < high) {
            const int middle =
                low
                + (high - low) / 2;

            const SectionHeights middleHeights =
                applyGrowthWithoutTransitions(
                    result.openSections,
                    result.heights,
                    limits,
                    middle
                    );

            const TransitionResult middleTransition =
                automaticOpenSection(
                    result.openSections,
                    middleHeights,
                    limits,
                    compactHeights,
                    candidate.value()
                    );

            if (middleTransition.completed) {
                high =
                    middle;
            } else {
                low =
                    middle + 1;
            }
        }

        const int growthToThreshold =
            low;

        result.heights =
            applyGrowthWithoutTransitions(
                result.openSections,
                result.heights,
                limits,
                growthToThreshold
                );

        remainingGrowth -=
            growthToThreshold;

        const TransitionResult transition =
            automaticOpenSection(
                result.openSections,
                result.heights,
                limits,
                compactHeights,
                candidate.value()
                );

        if (!transition.completed) {
            /*
             * This should be impossible if the monotonic
             * threshold search above is correct.
             *
             * Preserve a conservative failure result rather
             * than inventing geometry.
             */
            result.unconsumedDelta =
                remainingGrowth;

            return result;
        }

        result.openSections =
            transition.openSections;

        result.heights =
            transition.heights;

        result.transitions.append(
            candidate.value()
            );

        /*
         * Continue with the remaining growth using the new
         * open-section configuration.
         */
    }

    result.unconsumedDelta =
        remainingGrowth;

    return result;
}