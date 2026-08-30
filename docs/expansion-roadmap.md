# TraceScope Expansion Roadmap

## Overview

TraceScope began as a focused Qt/C++ desktop application for inspecting structured JSON Lines telemetry logs. The original prototype provides a complete investigation workflow for its supported schema, including loading, filtering, searching, event inspection, grouped issue analysis, timeline visualization, and CSV export.

The next stage of development expands TraceScope into a configurable native desktop workbench that developers can use to import, normalize, inspect, filter, compare, annotate, and report on application and system logs.

The intended product position is:

> TraceScope supports multiple built-in log formats and reusable import profiles that map source fields into a common investigation model.

TraceScope is not intended to claim automatic understanding of every arbitrary log format. Import behavior will remain explicit, configurable, testable, and reproducible.

## Product Principles

The expansion will follow these principles:

* Remain an offline native desktop application built with C++ and Qt.
* Support multiple built-in formats through a common importer architecture.
* Use reusable, versioned import profiles for source-specific field mappings.
* Treat canonical log fields as optional whenever practical.
* Preserve unknown source fields as custom attributes.
* Preserve raw source records and source-location metadata.
* Limit missing-field effects to features that depend on those fields.
* Keep analysis deterministic and explainable.
* Keep prospective-user and employer-facing claims conservative and directly supported by the implementation.
* Avoid paid infrastructure and unnecessary external services.
* Complete each phase with passing tests and downloadable Windows and Linux packages.

## Product Positioning, Adoption, and Scope Discipline

TraceScope remains focused on file-based telemetry and diagnostic investigation for applications, services, simulated devices, sensors, QA runs, field-support packages, and engineering test systems. Broader adoption is desirable, but new capabilities should strengthen this primary use case rather than reposition the application around unrelated markets.

TraceScope occupies the space between raw-file and text-log viewers and centralized observability platforms. Its value is a structured, repeatable, offline investigation workflow without requiring log-shipping infrastructure, a hosted backend, user accounts, or an indexing service.

The project should be evaluated against the tools its intended users may already reach for, including text editors and command-line utilities, fast raw-log viewers, structured desktop log-analysis tools, and centralized observability systems. TraceScope does not need to outperform every category at its specialty. Instead, it should reduce the friction of moving from unfamiliar files to a useful structured investigation while preserving source-specific information and keeping the workflow local and reproducible.

Format breadth is an enabling capability rather than the primary product differentiator. TraceScope should support a practical set of representative structured and operational log families plus reusable configurable import profiles. Additional formats should be added only when they materially reduce friction for the intended audience or reuse existing importer architecture at low incremental cost.

Once representative ingestion coverage is established, development priority shifts from format count to investigation depth. Large-file responsiveness, multi-session investigation, advanced filtering and navigation, findings, deterministic analytics, session comparison, persistence, reporting, and live following are expected to provide more target-user value than indefinitely expanding the built-in format list.

Roadmap evolution should normally substitute, reinterpret, or reprioritize planned work rather than increase the overall project scope. New work should earn its place through demonstrated target-user value, architectural leverage, or replacement of lower-value planned work. The expansion should remain finishable on approximately the scale originally intended.

Important target workflows include:

* moving quickly from unfamiliar source files to structured investigation
* reusing parsing and normalization configuration instead of rebuilding one-off scripts
* preserving source-specific fields alongside common investigation fields
* investigating related logs from multiple applications or system components
* comparing failed, degraded, and known-good runs
* navigating efficiently around warnings, errors, bursts, and surrounding context
* recording findings and producing useful investigation output

## Original Prototype Baseline

The original prototype uses:

* C++17
* Qt 6
* Qt Widgets
* Qt Charts
* CMake
* Qt Test
* MinGW 64-bit on Windows

Original-prototype capabilities include:

* JSON Lines log loading
* structured telemetry parsing
* telemetry event table display
* severity filtering
* subsystem filtering
* full-field text search
* selected-event detail inspection
* session summary counts
* grouped warning and error analysis
* event-count timeline visualization
* filtered CSV export
* included sample log files
* automated tests for parsing, filtering, grouped analysis, timeline analysis, and CSV export

The original prototype began with a fixed telemetry-event presentation, a `QTableWidget` event display, and a `MainWindow` that owned substantial application state and UI orchestration.

Beginning with `v0.2.0`, the ingestion layer introduced a flexible investigation-record and import domain. `v0.3.0` moved JSON Lines behavior behind the common importer architecture, and `v0.4.0` migrated the primary event display to Qt model/view architecture so flexible investigation records and dynamic custom attributes now reach the desktop UI directly.

These original constraints form the starting point for the expansion rather than defects in the completed prototype.

## Release Discipline

Each completed development phase should produce:

* passing Windows and Linux CI builds
* passing automated tests on both platforms
* Release-mode native executables
* a portable Windows x64 ZIP produced with `windeployqt`
* a Linux x86_64 AppImage produced with `linuxdeploy`
* a platform-neutral samples ZIP containing demonstration logs and reusable profiles
* startup smoke tests for both packaged applications
* a version tag
* a GitHub prerelease or release
* attached release notes
* attached Windows, Linux, and samples assets
* clean smoke tests of the downloaded release packages

GitHub Actions artifacts are used for build verification. Approved packages are then attached to GitHub Releases as permanent public downloads for prospective users and as directly verifiable project artifacts for employers and reviewers.

Completed release milestones:

