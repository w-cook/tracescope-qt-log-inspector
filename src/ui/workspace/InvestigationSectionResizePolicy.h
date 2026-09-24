#pragma once

#include <optional>

#include <QList>

class InvestigationSectionResizePolicy final
{
public:
    enum class Section
    {
        Timeline,
        Events,
        LowerDetails
    };

    struct OpenSections
    {
        bool timeline = false;
        bool events = false;
        bool lowerDetails = false;
    };

    struct SectionHeights
    {
        int timeline = 0;
        int events = 0;
        int lowerDetails = 0;
    };

    struct ResizeLimits
    {
        int timelinePreferred = 0;
        int timelineMinimum = 0;

        int eventsMinimum = 0;

        int lowerPreferred = 0;
        int lowerMinimum = 0;
    };

    struct CompactHeights
    {
        int timeline = 0;
        int events = 0;
        int lowerDetails = 0;
    };

    struct TransitionResult
    {
        OpenSections openSections;
        SectionHeights heights;

        /*
     * False means the requested transition could not be
     * completed without pushing an existing open section
     * below its minimum useful height.
     */
        bool completed = false;
    };

    struct AutomaticResizeResult
    {
        OpenSections openSections;
        SectionHeights heights;

        /*
     * Sections opened or collapsed during this resize,
     * in transition order.
     *
     * A positive resize contains only openings.
     * A negative resize contains only collapses.
     */
        QList<Section> transitions;

        /*
     * Normally zero.
     *
     * A negative remainder means even the completely
     * collapsed presentation could not absorb the full
     * requested shrink.
     */
        int unconsumedDelta = 0;
    };

    struct ResizeResult
    {
        SectionHeights heights;

        /*
     * Normally zero.
     *
     * A negative value means the requested shrink could
     * not be completely absorbed because every open
     * section reached its minimum useful height.
     *
     * That remaining deficit is what the capacity layer
     * will later use to decide that a section must
     * collapse.
     */
        int unconsumedDelta = 0;
    };

    /*
     * While growing, these sections recover toward their
     * preferred heights in this order.
     *
     * Once those recoveries are complete, growth belongs
     * to growthSurplusOwner().
     */
    static QList<Section> growthRecoveryOrder(
        const OpenSections &openSections
        );

    /*
     * Section that owns unrestricted surplus once every
     * applicable preferred-height recovery is satisfied.
     */
    static std::optional<Section> growthSurplusOwner(
        const OpenSections &openSections
        );

    /*
     * While shrinking, open sections give up height toward
     * their minimum useful heights in this order.
     */
    static QList<Section> shrinkOrder(
        const OpenSections &openSections
        );

    /*
     * Once every currently open section has reached its
     * minimum useful height, automatic capacity collapse
     * occurs in this order.
     */
    static QList<Section> automaticCollapseOrder(
        const OpenSections &openSections
        );

    /*
     * Automatic reopening order, excluding sections that
     * the user explicitly prefers collapsed.
     */
    static QList<Section> automaticOpenOrder(
        bool timelinePreferredCollapsed,
        bool eventsPreferredCollapsed,
        bool lowerPreferredCollapsed
        );

    static ResizeResult resizeOpenSections(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        int delta
        );

    static TransitionResult automaticCollapseSection(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const CompactHeights &compactHeights,
        Section candidate
        );

    static TransitionResult automaticOpenSection(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        const CompactHeights &compactHeights,
        Section candidate
        );

    static AutomaticResizeResult resizeAutomatically(
        const OpenSections &openSections,
        const SectionHeights &currentHeights,
        const ResizeLimits &limits,
        const CompactHeights &compactHeights,
        bool timelinePreferredCollapsed,
        bool eventsPreferredCollapsed,
        bool lowerPreferredCollapsed,
        int delta
        );
};