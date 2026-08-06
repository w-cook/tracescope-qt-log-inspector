# TraceScope — Qt Telemetry Log Inspector

[![TraceScope CI](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml/badge.svg)](https://github.com/w-cook/tracescope-qt-log-inspector/actions/workflows/ci.yml)

TraceScope is a native Qt/C++ desktop application for loading, filtering, visualizing, inspecting, and exporting structured telemetry and diagnostic log files.

The completed original prototype provides a focused investigation workflow for a built-in JSON Lines telemetry format. TraceScope is now being expanded into a configurable native log-analysis workbench with multiple built-in formats and reusable import profiles.

TraceScope is intended for file-based logs produced by applications, services, simulated devices, sensors, QA runs, field-support packages, and engineering test systems.

## Downloads

Portable packages are published through [GitHub Releases](https://github.com/w-cook/tracescope-qt-log-inspector/releases).

The original prototype package set uses these filenames:

```text
TraceScope-v0.1.0-windows-x64.zip
TraceScope-v0.1.0-linux-x86_64.AppImage
TraceScope-v0.1.0-samples.zip
```

### Windows

1. Download `TraceScope-v0.1.0-windows-x64.zip`.
2. Extract the complete ZIP to a local directory.
3. Launch `TraceScope.exe`.
4. Open a file from the included `samples` directory.

The Windows package includes the required Qt libraries, plugins, and MinGW runtime dependencies.

### Linux

1. Download `TraceScope-v0.1.0-linux-x86_64.AppImage`.
2. Make the file executable:

```bash
chmod +x TraceScope-v0.1.0-linux-x86_64.AppImage
```

3. Launch it:

```bash
./TraceScope-v0.1.0-linux-x86_64.AppImage
```

The AppImage contains TraceScope and its required Qt dependencies. The same demonstration logs are also available through the standalone sample archive.

### Sample Logs

Download `TraceScope-v0.1.0-samples.zip` for a platform-neutral copy of all included demonstration logs.

After extraction, the archive contains:

```text
TraceScope-v0.1.0-samples/
├── README.md
├── LICENSE
└── samples/
    └── included JSON Lines telemetry sessions
```

Qt, Qt Creator, CMake, Git, and a local compiler are not required to run the packaged applications.

## Current Features

- Open local JSON Lines telemetry log files
- Parse structured records into C++ domain objects
- Display telemetry events in a sortable table
- Filter events by severity
- Filter events by subsystem
- Search across timestamp, level, subsystem, event code, message, and entity ID
- Inspect complete details for a selected event
- View session-level event counts
- Group warnings and errors by subsystem
- Visualize filtered event counts over time
- Export the currently visible filtered results to CSV
- Use included sample files for demonstration and testing
- Build and test the application on Windows and Linux through GitHub Actions
- Produce a portable Windows x64 ZIP through automated CI
- Produce a portable Linux x86_64 AppImage through automated CI
- Produce a platform-neutral sample-log ZIP
- Smoke-test packaged Windows and Linux applications in CI

## Screenshots

### TraceScope Dashboard

The main dashboard combines session summary information, filtering controls, an event-count timeline, the telemetry event table, grouped issue counts, and selected-event details.

![TraceScope Dashboard](docs/screenshots/tracescope-dashboard.png)

### Filtered Warnings

Severity, subsystem, and text filters update the event table, summary information, grouped issue panel, and timeline chart together.

![TraceScope Filtered Warnings](docs/screenshots/tracescope-filtered-warnings.png)

### Exported CSV

The export workflow writes the currently filtered event set to a CSV file for additional review or sharing.

![TraceScope Exported CSV](docs/screenshots/tracescope-exported-csv.png)

## Expansion in Progress

TraceScope is being expanded from a fixed-schema telemetry inspector into a configurable native desktop workbench for importing and investigating developer logs.

The planned product position is:

> TraceScope supports multiple built-in log formats and reusable import profiles that map source fields into a common investigation model.

The expansion will not claim to automatically understand every arbitrary log format. Import behavior will remain explicit and reproducible through built-in importers and versioned profiles.

Planned capabilities include:

- configurable JSON Lines imports
- CSV and TSV imports
- structured JSON document imports
- regex-configurable plain-text imports
- optional canonical investigation fields
- preservation of custom source attributes
- reusable import profiles
- import preview and diagnostics
- dynamic table columns
- responsive large-file processing
- multiple investigation sessions
- advanced filtering and navigation
- bookmarks, notes, and findings
- deterministic burst analysis
- session comparison
- local workspace persistence
- live file following
- offline investigation reports

See the [TraceScope Expansion Roadmap](docs/expansion-roadmap.md) for the planned architecture and development phases.

Planned features are not presented as implemented until their corresponding phases are completed, tested, packaged, and released.

## Tech Stack

- C++17
- Qt 6
- Qt Widgets
- Qt Charts
- CMake
- Qt Test
- MinGW 64-bit on Windows
- GCC on Linux
- GitHub Actions
- `windeployqt`
- `linuxdeploy`
- AppImage

The trusted local Windows development baseline uses Qt 6.11.1 with MinGW 64-bit. Continuous integration currently uses a pinned Qt 6.10.3 environment on Windows and Linux.

## Current Sample Format

The original prototype reads newline-delimited JSON records. Each line represents one telemetry event:

```json
{"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"}
```

The current built-in schema includes:

- `timestamp`
- `level`
- `subsystem`
- `eventCode`
- `message`
- `entityId`

Sample files are included in the repository’s `samples` directory, the Windows package, the Linux AppImage, and the standalone sample archive.

Flexible field mapping and additional formats are part of the expansion roadmap and are not yet implemented in the original prototype.

## Project Structure

```text
.github/
└── workflows/
    └── ci.yml                     # Windows, Linux, and sample packaging workflow

docs/
├── original-prototype-plan.md    # Historical initial implementation plan
├── expansion-roadmap.md          # Current expansion architecture and phases
└── screenshots/                   # Portfolio screenshots

packaging/
└── linux/
    ├── tracescope.desktop         # Linux desktop metadata
    └── tracescope.svg             # Application icon

samples/                            # Example telemetry sessions

src/
├── analysis/                      # Grouped issue and timeline analysis
├── domain/                        # Telemetry event model
├── exporting/                     # Filtered CSV export
├── filtering/                     # Severity, subsystem, and text filtering
├── parsing/                       # JSON Lines parsing
├── MainWindow.cpp                 # Current Qt Widgets UI orchestration
├── MainWindow.h
└── main.cpp

tests/                              # Qt Test coverage for core application logic
```

## Building Locally

### Requirements

- Qt 6
- Qt Charts
- CMake
- a C++17-compatible compiler

### Qt Creator

1. Open the root `CMakeLists.txt` in Qt Creator.
2. Select a Qt 6 kit that includes Qt Charts.
3. Configure the project.
4. Build the `TraceScope` target.
5. Run the application.
6. Open one of the included files from the `samples` directory.

### Command Line

Use a terminal or developer environment in which Qt, CMake, and the selected compiler are configured correctly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run the generated `TraceScope` executable from the build directory.

The exact executable location may vary by generator, platform, and development environment.

## Running Tests

After configuring the project:

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The current CTest suite includes:

- `ParserTests`
- `FilterTests`
- `IssueAnalyzerTests`
- `CsvExporterTests`
- `EventTimelineAnalyzerTests`

Together, these tests cover:

- JSON Lines parsing
- file parsing
- event filtering
- grouped warning and error analysis
- timeline bucket analysis
- CSV export

The same CTest suite runs in GitHub Actions on Windows and Linux.

## Continuous Integration and Packaging

The GitHub Actions workflow runs three parallel jobs with read-only repository permissions.

### Windows x64

- installs a pinned Qt and MinGW environment
- configures a Release build
- builds the application
- runs all registered CTest tests
- deploys Qt and compiler dependencies with `windeployqt`
- verifies required runtime files
- includes documentation and sample logs
- creates `TraceScope-v0.1.0-windows-x64.zip`
- extracts and starts the packaged executable
- uploads the ZIP as a workflow artifact

### Linux x86_64

- runs on a pinned Ubuntu 22.04 environment
- installs the same pinned Qt version
- configures a Release build with GCC
- builds the application
- runs all registered CTest tests
- assembles an AppDir
- deploys Qt dependencies with `linuxdeploy`
- creates `TraceScope-v0.1.0-linux-x86_64.AppImage`
- starts the AppImage using the offscreen Qt platform
- uploads the AppImage as a workflow artifact

### Sample Logs

- verifies that the repository contains sample files
- copies all samples into a platform-neutral package
- verifies that the packaged count matches the source count
- creates `TraceScope-v0.1.0-samples.zip`
- uploads the ZIP as a workflow artifact

Workflow artifacts are used to validate candidate packages. Approved packages are attached permanently to GitHub Releases.

## Current Status

### Original Prototype

The original prototype is complete for its fixed JSON Lines telemetry schema.

Implemented:

- JSON Lines import
- telemetry event parsing
- event table display
- severity and subsystem filtering
- full-field text search
- selected-event details
- grouped warning and error summaries
- filter-aware event-count timeline chart
- filtered CSV export
- sample telemetry sessions
- automated core-logic tests
- cross-platform Windows and Linux CI
- portable Windows x64 packaging
- portable Linux x86_64 AppImage packaging
- standalone sample-log packaging
- packaged-application startup smoke tests

### Expansion

The configurable workbench expansion is in progress.

Current expansion priorities are:

1. publish the packaged original-prototype release baseline;
2. introduce a flexible canonical investigation record;
3. move JSON Lines behavior behind a reusable importer abstraction;
4. replace the fixed table with Qt’s model/view architecture;
5. add versioned import profiles and import configuration workflows.

See the [expansion roadmap](docs/expansion-roadmap.md) for the complete planned sequence.

## Design Goals

TraceScope emphasizes:

- practical native desktop development
- clear separation of parsing, filtering, analysis, exporting, and UI concerns
- testable non-UI application logic
- explicit and reproducible import behavior
- preservation of source information
- responsive investigation workflows
- offline operation
- conservative and defensible product claims
- repeatable cross-platform releases

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

TraceScope uses Qt Charts, which is available to open-source users under the GNU General Public License v3.0.