| Version | Milestone | Status |
| --- | --- | --- |
| `v0.1.0` | Original Prototype | prerelease |
| `v0.2.0` | Flexible Record and Import Domain | prerelease |
| `v0.3.0` | Importer Abstraction and Configurable JSON Lines | prerelease |
| `v0.4.0` | Qt Model/View Architecture | prerelease |
| `v0.4.1` | Dynamic Attribute CSV Export | prerelease |
| `v0.5.0` | Import Profiles and Preview Logic | prerelease |
| `v0.6.0` | Import Configuration Interface | prerelease |
| `v0.7.0` | Additional Built-In Formats | prerelease |
| `v0.8.0` | Responsive Large-File Import | prerelease |
| `v0.9.0` | Multi-Session Investigation Workspace | prerelease |
| `v0.10.0` | Advanced Filtering and Navigation | prerelease |
| `v0.11.0` | Bookmarks, Notes, and Findings | prerelease |
| `v0.12.0` | Analytics and Burst Detection | prerelease |
| `v0.13.0` | Session Comparison | prerelease |
| `v0.14.0` | Workspace and Profile Persistence | prerelease |

Release assets follow a consistent naming convention:

```text
TraceScope-v<version>-windows-x64.zip
TraceScope-v<version>-linux-x86_64.AppImage
TraceScope-v<version>-samples.zip
```

## Canonical Investigation Record

`v0.2.0` introduced a common flexible investigation record with optional standard fields:

* timestamp
* severity
* subsystem
* event code
* entity ID
* message

The record also preserves:

* custom source attributes
* the raw source record
* source file information
* source line or record number
* stable record identity

Import processing separately returns structured diagnostics that can identify malformed records or canonical values that could not be mapped.

Missing canonical fields do not automatically invalidate an otherwise useful source record. Features that require a timestamp, severity, event code, or another specific field may be unavailable when that field is absent, while unrelated inspection and future search features can continue to use the preserved record content.

The `v0.2.0` implementation includes typed severity parsing, ISO timestamp parsing, dynamic custom attributes, raw-source preservation, source metadata, deterministic stable identities, import results, and import diagnostics.

## Import Architecture

Implemented foundations through `v0.14.0`:

* flexible investigation records with optional canonical fields and preserved source data
* structured import results and diagnostics
* typed severity and timestamp parsing
* stable record identity and source metadata
* `ILogImporter` abstraction and internal importer registry
* dedicated JSON Lines importer with configurable dot-delimited paths
* nested canonical mappings and preservation of source attributes
* versioned import profiles with canonical/custom mappings, severity aliases, timestamp rules, validation, and JSON serialization
* profile-aware JSON Lines importing and bounded preview services
* desktop source selection, drag-and-drop, format suggestion, profile editing, mapping-aware preview, validation, and profile save/load
* built-in import support for CSV, TSV, structured JSON, key-value/logfmt records, Syslog RFC 3164 and RFC 5424, IIS W3C, and structured XML
* reusable built-in profiles for Apache Common and Apache/Nginx Combined access logs
* Windows Event XML detection and presets built on the structured XML importer
* Qt model/view presentation of flexible records and dynamic custom attributes
* investigation-record CSV export with user-facing canonical headers and configured custom-field names
* representative sample logs and reusable profiles across the supported source families
* streamed file processing for line-oriented and XML importers where the source format permits incremental parsing
* import parsing outside the UI thread with progress and cooperative cancellation
* large structured-document preview safeguards with cancellable background preview generation
* scalable event-count timeline rendering with automatic and manual resolutions, windowed fine-resolution navigation, and bounded on-screen bucket materialization
* multi-session workspace ownership with independent per-session investigation controllers and retained import context
* session switching, closing, and in-place reload using stable session identities
* generic workspace-document hosting that supports session and comparison documents in the main window or detachable/re-dockable workspace windows
* tab reordering, tab tear-out, re-docking, movement between detached windows, multi-document detached windows, and deterministic document closing
* immutable structured comparison snapshots built from complete imported-session records rather than the sessions' current filtered views
* directional Baseline → Comparison analysis with explicit Comparison − Baseline deltas and capability-aware unavailable states
* comparison of event-code appearance/disappearance/change, severity counts, elevated subsystem/entity activity, conservative shared custom fields, session context, and optionally shared-settings burst analysis
* dedicated comparison documents with impact-first presentation, compact source orientation, and no causal or root-cause claims
* narrow-workspace hardening for split-screen and portrait use, including summary elision with full tooltips, responsive filter/control layouts, shrinkable review/detail surfaces, and width-aware fine-resolution timeline windows
* persistent recent-file and recent-profile history backed by local application settings
* advanced canonical and source-specific filtering with multi-severity, time-range, event-code, entity, and dynamic custom-field criteria
* persistent named filter presets backed by local application settings
* adjacent-event and warning/error-class navigation over the current sorted and filtered investigation
* grouped issue-summary and timeline-bucket drill-down that narrows the active filter state without discarding unrelated criteria
* session-local investigation state keyed by stable record identities for bookmarks, analyst notes, and finding status
* bookmark-only and finding-status filtering integrated with the existing filter model and reusable presets
* a dedicated findings review panel with finding counts, source-record context, timestamps, and multiline analyst notes
* direct navigation from findings back to source records with targeted filter relaxation when the record is hidden
* deterministic event-code, entity, subsystem-frequency, subsystem-trend, and severity-trend analysis that degrades independently when canonical fields are absent
* reusable deterministic analysis time-bucket logic shared by timeline and trend analysis
* severity/subsystem timeline breakdown selection with session-local presentation state and UI-only Top-N subsystem display
* adaptive timestamp-cadence analysis with transparent statistics, sparse-data fallback behavior, and human-readable Auto burst timing
* deterministic configurable WARN/ERROR/CRITICAL burst detection with inclusive windows, merged episodes, trigger explanations, and contributing record/subsystem/event-code/entity summaries
* Auto and Manual burst timing modes with analyst-controlled thresholds
* burst drill-down that narrows time/severity context without discarding unrelated active filters
* realistic fictional investigation samples spanning business-application incidents, engineering QA runs, and known-good/degraded field-support captures
* local workspace save/open using versioned human-readable JSON, with session source paths and complete import-profile context preserved for reproducible re-import
* persistence of bookmarks, analyst notes, finding status, active filters, and investigation/comparison presentation state across application restarts
* immutable comparison-snapshot persistence that remains independent of later source-session filtering, reload, closure, or missing-source recovery
* restoration of workspace document order, active documents, detached document groups, detached-window geometry/maximized state, and primary-window geometry/state
* staged workspace restoration that leaves the currently open workspace untouched until recoverable source imports complete successfully
* missing-source recovery with locate, skip-session, and cancel-open choices so unavailable source files do not force unrelated persisted investigation state to be discarded
* persistent recent-workspace history alongside recent files and profiles, including stale-path cleanup and reopening through the normal workspace-loading path
* workspace serialization round-trip and compatibility coverage for versioned schemas and later-added optional presentation/layout fields

