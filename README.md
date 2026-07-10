# TraceScope — Qt Telemetry Log Inspector

TraceScope is a native Qt/C++ desktop application for loading, filtering, visualizing, and inspecting structured telemetry or diagnostic log files.

The project is intended to demonstrate a practical native desktop workflow for engineers, QA testers, support teams, and field technicians who need to inspect logs from simulated devices, services, sensors, or diagnostic systems.

## Planned v1 Scope

- Open/import local JSON Lines telemetry log files
- Parse structured log records into C++ domain objects
- Display events in a table
- Filter by severity level
- Filter by subsystem
- Search event text
- Display selected event details
- Show session summary counts
- Export filtered results
- Include unit tests for parser and filtering logic
- Include sample log files and screenshots

## Current Status

Implemented:
- Qt Widgets application scaffold
- CMake-based build setup
- Basic main window shell
- Telemetry event domain type
- JSON Lines parser for structured telemetry events
- Parser behavior for valid records, empty lines, and malformed JSON
- Qt Test coverage for parser foundation
- Sample telemetry log file
- File open/import workflow for local log files
- Table display for parsed telemetry events
- Basic session summary counts by severity
- Parser support for reading JSON Lines files from disk
- Severity filtering
- Subsystem filtering
- Text search across event fields
- Filtered summary count display
- Qt Test coverage for filtering logic

Not implemented yet:
- Event detail panel
- Grouped warnings/errors
- Export filtered results
- Timeline or chart-style visualization

## Tech Stack

- C++
- Qt Widgets
- CMake
- Qt Test

## Sample Log Format

TraceScope v1 will use JSON Lines records like:

```json
{"timestamp":"2026-07-07T10:14:22.381Z","level":"WARN","subsystem":"Tracking","eventCode":"TRACK_LOST","message":"Track 402 lost for 1200ms","entityId":"TRK-402"}
```

## Why This Project Exists

This project is designed as a practical native diagnostic tool rather than a web app. A desktop workflow makes sense for file-based telemetry, QA logs, hardware test logs, simulation output, and field-support diagnostic packages.

## Running Locally

Requirements:
- Qt 6
- CMake
- C++17-compatible compiler

Configure and build with CMake through Qt Creator, or from a terminal using your local Qt/CMake setup.

## Project Status

This project is in early development.