# TraceScope — Qt Telemetry Log Inspector

[![TraceScope CI](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml/badge.svg)](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml)

TraceScope is a native C++/Qt desktop application for importing, normalizing, filtering, inspecting, visualizing, and exporting structured telemetry and diagnostic logs.

It began as a focused JSON Lines inspector and has grown into a configurable offline workbench. The current `v0.11.0` release adds session-local bookmarks, analyst notes, finding statuses, and a dedicated findings review workflow tied to stable source-record identities. Investigators can mark records for follow-up, preserve conclusions while a session remains open, filter by bookmark or finding status, and navigate from a finding back to its source record without discarding unrelated investigation context. This builds on the advanced filtering and navigation, multi-session workspace, responsive background import, cancellation, large-document preview safeguards, and scalable timeline behavior completed in earlier milestones.

TraceScope is intended for file-based logs from applications, services, simulated devices, sensors, QA runs, field-support packages, and engineering test systems. It does not claim to automatically understand every arbitrary log format or guarantee a fixed maximum file size. Import behavior stays explicit, testable, and reproducible.

## Documentation

- [Expansion Roadmap](docs/expansion-roadmap.md) — product direction, phased implementation plan, release discipline, and scope boundaries
- [Performance Notes](docs/performance.md) — measured large-file scenarios, methodology, environment, and interpretation limits
- [Original Prototype Plan](docs/original-prototype-plan.md) — historical plan for the initial focused JSON Lines inspector

## Screenshots

The `v0.11.0` screenshots use the repository samples and reusable profiles to show the current import, multi-session investigation, advanced filtering and navigation, findings, timeline, recent-history, large-file, and export workflows. The structured XML engineering session remains the primary walkthrough source, while CSV and JSON Lines samples demonstrate related sessions and denser warning/error investigations.

### Import Configuration

Choose or drag in a source file, review the suggested format, load or edit a reusable profile, inspect mapped preview records and multiline raw source, and validate the configuration before importing.

![TraceScope Import Configuration](docs/screenshots/tracescope-import-configuration.png)

### TraceScope Dashboard

The main investigation view combines summary counts, advanced filtering controls, selectable timeline resolution, dynamic source-specific columns, grouped issue counts, event/issue navigation, bookmark and finding controls, visible/source-record position context, and selected-record details.

![TraceScope Dashboard](docs/screenshots/tracescope-dashboard.png)

### Multi-Session Workspace

Multiple imported sources can remain open together as independent investigation sessions. Switching tabs preserves each session's filters, model state, source context, import profile, diagnostics, presentation state, and session-local bookmarks, notes, and findings without re-importing the file.

![TraceScope Multi-Session Workspace](docs/screenshots/tracescope-multi-session-workspace.png)

### Advanced Filtering and Navigation

Canonical and source-specific filters can be combined across severity, subsystem, event code, entity, time range, search text, dynamic custom fields, bookmark state, and finding status. Named presets preserve reusable filter states, while adjacent-event and warning/error-class navigation move through the currently visible sorted investigation without losing source-record context.

![TraceScope Advanced Filtering and Navigation](docs/screenshots/tracescope-filtered-warnings.png)

### Findings Review

Bookmarks, analyst notes, and Open, Resolved, or Dismissed finding states create a lightweight investigation record within each open session. The Findings tab summarizes classified findings, preserves multiline analyst notes, and supports direct navigation back to the underlying source record while adapting only filters that would otherwise hide it.

![TraceScope Findings Review](docs/screenshots/tracescope-findings.png)

### Fine-Resolution Timeline Navigation

Manual fine-resolution buckets use a bounded visible window with horizontal navigation instead of attempting to render an unbounded number of chart buckets at once. Timeline labels, legend placement, and vertical scaling remain usable while navigating, and double-clicking a visible bar drills into its exact time bucket and represented severity when applicable.

![TraceScope Timeline Navigation](docs/screenshots/tracescope-timeline-navigation.png)

### Responsive Large-File Import

Large imports run outside the UI thread. Streamed importers can report determinate progress and support cancellation while the desktop interface remains responsive.

![TraceScope Large-File Import](docs/screenshots/tracescope-large-file-import.png)

### Recent Files

Persistent recent-file history keeps frequently revisited sources easy to reopen through the normal Import Configuration workflow. Recent import profiles are available from the Import Configuration dialog through the same validated profile-loading path.

![TraceScope Recent Files](docs/screenshots/tracescope-recent-files.png)