Reusable import profiles are versioned, human-readable JSON so mappings can be reused, shared, and committed alongside the applications that produce the logs. The desktop workflow now exposes those profile and preview services directly while keeping import behavior explicit and reproducible.

Importers are registered internally. An external binary plugin ecosystem is not part of the initial expansion.

Phase 6 established the representative ingestion baseline in `v0.7.0`. Phase 7 then shifted attention from format breadth to responsiveness and investigation scalability. The `v0.8.0` release keeps supported imports responsive through background parsing, progress/cancellation behavior, large-structured-document preview safeguards, and timeline scaling work. Phase 8 completed the transition from a single replaceable investigation to a multi-session workspace in `v0.9.0`. Phase 9 completed the advanced filtering, preset, navigation, and drill-down workflow in `v0.10.0`. Phase 10 added bookmarks, notes, finding status, findings review, and source navigation in `v0.11.0`. Phase 11 completed deterministic analytics, subsystem/severity trend presentation, adaptive cadence analysis, and configurable burst detection in `v0.12.0`. Phase 12 completed structured session comparison, generalized detachable workspace documents, and the responsive hardening needed to make those documents practical in split-screen and portrait layouts in `v0.13.0`. Phase 13 completed local workspace persistence in `v0.14.0`, allowing source/profile context, investigation state, immutable comparison snapshots, document organization, and detached-window layouts to survive application restarts with explicit missing-source recovery. Phase 14 is now active and completes the current investigation workflow with reporting and export before Phase 15 extends the same file-oriented model to live-followed sessions and future live comparisons. Final UI/documentation hardening remains reserved for Phase 16 rather than returning to open-ended format expansion.

## Development Phases

### Phase 0 — CI, Packaging, and Prototype Release Baseline

**Status: Completed in `v0.1.0`.**

Established repeatable cross-platform verification and a downloadable baseline for the completed original prototype.

Completed deliverables:

* GitHub Actions workflow
* Windows and Linux builds
* CTest execution on both platforms
* pinned Qt version
* minimal workflow permissions
* Release-mode Windows and Linux builds
* Windows deployment with `windeployqt`
* Linux AppImage deployment with `linuxdeploy`
* bundled documentation and sample logs
* portable Windows x64 ZIP
* portable Linux x86_64 AppImage
* platform-neutral sample-log ZIP
* packaged-application startup tests
* CI artifact uploads
* downloaded-package smoke tests
* `v0.1.0` prerelease
* README build and release instructions

### Phase 1 — Flexible Record and Import Domain

**Status: Completed in `v0.2.0`.**

Replace rigid event assumptions with a flexible investigation domain.

Completed deliverables:

* optional typed canonical fields
* typed severity representation
* timestamp parsing
* dynamic custom attributes
* raw source preservation
* source file and record metadata
* stable record identity
* import results
* import diagnostics
* compatibility with the existing prototype UI and sample workflow
* expanded automated test coverage for the new domain and parser behavior
* Windows and Linux CI verification
* `v0.2.0` prerelease and downloadable package verification

### Phase 2 — Importer Abstraction and Configurable JSON Lines

**Status: Completed in `v0.3.0`.**

Move existing JSON Lines behavior behind the common importer architecture.

Completed deliverables:

* `ILogImporter`
* importer registry
* dedicated JSON Lines importer
* configurable dot-delimited JSON field paths
* nested object-path mapping
* default mappings compatible with existing sample files
* source-attribute preservation for nested mappings
* legacy parser compatibility adapter for the existing UI
* comprehensive importer tests
* local full-suite and UI regression verification
* `v0.3.0` prerelease and downloadable package verification

### Phase 3 — Qt Model/View Architecture

**Status: Completed in `v0.4.0`.**

Replace the fixed table implementation and reduce UI orchestration responsibilities in `MainWindow`.

Completed deliverables:

* `InvestigationTableModel` built on `QAbstractTableModel`
* `InvestigationFilterProxyModel` built on `QSortFilterProxyModel`
* sortable canonical and custom-attribute columns
* dynamically generated columns for preserved custom source attributes
* typed timestamp and severity sort values
* severity and subsystem filtering through the proxy model
* case-insensitive text search across canonical fields and custom attributes
* correct proxy-to-source record mapping after filtering and sorting
* selected-record details backed directly by `InvestigationRecord`
* selected-record display of dynamic custom attributes
* `InvestigationController` decomposition for record, filter, subsystem, visibility, and source/proxy coordination
* migration of the primary event display from `QTableWidget` to `QTableView`
* preservation of session summaries, grouped warning/error analysis, timeline visualization, selected-event inspection, and filtered CSV export
* compatibility adapter for remaining telemetry-oriented analysis components
* `dynamic-attributes-session.jsonl` demonstration sample with heterogeneous custom fields and multi-minute timeline activity
* automated coverage for the table model, filter proxy, and investigation controller
* local full-suite and UI regression verification
* Windows and Linux CI verification
* `v0.4.0` prerelease and downloadable package verification

