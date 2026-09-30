# Troubleshooting TraceScope

Use this reference when TraceScope behaves differently from what you expected. Start with the symptom rather than changing several settings at once.

A problem that appears to be missing evidence may instead be caused by an import/profile mismatch, an active filter, a difference between visible and analytical scope, an external source that changed after evidence was admitted, the investigation's current backing mode, or a comparison/report using an earlier captured scope.

**Before experimenting with Reload, source replacement, reconnection, or backing-mode changes, preserve work you need to keep.** If an external log may be truncated, replaced, rotated, moved, or deleted, make sure already admitted evidence is safely preserved before testing recovery operations. See [Saving and Restoring](saving-and-restoring.md#1-understand-what-tracescope-saves) for more information.

## Find your problem

| I'm having trouble with... | Start here |
| --- | --- |
| Installing, launching, scaling, or locating a control | [Launch and interface problems](#1-launch-and-interface-problems) |
| TraceScope does not recognize or correctly parse a source | [Format recognition and source-shape problems](#format-recognition-and-source-shape-problems) |
| Profile mappings, timestamps, severities, or custom fields are wrong | [Profile, mapping, and preview problems](#profile-mapping-and-preview-problems) |
| Event Table rows, filters, timeline, or analysis results seem wrong | [Filtering, navigation, and analysis problems](#3-filtering-navigation-and-analysis-problems) |
| Bookmarks, findings, or analyst notes behave unexpectedly | [Findings and annotations](#4-findings-and-annotations) |
| A live source is not updating or changed while being followed | [Live-follow and source-change problems](#5-live-follow-and-source-change-problems) |
| A workspace or investigation cannot be restored as expected | [Saving, reopening, and source recovery](#6-saving-reopening-and-source-recovery) |
| A session comparison does not show the population or result expected | [Session-comparison problems](#7-session-comparison-problems) |
| CSV/report output is missing data or renders unexpectedly | [Export and report problems](#8-export-and-report-problems) |
| The problem persists and needs to be reproduced | [Collect useful diagnostic information](#9-collect-useful-diagnostic-information) |

## Before you change anything

Use these checks to distinguish a presentation or configuration problem from actual evidence loss:

1. **Identify the active document.** Determine whether you are looking at an investigation or an immutable comparison.
2. **Check the current scope.** Inspect active filters and record counts before concluding that records are missing.
3. **Distinguish visible and analytical populations.** Review-only filters do not necessarily constrain every analysis view.
4. **Inspect a representative source record.** For import problems, compare Source Record Preview, Selected Raw Source, profile mappings, Validation, and import diagnostics.
5. **Check the external source.** If the investigation is SourceBacked or Hybrid, determine whether the source still exists and whether it has been truncated, replaced, rotated, or moved.
6. **Preserve evidence before recovery experiments.** Save the workspace and, when appropriate, deliberately preserve the investigation independently of the changing source.

Do not modify an important original log merely to reproduce a problem. Use a copy or disposable generated source instead. See [Saving and Restoring](saving-and-restoring.md#7-keep-workspaces-usable-and-safe) for related guidance.

## 1. Launch and interface problems

| Situation | What to check |
| --- | --- |
| The Windows application does not start from the release package | Extract the **entire** Windows archive before starting `TraceScope.exe`. Keep the executable with its packaged runtime files rather than moving only the executable. See [Getting Started: Windows](getting-started.md#windows) for more information. |
| The Linux AppImage does not launch | Confirm that the AppImage is executable, for example with `chmod +x`, and launch it from a terminal if you need to inspect a startup message. See [Getting Started: Linux](getting-started.md#linux) for more information. |
| A bundled sample or matching profile cannot be found | The source log and profile are separate files. Check the packaged samples or repository `samples/` and `samples/profiles/` directories. See [Find the example log and its profile](getting-started.md#2-find-the-example-log-and-its-profile) for more information. |
| Import Configuration does not match a screenshot or expected layout | The dialog responds to available width and **Interface Scale**. Resize it or adjust **View > Interface Scale**. See [Find your way around Import Configuration](importing-logs.md#2-find-your-way-around-import-configuration) for more information. |
| An investigation section or control appears to be missing | Check whether the section is collapsed or constrained by the current window height. Enlarge the window, expand the section, or reduce Interface Scale. Some controls also depend on fields supplied by the source. See [Open an investigation and get oriented](investigating-logs.md#1-open-an-investigation-and-get-oriented) for more information. |
| An export or investigation command is unavailable | Check which workspace document is active. Available commands depend on whether the active document is an investigation or comparison and on the available workspace state. See [Choose the right export for the task](reporting-and-export.md#1-choose-the-right-export-for-the-task) and [Create the comparison](comparing-sessions.md#3-create-the-comparison) for related information. |

## 2. Import and profile problems

Passing **Validation** means a profile is structurally acceptable. It does not prove that the profile correctly describes the selected source.

### Format recognition and source-shape problems

| Situation | What to check |
| --- | --- |
| No likely-format suggestion appears for a `.log` or `.txt` file | Those extensions do not identify one format. Inspect representative records and choose the matching importer manually when necessary. See [Supported Formats: Format suggestions](supported-formats.md#2-format-suggestions) for more information. |
| TraceScope suggests the wrong format | Treat the suggestion as advisory. Compare the actual source structure with the documented formats before importing. See [Supported formats at a glance](supported-formats.md#1-supported-formats-at-a-glance) and [Format reference](supported-formats.md#3-format-reference) for related information. |
| A JSON source imports no records or the wrong records | Distinguish **JSON Lines** from **Structured JSON** and check the Structured JSON Record path when applicable. See [JSON Lines](supported-formats.md#json-lines) and [Structured JSON](supported-formats.md#structured-json) for related information. |
| Nested Structured JSON imports the wrong objects | Verify that Record path starts at the outer object document and resolves to the intended object or record array. Field paths are then relative to each selected record. See [Import Profiles: Structured JSON](import-profiles.md#structured-json) for more information. |
| Structured XML imports no records or the wrong elements | An empty Record path uses the document root; repeated nested records require their actual dot-separated element path. See [Supported Formats: Structured XML](supported-formats.md#structured-xml) and [Import Profiles: Structured XML](import-profiles.md#structured-xml) for related information. |
| CSV or TSV columns are shifted or records are skipped | Check the importer, header row, delimiter, quoting, and physical record layout. Quoted fields spanning multiple physical lines are not supported. See [CSV](supported-formats.md#csv) and [TSV](supported-formats.md#tsv) for related information. |
| Regex Plain Text rejects lines even though the pattern matches part of them | The configured expression must match the **entire record**. See [Regex Plain Text](supported-formats.md#regex-plain-text) and [Import Profiles: Regex Plain Text](import-profiles.md#regex-plain-text) for related information. |
| A Windows event source is rejected | TraceScope imports XML-formatted Windows Event data, not native binary `.evtx` files. See [Windows Event XML](supported-formats.md#windows-event-xml) for more information. |
| Expected rotated files were not imported | Select the active source and review **Include rotations** / **Review...**. Confirm the filenames, ordering, and logical source family. See [Import a rotated source family](importing-logs.md#6-import-a-rotated-source-family) for more information. |
| A Structured JSON/XML source imports statically but cannot be followed live | Live Structured JSON needs a repeated record array; live Structured XML needs a nonempty Record path selecting repeated elements. See [Live Following compatibility](supported-formats.md#5-live-following-compatibility) for more information. |

### Profile, mapping, and preview problems

| Situation | What to check |
| --- | --- |
| Preview contains records, but canonical columns are blank or incorrect | Compare Selected Raw Source with the exact source paths. Check spelling, case, nesting, and whether the mapped value is textual. See [Profile settings and canonical fields](import-profiles.md#2-profile-settings-and-canonical-fields) and [Validation, preview, and reuse](import-profiles.md#8-validation-preview-and-reuse) for related information. |
| A numeric or boolean JSON field will not populate a canonical field | Canonical extraction is text-oriented. Keep nontextual values as custom data unless the importer supplies an appropriate normalized text value. See [Canonical values are text-oriented](import-profiles.md#canonical-values-are-text-oriented) for more information. |
| Severity is missing or incorrect | Check the canonical Severity path and configured aliases. Aliases interpret an extracted value; they do not create severity where the source has none. See [Severity aliases](import-profiles.md#severity-aliases) for more information. |
| Timestamps are unavailable or invalid | Check the Timestamp path and timestamp rules. When Timestamp is mapped, at least one applicable rule is required. See [Timestamp rules](import-profiles.md#timestamp-rules) for more information. |
| Timestamps appear shifted or have an unexpected date | Check timezone semantics and importer-specific inference in addition to parsing syntax. RFC 3164 requires year and timezone inference. See [Supported Formats: Syslog](supported-formats.md#syslog-rfc-5424-and-rfc-3164) and [Timestamp quality depends on the source](supported-formats.md#timestamp-quality-depends-on-the-source) for related information. |
| A source-specific value exists in Raw Source but not as a custom attribute | Add an explicit custom mapping or enable **Preserve unmapped source fields** when appropriate. See [Custom fields and unmapped data](import-profiles.md#5-custom-fields-and-unmapped-data) for more information. |
| A nested JSON/XML value cannot be mapped | Confirm that the path begins at the selected record. XML attributes use `@`; Windows Event named data uses normalized paths such as `EventData.NamedData.DeviceId`. See [Source paths and format-specific configuration](import-profiles.md#3-source-paths-and-format-specific-configuration) for more information. |
| An RFC 5424 structured-data parameter is missing | Map the parser's normalized path under the correct SD-ID. If unmapped preservation is disabled, explicitly map values you need. See [Import Profiles: Syslog](import-profiles.md#syslog) and [RFC 5424 structured data](import-profiles.md#rfc-5424-structured-data) for related information. |
| A regex profile loads but produces incorrect fields or too few records | Verify its named capture groups against several representative lines and confirm whole-record matching. See [Regex Plain Text](import-profiles.md#regex-plain-text) for more information. |
| A profile will not load | Confirm valid JSON, schema version `1`, required properties/types, successful profile validation, and a supported importer ID. See [Saved profile JSON reference](import-profiles.md#6-saved-profile-json-reference) and [Loading a profile](import-profiles.md#loading-a-profile) for related information. |
| A profile works for one file but not another with the same extension | Profiles describe source schemas rather than extensions. Compare field names, nesting, timestamps, and severity vocabulary. See [One importer can use many profiles](import-profiles.md#one-importer-can-use-many-profiles) for more information. |
| Large Structured JSON/XML does not preview automatically | Use **Refresh Preview**. Large structured documents deliberately defer automatic preview work. See [Large structured documents have an explicit preview action](importing-logs.md#large-structured-documents-have-an-explicit-preview-action) for more information. |
| Preview looks correct, but later records fail during full import | Preview is bounded to the first 50 processed records. Inspect full-import diagnostics and later source records. See [Verify the preview and validation](importing-logs.md#5-verify-the-preview-and-validation) and [Import large files](importing-logs.md#7-import-large-files) for related information. |
| **New From Source** missed an important field | Auto-detection uses bounded preview data rather than a complete schema scan. Add late-appearing important fields explicitly. See [Auto-detected custom fields](import-profiles.md#auto-detected-custom-fields) for more information. |

**Useful isolation test:** Import a bundled sample using its matching supplied profile. If that succeeds, return to the original source and compare its actual structure and values rather than changing several settings simultaneously. See [Supported Formats: Bundled examples](supported-formats.md#4-bundled-examples) for sample/profile pairs.

## 3. Filtering, navigation, and analysis problems

| Situation | What to check |
| --- | --- |
| The Event Table becomes empty after filtering | Reset filters and add the criteria back one at a time. A record must satisfy the complete active filter set. See [Understand what filtering changes](investigating-logs.md#2-understand-what-filtering-changes) for more information. |
| A record disappears under a custom-field filter | Inspect its exact normalized field name and value in Event Details. Not every record necessarily contains that custom field. See [Filter using source-specific custom attributes](investigating-logs.md#5-filter-using-source-specific-custom-attributes) for more information. |
| A time-range filter excludes a record you expected | Check the UTC boundaries, imported timestamp validity, and source timezone conventions. See [Focus on a time window](investigating-logs.md#7-focus-on-a-time-window) for more information. |
| Timeline, Issue Summary, or Analytics counts differ from the Event Table | Distinguish the complete, visible, and analysis populations. Analytical views ignore review-only Bookmarks/Finding-status restrictions while retaining source-data filters. See [Understand what filtering changes](investigating-logs.md#2-understand-what-filtering-changes) for more information. |
| An analytical section is missing or unavailable | Check whether the imported source contains the required canonical data. See [Open an investigation and get oriented](investigating-logs.md#1-open-an-investigation-and-get-oriented) and [Import Profiles](import-profiles.md#2-profile-settings-and-canonical-fields) for related information. |
| Next/Previous navigation seems to skip records | Navigation follows the current visible records and sort order. Remove restrictive filters and verify ordering. See [Follow records using navigation and sorting](investigating-logs.md#6-follow-records-using-navigation-and-sorting) for more information. |
| The timeline is hard to read across a large span | Use Auto or a broader Bucket size and horizontal scrolling where appropriate. See [Explore the event timeline](investigating-logs.md#8-explore-the-event-timeline) for more information. |
| Detected Bursts changes after filtering or drilling into a result | Burst detection reruns against the filtered analysis population; Auto timing can also recalculate for that new population. See [Bursts: concentrated elevated activity](investigating-logs.md#bursts-concentrated-elevated-activity) for more information. |
| No burst is detected even though an incident occurred | Burst detection is one analytical lens, not an incident classifier. Check timestamps, severity, current filters, and other investigation views. See [Bursts: concentrated elevated activity](investigating-logs.md#bursts-concentrated-elevated-activity) for more information. |

## 4. Findings and annotations

| Situation | What to check |
| --- | --- |
| A bookmarked or noted event is missing from Findings | Bookmarks and notes alone do not classify a finding. Assign Open, Resolved, or Dismissed. See [Create an Open finding](findings.md#3-create-an-open-finding) for more information. |
| A finding is listed but hidden from the Event Table | Findings spans classified records independently of current Event Table filters. Preview or reveal the finding and adjust filters if necessary. See [Navigate between Findings and the Event Table](findings.md#5-navigate-between-findings-and-the-event-table) for more information. |
| A finding still says `(No analyst note)` | Add and save a note for the intended record. See [Add and revise an analyst note](findings.md#4-add-and-revise-an-analyst-note) for more information. |
| A findings export contains fewer records than expected | Check whether you selected All Findings, Filtered Findings, or Bookmarked Findings. See [Export the findings you need](findings.md#7-export-the-findings-you-need) for more information. |

## 5. Live-follow and source-change problems

**Following** controls ingestion. **Follow Newest** controls table positioning. One can operate without the other.

| Situation | What to check |
| --- | --- |
| Live-follow controls are unavailable | Confirm that the investigation has a connected external source and a supported live-capable profile. See [Understand what Live Following does](live-following.md#1-understand-what-live-following-does) for more information. |
| Following will not start for Structured JSON/XML | Verify the repeated-record structure required for live ingestion. See [Live Following compatibility](supported-formats.md#5-live-following-compatibility) for more information. |
| Status remains Stopped or becomes Error | Confirm that the configured active source still exists and is accessible. If it moved, use the explicit relocation/reconnection workflow. See [Understand file changes during live following](live-following.md#6-understand-file-changes-during-live-following) and [Reload, relocate, and resume work deliberately](saving-and-restoring.md#6-reload-relocate-and-resume-work-deliberately) for related information. |
| Status says Following, but no records appear | Confirm the producer is writing to the same path, has emitted a complete supported record, and current filters permit the record to appear. See [Investigate while new records arrive](live-following.md#5-investigate-while-new-records-arrive) for more information. |
| A partial record does not appear immediately | This is expected. TraceScope waits for a complete line, JSON object, or XML element before admitting it as evidence. See [Incomplete writes](live-following.md#incomplete-writes) for more information. |
| New records arrive, but the table does not scroll | Check **Follow Newest**, sort order, and active filters. See [Use the live controls deliberately](live-following.md#4-use-the-live-controls-deliberately) for more information. |
| Resume does not recover every record written while paused | Check whether the producer truncated, replaced, or rotated the file while paused. TraceScope cannot recover unread bytes already removed by the producer. See [Understand file changes during live following](live-following.md#6-understand-file-changes-during-live-following) for more information. |
| Previously admitted live records disappear after Reload/reopen | Check backing mode and current source authority. Preserve evidence independently when it must outlive a changing source. See [Preserve evidence before it disappears](live-following.md#7-preserve-evidence-before-it-disappears) and [Understand the three backing modes](saving-and-restoring.md#5-understand-the-three-backing-modes) for related information. |
| Truncation/replacement and rotation seem to be treated differently | Same-path truncation/replacement creates source generations; rotated filenames remain distinct physical sources in one logical source family. See [Understand file changes during live following](live-following.md#6-understand-file-changes-during-live-following) and [Import a rotated source family](importing-logs.md#6-import-a-rotated-source-family) for related information. |
| The Live-Log Generator writes to the wrong place or completes too quickly | Check Scenario, Output, Format, speed, and looping settings. Use a disposable output path. See [Live-Log Generator](live-log-generator.md) for more information. |

## 6. Saving, reopening, and source recovery

| Situation | What to check |
| --- | --- |
| A copied `.tsw` opens incompletely | Keep the `.tsw` file and matching `.sessions/` directory together. SourceBacked investigations may also require their external source. See [The two parts of a saved workspace](saving-and-restoring.md#the-two-parts-of-a-saved-workspace) for more information. |
| Workspace recovery reports a missing external source | Use Locate File when the same source moved; use Open Saved Snapshot when preserved workspace evidence is available and appropriate. See [Recover when the original source is missing](saving-and-restoring.md#3-optional-walkthrough-recover-when-the-original-source-is-missing) for more information. |
| Open Saved Snapshot is unavailable | Verify that the workspace's `.sessions/` directory and managed snapshots were preserved. See [The two parts of a saved workspace](saving-and-restoring.md#the-two-parts-of-a-saved-workspace) for more information. |
| Reopening/reloading contains fewer records than an earlier live investigation | Check backing mode and whether the current external source still contains the earlier evidence. See [Understand the three backing modes](saving-and-restoring.md#5-understand-the-three-backing-modes) for more information. |
| `.tsinv` restores records but not bookmarks, notes, filters, or layout | A standalone investigation snapshot stores evidence, not the complete workspace state. See [Understand what TraceScope saves](saving-and-restoring.md#1-understand-what-tracescope-saves) and [Save or open an individual investigation snapshot](saving-and-restoring.md#4-save-or-open-an-individual-investigation-snapshot) for related information. |
| SnapshotBacked evidence cannot Live Follow | It has no active connected external source. Reconnect deliberately when valid continuity information is available. See [Reconnect preserved evidence to an external source](saving-and-restoring.md#reconnect-preserved-evidence-to-an-external-source) for more information. |
| Reconnection or relocation rejects a candidate source | Verify that the candidate is genuinely the recorded source. Relocation is not arbitrary file substitution. See [Reload, relocate, and resume work deliberately](saving-and-restoring.md#6-reload-relocate-and-resume-work-deliberately) for more information. |
| You want Hybrid evidence to return to ordinary source authority | Use the explicit return-to-source workflow and understand what preserved baseline authority is being discarded. See [Deliberately return a Hybrid investigation to source authority](saving-and-restoring.md#deliberately-return-a-hybrid-investigation-to-source-authority) for more information. |
| Reload might destroy access to evidence no longer present in the source | Preserve the currently admitted evidence before reloading. Reload is not an evidence-recovery operation. See [Preserve an open investigation as snapshot-only](saving-and-restoring.md#preserve-an-open-investigation-as-snapshot-only) for more information. |

## 7. Session-comparison problems

| Situation | What to check |
| --- | --- |
| Compare Sessions is unavailable | At least two distinct investigation sessions must be open in the workspace. See [Open the baseline and comparison investigations](comparing-sessions.md#2-open-the-baseline-and-comparison-investigations) for more information. |
| A displayed delta has the opposite sign from what you expected | Verify Baseline and Comparison orientation. Every delta is **Comparison − Baseline**. See [Choose sessions that answer a useful question](comparing-sessions.md#1-choose-sessions-that-answer-a-useful-question) for more information. |
| Comparison counts differ from current Event Table counts | Check the captured scope. Comparisons use complete admitted populations unless an active Time Range was explicitly captured; other filters do not define comparison scope. See [Create the comparison](comparing-sessions.md#3-create-the-comparison) and [Optionally compare selected time ranges](comparing-sessions.md#optionally-compare-selected-time-ranges) for related information. |
| An active Time Range did not affect the comparison | The corresponding **Use this session's active time range** checkbox must be enabled at comparison creation. See [Optionally compare selected time ranges](comparing-sessions.md#optionally-compare-selected-time-ranges) for more information. |
| A time-scope checkbox is disabled | Configure at least one usable time boundary that selects timestamped records. See [Optionally compare selected time ranges](comparing-sessions.md#optionally-compare-selected-time-ranges) for more information. |
| A time-scoped comparison includes records hidden by other filters | This is expected. Captured time scope applies only its time boundaries to the complete admitted population. See [Optionally compare selected time ranges](comparing-sessions.md#optionally-compare-selected-time-ranges) for more information. |
| A field is marked Unavailable | Both captured populations must contain usable data for that dimension. Unavailable is not equivalent to zero difference. See [Read the comparison from top to bottom](comparing-sessions.md#4-read-the-comparison-from-top-to-bottom) for more information. |
| A custom field is absent from the comparison | Confirm that both investigations use the same exact custom-field name and that the comparison rules apply to its values. See [Understand the custom-field limitations](comparing-sessions.md#understand-the-custom-field-limitations) for more information. |
| Burst Comparison is missing or unavailable | It may have been disabled, or the selected populations/settings may not support the requested analysis. See [Interpret Burst Comparison](comparing-sessions.md#5-interpret-burst-comparison) for more information. |
| Window-Average Records / Minute does not appear | At least one side needs both captured boundaries with a positive interval. Complete, start-only, and end-only scopes do not have a bounded-window average. See [Use Session Context](comparing-sessions.md#6-use-session-context-to-keep-the-differences-in-perspective) for more information. |
| You changed a source session, but the existing comparison did not change | Comparisons are immutable captures. Create another comparison for new evidence or new scope. See [Know when you need a new comparison](comparing-sessions.md#know-when-you-need-a-new-comparison) for more information. |
| A saved comparison remains available without its source investigation | The captured comparison analysis is independent of the original session objects. Event-level source review still requires the investigation evidence itself. See [Save or share the comparison](comparing-sessions.md#8-save-or-share-the-comparison) for more information. |

## 8. Export and report problems

| Situation | What to check |
| --- | --- |
| Export Filtered Results is unavailable or exports fewer records than expected | Activate the intended investigation and verify its filters and current matching population. See [Export the currently filtered records to CSV](reporting-and-export.md#3-export-the-currently-filtered-records-to-csv) for more information. |
| A finding is absent from a findings CSV | Check whether you selected All Findings, Filtered Findings, or Bookmarked Findings. See [Export classified findings to CSV](reporting-and-export.md#4-export-classified-findings-to-csv) for more information. |
| HTML report Export is disabled | Enter a nonempty title and select at least one eligible investigation or comparison. See [Configure and export the HTML report](reporting-and-export.md#6-configure-and-export-the-html-report) for more information. |
| Expected findings or evidence are absent from the HTML report | Verify the selected source investigations, investigator state, and supporting-evidence option. See [Configure and export the HTML report](reporting-and-export.md#6-configure-and-export-the-html-report) and [Read and verify the HTML report](reporting-and-export.md#7-read-and-verify-the-html-report) for related information. |
| Technical profile details are absent | Check **Include technical import/profile appendix**. See [Configure and export the HTML report](reporting-and-export.md#6-configure-and-export-the-html-report) for more information. |
| A comparison and its source section show different scopes in the same report | This can be correct. The comparison retains its earlier complete/time-scoped capture, while the investigation section captures its later report-time state. See [Review the included comparison](reporting-and-export.md#review-the-included-comparison) for more information. |
| The report looks as though details are missing | Many detail sections start collapsed. Use their controls or **Expand All / Collapse All**. See [Read and verify the HTML report](reporting-and-export.md#7-read-and-verify-the-html-report) for more information. |
| Printing or Save as PDF appears to omit collapsed content | Printing temporarily expands all report detail automatically, then restores its previous state. Inspect browser Print Preview if anything remains missing or awkward. See [Read and verify the HTML report](reporting-and-export.md#7-read-and-verify-the-html-report) for more information. |
| A wide table or evidence record breaks awkwardly across PDF pages | Review the browser's Print Preview. Exact page breaks depend on browser, paper settings, and content even though the report supplies print-oriented layouts. See [Read and verify the HTML report](reporting-and-export.md#7-read-and-verify-the-html-report) for more information. |
| A report does not update after the investigation changes | The HTML report is a frozen point-in-time handoff. Generate a new report for newer state. See [Read and verify the HTML report](reporting-and-export.md#7-read-and-verify-the-html-report) for more information. |
| A report or CSV contains information that should not be shared | Review raw evidence, custom fields, analyst text, source provenance, included documents, and optional detail before distributing it. See [What each export captures and what to check before sharing](reporting-and-export.md#8-what-each-export-captures-and-what-to-check-before-sharing) for more information. |

## 9. Collect useful diagnostic information

If the checks above do not resolve the problem, reduce it to the smallest reproducible case before changing more settings.

Useful diagnostic context includes:

- TraceScope version and operating system
- exact reproduction steps
- selected importer and profile
- representative source records
- Validation and import diagnostics
- current filters and population counts
- external-source lifecycle changes
- investigation backing mode
- comparison orientation and captured scope
- report/export configuration when applicable

For an import problem, start with one copied source file, one profile, and the smallest record set that still demonstrates the behavior.

For Live Following, use a disposable source or the deterministic [Live-Log Generator](live-log-generator.md) rather than modifying a production log.

For recovery testing, preserve an untouched source copy and workspace package before trying relocation, reload, or backing-mode transitions. See [Keep workspaces usable and safe](saving-and-restoring.md#7-keep-workspaces-usable-and-safe) for related guidance.

### Protect sensitive information

Logs and TraceScope artifacts can contain personal information, credentials or tokens, internal hostnames, local source paths, domain identifiers, analyst notes, and raw application messages.

Remove or replace sensitive values before sharing reproduction material while preserving the source structure and event ordering needed to demonstrate the problem.

Do not share a production workspace merely because it contains a convenient reproduction.

## Related documentation

- [Getting Started](getting-started.md) — installation and a reproducible first investigation.
- [Supported Formats](supported-formats.md) — accepted source layouts and Live Following compatibility.
- [Import Profiles](import-profiles.md) — profile schema, mappings, paths, severity aliases, and timestamp rules.
- [Importing Logs](importing-logs.md) — source selection, preview, configuration, rotations, and large-file imports.
- [Investigating Logs](investigating-logs.md) — filters, navigation, timeline, issue analysis, and bursts.
- [Findings](findings.md) — bookmarks, classifications, notes, and findings exports.
- [Live Following](live-following.md) — live ingestion and external source lifecycle.
- [Saving and Restoring](saving-and-restoring.md) — workspace persistence, evidence backing, recovery, and reconnection.
- [Comparing Sessions](comparing-sessions.md) — immutable complete/time-scoped comparisons and shared burst analysis.
- [Reporting and Exporting](reporting-and-export.md) — record exports, findings exports, HTML reports, and browser/PDF handoff.
