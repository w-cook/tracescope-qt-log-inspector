# TraceScope — Qt Telemetry Log Inspector

[![TraceScope CI](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml/badge.svg)](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml)

TraceScope is a native C++/Qt desktop application for importing, normalizing, filtering, inspecting, visualizing, and exporting structured telemetry and diagnostic logs.

The project began as a focused JSON Lines telemetry inspector and is being expanded into a configurable offline log-analysis workbench. The current `v0.5.0` milestone adds reusable, versioned import profiles and profile-aware preview logic on top of the flexible investigation-record, importer, and Qt model/view foundations delivered in earlier releases.

TraceScope is designed for file-based logs produced by applications, services, simulated devices, sensors, QA runs, field-support packages, and engineering test systems. Import behavior is explicit and reproducible rather than presented as automatic understanding of arbitrary log formats.

## Downloads

Portable packages are published through [GitHub Releases](https://github.com/w-cook/tracescope-qt-log-inspector/releases).

The `v0.5.0` package set uses:

```text
TraceScope-v0.5.0-windows-x64.zip
TraceScope-v0.5.0-linux-x86_64.AppImage
TraceScope-v0.5.0-samples.zip
```

Historical `v0.1.0` through `v0.4.1` prereleases remain available as earlier development milestones.

### Windows

1. Download `TraceScope-v0.5.0-windows-x64.zip`.
2. Extract the complete ZIP.
3. Launch `TraceScope.exe`.
4. Open a file from the included `samples` directory.

The package includes the required Qt libraries, plugins, and MinGW runtime dependencies.

### Linux

1. Download `TraceScope-v0.5.0-linux-x86_64.AppImage`.
2. Make it executable:

```bash
chmod +x TraceScope-v0.5.0-linux-x86_64.AppImage
```

3. Launch it:

```bash
./TraceScope-v0.5.0-linux-x86_64.AppImage
```

### Sample Logs

`TraceScope-v0.5.0-samples.zip` provides a platform-neutral copy of the demonstration logs. The primary flexible-record demonstration, `dynamic-attributes-session.jsonl`, mixes canonical fields with heterogeneous custom attributes across multiple timeline minutes so dynamic columns, search, sorting, filtering, timeline updates, selected-record details, and CSV export can be exercised together.

Qt, Qt Creator, CMake, Git, and a local compiler are not required to run the packaged applications.

## Current Capabilities

### Import and Normalization

- Import newline-delimited JSON through a dedicated `JsonLinesImporter`
- Normalize source records into a flexible `InvestigationRecord`
- Treat timestamp, severity, subsystem, event code, entity ID, and message as optional canonical fields
- Preserve raw source records, source file/record metadata, and deterministic stable record identities
- Preserve noncanonical JSON values as dynamic custom attributes
- Return structured import counts and diagnostics for malformed or partially mappable records
- Resolve canonical JSON fields through configurable dot-delimited top-level or nested object paths
- Preserve the original JSON Lines layout as the default mapping for backward compatibility
- Register file importers through a common `ILogImporter` abstraction and internal importer registry

### Import Profiles and Preview Logic

- Define reusable import profiles with a versioned schema
- Configure canonical field mappings and explicit custom-field mappings
- Configure source-specific severity aliases
- Configure ordered ISO 8601 and Qt-format timestamp parsing rules
- Control whether unmapped JSON fields are preserved as custom attributes
- Validate profile structure and mapping configuration before import
- Serialize and deserialize profiles as human-readable JSON
- Round-trip complete profile configuration through automated tests
- Apply profiles directly to JSON Lines imports
- Preview a bounded set of source records without changing the active investigation session
- Return preview records, source metadata, counts, truncation state, and import diagnostics through the existing import-result model

These profile capabilities are currently implemented and tested at the service/importer layer. The desktop workflow for configuring, validating, saving, loading, and previewing profiles is the next development phase.

### Investigation Workflow

- Display flexible investigation records in a `QTableView` backed by `QAbstractTableModel`
- Add discovered custom attributes as dynamic columns alongside the canonical columns
- Sort canonical fields using typed timestamp and severity values
- Sort and filter through `QSortFilterProxyModel`
- Filter by severity and subsystem
- Search case-insensitively across canonical fields and custom-attribute values
- Preserve correct selected-record mapping after sorting and filtering
- Inspect canonical fields and dynamic custom attributes for the selected record
- Coordinate record, filter, subsystem, visibility, and proxy/source operations through `InvestigationController`
- View session-level event counts and grouped warning/error summaries
- Visualize filtered event counts in minute-based timeline buckets

### Export, Samples, and Verification

- Export the currently visible investigation records to CSV
- Preserve canonical and dynamic custom-attribute columns in CSV output
- Use deterministic custom-column ordering and blank cells for attributes absent from individual records
- Retain CSV escaping and compact JSON serialization for structured custom values
- Include demonstration files for testing and portfolio review
- Build and test on Windows and Linux through GitHub Actions
- Produce portable Windows x64, Linux AppImage, and sample-log packages
- Smoke-test packaged Windows and Linux applications in CI

The desktop UI consumes `InvestigationRecord` data directly for the primary table, filtering, searching, selected-record inspection, and CSV export. Existing telemetry-oriented summary and timeline components remain connected through a compatibility adapter while later phases continue replacing legacy assumptions where useful.

## Screenshots

### TraceScope Dashboard

The main dashboard combines session summary information, filtering controls, an event-count timeline, the investigation table, grouped issue counts, and selected-record details.

![TraceScope Dashboard](docs/screenshots/tracescope-dashboard.png)

### Filtered Warnings

Severity, subsystem, and text filters update the event table, summary information, grouped issue panel, and timeline chart together.

![TraceScope Filtered Warnings](docs/screenshots/tracescope-filtered-warnings.png)

### Exported CSV

The export workflow writes the currently filtered investigation-record set to CSV, preserving canonical fields and the custom attributes present in the visible records.

![TraceScope Exported CSV](docs/screenshots/tracescope-exported-csv.png)

## Import Model

### Canonical Investigation Fields

TraceScope normalizes source data into six optional canonical fields:

- `timestamp`
- `severity`
- `subsystem`
- `eventCode`
- `entityId`
- `message`

A source record can remain useful when one or more canonical fields are missing. Features that depend on a missing field may be unavailable, while unrelated inspection and search behavior can continue to use the preserved source content.

### JSON Lines

The currently exposed built-in source format is newline-delimited JSON, with one source record per line:

```json
{"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"}
```

The default mapping preserves compatibility with the original field layout:

- `timestamp` → timestamp
- `level` → severity
- `subsystem` → subsystem
- `eventCode` → event code
- `entityId` → entity ID
- `message` → message

Import profiles can instead map alternate or nested paths such as `metadata.occurredAt`, `event.code`, or `context.requestId`. Explicit custom-field mappings can promote nested values into named custom attributes, while profile configuration controls whether other unmapped top-level values are preserved.

For example:

```json
{"timestamp":"2026-08-08T14:02:25.115Z","level":"ERROR","subsystem":"Payments","eventCode":"UPSTREAM_TIMEOUT","message":"Payment provider request timed out","entityId":"PAY-8841","host":"api-03","environment":"staging","latencyMs":5032,"provider":"sandbox-payments"}
```

Here, the six canonical fields are normalized into the common investigation model while `host`, `environment`, `latencyMs`, and `provider` remain available as custom attributes under the default preservation behavior.

## Development Status

Completed expansion milestones:

| Version | Milestone |
| --- | --- |
| `v0.1.0` | Original prototype and cross-platform packaging baseline |
| `v0.2.0` | Flexible investigation-record and import domain |
| `v0.3.0` | Importer abstraction and configurable JSON Lines |
| `v0.4.0` | Qt model/view architecture and dynamic custom-attribute UI |
| `v0.4.1` | Investigation-record CSV export with dynamic attributes |
| `v0.5.0` | Versioned import profiles, validation, serialization, profile-aware importing, and preview logic |

The next phase is **Import Configuration Interface**, which will expose the implemented profile and preview services through the desktop workflow. Planned later phases add additional built-in formats, responsive large-file processing, multi-session investigations, advanced filtering/navigation, findings, deterministic analytics, session comparison, persistence, live file following, and broader reporting.

See the [TraceScope Expansion Roadmap](docs/expansion-roadmap.md) for the detailed architecture, phase deliverables, release discipline, and scope boundaries.

Planned capabilities are not presented as implemented until their corresponding phases are completed, tested, packaged, and released.

## Tech Stack

- C++17
- Qt 6
- Qt Widgets
- Qt Charts
- Qt Model/View
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
└── screenshots/                  # Portfolio screenshots

packaging/
└── linux/
    ├── tracescope.desktop         # Linux desktop metadata
    └── tracescope.svg             # Application icon

samples/                           # Demonstration JSON Lines sessions

src/
├── analysis/                      # Grouped issue and timeline analysis
├── compatibility/                # Flexible-record adapters for legacy telemetry components
├── controllers/                  # Investigation model/proxy coordination
├── domain/                       # Telemetry and flexible investigation-record domain
├── exporting/                    # Investigation-record CSV export
├── filtering/                    # Legacy telemetry filtering retained for compatibility/tests
├── importing/                    # Importers, profiles, validation, serialization, preview, results, diagnostics
├── models/                       # Investigation table and filter proxy models
├── parsing/                      # JSON Lines compatibility facade
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

The current CTest suite contains 17 executables:

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
- `InvestigationTableModelTests`
- `InvestigationFilterProxyModelTests`
- `InvestigationControllerTests`
- `ImportProfileTests`
- `ImportProfileValidatorTests`
- `ImportProfileSerializationTests`
- `ImportPreviewServiceTests`

Coverage includes flexible-record parsing and identity, import results and diagnostics, configurable JSON mappings, import-profile behavior and validation, JSON profile serialization/round trips, bounded profile-aware previews, legacy compatibility, filtering and analysis, dynamic CSV export, Qt model/view behavior, proxy/source mapping, and controller coordination.

The same CTest suite runs in GitHub Actions on Windows and Linux.

## Continuous Integration and Packaging

The GitHub Actions workflow runs three parallel jobs with read-only repository permissions:

- **Windows x64:** pinned Qt/MinGW Release build, CTest, `windeployqt`, package verification, startup smoke test, and `TraceScope-v0.5.0-windows-x64.zip`
- **Linux x86_64:** pinned Qt/GCC Release build on Ubuntu 22.04, CTest, `linuxdeploy`, AppImage verification, offscreen startup smoke test, and `TraceScope-v0.5.0-linux-x86_64.AppImage`
- **Sample Logs:** verifies and packages the repository samples as `TraceScope-v0.5.0-samples.zip`

Workflow artifacts validate candidate packages. Approved packages are attached permanently to GitHub Releases.

## Design Goals

TraceScope emphasizes practical native desktop development, clear separation of importing/domain/model-view/analysis/export/UI concerns, testable non-UI logic, explicit and reproducible import behavior, source preservation, offline operation, conservative product claims, and repeatable cross-platform releases.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

TraceScope uses Qt Charts, which is available to open-source users under the GNU General Public License v3.0.