Post-release patch completed in `v0.4.1`:

* investigation-record CSV export without conversion back to the legacy telemetry model
* preservation of dynamic custom attributes in filtered CSV output
* deterministic custom-column ordering across the visible record set
* blank cells for custom attributes absent from individual records
* retained CSV escaping for canonical and custom values
* compact JSON serialization for structured custom values
* expanded CSV exporter regression coverage

### Phase 4 — Import Profiles and Preview Logic

**Status: Completed in `v0.5.0`.**

Defined the reusable configuration and preview layer used to map source logs into the canonical investigation model.

Completed deliverables:

* versioned import-profile schema
* canonical field mappings
* explicit custom-field mappings
* source-specific severity aliases
* ordered ISO 8601 and Qt-format timestamp rules
* configurable preservation of unmapped source fields
* deterministic profile validation
* human-readable JSON profile serialization and deserialization
* profile serialization round-trip tests
* profile-aware JSON Lines importing
* bounded import-preview services with records, counts, diagnostics, source metadata, and truncation state
* automated coverage for the profile domain, validator, serializer, importer integration, and preview service
* local full-suite and UI regression verification
* Windows and Linux CI verification

Additional maintenance completed during the phase:

* fixed selected-event detail refresh after filtering by responding to actual row selection changes rather than only current-row changes

### Phase 5 — Import Configuration Interface

**Status: Completed in `v0.6.0`.**

Exposed the profile and preview foundations through the desktop workflow.

Completed deliverables:

* source-file selection and drag-and-drop
* likely-format suggestions
* bounded source-record preview before import
* mapping-aware canonical and custom-field preview columns
* selected-record raw-source inspection
* automatic custom-field detection for new source-derived profiles
* canonical and custom-field mapping controls
* severity-alias and timestamp-rule configuration
* unmapped-field preservation control
* immediate validation feedback with debounced preview refresh
* preservation of the last valid preview during temporarily invalid edits
* explicit new-profile-from-source workflow
* profile saving and nondestructive validated profile loading
* intentional profile retention when switching sources for compatibility checks
* resizable preview columns
* reusable sample import profiles, including a curated profile for `dynamic-attributes-session.jsonl`
* coordinated release screenshots demonstrating that same mapped session across configuration, investigation, filtering, and CSV export
* local full-suite and UI regression verification
* Windows and Linux CI verification

Additional maintenance completed during the phase:

* normalized CSV canonical headers to user-facing display names while retaining configured custom-field names

### Phase 6 — Additional Built-In Formats

**Status: Completed in `v0.7.0`.**

Established broad, representative source coverage while keeping import behavior explicit, profile-driven, testable, and reproducible.

Completed deliverables:

* CSV and TSV import through a shared delimited-text importer
* structured JSON arrays and nested JSON documents with configurable record paths
* regex-configurable plain-text logs using named capture groups
* key-value and logfmt-style record import
* Syslog RFC 3164 and RFC 5424 parsing
* Apache Common access logs through a reusable built-in regex profile
* Apache/Nginx Combined access logs through a reusable built-in regex profile
* IIS W3C Extended Log parsing with field-header handling and a reusable preset
* structured XML import with nested elements, attributes, repeated elements, record paths, direct-text preservation, and raw-source preservation
* named XML `<Data Name="...">` handling so Windows-style event data remains addressable by field name
* Windows Event XML detection and reusable presets for single events and event collections
* Windows Event severity aliases for the standard numeric event levels used by the sample/preset workflow
* format suggestions and preview integration across the new source families
* representative sample logs and reusable profiles for each major supported family
* automated importer, preview, profile, and format-suggestion coverage
* local full-suite and UI regression verification
* Windows and Linux CI verification
* refreshed release screenshots using the structured XML engineering-session workflow
* `v0.7.0` prerelease packaging and documentation preparation

Additional maintenance completed during the phase:

* preserved empty timeline intervals across filtered ranges so gaps remain visible
* hid canonical table columns that are unused by the loaded dataset
* improved subsystem-filter usability for long values
* expanded CI package verification to cover representative `v0.7.0` source/profile pairs

Phase 6 establishes sufficient ingestion breadth for the current expansion. Native EVTX ingestion, CEF, LEEF, and additional format families are not part of `v0.7.0`. They may be reconsidered later only if target-user demand or architectural leverage justifies replacing higher-cost or lower-value planned work rather than silently increasing total scope.

### Phase 7 — Responsive Large-File Import

**Status: Completed in `v0.8.0`.**

With representative ingestion coverage established in `v0.7.0`, this phase shifts development priority from additional format count to the responsiveness and scalability of practical investigations.

The goal is to keep the structured investigation workflow usable as representative engineering and diagnostic files grow, while documenting observed behavior conservatively rather than implying a universal maximum file size or throughput guarantee.

Completed deliverables:

* streamed file reading for line-oriented and XML importers where the source format permits incremental processing
* import parsing outside the UI thread so long-running imports do not freeze the desktop interface
* determinate byte/record progress reporting for streamed import paths
* indeterminate progress for structured JSON, where meaningful incremental percentage reporting is not currently available
* cooperative import cancellation that preserves the previously loaded investigation and does not install partial results
* responsive large structured-JSON and XML configuration by suppressing expensive automatic previews above the large-document threshold
* explicit background preview generation for large structured documents
* cooperative cancellation of obsolete background previews when the selected source or profile changes
* bounded preview generation that continues to honor the configured preview-record limit
* measured end-to-end Release-build performance scenarios across representative JSON Lines, CSV, IIS W3C, Windows Event XML, and structured JSON sources
* conservative performance documentation that reports test-system observations rather than maximum supported file sizes, record counts, or parser-throughput guarantees

