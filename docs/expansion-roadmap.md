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
* Complete each phase with passing tests and a downloadable Windows build.

## Current Prototype Baseline

The original prototype currently uses:

* C++17
* Qt 6
* Qt Widgets
* Qt Charts
* CMake
* Qt Test
* MinGW 64-bit on Windows

Implemented capabilities include:

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

The current implementation uses a fixed telemetry-event structure, a hard-coded JSON Lines parser, a `QTableWidget`-based event display, and a `MainWindow` that owns substantial application state and UI orchestration.

These constraints form the starting point for the expansion rather than defects in the completed original prototype.

## Release Discipline

Each completed development phase should produce:

* passing Windows and Linux CI builds
* passing automated tests on both platforms
* Release-mode native executables
* a portable Windows x64 ZIP produced with `windeployqt`
* a Linux x86_64 AppImage produced with `linuxdeploy`
* a platform-neutral sample-log ZIP
* startup smoke tests for both packaged applications
* a version tag
* a GitHub prerelease or release
* attached release notes
* attached Windows, Linux, and sample assets
* clean smoke tests of the downloaded release packages

GitHub Actions artifacts are used for build verification. Approved packages are then attached to GitHub Releases as permanent employer-facing downloads.

The original prototype package set is:

* Tag: `v0.1.0`
* Release title: `TraceScope 0.1.0 — Original Prototype`
* Status: prerelease
* Windows asset: `TraceScope-v0.1.0-windows-x64.zip`
* Linux asset: `TraceScope-v0.1.0-linux-x86_64.AppImage`
* Sample asset: `TraceScope-v0.1.0-samples.zip`

## Canonical Investigation Record

The flexible-ingestion architecture will introduce a common investigation record with optional standard fields such as:

* timestamp
* severity
* subsystem
* event code
* entity ID
* message

The record will also preserve:

* custom source attributes
* the raw source record
* source file information
* source line or record number
* import diagnostics
* stable record identity

Missing canonical fields should not automatically invalidate an otherwise useful source record. Features that require a timestamp, severity, event code, or another specific field may be unavailable when that field is absent, while unrelated inspection and search features should remain usable.

## Planned Import Architecture

The planned ingestion design includes concepts such as:

* `ILogImporter`
* importer registry
* import profile
* field mapping
* severity mapping
* timestamp configuration
* import preview
* import result
* import diagnostic
* flexible investigation record

The existing JSON Lines parser will become the first implementation behind the importer abstraction.

Importers will be registered internally. An external binary plugin ecosystem is not part of the initial expansion.

Reusable import profiles will use versioned, human-readable JSON so developers can save, reuse, share, and commit mappings alongside the applications that produce their logs.

## Development Phases

### Phase 0 — CI, Packaging, and Prototype Release Baseline

Establish repeatable cross-platform verification and a downloadable baseline for the completed original prototype.

Planned deliverables:

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

Replace rigid event assumptions with a flexible investigation domain.

Planned deliverables:

* optional typed canonical fields
* typed severity representation
* timestamp parsing
* dynamic custom attributes
* raw source preservation
* source file and record metadata
* stable record identity
* import results
* import diagnostics

### Phase 2 — Importer Abstraction and Configurable JSON Lines

Move existing JSON Lines behavior behind the common importer architecture.

Planned deliverables:

* `ILogImporter`
* importer registry
* JSON Lines importer
* configurable JSON field paths
* compatibility with existing sample files
* comprehensive importer tests

### Phase 3 — Qt Model/View Architecture

Replace the fixed table implementation and reduce UI orchestration responsibilities in `MainWindow`.

Planned deliverables:

* `QAbstractTableModel` event model
* proxy sorting and filtering
* dynamic columns
* correct source/proxy selection mapping
* decomposed UI responsibilities
* preserved existing investigation behavior

### Phase 4 — Import Profiles and Preview Logic

Define the reusable configuration format used to map source logs into the canonical investigation model.

Planned deliverables:

* versioned import-profile schema
* canonical field mappings
* custom-field mappings
* severity aliases
* timestamp rules
* profile validation
* import preview services
* profile serialization and round-trip tests

### Phase 5 — Import Configuration Interface

Add the desktop workflow for configuring and reusing imports.

Planned deliverables:

* file selection
* drag-and-drop
* likely-format suggestions
* source-record preview
* field mapping controls
* validation feedback
* profile saving
* profile loading

### Phase 6 — Additional Built-In Formats

Expand supported source formats while keeping behavior explicit and profile-driven.

Planned format order:

1. CSV and TSV
2. JSON arrays and structured JSON documents
3. regex-configurable plain-text logs
4. possibly key-value logs after the primary formats are stable

Each format will include representative sample files, reusable profiles, and automated tests.

### Phase 7 — Responsive Large-File Import

Improve responsiveness and memory behavior for larger log files.

Planned deliverables:

* streamed file reading
* parsing outside the UI thread
* progress reporting
* cancellation
* measured performance scenarios
* documented, conservative performance claims

### Phase 8 — Multi-Session Investigation Workspace

Allow multiple imported sessions to coexist within one application instance.

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

Expand investigation controls beyond the prototype filters.

Planned deliverables:

* multiple severity selection
* time-range filtering
* event-code filtering
* entity filtering
* dynamic custom-field filtering
* filter reset
* saved filter presets
* next and previous warning/error navigation
* surrounding-event navigation
* drill-down from charts and summaries

### Phase 10 — Bookmarks, Notes, and Findings

Add local investigation state tied to stable event identities.

Planned deliverables:

* event bookmarks
* analyst notes
* finding status
* findings panel
* bookmark filtering
* navigation from findings back to source records

### Phase 11 — Analytics and Burst Detection

Add deterministic, explainable investigation summaries.

Planned deliverables:

* adaptive timeline buckets
* full date/time-aware grouping
* event-code frequencies
* top entities
* subsystem trends
* severity trends
* deterministic warning and error burst detection
* drill-down to underlying records

Deterministic burst detection will not be described as AI anomaly detection.

### Phase 12 — Session Comparison

Provide structured comparisons between two imported sessions.

Planned deliverables:

* total-record differences
* severity-count differences
* subsystem differences
* event-code differences
* values appearing only in one session
* duration differences
* event-rate differences
* burst differences
* dedicated comparison interface

### Phase 13 — Workspace and Profile Persistence

Allow investigations to be saved and reopened locally.

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

Support files that are actively receiving appended records.

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

Expand investigation output while retaining the existing CSV workflow.

Planned deliverables:

* dynamic custom-field CSV export
* findings export
* selected-record copy as JSON
* selected-record copy as formatted text
* offline HTML investigation reports

### Phase 16 — Documentation and 1.0 Release

Complete the expansion with polished documentation and a stable downloadable release.

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