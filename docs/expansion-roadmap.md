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
* Keep employer-facing claims conservative and directly supported by the implementation.
* Avoid paid infrastructure and unnecessary external services.
* Complete each phase with passing tests and downloadable Windows and Linux packages.

## Product Positioning, Adoption, and Scope Discipline

TraceScope remains focused on file-based telemetry and diagnostic investigation for applications, services, simulated devices, sensors, QA runs, field-support packages, and engineering test systems. Broader adoption is desirable, but new capabilities should strengthen this primary use case rather than reposition the application around unrelated markets.

TraceScope occupies the space between raw-file and text-log viewers and centralized observability platforms. Its value is a structured, repeatable, offline investigation workflow without requiring log-shipping infrastructure, a hosted backend, user accounts, or an indexing service.

The project should be evaluated against the tools its intended users may already reach for, including text editors and command-line utilities, fast raw-log viewers, structured desktop log-analysis tools, and centralized observability systems. TraceScope does not need to outperform every category at its specialty. Instead, it should reduce the friction of moving from unfamiliar files to a useful structured investigation while preserving source-specific information and keeping the workflow local and reproducible.

Format breadth is an enabling capability rather than the primary product differentiator. TraceScope should support a practical set of representative structured and operational log families plus reusable configurable import profiles. Additional formats should be added only when they materially reduce friction for the intended audience or reuse existing importer architecture at low incremental cost.

Once representative ingestion coverage is established, development priority shifts from format count to investigation depth. Large-file responsiveness, multi-session investigation, advanced filtering and navigation, findings, deterministic analytics, session comparison, persistence, live following, and reporting are expected to provide more target-user value than indefinitely expanding the built-in format list.

Roadmap evolution should normally substitute, reinterpret, or reprioritize planned work rather than increase the overall project scope. New work should earn its place through demonstrated target-user value, architectural leverage, or replacement of lower-value planned work. The expansion should remain finishable on approximately the scale originally intended.

Important target workflows include:

* moving quickly from unfamiliar source files to structured investigation
* reusing parsing and normalization configuration instead of rebuilding one-off scripts
* preserving source-specific fields alongside common investigation fields
* investigating related logs from multiple applications or system components
* comparing failed, degraded, and known-good runs
* navigating efficiently around warnings, errors, bursts, and surrounding context
* recording findings and producing useful investigation output

## Current Prototype Baseline

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

GitHub Actions artifacts are used for build verification. Approved packages are then attached to GitHub Releases as permanent employer-facing downloads.

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

Current release target:

| Version | Milestone | Status |
| --- | --- | --- |
| `v0.8.0` | Responsive Large-File Import | release preparation in progress |

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

Implemented foundations through the current Phase 7 development branch:

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

Reusable import profiles are versioned, human-readable JSON so mappings can be reused, shared, and committed alongside the applications that produce the logs. The desktop workflow now exposes those profile and preview services directly while keeping import behavior explicit and reproducible.

Importers are registered internally. An external binary plugin ecosystem is not part of the initial expansion.

Phase 6 established the representative ingestion baseline in `v0.7.0`. Phase 7 then shifted attention from format breadth to responsiveness and investigation scalability. The `v0.8.0` candidate keeps supported imports responsive through background parsing, progress/cancellation behavior, large-structured-document preview safeguards, and timeline scaling work. Later phases now prioritize multi-session investigation, advanced navigation and filtering, findings, comparison, persistence, live following, deterministic analytics, and reporting rather than continuing to accumulate built-in formats.

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

**Status: Implementation complete; `v0.8.0` release preparation in progress.**

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

Allow multiple imported sessions to coexist within one application instance so related logs from different applications, services, devices, test runs, or system components can be investigated without repeatedly replacing the active source.

The goal is not merely tabbed file viewing. The workspace should preserve enough per-session context that an engineer can move between related evidence while retaining the source, diagnostics, and profile information needed to understand how each session was imported.

Planned deliverables:

* multiple open sessions
* session switching
* session closing
* session reloading
* per-session source metadata
* per-session diagnostics
* recent files
* recent profiles

### Phase 9 — Advanced Filtering and Navigation

Expand investigation controls beyond the prototype filters so users can move efficiently from a large normalized record set to the small portion relevant to a failure, warning pattern, subsystem, entity, time window, or source-specific field.

Canonical fields remain optional. Filters that depend on event code, entity ID, severity, or other canonical values should be available when those values exist without preventing investigation of sources that rely primarily on dynamic custom attributes.

Planned deliverables:

