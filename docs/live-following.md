# Following Live Logs in TraceScope

You do not have to wait for an application to finish writing its log before investigating it. **Live Following** lets TraceScope continue reading a supported file as another process adds records. Incoming records join the same investigation as the initial import, so you can filter, inspect, navigate, bookmark, and analyze them without switching to a separate monitoring interface.

This guide explains the live-following controls and what happens when an external log file changes. It also includes an **optional hands-on walkthrough** with the standalone TraceScope Live-Log Generator. You can follow the main instructions with your own actively written log; downloading the generator is not required to use TraceScope.

**Having trouble following a log?** [Jump straight to Troubleshooting](#troubleshooting).

**In this guide, you will:**

1. [Start following a file](#3-start-following-the-generated-file) that another application is writing.
2. [Use Pause, Resume, Stop, and Follow Newest](#4-use-the-live-controls-deliberately) without losing track of your investigation.
3. [Understand how newly arriving records interact with filters and analytical views](#5-investigate-while-new-records-arrive).
4. [Recognize truncation, replacement, rotation, and incomplete writes](#6-understand-file-changes-during-live-following).
5. [Preserve collected evidence](#7-preserve-evidence-before-it-disappears) before the external source changes or disappears.

If you are new to importing or filtering files, begin with [Getting Started](getting-started.md), [Importing Logs](importing-logs.md), and [Investigating Logs](investigating-logs.md). For the generator's design, additional scenarios, and command-line options, see the separate [Live-Log Generator documentation](live-log-generator.md).

## 1. Understand what Live Following does

A normal import reads the available source records into a TraceScope investigation. When you enable Live Following, TraceScope also observes the **active external source file** for additional records. As complete records arrive, they enter the existing Event Table; TraceScope does not create a second live-only investigation or require the producer to communicate with it.

The source remains an ordinary file written by the external application. TraceScope is an observer, not the producer, and cannot control how long that application retains its logs.

The main distinctions are:

- **Import** reads the records available when you open the source and establishes the investigation.
- **Live Following** continues admitting newly written records from its active external source while it is running.
- **Pause** temporarily stops reading while retaining the current follow position, allowing TraceScope to catch up when you resume *if the unread source data is still available*.
- **Stop** ends the current follow operation. Starting again uses the retained follow position when the physical source still permits it.
- **Follow Newest** is a separate navigation preference. It keeps the Event Table positioned at the latest visible end and can keep your selected record moving forward if you explicitly select that latest row.

Live Following is available when an investigation has an external source and a supported import profile. A standalone snapshot with no connected external source is not, by itself, an actively followable file. The file must remain accessible at the configured source path.

**Important:** Live Following does not guarantee that every byte ever written by a producer can be recovered. A source that is truncated, deleted, or replaced while TraceScope is not reading it may lose unread data. Later sections explain how to recognize source changes and preserve evidence that TraceScope has already admitted.

## 2. Optional: prepare the Live-Log Generator walkthrough

If you already have an application writing a supported log, skip to [Start following your own log](#start-following-your-own-log). Otherwise, the generator supplies a predictable fictional incident to investigate without requiring another application or a network service.

The generator is a **separate utility**, not part of the main TraceScope application download. TraceScope does not depend on it. It simply reads the output file the generator writes, exactly as it would read a log produced by another application.

### Obtain the generator and sample files

1. Visit [TraceScope Releases](https://github.com/w-cook/tracescope-qt-log-inspector/releases) and download the **separate Live-Log Generator package** for your platform. Starting with v1.0, generator downloads are provided for both Windows and Linux alongside the main TraceScope application packages. The generator is optional; you do not need it to follow your own logs.
2. Extract the generator package and launch `TraceScopeLiveLogGeneratorLauncher`. Keep its companion `TraceScopeLiveLogGenerator` executable with the launcher; the launcher runs that program to write logs. For command-line use or source builds, see the [Live-Log Generator documentation](live-log-generator.md).
3. In the **samples included with the generator**, locate `field-gateway-live-scenario.json` and its matching `field-gateway-live-profile.json`. The scenario and profile are different files: the generator reads the **scenario**, while TraceScope loads the **import profile**. These live demonstration assets are distributed with the generator, separately from the main application's sample collection.
4. Choose a writable, easily accessible location for the generated output, such as a temporary demonstration directory. Use a new filename—for example, `field-gateway-live.jsonl`—rather than an existing log you need to keep.

**Treat generated output as disposable.** The scenario intentionally truncates its file partway through playback, and some generator settings recreate output files between iterations. Do not point it at important data.

### Configure and run the demonstration

In the graphical launcher:

1. Select **Browse...** beside **Scenario** and choose `field-gateway-live-scenario.json`.
2. Select **Browse...** beside **Output** and choose the new `field-gateway-live.jsonl` destination.
3. Select **`jsonl`** as the output format and **`0.5x`** as the playback speed. The slower speed gives you more time to open the growing file in TraceScope.
4. Enable **Loop scenario** and leave **Loop behavior** set to **Append continuously** if you want the demonstration to keep running while you explore. The scenario's *own* truncation step still occurs during each iteration; append mode only controls what happens **between** iterations.
5. Select **Play**. The generator creates and begins writing the output file. Leave it running while you open the file in TraceScope.

The scenario moves from healthy gateway activity through rising latency, retries, timeout pressure, and recovery. It includes a partial write and an **intentional in-place truncation**. At `0.5x`, that truncation occurs relatively early in the demonstration, so you may first see records from an already-changing file. That is not a problem with the walkthrough. If you prefer a slower initial run, the command-line generator also accepts playback multipliers not offered by the launcher's preset list; see [Runtime Options](live-log-generator.md#runtime-options). You can also stop playback and restart with a fresh output filename to repeat the exercise.

Do **not** change the generator's output format to CSV, structured JSON, or another format while continuing to use the JSON Lines profile from this example. Other formats require suitable matching import profiles.

## 3. Start following the generated file

Once the output file exists and is growing:

1. In TraceScope, choose **File > Open Log File...**.
2. Select the generator's output file, `field-gateway-live.jsonl`.
3. In **Import Configuration**, choose **Load Profile...** and open `field-gateway-live-profile.json`. Confirm that the format is **JSON Lines** and review the preview. As the generator continues writing, the sample preview can differ depending on when you open it.
4. Select **Import** to open the investigation.
5. Find the compact live controls beside the investigation's **workspace tab**. Hover over the buttons if you need their full labels. The status initially reads **Stopped**; select **Start live following** (the play icon).
6. Look at **Telemetry Events**. When the producer writes complete new records, they should appear in the current investigation. The live status changes to **Following**.

TraceScope establishes an import-to-follow handoff rather than assuming no records were written during the import. If the initial import and first live read overlap, the session uses record identities to prevent duplicate admission of the same initial-generation records.

The demonstration deliberately changes the file while it is being read. If you encounter the truncation almost immediately, keep following and observe the next source generation. To inspect the beginning of the scenario at a more relaxed pace, stop the generator, choose a fresh output filename, and repeat the import.

### Start following your own log

For your own source, the procedure is the same: import the existing file with the correct profile, then select **Start live following** beside its workspace tab. Ensure another application is actually writing **that same active file path**. A static file will still open as an investigation, but no additional records will appear until the source grows.

If the live controls are absent, first confirm that this is an investigation with a connected external file rather than an unconnected snapshot, and that the chosen import profile supports live record detection. For help selecting or configuring a profile, see [Importing Logs](importing-logs.md).

## 4. Use the live controls deliberately

The compact control area beside a supported investigation tab shows the current status and its available actions. The icons change with the follow state; hovering reveals their names.

| Control | What it does | What to expect |
| --- | --- | --- |
| **Start live following** | Begins observing the configured active file. | Status becomes **Following** when startup succeeds. |
| **Pause live following** | Temporarily suspends ingestion. | Status becomes **Paused**. The producer can continue writing independently. |
| **Resume live following** | Continues from the retained follow state. | TraceScope reads available unread data and returns to **Following**. |
| **Stop live following** | Ends the current following operation. | Status becomes **Stopped**. This does not delete records already in the investigation. |
| **Follow newest data** | Enables automatic latest-end navigation while actively following. | The Event Table moves to its newest visible end; the timeline can also track its latest window. |

### Try Pause and Resume

While the generator is running, select **Pause live following**. Leave the generator active for a short interval, then select **Resume live following**. New records that remain in the source should be admitted as TraceScope catches up.

**Do not use a long pause as a retention guarantee.** In this particular scenario, a pause that spans its intentional truncation can make some unread records unrecoverable. That mirrors the behavior of real log producers that rotate or rewrite files while an observer is paused.

**Pause is not Stop.** Resume retains the active parser and read position. Stop ends the following operation; a later Start can continue from its retained position when the original file remains available, or begin a new source generation if the physical source has changed in the meantime.

### Choose when to Follow Newest

Turn on **Follow newest data** when you want to watch new activity arrive. TraceScope scrolls the Event Table to its latest visible end; if a manually sized timeline has a scrollable time range, it can also move to its latest visible window.

There are two related but distinct behaviors:

- **Following the view:** enabling Follow Newest keeps the table's latest visible end in view as records arrive.
- **Following the selection:** if you *explicitly select the last visible row while Follow Newest is enabled*, TraceScope can also advance the selected row and Event Details to newly arriving visible records. Simply enabling the option does not automatically replace your existing record selection.

If you choose an older row, TraceScope respects that investigative selection rather than continuously replacing it with each new event. For chronological observation, keep a suitable timestamp or source-order sort; the table's last displayed row depends on the current sort and filters.

Follow Newest is separate from ingestion. You can turn it off to examine historical records **without stopping live following**. Pausing or stopping live following turns Follow Newest off, so re-enable it if you want that navigation behavior after resuming or restarting.

## 5. Investigate while new records arrive

A live investigation uses the same filtering and review controls as a static one. In the Field Gateway demonstration, try these steps while following continues:

1. Select the `WARN`, `ERROR`, and `CRITICAL` severity values as they become available. New elevated events matching your filter enter the visible Event Table; other incoming severities still belong to the underlying investigation.
2. Select an arriving event and read its **Event Details**, including its event code, entity, and available custom fields such as latency or queued-record counts.
3. Open **Issue Summary** or **Analytics** to examine the current elevated-event groups or event-code frequencies. Use the timeline to observe changes over time.
4. Turn **Follow newest data** off when you want to investigate a specific event without the viewport moving to the latest activity. Enable it again when you are ready to watch the incoming end.
5. If a record is worth reviewing later, bookmark it, add an analyst note, or give it a finding status as described in [Findings](findings.md).

New matching records enter the Event Table incrementally. More expensive derived views—such as summary, analytics, and general timeline refreshes—are updated on a coordinated schedule rather than being rebuilt on every live-file poll. Their displayed counts or visualizations can therefore briefly lag behind the most recent Event Table rows. An actively visible timeline with Follow Newest enabled receives a separate, faster coalesced refresh path.

As with a static investigation, source-data filters affect the analysis population, while review-only **Bookmarks only** and finding-status filters affect Event Table visibility without redefining the underlying analytical population. See [Investigating Logs](investigating-logs.md#2-understand-what-filtering-changes) for that distinction.

Live Following records observations; it does not automatically identify root causes. A warning near a timeout, or a change in a chart, is a reason to inspect the associated records—not proof of causation.

## 6. Understand file changes during live following

Real applications do more than append new lines. They may pause output, write a record in pieces, truncate a file, replace it at the same path, or rotate it to an archive. TraceScope has distinct handling for these cases.

### Incomplete writes

A producer might write part of a record and finish it later. For supported live import paths, TraceScope waits for enough source content to complete that record instead of treating every newly written fragment as a finished event. This applies to ordinary line-oriented sources and appropriately configured repeated-record structured sources, although their framing rules differ.

If an expected record does not appear immediately, the producer may not have finished writing it. First confirm that a complete new record has actually reached the file.

### Truncation and same-path replacement

**Truncation** means the current file is shortened and then written again. **Same-path replacement** means a new physical file takes over the configured path. When TraceScope detects such a change, it starts a **new source generation**, preserving the distinction between previously admitted evidence and newly arriving records.

In the optional Field Gateway walkthrough, the generator intentionally truncates its output before writing recovery records. Keep an eye on the Event Table as playback crosses that point. Records already admitted to the open investigation remain available, and later records are associated with the new source generation.

This is not a promise that records which disappeared **before TraceScope read them** can be reconstructed. If the producer overwrites or removes unread material during a pause, stop, or other gap, that missing material may be permanently unavailable.

### Rotated source families

Rotation is different from a same-path source generation. A producer may rename an older log to a numbered or dated archive, then continue writing a fresh active file at the original path. TraceScope can import a configured **rotated source family** as one logical investigation while retaining the physical files' individual source identities. See [Rotated Source Files](importing-logs.md#6-import-a-rotated-source-family) for discovery, ordering, and import setup.

**Live Following targets the configured active file**, not every archive in a directory. When that active path is replaced by a new file, TraceScope can recognize a new generation of the active source. It does not mean newly appearing rotated archives are automatically reimported as new family members during following.

For a dedicated manual rotation exercise, the generator package also includes `warehouse-sync-deployment-rotation-live-scenario.json`, which includes replacement and multiple rotations. Use the [generator documentation](live-log-generator.md#scenario-library) for scenario details, and configure a matching output format, import profile, and optional source-family rules before trying it.

## 7. Preserve evidence before it disappears

A live investigation may contain records that **no longer exist** in the producer's current file. That can happen after truncation, replacement, rotation, or routine log cleanup. Treat the current source file and the evidence already collected by TraceScope as two potentially different things.

### Save your complete workspace

Choose **File > Save Workspace** (or **Save Workspace As...**) to save your current work as a `.tsw` workspace. A workspace save also writes durable `.tsinv` evidence snapshots into its companion `<workspace-name>.sessions/` directory. Keep the workspace file and that directory together when moving or backing up the workspace.

The workspace saves broader investigation context, including bookmarks, notes, finding statuses, filters, and other workspace state. By contrast, a standalone `.tsinv` is an **evidence snapshot**, not a complete workspace with all analyst annotations.

**Source-backed recovery is a choice:** saving a SourceBacked workspace captures a snapshot fallback without automatically changing the open session's backing mode. If the external file is *missing* when that workspace is reopened, TraceScope can offer **Open Saved Snapshot** to recover the saved evidence. If the external source still exists but has since been truncated or rewritten, ordinary SourceBacked loading still treats the external source as authoritative. Do not assume a normal reload will automatically restore older live generations from the fallback.

### Deliberately switch to preserved evidence when appropriate

If your goal is to retain the exact evidence admitted so far **independently of what happens to the current source**, use the investigation tab's context menu: **Source > Preserve as Snapshot Only...**. This captures durable evidence and changes the investigation to **SnapshotBacked**. Once detached, that investigation is no longer actively following an external source. **Save the workspace after this transition**: the newly preserved snapshot initially belongs to the current working workspace, and saving commits the new backing state and its snapshot into the workspace package.

If you later want continued ingestion from a compatible, verified external source, **Source > Reconnect Source** can restore a **Hybrid** investigation where preserved evidence remains the floor and the external source can contribute continuation records. A Hybrid investigation can also be explicitly returned to SnapshotBacked. **Use Source as Authoritative...** is a separate, potentially evidence-discarding choice that rebuilds from the connected source; do not select it if your purpose is to retain historical snapshot-only records.

You can also use **File > Save Investigation Snapshot...** to create a standalone `.tsinv` capture without changing the currently open investigation's backing mode. That is useful for a point-in-time evidence copy, but it does not replace saving your `.tsw` workspace when you need findings, notes, filters, and layout to be restored together.

For the complete backing-state, reconnection, and recovery workflow, see [Saving and Restoring](saving-and-restoring.md).

## Troubleshooting

| Situation | What to check |
| --- | --- |
| There is no live-follow control beside the tab. | Confirm the session has a connected external source and a supported live-capable import profile. A disconnected standalone snapshot cannot be followed without reconnection. See [Understand what Live Following does](#1-understand-what-live-following-does) and [Use the live controls deliberately](#4-use-the-live-controls-deliberately) for related information. |
| Status stays **Stopped** or shows **Error** when you try to start. | Confirm the active file still exists at the configured path and is accessible. Hover over the error status for its message. If the source moved, use the appropriate source-path/reconnection workflow instead of silently treating a different file as the same source. See [Use the live controls deliberately](#4-use-the-live-controls-deliberately) and [Understand file changes during live following](#6-understand-file-changes-during-live-following) for related information. |
| The table does not grow, but the status says **Following**. | Confirm the producer is writing to the exact active file path; check whether it has actually emitted *complete* records, and whether your current filters exclude the incoming values. See [Start following the generated file](#3-start-following-the-generated-file) and [Investigate while new records arrive](#5-investigate-while-new-records-arrive) for related information. |
| The producer keeps writing but the Event Table seems stationary. | You may have Follow Newest disabled, or your filters or sort may put new events somewhere other than the expected visible end. The producer and ingestion can continue while you inspect an older record. See [Use the live controls deliberately](#4-use-the-live-controls-deliberately) for more information. |
| Resume does not recover some earlier records. | Check whether the file was truncated, replaced, or rotated while paused. TraceScope cannot read bytes that the producer has already removed. See [Use the live controls deliberately](#4-use-the-live-controls-deliberately) and [Understand file changes during live following](#6-understand-file-changes-during-live-following) for related information. |
| Some records appear to have vanished after reloading a saved workspace. | Check whether the session is SourceBacked and the existing external file has since changed. Use a preserved SnapshotBacked/Hybrid workflow for evidence that must outlive source rewriting, and retain the `.tsw` companion snapshots. See [Preserve evidence before it disappears](#7-preserve-evidence-before-it-disappears) and [Saving and Restoring](saving-and-restoring.md#5-understand-the-three-backing-modes) for related information. |
| The generator demonstration finishes before you can import it. | Select **0.5x** and **Loop scenario** in the launcher, or run the command-line generator at a slower multiplier. Use a fresh disposable output filename when repeating the setup. See [Optional: prepare the Live-Log Generator walkthrough](#2-optional-prepare-the-live-log-generator-walkthrough) for more information. |

## What to read next

- [Investigating Logs](investigating-logs.md) — filtering, navigation, timeline, issue summaries, and analytics.
- [Findings](findings.md) — bookmarks, notes, review statuses, and finding exports.
- [Importing Logs](importing-logs.md#6-import-a-rotated-source-family) — selecting and reviewing rotated source families.
- [Live-Log Generator](live-log-generator.md) — all sample scenarios, supported renderers, loop behavior, and building the utility.
- [Saving and Restoring](saving-and-restoring.md) — detailed workspace, snapshot, source-continuity, and recovery procedures.

The optional generator is only a way to produce repeatable demonstration files. In everyday use, TraceScope follows ordinary supported log files written by your own applications and services.
