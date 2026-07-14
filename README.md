# TraceScope — Qt Telemetry Log Inspector

TraceScope is a native Qt/C++ desktop application for loading, filtering, visualizing, inspecting, and exporting structured telemetry or diagnostic log files.

It is designed as a practical diagnostic utility for file-based logs produced by simulated devices, services, sensors, QA runs, field-support packages, or engineering test systems.

## Features

- Open local JSON Lines telemetry log files
- Parse structured log records into C++ domain objects
- Display telemetry events in a sortable table
- Filter events by severity level
- Filter events by subsystem
- Search across timestamp, level, subsystem, event code, message, and entity ID
- Inspect the complete details of a selected event
- View session-level event counts
- Group warnings and errors by subsystem
- Visualize filtered event counts over time
- Export the currently visible filtered results to CSV
- Use included sample log files for local demonstration and testing

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

## Tech Stack

- C++17
- Qt 6
- Qt Widgets
- Qt Charts
- CMake
- Qt Test

## Sample Log Format

TraceScope v1 reads newline-delimited JSON records. Each line represents one telemetry event:

```json
{"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"}
```

Each event includes:

- `timestamp`
- `level`
- `subsystem`
- `eventCode`
- `message`
- `entityId`

Sample files are included in the `samples/` directory.

## Project Structure

```text
src/
├── analysis/    # Grouped issue counts and event timeline analysis
├── domain/      # Telemetry event and session models
├── exporting/   # Filtered CSV export
├── filtering/   # Severity, subsystem, and text filtering
├── parsing/     # JSON Lines parsing
└── ui/          # Qt Widgets interface

tests/           # Qt Test coverage for core logic
samples/         # Example telemetry sessions
docs/screenshots # Portfolio screenshots
```

## Running Locally

### Requirements

- Qt 6
- Qt Charts
- CMake
- A C++17-compatible compiler

### Qt Creator

1. Open the repository's root `CMakeLists.txt` in Qt Creator.
2. Select a Qt 6 kit that includes Qt Charts.
3. Configure and build the project.
4. Run the `TraceScope` target.
5. Open one of the included files from the `samples/` directory.

### Command Line

From the repository root, using a configured development environment:

```bash
cmake -S . -B build
cmake --build build
```

Run the generated `TraceScope` executable from the build directory.

## Running Tests

From the repository root:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers core behavior including:

- JSON Lines parsing
- file parsing
- event filtering
- grouped warning and error analysis
- timeline bucket analysis
- CSV export

## Current Status

TraceScope v1 is feature-complete for portfolio demonstration.

Implemented:

- JSON Lines file import
- telemetry event parsing
- event table display
- severity and subsystem filtering
- full-field text search
- selected event details
- grouped warning and error summaries
- filter-aware event-count timeline chart
- CSV export for filtered results
- sample telemetry sessions
- automated tests for core parser, filtering, analysis, and export logic

Not implemented:

- packaged installer or release bundle
- multi-file session comparison
- event bookmarks or annotations
- advanced burst or anomaly detection
- live log streaming

## Design Goals

TraceScope was built to demonstrate practical native application development rather than a production monitoring platform. The project emphasizes:

- clear separation between parsing, filtering, analysis, exporting, and UI concerns
- testable non-UI application logic
- a compact diagnostic workflow for inspecting structured logs
- responsive filtering across multiple views
- an employer-facing C++/Qt portfolio project with conservative, defensible claims

## Possible Future Improvements

- Compare multiple telemetry sessions
- Add bookmarks and analyst notes
- Detect warning or error bursts
- Support additional structured log formats
- Add configurable chart intervals
- Package the application for easier installation
- Add live or streaming log ingestion

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

TraceScope uses Qt Charts, which is available to open-source users under the GNU General Public License v3.0.