* multiple severity selection
* time-range filtering
* event-code filtering when present
* entity filtering when present
* dynamic custom-field filtering
* filter reset
* saved filter presets
* next and previous warning/error navigation
* surrounding-event navigation
* drill-down from charts and summaries

### Phase 10 — Bookmarks, Notes, and Findings

Add local investigation state tied to stable event identities so engineers can preserve what they discovered while working through QA failures, field-support packages, engineering test results, and other diagnostic sessions.

This phase should turn transient navigation into a lightweight investigation record without introducing a collaborative backend, ticketing system, or account model.

Planned deliverables:

* event bookmarks
* analyst notes
* finding status
* findings panel
* bookmark filtering
* navigation from findings back to source records

### Phase 11 — Analytics and Burst Detection

Add deterministic, explainable investigation summaries that help users recognize timing, frequency, severity, subsystem, entity, and event-code patterns without turning TraceScope into a generalized observability dashboard.

The scalable timeline foundation originally planned in this phase was pulled forward and completed during Phase 7 because large-file verification showed that timeline resolution, bucket sizing, and navigation were necessary to keep large investigations usable. Phase 11 should build new analysis on that existing timeline behavior rather than reimplement it.

Analytics should degrade gracefully when a source does not provide a particular canonical field. A missing severity, event code, subsystem, or entity ID should disable or reduce only the analysis that depends on that field rather than reducing the usefulness of unrelated records.

Planned deliverables:

* event-code frequencies when event codes are present
* top entities when entity IDs are present
* subsystem trends when subsystem data is present
* severity trends when severity data is present
* deterministic warning and error burst detection
* timeline/summary drill-down to underlying records where it materially improves investigation flow

Deterministic burst detection will not be described as AI anomaly detection.

### Phase 12 — Session Comparison

Provide structured comparisons between two imported sessions, with particular value for engineering workflows such as comparing a failed or degraded run against a known-good run.

Comparison should use shared canonical dimensions where they are available while remaining useful when one or both sessions omit particular canonical fields. The feature should surface meaningful differences without claiming causal diagnosis.

Planned deliverables:

* total-record differences
* severity-count differences when severity is available
* subsystem differences when subsystem data is available
* event-code differences when event codes are available
* values appearing only in one session
* duration differences
* event-rate differences
* burst differences
* dedicated comparison interface

### Phase 13 — Workspace and Profile Persistence

Allow investigations to be saved and reopened locally so a useful multi-session investigation does not disappear when the application closes.

Persistence should preserve the local, reproducible nature of TraceScope and should support continued work on investigations without introducing infrastructure that is unnecessary for the target workflow.

Planned deliverables:

* loaded-session persistence
* bookmark persistence
* note persistence
* filter persistence
* comparison-selection persistence
* versioned workspace schemas
* missing-source handling
* serialization round-trip tests

Persistence will use local, versioned JSON unless later requirements demonstrate a practical need for a database.

### Phase 14 — Live File Following

Support files that are actively receiving appended records so TraceScope can be used during application runs, QA execution, simulations, engineering tests, and other situations where investigators need to observe a diagnostic log as it grows.

Live following should extend the existing investigation model rather than create a separate monitoring product. TraceScope will remain file-oriented and offline rather than becoming a centralized collection service.

Planned deliverables:

* follow appended records
* pause and resume
* incremental parsing
* partial-line handling
* truncation handling
* file-replacement handling
* live summaries
* live filtering

### Phase 15 — Reporting and Export

Expand investigation output beyond the existing investigation-record CSV workflow so useful findings can leave TraceScope and be shared with other engineers, QA, field support, or downstream issue/documentation workflows.

Reporting should summarize and preserve investigation results rather than attempt to become a collaborative case-management system.

Planned deliverables:

* findings export
* selected-record copy as JSON
* selected-record copy as formatted text
* offline HTML investigation reports

### Phase 16 — Documentation and 1.0 Release

Complete the expansion with polished documentation and a stable downloadable release that accurately presents TraceScope as a configurable offline log-analysis workbench for engineering and diagnostic use.

Final documentation should make the supported-format boundary, optional canonical-field model, profile-driven import architecture, large-file behavior, investigation workflow, and scope exclusions clear enough that both prospective users and employers can understand what the application does without overstating its capabilities.

Planned deliverables:

* final Windows distributable
* automated release packaging
* architecture documentation
* import-profile specification
* supported-format documentation
* test strategy
* performance notes
* sample investigations
* polished screenshots
* final README
* portfolio-claims review
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