### Exported CSV

The export workflow writes the currently visible records to CSV using readable canonical headers and the configured names of mapped custom fields.

![TraceScope Exported CSV](docs/screenshots/tracescope-exported-csv.png)

## Downloads

Portable packages are published through [GitHub Releases](https://github.com/w-cook/tracescope-qt-log-inspector/releases).

The current `v0.11.0` package set uses:

```text
TraceScope-v0.11.0-windows-x64.zip
TraceScope-v0.11.0-linux-x86_64.AppImage
TraceScope-v0.11.0-samples.zip
```

Historical `v0.1.0` through `v0.10.0` prereleases remain available as earlier development milestones.

### Windows

1. Download `TraceScope-v0.11.0-windows-x64.zip`.
2. Extract the complete ZIP.
3. Launch `TraceScope.exe`.
4. Open a file from the included `samples` directory.

The package includes the required Qt libraries, plugins, MinGW runtime dependencies, demonstration logs, and reusable sample import profiles.

### Linux

1. Download `TraceScope-v0.11.0-linux-x86_64.AppImage`.
2. Make it executable:

```bash
chmod +x TraceScope-v0.11.0-linux-x86_64.AppImage
```

3. Launch it:

```bash
./TraceScope-v0.11.0-linux-x86_64.AppImage
```

### Sample Logs and Profiles

`TraceScope-v0.11.0-samples.zip` provides a platform-neutral copy of the repository samples and reusable profiles.

Representative source/profile pairs include:

- Structured XML: `structured-engineering-session.xml` with `structured-xml-engineering-session-profile.json`
- Windows Event XML: `windows-event-engineering-session.xml` with `windows-event-engineering-session-profile.json`
- CSV and TSV: `service-session.csv` / `telemetry-batch.tsv` with matching profiles
- Structured JSON: array and nested-document samples with matching profiles
- Syslog: RFC 3164 and RFC 5424 samples with matching profiles
- Web access logs: Apache Common, Apache/Nginx Combined, and IIS W3C samples
- Configurable text: regex-based application-log samples and a logfmt-style service session

The samples are intentionally varied so format detection, profile reuse, canonical mappings, custom fields, severity aliases, timestamps, preview behavior, filtering, timeline analysis, and export can all be exercised without external services.

Qt, Qt Creator, CMake, Git, and a local compiler are not required to run the packaged applications.

## Current Capabilities

### Import and Normalization

- Import JSON Lines, structured JSON documents and arrays, CSV, TSV, key-value/logfmt records, Syslog RFC 3164 and RFC 5424, IIS W3C logs, and structured XML
- Use reusable built-in profiles for Apache Common and Apache/Nginx Combined access logs
- Recognize Windows Event XML and apply Windows-oriented XML presets for common system fields
- Use regex-configurable profiles for line-oriented text logs that do not have a dedicated importer
- Suggest likely formats from file extension and source content where practical
- Normalize supported sources into a common `InvestigationRecord`
- Treat timestamp, severity, subsystem, event code, entity ID, and message as optional canonical fields
- Preserve raw source records, source file/record metadata, and stable record identities
- Preserve source-specific values as dynamic custom attributes
- Return structured import counts and diagnostics for malformed or partially mappable records
- Keep source-specific mapping rules in reusable, human-readable import profiles

### Import Profiles and Configuration

- Define versioned JSON import profiles
- Configure canonical field paths, named custom-field mappings, severity aliases, timestamp rules, and unmapped-field preservation
- Use record paths for structured JSON and XML documents that contain collections of records
- Validate profile structure and mapping configuration before import
- Select or drag-and-drop a source file and display a likely-format suggestion
- Create a fresh source-derived profile with automatic custom-field detection
- Preview a bounded set of mapped records without changing the active investigation session
- Inspect the complete raw source for the selected preview record in a vertically resizable area
- Keep the last valid preview visible while temporarily invalid edits are corrected
- Resize preview columns to keep long values readable
- Disable automatic preview for large structured documents so configuration remains responsive
- Generate large structured-document previews in the background and cancel obsolete preview work when the source or configuration changes
- Save reusable profiles and load them only after validation
- Reopen recently used profiles through persistent recent-profile history while retaining the same validation path
- Retain an intentional profile when switching sources so compatibility can be checked without silently resetting the configuration

### Large-File Responsiveness

- Run file importing outside the UI thread so the desktop interface remains responsive during parsing and result preparation
- Stream source reading for supported line-oriented and XML import paths instead of requiring the complete source byte stream in memory before parsing
- Report determinate byte/record progress when an importer can provide meaningful incremental progress
- Use indeterminate progress for complete-document processing such as structured JSON rather than presenting an artificial percentage
- Support cooperative cancellation for streamed imports without replacing the current investigation with partial results
- Keep normalized records, raw-source values, dynamic attributes, and investigation state resident for each open session rather than claiming constant-memory behavior

Measured Release-build scenarios cover representative JSON Lines, CSV, IIS W3C, Windows Event XML, and structured JSON investigations containing up to 220,000 records and approximately 109 MiB on the documented test system. These are measured observations rather than maximum supported file-size or throughput guarantees.

See [Performance Notes](docs/performance.md) for the measurement method, test environment, scenario results, cancellation checks, and interpretation boundaries.

### Multi-Session Workspace

- Keep multiple imported log sessions open within one application instance
- Switch and close sessions through a tabbed workspace without replacing unrelated investigation state
- Preserve an independent investigation controller, filters, source context, import profile, diagnostics, and cached presentation state for each session
- Reload a session in place using its existing source and import profile without creating a duplicate tab
- Refresh source metadata and investigation data on reload while retaining applicable filters and stable session identity
- Preserve the existing session unchanged when a reload is cancelled
- Maintain persistent, bounded, deduplicated recent-file and recent-profile history through local application settings
- Reopen recent files through Import Configuration rather than silently importing them, and remove stale recent paths when their menus are opened

### Investigation Workflow

- Display investigation records in a sortable Qt model/view table
- Show canonical fields only when they are present in the loaded dataset
- Add discovered source-specific attributes as dynamic columns
- Filter by multiple severities and by subsystem
- Expose event-code and entity filters only when those canonical values are present
- Filter by optional UTC time-range bounds when timestamps are available
- Add multiple exact-value custom-field criteria, using OR semantics for multiple values of the same field and AND semantics across different fields and other filter categories
- Add exact-value custom-field filters directly from visible table cells through the context menu
- Search case-insensitively across canonical fields and custom attributes
- Reset the complete active filter state
- Save, overwrite, apply, and delete persistent named filter presets stored in local application settings
- Restore reusable preset criteria safely across sessions, including bookmark-only and finding-status filters, while ignoring criteria unavailable in the active investigation
- Apply complete filter-state changes as one model update to avoid repeated large-dataset proxy resets
- Preserve correct selected-record mapping after sorting and filtering
- Inspect canonical fields, custom attributes, and raw source for the selected record
- Bookmark source records through stable record identities and filter the active investigation to bookmarked records
- Add multiline analyst notes without blocking inspection of other source records
- Classify records as Open, Resolved, or Dismissed findings and filter by one or more finding statuses
- Review classified findings in a dedicated panel with status counts, source-record context, timestamps, and preserved analyst notes
- Navigate from a finding back to its exact source record, adapting only active filters that would otherwise hide the target
- Retain applicable bookmark, note, and finding state through in-place reloads when stable record identities survive; disk-backed workspace persistence remains a later phase
- Navigate to previous/next visible events in the current filtered and sorted order
- Navigate cyclically between visible warning, error, and critical records
- Show visible event position together with the preserved source-record number
- View session-level event counts and grouped warning/error summaries
- Double-click grouped issue-summary cells to narrow the investigation by subsystem and represented issue class without discarding unrelated active filters
- Visualize filtered event counts with automatic or manually selected timeline resolutions from millisecond through day-scale intervals
- Preserve empty timeline intervals within the displayed range so gaps remain visible
- Use bounded windowed rendering and horizontal navigation when fine resolutions would otherwise produce too many timeline buckets
- Double-click timeline bars to narrow the investigation to the exact represented time bucket and severity when applicable
- Keep timeline range context, legend placement, and vertical scaling stable while navigating

### Export, Samples, and Verification

- Export the currently visible investigation records to CSV
- Use user-facing canonical column names and configured custom-field names in CSV output
- Preserve deterministic custom-column ordering and blank cells for attributes absent from individual records
- Retain CSV escaping and compact JSON serialization for structured custom values
- Include demonstration logs and reusable profiles for every major supported source family
- Build and test on Windows and Linux through GitHub Actions
- Produce portable Windows x64, Linux AppImage, and platform-neutral sample packages
- Verify representative packaged samples and smoke-test the packaged Windows and Linux applications in CI

The desktop UI consumes `InvestigationRecord` data directly for the primary table, filtering, searching, selected-record inspection, and CSV export. Existing telemetry-oriented summary and timeline analysis remain connected through a compatibility adapter while later phases continue reducing legacy assumptions where useful.

## Import Model

### Canonical Investigation Fields

TraceScope normalizes source data into six optional canonical fields:

- `timestamp`
- `severity`
- `subsystem`
- `eventCode`
- `entityId`
- `message`

A source record can remain useful when one or more canonical fields are missing. A feature that depends on a missing field may be unavailable, while unrelated inspection, search, and source-specific fields remain usable.

### Supported Formats and Profiles

Different source formats need different amounts of configuration. TraceScope keeps that distinction visible rather than pretending every file can be interpreted automatically.

| Source family | How TraceScope handles it |
| --- | --- |
| JSON Lines | Dedicated importer with configurable field paths |
| Structured JSON | Imports arrays or records selected from nested documents |
| CSV / TSV | Delimited-text import with header-based field mapping |
| Regex-configurable text | Named regular-expression captures map line-oriented text into fields |
| Key-value / logfmt | Parses key-value records and maps discovered fields |
| Syslog | Dedicated RFC 3164 and RFC 5424 parsing |
| Apache / Nginx access logs | Reusable built-in regex profiles for common access-log layouts |
| IIS W3C | Dedicated importer with W3C field-header support |
| Structured XML | Nested elements, attributes, repeated elements, record paths, and raw XML preservation |
| Windows Event XML | Uses the XML importer with Windows Event detection, presets, severity aliases, and named `EventData` fields |

Import profiles map source-specific field names into the optional canonical fields and can also promote useful source values into readable custom columns. Values that are not explicitly mapped can still be preserved.

Windows Event XML support covers XML-formatted events and collections. Native binary `.evtx` ingestion is not part of `v0.11.0`.

## Development Status

Implemented expansion milestones:

| Version | Milestone |
| --- | --- |
| `v0.1.0` | Original prototype and cross-platform packaging baseline |
| `v0.2.0` | Flexible investigation-record and import domain |
| `v0.3.0` | Importer abstraction and configurable JSON Lines |
| `v0.4.0` | Qt model/view architecture and dynamic custom-attribute UI |
| `v0.4.1` | Investigation-record CSV export with dynamic attributes |
| `v0.5.0` | Versioned import profiles, validation, serialization, profile-aware importing, and preview logic |
| `v0.6.0` | Desktop import configuration, mapping-aware preview, reusable profile save/load, and sample profiles |
| `v0.7.0` | Additional built-in formats, format detection/presets, and expanded cross-format samples |
| `v0.8.0` | Responsive large-file import, progress/cancellation, scalable timeline resolution/navigation, and measured performance scenarios |
| `v0.9.0` | Multi-session workspace, per-session context and reload, persistent recent files/profiles, and session-switching responsiveness |
| `v0.10.0` | Advanced canonical/custom filtering, persistent filter presets, event/issue navigation, and summary/timeline drill-down |
| `v0.11.0` | Session-local bookmarks, analyst notes, finding status, findings review, bookmark/finding filtering, and source-record navigation |

The current release is **`v0.11.0`**. **Phase 11 — Analytics and Burst Detection** is now active. Later phases add session comparison, workspace persistence, live file following, reporting, responsive display hardening, and the stable `v1.0.0` release.

Planned capabilities are not presented as implemented until their corresponding phases are completed and verified.

## Tech Stack

- C++17
- Qt 6
- Qt Widgets
- Qt Charts
- Qt Model/View
- Qt Concurrent
- CMake
- Qt Test
- MinGW 64-bit on Windows
- GCC on Linux
- GitHub Actions
- `windeployqt`
- `linuxdeploy`
- AppImage

The trusted local Windows development baseline uses Qt 6.11.1 with MinGW 64-bit. Continuous integration uses a pinned Qt 6.10.3 environment on Windows and Linux.

## Project Structure

```text
.github/
└── workflows/
    └── ci.yml                     # Windows, Linux, and sample packaging

docs/
├── original-prototype-plan.md    # Historical initial implementation plan
├── expansion-roadmap.md          # Expansion architecture and phased roadmap
├── performance.md                # Measured large-file scenarios and interpretation
└── screenshots/                  # Portfolio screenshots

packaging/
└── linux/
    ├── tracescope.desktop         # Linux desktop metadata
    └── tracescope.svg             # Application icon

samples/                           # Demonstration logs and reusable profiles
└── profiles/                      # Versioned sample import profiles

src/
├── analysis/                      # Grouped issue and timeline analysis
├── compatibility/                # Flexible-record adapters for legacy telemetry components
├── controllers/                  # Investigation model/proxy coordination
├── domain/                       # Telemetry and flexible investigation-record domain
├── exporting/                    # Investigation-record CSV export
├── filtering/                    # Legacy telemetry filtering retained for compatibility/tests
├── importing/                    # Importers, profiles, validation, preview, results, and diagnostics
├── models/                       # Investigation table and filter proxy models
├── parsing/                      # JSON Lines compatibility facade
├── preferences/                  # Persistent recent histories and filter presets
├── ui/                           # Import configuration interface
├── workspace/                    # Multi-session workspace, per-session context, and investigation state
├── MainWindow.cpp                # Qt Widgets presentation and workflow orchestration
├── MainWindow.h
└── main.cpp

tests/                             # Qt Test coverage for core application logic
```

## Building Locally

### Requirements

- Qt 6
- Qt Charts
- CMake
- a C++17-compatible compiler

### Qt Creator

1. Open the root `CMakeLists.txt`.
2. Select a Qt 6 kit that includes Qt Charts.
3. Configure and build the `TraceScope` target.
4. Run the application.
5. Open a file from `samples`.

### Command Line

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The exact executable location varies by generator, platform, and development environment.

## Running Tests

After configuring the project:

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The current CTest suite contains 30 executables:

- `ParserTests`
- `RecordSeverityTests`
- `InvestigationRecordTests`
- `ImportResultTests`
- `FilterTests`
- `IssueAnalyzerTests`
- `CsvExporterTests`
- `EventTimelineAnalyzerTests`
- `ImporterRegistryTests`
- `JsonLinesImporterTests`
- `DelimitedTextImporterTests`
- `StructuredJsonImporterTests`
- `RegexTextImporterTests`
- `KeyValueTextImporterTests`
- `SyslogImporterTests`
- `IisW3cImporterTests`
- `XmlImporterTests`
- `InvestigationTableModelTests`
- `InvestigationFilterProxyModelTests`
- `InvestigationControllerTests`
- `ImportProfileTests`
- `ImportProfileValidatorTests`
- `ImportProfileSerializationTests`
- `ImportPreviewServiceTests`
- `ImportFormatSuggestionServiceTests`
- `InvestigationSessionTests`
- `InvestigationWorkspaceTests`
- `InvestigationStateStoreTests`
- `RecentItemsStoreTests`
- `FilterPresetStoreTests`

Coverage includes the flexible record domain, import results and diagnostics, each built-in importer family, import execution progress/cancellation, profile validation and serialization, preview and format-suggestion behavior, advanced filtering and analysis, complete filter-state batching, filter-preset persistence, bookmark and finding filtering, stable-record investigation state, event/issue/finding navigation, dynamic CSV export, Qt model/view behavior, proxy/source mapping, controller coordination, per-session state, multi-session workspace behavior, reload semantics, and persistent recent-item history.

The same CTest suite runs in GitHub Actions on Windows and Linux.

## Continuous Integration and Packaging

The GitHub Actions workflow runs three parallel jobs with read-only repository permissions:

- **Windows x64:** pinned Qt/MinGW Release build, CTest, `windeployqt`, package verification, startup smoke test, and `TraceScope-v0.11.0-windows-x64.zip`
- **Linux x86_64:** pinned Qt/GCC Release build on Ubuntu 22.04, CTest, `linuxdeploy`, AppImage verification, offscreen startup smoke test, and `TraceScope-v0.11.0-linux-x86_64.AppImage`
- **Samples:** verifies representative source/profile pairs and packages the complete `samples` directory as `TraceScope-v0.11.0-samples.zip`

Workflow artifacts validate candidate packages. Approved packages are attached permanently to GitHub Releases.

## Design Goals

TraceScope emphasizes practical native desktop development, clear separation between importing, investigation data, presentation, analysis, and export, testable non-UI logic, explicit source mapping, raw-source preservation, offline operation, conservative product claims, responsive investigation workflows, and repeatable cross-platform releases.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

TraceScope uses Qt Charts, which is available to open-source users under the GNU General Public License v3.0.