Measured Phase 7 scenarios on the documented Windows development system:

| Source family | Records | Approx. source size | Median end-to-end import time | Import UI behavior |
| --- | ---: | ---: | ---: | --- |
| JSON Lines | 220,000 | 109.4 MiB | 7.2 s | responsive; determinate progress |
| CSV | 180,000 | 30.7 MiB | 3.0 s | responsive; determinate progress |
| IIS W3C | 180,000 | 19.6 MiB | 9.2 s | responsive; determinate progress |
| Windows Event XML collection | 90,000 | ~102 MiB | 14.8 s | responsive; determinate progress |
| Structured JSON | 120,000 | 64.1 MiB | 6.5 s | responsive; indeterminate progress |

All measured scenarios remained responsive after the resulting investigation was displayed. Cancellation was also manually verified against large JSON Lines and Windows Event XML imports, with the prior investigation preserved and no partial result installed. These observations are evidence from one test system and are not hard limits or guarantees for other files or machines.

Timeline scalability work was pulled forward from the later analytics phase because large-file testing exposed it as part of the same practical responsiveness problem. Completed timeline work now includes:

* user-selectable timeline resolutions from millisecond through day-scale intervals
* automatic/adaptive resolution selection based on the investigation time span
* full date/time-aware bucket calculation and context-sensitive labels
* bounded windowed rendering for fine-resolution timelines instead of materializing enormous bucket ranges
* horizontal navigation when the selected resolution produces more buckets than can be displayed at once
* scaled scrollbar handling for very large bucket-index ranges
* live visible-time-range feedback while dragging the timeline scrollbar
* stable Y-axis scaling while navigating horizontally
* layout refinements that keep timeline axes and legend information usable while fitting the chart vertically within its designated area

Additional maintenance completed during the phase:

* made the Import Configuration raw-source preview vertically resizable for multiline structured JSON and XML records
* corrected large-XML preview behavior discovered during performance verification without weakening conservative Windows Event XML format detection

Performance work should continue to be measured against practical investigation workflows. Future documentation must keep observed test scenarios distinct from unsupported maximum-size, memory, or throughput claims.

### Phase 8 — Multi-Session Investigation Workspace

**Status: Completed in `v0.9.0`.**

Expanded TraceScope from a single replaceable investigation into a workspace where related logs from different applications, services, devices, test runs, or system components can remain open together.

The implementation preserves meaningful per-session context rather than treating tabs as interchangeable file views. Each session owns its investigation controller and retains the source, import profile, diagnostics, filtering state, and presentation metadata needed to move between related evidence without rebuilding the investigation.

Completed deliverables:

* multiple open investigation sessions within one application instance
* tabbed session switching with independent per-session filter and model state
* session closing with deterministic active-session selection and a consistent empty-workspace state
* stable session identities so session operations do not depend on mutable tab indexes
* in-place session reloading using the original source path and import profile without creating duplicate tabs
* reload behavior that refreshes source metadata and imported records while retaining applicable filters
* cooperative reload cancellation that leaves the existing session unchanged
* per-session source metadata including path, name, size, last-modified time, and import time
* per-session retention of the import profile, import diagnostics, processed-record count, and source-truncation state
* per-session cached data capabilities, timeline bounds, and column widths to avoid repeated full-dataset work while switching sessions
* responsive switching verified with representative large performance fixtures
* persistent recent-file history using local application settings
* recent files reopened through the normal Import Configuration workflow rather than imported silently
* persistent recent-profile history using the existing validated profile-loading path
* bounded, deduplicated most-recent-first history for files and profiles with stale-path cleanup
* automated coverage for session state, workspace add/switch/close/reload behavior, and recent-item persistence
* local full-suite and UI regression verification
* Windows and Linux CI verification
* refreshed release screenshots covering the multi-session and recent-history workflows

### Phase 9 — Advanced Filtering and Navigation

**Status: Completed in `v0.10.0`.**

Expanded investigation controls beyond the prototype filters so users can move efficiently from a large normalized record set to the portion relevant to a failure, warning pattern, subsystem, entity, time window, or source-specific field.

Canonical fields remain optional. Controls that depend on event code, entity ID, severity, timestamp, or another canonical value appear only when the active investigation provides the required data, while search and source-specific custom-field filtering remain useful for less standardized sources.

Completed deliverables:

* multi-severity filtering through a compact multi-select control
* time-range filtering with optional UTC start and end bounds
* event-code filtering when event-code data is present
* entity filtering when entity data is present
* dynamic exact-value custom-field filtering when custom attributes are present
* multiple values for the same custom field using OR semantics, combined with AND semantics across different custom fields and canonical filter categories
* exclusion of records missing a custom field that is actively filtered
* cached custom-field capability discovery without eagerly enumerating high-cardinality custom values
* modeless time-range and custom-field filter interfaces with compact active-state summaries
* custom-field table-cell context actions for copying values and adding exact-value filters directly from visible records
* filter reset across canonical, text, time-range, and custom-field criteria
* persistent named filter presets stored in local application settings
* complete preset restoration across severity, subsystem, search, event code, entity, time range, and custom-field filters, with unavailable criteria ignored safely in different sessions
* batched complete-filter-state application so one logical filter or preset change does not cause repeated large proxy-model resets
* previous/next navigation across visible events in the current filtered and sorted table order
* previous/next navigation across visible warning, error, and critical records, including wraparound behavior
* compact navigation context showing visible position and preserved source-record number
* grouped warning/error summary drill-down by subsystem and issue class while preserving unrelated active filters
* timeline bucket drill-down to exact time ranges, with severity-aware bars narrowing to the represented severity while preserving unrelated active filters
* drill-down behavior for automatic and manually selected timeline resolutions, including horizontally windowed fine-resolution timelines
* automated coverage for preset persistence, complete filter-state batching, and controller navigation behavior
* manual regression verification across representative and large-scale samples
* Windows and Linux CI verification
* refreshed release screenshots covering the advanced filtering and navigation workflow

