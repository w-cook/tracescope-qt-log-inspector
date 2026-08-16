# TraceScope Performance Notes

TraceScope is designed for offline investigation of file-based application, service, QA, engineering-test, field-support, and diagnostic logs.

Large-file behavior is measured using representative end-to-end investigation workflows rather than synthetic parser-only throughput tests. The measurements below are observations from one development system and are not maximum supported file-size, record-count, or throughput guarantees.

## Measurement Method

Each scenario was tested using a Release build.

Elapsed time was measured manually from clicking **Import** in the Import Configuration dialog until the imported investigation was fully displayed and responsive in the main TraceScope window.

Each source was imported three times. The median of those three runs is reported.

The timing therefore includes more than source parsing alone. It also includes the user-visible work required to install the imported records into the investigation model and refresh the table, filters, summaries, and timeline.

Large structured-document preview behavior was evaluated separately from full import behavior.

## Test Environment

Measurements were taken on:

- Operating system: Microsoft Windows 10 Home 64-bit, version 10.0.19045
- Processor: Intel(R) Core(TM) i5-8400 CPU @ 2.80 GHz
- Processor cores: 6
- Logical processors: 6
- Installed memory: 27.9 GB
- Build configuration: Release
- Qt: 6.11.1
- Compiler/toolchain: MinGW 64-bit

These results are specific to this machine and build environment. Different storage devices, processors, memory configurations, operating systems, compiler versions, source layouts, mapping complexity, and record contents can materially affect elapsed time.

## Measured Import Scenarios

| Source family | Records | Approx. source size | Run 1 | Run 2 | Run 3 | Median |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| JSON Lines | 220,000 | ~109 MiB | 7.0 s | 7.2 s | 8.4 s | **7.2 s** |
| CSV | 180,000 | ~31 MiB | 3.0 s | 2.9 s | 3.5 s | **3.0 s** |
| IIS W3C | 180,000 | ~20 MiB | 9.2 s | 9.0 s | 9.6 s | **9.2 s** |
| Windows Event XML collection | 90,000 | ~102 MiB | 14.9 s | 14.8 s | 14.7 s | **14.8 s** |
| Structured JSON | 120,000 | ~64 MiB | 7.3 s | 6.2 s | 6.5 s | **6.5 s** |

The application remained responsive during import and after the resulting investigation was displayed in all five scenarios.

Elapsed times should not be compared as parser-throughput benchmarks between formats. Different importers perform different amounts and types of parsing, mapping, raw-source preservation, and structural processing.

## Progress Reporting

The tested JSON Lines, CSV, IIS W3C, and Windows Event XML imports reported determinate progress while processing the source.

The structured JSON scenario used indeterminate progress. Structured JSON documents are currently parsed as complete structured documents rather than through the streamed record-by-record path used by several other importers. The parsing work still executes outside the UI thread so the desktop interface remains responsive.

TraceScope does not display an artificial percentage when meaningful incremental progress is not available.

## Cancellation

Cancellation was manually verified with representative large JSON Lines and Windows Event XML imports.

In both cases:

- cancellation was responsive
- the import progress interface closed normally
- the existing investigation remained loaded
- partial imported results did not replace the existing investigation
- the application remained usable immediately afterward
- another source could be opened or imported without restarting TraceScope

Importers that support cooperative streamed processing check for cancellation while parsing rather than waiting for the complete source to finish.

## Large Structured-Document Preview

Automatic preview is disabled for large structured JSON and XML documents once the configured size threshold is exceeded.

This keeps the Import Configuration interface available instead of synchronously attempting potentially expensive structured-document preview work on the UI thread.

A preview can still be requested explicitly. Manual large-document previews run in the background and are limited to the normal preview record count.

Background preview work is cooperatively cancellable. Changing the source or profile invalidates and cancels an obsolete preview rather than allowing stale preview processing to block later configuration work.

For structured XML, using an appropriate record path also allows the XML importer to stop after the configured preview record limit rather than processing unrelated later records.

## Memory and File-Size Claims

TraceScope does not currently claim a fixed maximum supported file size or record count.

Several line-oriented and XML import paths use streamed source reading so the entire source file does not need to be loaded into one source byte buffer before parsing.

This does **not** mean import memory usage is constant. Normalized `InvestigationRecord` objects, raw-source values, dynamic attributes, model data, and analysis state remain available in memory for the active investigation.

Structured JSON processing also differs from the streamed import paths because the structured document is parsed as a complete document.

For these reasons, practical limits depend on source format, record complexity, mapping configuration, machine memory, and the investigation workload rather than file size alone.

## Interpretation

The Phase 7 measurements demonstrate that representative investigations containing tens or hundreds of thousands of records and source files ranging to approximately 100 MiB can remain usable on the documented test system.

They should be interpreted as repeatable observed scenarios, not as guarantees that every file of a similar size or record count will perform the same way.

Future performance work should continue using representative investigation workflows and should extend these measurements when application architecture or supported workflows materially change.