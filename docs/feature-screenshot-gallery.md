# TraceScope Feature Screenshot Gallery

This gallery provides a visual walkthrough of the TraceScope `v0.13.0` investigation workflow. The screenshots use fictional repository samples and reusable import profiles so the demonstrated behavior can be reproduced without external services.

For a product overview, downloads, supported formats, and current capabilities, see the [main README](../README.md).

## Investigation Workspace

TraceScope combines source/session context, investigation filters, a scalable event timeline, sortable normalized records, review panels, navigation controls, and selected-record details in one native desktop workspace.

The example below uses the fictional Order Fulfillment Incident sample, which contains distinct periods of database, payment/dependency, and messaging degradation.

![TraceScope Dashboard](screenshots/tracescope-dashboard.png)

## Import Configuration

The Import Configuration workflow keeps source interpretation explicit. Users can choose or drag in a file, review the suggested format, load or edit a reusable profile, inspect normalized preview records alongside raw source, and validate mappings before importing.

The example below uses the degraded Field Gateway support capture in key-value/logfmt format.

![TraceScope Import Configuration](screenshots/tracescope-import-configuration.png)

## Advanced Filtering and Navigation

Canonical and source-specific criteria can be combined to narrow an investigation without discarding the underlying source context. TraceScope supports multi-severity, subsystem, event-code, entity, UTC time-range, search, custom-field, bookmark, and finding-status filtering, plus reusable named presets.

Navigation and drill-down actions operate directly on the active investigation rather than requiring a separate query interface.

![TraceScope Advanced Filtering and Navigation](screenshots/tracescope-advanced-filtering.png)

## Analytics Overview

When the relevant canonical fields are available, the Analytics overview shows deterministic event-code frequencies and Top Entities. The timeline can simultaneously break activity down by the most frequent subsystems so investigators can connect frequency summaries with changes over time.

Top-N limits are presentation choices; the underlying deterministic analysis is not truncated.

![TraceScope Analytics Overview](screenshots/tracescope-analytics-overview.png)

## Deterministic Burst Detection

TraceScope groups qualifying clusters of WARN, ERROR, and CRITICAL records into deterministic investigation bursts. Each burst explains the thresholds and timing that caused it to qualify and summarizes contributing severity counts, subsystems, event codes, entities, and source records.

Auto mode derives timing from the investigation's timestamp cadence with an explicit fallback for sparse data; Manual mode allows direct timing control. Double-clicking a burst narrows the investigation to its contributing elevated-event range while preserving unrelated filters.

This feature is deterministic analysis, not AI anomaly detection or root-cause diagnosis.

![TraceScope Burst Detection](screenshots/tracescope-burst-detection.png)

## Findings Review

Bookmarks, multiline analyst notes, and Open/Resolved/Dismissed finding states let investigators preserve what they discovered while a session remains open.

The Findings panel summarizes classified records with source-record and timestamp context. Double-click navigation returns to the exact source record and relaxes only filters that would otherwise hide it.

The example below uses the fictional Environmental Chamber QA Run and records conclusions around DUT-specific thermal and power behavior.

![TraceScope Findings Review](screenshots/tracescope-findings.png)

## Fine-Resolution Timeline Navigation

Automatic timeline resolution provides an investigation overview, while manual resolutions preserve millisecond-through-day-scale detail when needed.

When a chosen resolution would create too many buckets, TraceScope materializes only a bounded visible window and provides horizontal navigation. The number of buckets shown also adapts to available width so labels remain readable in narrow workspaces without changing the selected resolution. Range context and Y-axis scaling remain stable while moving through the investigation.

![TraceScope Timeline Navigation](screenshots/tracescope-timeline-navigation.png)

## Multi-Session Workspace

Related sources can remain open as independent investigation sessions in one application instance. Session switching preserves each source's import context, active filters, model state, presentation state, bookmarks, notes, findings, and reload behavior.

The example below uses matched known-good and degraded fictional Field Gateway captures so an investigator can keep both sessions available while moving between their individual evidence.

![TraceScope Multi-Session Workspace](screenshots/tracescope-multi-session-workspace.png)

## Session Comparison Setup

A comparison is created explicitly between two complete imported sessions with a directional **Baseline → Comparison** orientation. The setup dialog makes that orientation visible, supports swapping the two sessions, prevents comparing a session with itself, and can apply one shared burst-detection configuration to both sides when burst comparison is requested.

Comparisons operate on complete session snapshots rather than the sessions' current filtered views, so temporary investigation filters do not silently change the comparison basis.

![TraceScope Session Comparison Setup](screenshots/tracescope-session-comparison-setup.png)

## Session Comparison

The dedicated comparison document surfaces investigation-relevant differences while avoiding unsupported causal claims. It prioritizes event codes that appeared, disappeared, or changed; severity changes; elevated subsystem/entity activity; conservative custom-field differences; burst differences when requested; and session-level context such as totals, duration, and event rate.

Unavailable dimensions are identified as unavailable rather than treated as zero. Unchanged or low-value output is omitted so the comparison stays focused on differences that may help an investigator decide where to look next.

The example below compares the matched known-good and degraded Field Gateway support sessions.

![TraceScope Session Comparison](screenshots/tracescope-session-comparison.png)

## Detachable Multi-Window Workspace

Investigation and comparison documents can be detached from the main window, grouped with other documents in detached workspace windows, moved between detached windows, and re-docked without losing their document state.

This supports practical workflows such as keeping a baseline session, degraded session, and comparison visible across multiple monitors or arranging related evidence side by side during an investigation.

![TraceScope Detached Workspace](screenshots/tracescope-detached-workspace.png)

## Constrained and Portrait Workspaces

Workspace documents adapt to constrained widths that are common in split-screen and portrait-monitor use. Long session summaries elide instead of forcing window width, filters reflow when needed, fine-resolution timelines reduce the visible bucket window, and Findings/Analytics receive more horizontal space than Selected Event Details when the review surface is width-constrained.

The goal is not a separate mobile-style interface; it is to keep the native desktop investigation workflow usable when detached documents occupy narrower engineering workspaces.

![TraceScope Portrait Workspace](screenshots/tracescope-portrait-workspace.png)

## Responsive Large-File Import

Import work runs outside the UI thread. Streamed import paths can report determinate progress and support cooperative cancellation while keeping the desktop interface responsive.

Measured large-file behavior and its interpretation limits are documented separately in [Performance Notes](performance.md).

![TraceScope Large-File Import](screenshots/tracescope-large-file-import.png)

## Recent Files and Workspaces

Bounded recent-file, recent-profile, and recent-workspace histories are stored in local application settings. Recent files reopen through the normal Import Configuration workflow rather than bypassing profile review and validation, while recent workspaces reopen saved investigations through the workspace-restoration workflow.

![TraceScope Recent Files](screenshots/tracescope-recent-files.png)

## CSV Export

TraceScope exports the currently visible investigation records to CSV using readable canonical headers and configured custom-field names. This allows an investigator to narrow a session first and hand off only the records relevant to the current question or finding.

![TraceScope Exported CSV](screenshots/tracescope-exported-csv.png)