### Phase 10 — Bookmarks, Notes, and Findings

**Status: Completed in `v0.11.0`.**

Added local investigation state tied to stable record identities so engineers can preserve what they discovered while working through QA failures, field-support packages, engineering test results, and other diagnostic sessions.

The completed workflow turns transient navigation into a lightweight in-session investigation record without introducing a collaborative backend, ticketing system, or account model. Disk-backed workspace persistence was subsequently completed in Phase 13.

Completed deliverables:

* session-local event bookmarks keyed by stable record identity
* compact bookmark indicators in the source-record gutter without consuming a data column
* bookmark-only filtering integrated with complete filter state and reusable filter presets
* multiline analyst notes edited through a modeless, wrapping note editor so source records remain inspectable while notes are open
* independent Open, Resolved, and Dismissed finding statuses
* multi-select finding-status filtering integrated with complete filter state and reusable filter presets
* dedicated Findings review panel with Open, Resolved, and Dismissed counts, compact source-record/time context, preserved multiline notes, and explicit no-note summaries
* findings review layout that adapts available space without crowding selected-record details
* direct double-click navigation from findings back to their exact source records using stable record IDs
* targeted filter adaptation when a finding's source record is hidden, preserving unrelated active criteria instead of resetting the investigation
* retention of applicable bookmark, note, and finding state across in-place reloads when stable record identities survive, with stale state pruned for removed records
* automated coverage for investigation-state storage, bookmark/finding filtering, preset round trips, reload retention, and stable-record proxy lookup
* local full-suite and UI regression verification
* Windows and Linux CI verification
* refreshed release screenshots including the dedicated Findings workflow
* `v0.11.0` prerelease and downloadable package verification

### Phase 11 — Analytics and Burst Detection

**Status: Completed in `v0.12.0`.**

Added deterministic, explainable investigation summaries that help users recognize timing, frequency, severity, subsystem, entity, and event-code patterns without turning TraceScope into a generalized observability dashboard.

The scalable timeline foundation originally planned for this phase was pulled forward during Phase 7 because large-file verification showed that resolution selection, bounded bucket materialization, and horizontal navigation were required for practical investigations. Phase 11 builds on that foundation and on the grouped-summary/timeline drill-down completed in Phase 9 rather than reimplementing either workflow.

Analytics degrade independently when canonical fields are missing. A source without severity, event code, subsystem, entity ID, or usable timestamps loses only the analysis that depends on that dimension while unrelated inspection and analysis remain available.

Completed deliverables:

* deterministic event-code frequency analysis when event codes are present
* deterministic entity frequency analysis when entity IDs are present, with Top-N selection kept in the UI rather than truncating analyzer output
* deterministic subsystem-frequency analysis used to rank timeline trend series
* severity trends presented through the existing scalable timeline
* subsystem timeline trends with configurable interval size, preserved empty buckets, deterministic Top 5/Top 10 presentation, stable legend membership, and stable Y-axis scaling while navigating
* reusable `AnalysisTimeBucketRange` logic shared by timeline and trend analyzers so bucket alignment and window semantics remain consistent
* windowed subsystem-trend analysis that materializes only the requested visible bucket range for fine-resolution timelines
* per-session persistence of Severity/Subsystem timeline breakdown and subsystem Top-N presentation choice
* drill-down from subsystem timeline bars to the represented subsystem and time range without broadening unrelated filters
* deterministic timestamp-cadence analysis using valid positive gaps, including minimum, median, mean, P90, maximum, zero-gap count, and positive-gap count
* adaptive human-readable burst timing recommendations with an explicit sparse-data fallback
* adaptive Auto burst-window recommendations constrained to at most one quarter of the current valid-timestamp investigation span and rounded down to the existing preferred-duration vocabulary
* deterministic WARN/ERROR/CRITICAL burst detection using inclusive time windows, explicit elevated-event and ERROR/CRITICAL thresholds, deterministic ordering, and configurable merge gaps
* merged burst episodes that retain exact first/last elevated-event boundaries, contributing stable record IDs, severity counts, subsystem/event-code/entity frequency maps, and trigger reasons
* compact Burst Settings interface with Auto and Manual timing modes
* analyst-controlled elevated-event and ERROR/CRITICAL thresholds in both timing modes
* preservation of Manual window/merge settings while Auto timing is selected so switching modes does not discard analyst choices
* transparent Auto timing details showing the recommended window/merge gap, cadence statistics, and whether adaptive or fallback timing is active
* dedicated Analytics review tab with Overview and Bursts pages
* capability-aware Event Code Frequencies and Top Entities presentation
* burst list and explanation panel showing timing mode, boundaries, duration, severity mix, trigger reasons, merge behavior, contributing records, and dimension summaries
* burst drill-down that narrows the current investigation to the burst time range and compatible elevated severities while preserving unrelated filters
* explicit semantic colors for severity timeline series so informational, warning, error, and critical activity is visually distinguishable
* consistent preserved source-record numbering in the Telemetry Events gutter, Findings table, and navigation context even after sorting or filtering
* realistic fictional investigation samples for an order-fulfillment incident, an environmental-chamber QA run, and matched known-good/degraded field-gateway support sessions, with reusable profiles
* automated coverage for event-code/entity frequencies, subsystem frequencies/trends, windowed trend materialization, trend scale calculation, shared time-bucket ranges, adaptive cadence, deterministic burst detection, and preserved source-record header behavior
* local full-suite and UI regression verification
* Windows and Linux CI verification
* full `v0.12.0` screenshot refresh and Feature Screenshot Gallery covering the current product workflow
* README and roadmap refresh centered on prospective-user understanding while retaining conservative, verifiable technical and employer-facing claims
* `v0.12.0` prerelease and downloadable package verification

