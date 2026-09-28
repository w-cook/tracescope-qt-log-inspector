# Getting Started with TraceScope

TraceScope is an offline desktop application for inspecting and investigating file-based diagnostic and telemetry logs. This guide takes you from launching the application to saving your first investigation, using a fictional software incident supplied with the project.

You do not need to write code, run a server, or use the separate Live-Log Generator for this walkthrough.

**In this guide, you will:**

1. [Download and launch TraceScope](#1-download-and-launch-tracescope).
2. [Find the example log and profile](#2-find-the-example-log-and-its-profile), then [import the log](#3-import-the-log).
3. [Narrow the investigation to warning and error events](#5-investigate-a-warning).
4. [Inspect an event](#5-investigate-a-warning) and [preserve your observations](#6-preserve-what-you-found).
5. [Save your workspace](#7-save-your-workspace) so you can return to it later.

## 1. Download and launch TraceScope

Download the appropriate application package from the [TraceScope Releases page](https://github.com/w-cook/tracescope-qt-log-inspector/releases). Prebuilt packages are intended for users; Qt, CMake, and a compiler are not required to run them.

### Windows

1. Download the Windows x64 ZIP package for the release you want to use.
2. Extract the **entire** archive into a folder of your choice. Keep its included files and directories together.
3. Run `TraceScope.exe` from the extracted folder.

The Windows package includes demonstration files and reusable import profiles under `samples/`.

### Linux

1. Download the Linux x86_64 AppImage package.
2. Open a terminal in the directory containing the downloaded file and make it executable, replacing `<downloaded-file>` with its actual filename:

   ```bash
   chmod +x <downloaded-file>.AppImage
   ```

3. Launch the AppImage:

   ```bash
   ./<downloaded-file>.AppImage
   ```

4. Download and extract the release's separate samples archive for this walkthrough, or obtain the sample files from the repository.

The application runs locally. You do not need to create an account or configure a hosted service.

## 2. Find the example log and its profile

We will investigate a fictional order-processing application. Its logs include ordinary order activity and periods of database, payment, and messaging trouble. These records are demonstration data, not logs from a real production system.

Locate these two files in the supplied samples:

| File | Purpose |
| --- | --- |
| `samples/order-fulfillment-incident.jsonl` | The source log to investigate |
| `samples/profiles/order-fulfillment-incident-profile.json` | A reusable import configuration for that log |

You can also find the [sample log](../samples/order-fulfillment-incident.jsonl) and [matching profile](../samples/profiles/order-fulfillment-incident-profile.json) in the repository.

The `.jsonl` extension means **JSON Lines**: each line holds an individual JSON record. The matching profile tells TraceScope where to find familiar fields such as timestamp, severity, subsystem, event code, entity ID, and message. It also identifies useful source-specific attributes.

## 3. Import the log

1. In TraceScope, choose **File > Open Log File...** (`Ctrl+O` on Windows).
2. In the **Import Configuration** dialog, select **Browse...** and choose `order-fulfillment-incident.jsonl`.
3. Select **Load Profile...** and choose `order-fulfillment-incident-profile.json` from the `samples/profiles/` directory.
4. Confirm that the profile is named **Order Fulfillment Incident**, the selected format is **JSON Lines**, and the configuration reports that the profile is valid.
5. Examine the preview. You should see recognizable timestamps, severities, subsystems, and messages rather than having to interpret raw JSON yourself.
6. Select **Import** to open the investigation.

The Import Configuration layout may differ according to your window size. Its controls and validation have the same purpose in the wide and compact layouts.

**Why load a profile?** Different sources may use different field names or structures. A TraceScope import profile makes those mappings explicit and reusable. In this example, the supplied profile maps the source's `level` field to severity and retains attributes such as host, correlation ID, latency, and queue depth. You do not need to create or edit a profile for this first investigation.

## 4. Get oriented in the investigation

After the import, the log is displayed as an investigation. Take a moment to explore the main areas:

- **Event table:** Browse, sort, and select normalized records. Source-specific fields appear alongside the recognized investigation fields where available.
- **Filters and search:** Narrow the event table by severity, subsystem, event code, and other available fields, or search across the records.
- **Event Details:** Select a record row in the **Telemetry Events** table. The separate **Event Details** panel displays that record's normalized values, source context, and additional attributes. This is also where you can bookmark and annotate evidence.
- **Timeline and analysis:** Explore how recorded activity changes over time and use the available summary and analytical views to investigate patterns.

### Adjust the layout for your screen

The investigation may look different on your computer depending on your window size, monitor resolution, operating-system display scaling, and TraceScope's own **Interface Scale** setting. The investigation adapts to its available space: on a smaller window, a low-resolution display, or a display with high scaling, the **Timeline**, **Telemetry Events**, or lower investigation panels may initially appear **collapsed**. This is intentional. The sections are still available; the application has reduced the visible layout to keep it usable in the available space.

If you want more room or need to change the size of the interface:

1. Try maximizing or enlarging the TraceScope window.
2. Open **View > Interface Scale**. The **100% (System)** option uses the operating system's display scaling without an additional TraceScope multiplier. If the interface is too large for your available space, try **90%**, **80%**, or another smaller option. If controls and text are too small, choose a larger percentage instead. Changes take effect while the application is open.
3. Use the small **chevron near the right side of each section's title** to collapse or expand the Timeline, Telemetry Events, or the entire lower investigation area. You can also drag the dividers between expanded sections to distribute the available space.

**When space is limited:** Expanding one section may cause a different section to collapse so the requested area can fit. If a section cannot expand, give the window more vertical space or reduce Interface Scale. You can also leave sections collapsed and open only the area you need; there is no requirement to match a particular layout to complete this walkthrough.

The lower investigation area contains the **Issue Summary**, **Findings**, and **Analytics** tabs alongside **Event Details**. If you cannot see these controls when a later step refers to them, expand that lower area first.

## 5. Investigate a warning

The sample begins with routine processing and later includes elevated database activity and other operational problems. Rather than reading every line, start by narrowing the investigation.

1. Open the severity filter, initially labeled **All severities**.
2. Select `WARN` and `ERROR` to limit the visible records to those severity levels. Notice that the severity values **WARN** and **ERROR** now appear in **bold** in the Event Table. This indicates that those values are included in your active severity filter.
3. Inspect the remaining events in the **Telemetry Events** table. Find a record with the event code `DB_POOL_HIGH`; the associated message reports elevated database connection-pool utilization.
4. Select that **record row in the Event Table**, then look at the separate **Event Details** panel below. Pay particular attention to its timestamp, subsystem, event code, message, source-record context, and any relevant custom attributes.

You have now moved from an entire source file to a specific record worth examining. The filter limits what you see; it does not remove records from the imported investigation.

You can use **Reset Filters** whenever you want to return to the full visible dataset.

**Interpretation note:** A warning is a useful investigative lead, not proof of a root cause. TraceScope helps you inspect and organize evidence; it does not automatically establish why an incident occurred.

## 6. Preserve what you found

With the `DB_POOL_HIGH` record still selected in the Event Table:

1. In **Event Details**, select **Bookmark Event**. Look back at the Event Table: a **star (★) appears next to that record's event number**. This gives you an immediate visual indication that the record is bookmarked.
2. In **Event Details**, change **Finding status** from **None** to **Open**. A bookmark marks a record for your attention; assigning a finding status also adds it to your formal findings list.
3. In the lower investigation area, select the **Findings** tab. If that area is collapsed, expand it using its chevron first. Find your newly created **Open** finding in the list. Its text initially begins with **(No analyst note)** and includes the event's code and message.
4. Return to **Event Details** for the selected record and select **Add Note**. Enter a brief observation, such as: `Review this database-pool warning alongside subsequent order-processing and dependency failures.` Select **Save** in the note editor.
5. Look at the **Findings** tab again. The entry's default **(No analyst note)** description has been replaced by **your note**. This demonstrates how the Event Table, Event Details, and Findings tab represent the same underlying record and investigation state.

You have now bookmarked a source record, classified it as an unresolved finding, and attached an observation directly to that evidence. You can return to it through the bookmark and finding controls as you continue investigating.

## 7. Save your workspace

Choose **File > Save Workspace** (`Ctrl+S` on Windows), select a location, and save your investigation as a `.tsw` workspace.

A workspace preserves the broader context of your work, including its open investigation, import-profile information, active filters, bookmarks, notes, finding state, and applicable presentation state. It is intended for returning to an investigation rather than merely exporting a list of records.

To verify this workflow, you can use **File > Open Workspace...** to reopen the saved file and check that your investigation state has been restored.

**What happens to the log evidence when you save?** Saving a workspace creates both the `.tsw` workspace file and a **companion folder containing a durable `.tsinv` snapshot for each open investigation**. The snapshots preserve each investigation's normalized evidence as it existed when you saved; the `.tsw` file preserves the broader workspace and investigation state. Keep the companion `<workspace-name>.sessions/` folder together with the `.tsw` file if you move, back up, or share the saved workspace; the `.tsw` file alone is not the complete saved package.

An investigation that was **SourceBacked** remains SourceBacked when you save: its original external log is still the authoritative source for normal reopening and reloading. If that source is missing when you reopen the workspace, TraceScope offers **Open Saved Snapshot** so you can recover the saved evidence without the original log. Choosing that option restores the affected investigation as **SnapshotBacked**. Saving therefore *does* preserve the investigation's log evidence; it does not automatically change that document's source-backing mode.

You can also save an individual investigation as a standalone `.tsinv` snapshot, separately from the full workspace. The detailed distinctions among source backing, saved snapshots, and recovery are covered in [Saving and Restoring](saving-and-restoring.md).

## What you've accomplished

You have imported a source file using a reusable profile, isolated relevant events, inspected source-specific details, recorded an investigative lead, and saved the state of your work.

From here, you can explore more advanced filtering and timeline navigation, compare a known-good session with a degraded session, follow an actively growing log, or export findings and offline HTML reports. See [Investigating Logs](investigating-logs.md), [Comparing Sessions](comparing-sessions.md), [Live Following](live-following.md), and [Reporting and Exporting](reporting-and-export.md) for those workflows.

If your own log does not import correctly, return to **Import Configuration** and inspect its format suggestion, profile mappings, validation messages, and source preview before attempting to investigate the resulting records.