Deterministic burst detection is not described as AI anomaly detection, automated diagnosis, or root-cause analysis.

### Phase 12 — Session Comparison

**Status: Completed in `v0.13.0`.**

Added structured, directional comparison between two complete imported sessions, with particular value for practical engineering workflows such as comparing a failed or degraded run against a known-good run.

Comparisons use an explicit Baseline → Comparison orientation, with all deltas presented as Comparison − Baseline. Analysis is built from complete imported-session records rather than the sessions' current filtered views, so temporary investigation filters cannot silently change the meaning of an existing comparison.

Canonical and source-specific dimensions remain capability-aware. A missing field is reported as unavailable for the comparison that depends on it rather than being treated as zero, and appearance/disappearance claims are made only when both sessions actually provide the relevant dimension. Comparison output remains deterministic and descriptive rather than claiming causal diagnosis or root cause.

The matched known-good and degraded field-gateway samples added in `v0.12.0` provide a representative comparison workflow and release demonstration.

Completed deliverables:

* dedicated session-comparison creation workflow with explicit Baseline and Comparison selection
* active-session defaults that orient the currently investigated run as the Comparison while allowing deliberate reversal
* immutable comparison snapshots with stable comparison identity, copied source metadata, and no live dependency on source-session objects after creation
* complete-session comparison semantics that remain unchanged when either source session is filtered
* total-record, duration, and event-rate context with explicit directional deltas
* event-code comparison that separates appeared, disappeared, and changed values when both sessions provide event-code data
* severity-count differences when severity is available
* elevated subsystem and entity activity when the corresponding canonical dimensions are available
* conservative shared custom-field comparison using numeric min/median/max summaries only when both sides are wholly finite numeric, and categorical appearance/disappearance only when that claim is supported
* optional burst comparison that applies one explicit shared `BurstDetectionSettings` configuration to both sessions rather than deriving independent Auto settings
* explicit distinction between burst comparison not requested, burst data unavailable, and valid zero-burst results
* dedicated comparison documents ordered around investigation impact while omitting unchanged/noisy comparable output
* compact Baseline → Comparison document titles with complete source orientation available through tooltips and document content
* source-session reload or closure without mutation of already-created comparison snapshots
* automated coverage for comparison analyzers, immutable snapshot construction, complete-record semantics, source-reload independence, burst-request state, dialog defaults, orientation swapping, validation, and shared burst defaults
* generalized workspace-document hosting so both investigations and comparison documents can be reordered, detached, re-docked, moved between detached windows, grouped in detached windows, and closed consistently
* responsive workspace hardening pulled forward from Phase 16 because detachable documents are expected to be used side-by-side and on portrait-oriented secondary displays
* narrow-layout behavior that elides long session summaries without losing full tooltip context, reflows filter and selected-event controls only when constrained, allows review tables to shrink/scroll instead of forcing window width, gives Findings/Analytics appropriate priority over Selected Event Details, and reduces the visible fine-resolution timeline bucket window as horizontal space decreases
* manual verification in representative horizontally split and portrait-monitor layouts
* local full-suite regression verification
* `v0.13.0` prerelease publication

### Phase 13 — Workspace and Profile Persistence

**Status: Completed in `v0.14.0`.**

Added local workspace persistence so a useful multi-session investigation can be saved, closed, and resumed without rebuilding its source configuration, annotations, filters, comparison documents, or workspace organization.

Persistence preserves the local, reproducible nature of TraceScope. Saved workspaces retain the source and import-profile context required to re-import sessions, while persisted investigation state and immutable comparison snapshots remain explicit local data rather than introducing a database, account system, or hosted backend.

Completed deliverables:

* loaded-session persistence with source paths and complete import-profile context required to reopen each investigation
* bookmark persistence keyed by stable record identity
* analyst-note persistence keyed by stable record identity
* finding-status persistence keyed by stable record identity
* active per-session filter-state persistence within saved workspaces
* investigation presentation-state persistence across event tables, selected records, detail/review/analytics panels, timeline/burst presentation, splitters, scroll positions, and other resumable session-view state
* open comparison-document persistence that preserves immutable Baseline → Comparison snapshot meaning rather than depending on live source-session state
* comparison-document presentation persistence including restored scroll position
* workspace document ordering, per-group current-document state, and global active-document restoration
* detached-workspace grouping and window-state restoration for side-by-side investigation layouts
* primary-window geometry and maximized-state restoration alongside detached workspace windows
* versioned, human-readable JSON workspace schemas with compatibility handling for later-added optional fields
* staged workspace opening that does not replace the currently open investigation until all recoverable source sessions have imported successfully
* clear missing-source handling with Locate File, Skip Session, and Cancel Open Workspace choices so one unavailable source does not silently discard unrelated recoverable workspace state
* preservation of immutable comparison snapshots even when one of their original source sessions is unavailable and skipped during workspace restoration
* atomic workspace saving through `QSaveFile`
* application-wide Open Log/Save Workspace/Save Workspace As shortcuts so those application-level commands remain available while detached TraceScope windows are focused
* bounded, deduplicated recent-workspace history for successfully opened or saved workspaces, stored locally alongside recent-file and recent-profile history
* serialization round-trip and backward-compatibility tests for workspace/session/comparison persistence and presentation/layout state
* local full-suite and manual workspace-restoration regression verification
* `v0.14.0` prerelease publication

Persistence uses local, versioned JSON. A database remains unnecessary for the current offline workspace model.

### Phase 14 — Reporting and Export

**Status: In progress; targeted for `v0.15.0`.**

Expand investigation output beyond the existing investigation-record CSV workflow so findings, deterministic analysis, comparison results, and supporting evidence can leave TraceScope and be shared with other engineers, QA, field support, or downstream issue/documentation workflows.

Reporting should complete the existing investigation workflow rather than become a configurable report designer or collaborative case-management system. Exported reports should be deterministic, self-contained, and understandable without requiring TraceScope or a hosted backend.

Report generation should capture immutable export state at the moment export begins. Generated output must not depend on mutable widgets or later changes to an investigation, and this capture boundary should remain valid when Phase 15 introduces live-followed sessions and potentially continuously updating live comparisons.

Planned deliverables:

* selected-record copy as structured JSON
* selected-record copy as formatted human-readable text
* structured findings export suitable for downstream QA, issue-tracking, spreadsheet, or documentation workflows
* offline HTML investigation reports that summarize source/session context, investigation findings, deterministic analytics, burst analysis when available, and relevant supporting records without claiming automated diagnosis
* offline HTML comparison reports that preserve explicit Baseline → Comparison orientation, source context, comparison settings, and deterministic comparison results
* explicit report-generation timestamps and captured record/time-span context so exported output has a stable point-in-time meaning
* immutable report/export models separated from live investigation and presentation widgets
* deterministic, capability-aware omission or unavailable-state handling when a report dimension is not supported by the underlying source data
* automated coverage for report-model capture, escaping/serialization, deterministic output, and representative investigation/comparison exports
* local full-suite and UI regression verification

HTML reporting should remain offline and self-contained. Browser printing may provide a practical PDF path without introducing a dedicated PDF-generation dependency unless later requirements demonstrate a clear need.

### Phase 15 — Live File Following

Support files that are actively receiving appended records so TraceScope can be used during application runs, QA execution, simulations, engineering tests, and other situations where investigators need to observe a diagnostic log as it grows.

Live following should extend the existing investigation model rather than create a separate monitoring product. TraceScope will remain file-oriented and offline rather than becoming a centralized collection service.

The existing Phase 12/13 comparison documents remain immutable comparison snapshots: they preserve the meaning of a Baseline → Comparison analysis at the time it was created and can survive later source-session reload, closure, or workspace restoration. Live following introduces a separate need for comparisons that can update as their source sessions receive appended records. Phase 15 should evaluate that behavior explicitly rather than silently changing existing snapshot semantics.

A likely model is to keep immutable comparison snapshots as stable evidence while allowing a live comparison document to reference active sessions, recompute as new records arrive, and be frozen into an immutable comparison snapshot for persistence, reporting, or later review. The exact live-comparison UI and comparison-window semantics should be finalized during Phase 15 implementation, including how unequal observed record counts or timestamp coverage are communicated.

Planned deliverables:

* follow appended records
* pause and resume
* incremental parsing
* partial-line handling
* truncation handling
* file-replacement handling
* live summaries
* live filtering
* explicit live-session state that remains compatible with the existing investigation model and persistence/report capture boundaries
* evaluation and implementation of continuously updating Baseline → Comparison behavior for live-followed sessions without weakening immutable comparison-snapshot semantics
* ability to capture/freeze a live comparison into a stable immutable comparison snapshot if the live-comparison workflow is implemented

### Phase 16 — Final UI Polish, Documentation, and 1.0 Release

Complete the expansion with final UI polish, polished documentation, and a stable downloadable release that accurately presents TraceScope as a configurable offline log-analysis workbench for engineering and diagnostic use.

The foundational constrained-layout work originally planned for this phase was pulled forward and completed during Phase 12 because detachable workspace documents make side-by-side and portrait-oriented use part of the normal investigation workflow rather than a final-release edge case. Phase 16 should build on that responsive foundation instead of reimplementing it.

Final documentation should make the supported-format boundary, optional canonical-field model, profile-driven import architecture, large-file behavior, multi-session and comparison workflows, persistence behavior, reporting capabilities, live-following boundary, and scope exclusions clear enough that prospective users can decide whether TraceScope fits their workflow. Employer-facing material should remain secondary to that product clarity while still making the architecture, testing, CI/release discipline, and conservative engineering decisions directly verifiable.

Planned deliverables:

* final cross-workflow UI consistency and small visual cleanup across features completed in earlier phases
* responsive regression verification for the final persistence, reporting, and live-following surfaces introduced after the Phase 12 hardening pass
* representative full-width, horizontally split, and portrait-layout regression verification before `v1.0.0`
* final Windows and Linux distributables
* automated release packaging
* architecture documentation
* import-profile specification
* supported-format documentation
* test strategy
* performance notes
* sample investigations
* polished screenshots
* final README
* product and portfolio claims review
* stable `v1.0.0` release

## Scope Exclusions

The current roadmap does not expand TraceScope into:

* a web application
* an ASP.NET backend
* a cloud-hosted service
* a user-account system
* a paid infrastructure project
* a relational database application without a demonstrated need
* an AI incident-analysis product
* a packet-capture tool
* an enterprise observability platform
* an external binary plugin ecosystem during the initial expansion

TraceScope will remain an offline, configurable, native developer tool.
