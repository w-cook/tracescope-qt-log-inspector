# TraceScope Performance Notes

This document records measured performance observations for the TraceScope v1.0 release.

The measurements use representative end-to-end investigation workflows rather than parser-only microbenchmarks. They are intended to show how the application behaved on a documented test system under specific workloads.

They are **not** maximum supported file-size, record-count, memory, or throughput guarantees.

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Understand what was measured | [1. Measurement method](#1-measurement-method) |
| See the test environment | [2. Test environment](#2-test-environment) |
| See the import results | [3. Measured import scenarios](#3-measured-import-scenarios) |
| Understand progress and cancellation behavior | [4. Progress and cancellation](#4-progress-and-cancellation) |
| Understand large structured-document preview behavior | [5. Large structured-document preview](#5-large-structured-document-preview) |
| Understand memory and practical limits | [6. Memory and file-size considerations](#6-memory-and-file-size-considerations) |
| Understand what the measurements establish | [7. Interpreting the results](#7-interpreting-the-results) |

## 1. Measurement method

All measurements were taken with a **Release build of TraceScope v1.0**.

For each import scenario:

1. the source and its matching import configuration were selected
2. elapsed time began when **Import** was activated in the Import Configuration dialog
3. timing ended when the resulting investigation was fully displayed and responsive
4. the import was repeated three times
5. the median of the three elapsed times was recorded

The reported time is therefore an **end-to-end user-visible import measurement**, not parser execution time alone.

It includes work such as:

- reading and parsing the source
- applying the configured field mappings
- creating normalized investigation records
- preserving raw-source and source-metadata values
- installing the records into the investigation model
- preparing the table and filter state
- refreshing summary and timeline presentation required for the completed investigation

The measurements were taken manually. The reported tenths of a second are useful for comparing repeated runs of these specific workflows, but they should not be interpreted as high-precision microbenchmark results.

Structured-document preview behavior and cancellation were verified separately and are not included in the import timings.

## 2. Test environment

The v1.0 measurements were taken on:

| Component | Test environment |
| --- | --- |
| Operating system | Microsoft Windows 10 Home 64-bit, version 10.0.19045 |
| Processor | Intel Core i5-8400 @ 2.80 GHz |
| Physical cores | 6 |
| Logical processors | 6 |
| Installed memory | 27.9 GB |
| Build configuration | Release |
| Qt | 6.11.1 |
| Compiler/toolchain | MinGW 64-bit |

Performance on another system can differ materially.

Relevant variables include:

- processor performance
- available memory
- storage device and filesystem behavior
- operating system
- Qt and compiler versions
- source format
- average record size
- source structure
- mapping complexity
- custom-attribute count and size
- raw-source size
- investigation size and analysis workload

The figures below should therefore be read together with this environment rather than as hardware-independent expectations.

## 3. Measured import scenarios

| Source family | Records | Approx. source size | Run 1 | Run 2 | Run 3 | Median |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| JSON Lines | 220,000 | ~109 MiB | 7.0 s | 7.2 s | 8.4 s | **7.2 s** |
| CSV | 180,000 | ~31 MiB | 3.0 s | 2.9 s | 3.5 s | **3.0 s** |
| IIS W3C | 180,000 | ~20 MiB | 9.2 s | 9.0 s | 9.6 s | **9.2 s** |
| Windows Event XML collection | 90,000 | ~102 MiB | 14.9 s | 14.8 s | 14.7 s | **14.8 s** |
| Structured JSON | 120,000 | ~64 MiB | 7.3 s | 6.2 s | 6.5 s | **6.5 s** |

All five measured investigations completed successfully and remained usable after import.

The application also remained responsive during the import operations rather than entering a prolonged non-responsive UI state.

### Why these are not parser-throughput rankings

The table should not be converted directly into cross-format MB/s or records-per-second rankings.

The importers perform different work.

For example:

- line-oriented and structured formats have different parsing models
- some formats contain headers or nested structure
- mapping work differs by source
- source records have different average sizes
- raw-source preservation costs differ
- structured-document processing can require different memory and traversal behavior

The useful comparison is therefore the observed **complete workflow for each documented source**, not which parser appears fastest after dividing source size by elapsed time.

## 4. Progress and cancellation

### Progress reporting

The measured JSON Lines, CSV, IIS W3C, and Windows Event XML imports reported determinate progress while processing their sources.

The Structured JSON scenario used indeterminate progress.

Structured JSON is parsed as a complete structured document rather than through the streamed record-by-record path used by several other importers. That work still runs outside the Qt UI thread, so the application remains responsive while the import is active.

TraceScope does not display a fabricated percentage when meaningful incremental progress is not available from the active import path.

### Cancellation

Cancellation was manually verified with representative large:

- JSON Lines
- Windows Event XML

imports.

In both cases:

- cancellation responded while processing was in progress
- the progress interface closed normally
- partially imported results did not replace the existing investigation
- the existing investigation remained available
- the application was immediately usable afterward
- another source could be opened or imported without restarting TraceScope

Streamed import paths that support cooperative cancellation check for cancellation while processing rather than waiting for the complete source to finish.

These checks establish the tested cancellation behavior for the representative paths above; they are not timing measurements.

## 5. Large structured-document preview

Import preview and full import have different performance requirements.

A preview is intended to help configure a source, so expensive structured-document work should not make the Import Configuration interface unusable before an import has even begun.

For sufficiently large Structured JSON and XML sources, automatic structured preview is therefore deferred rather than synchronously parsing the complete document whenever configuration changes.

The user can request the preview explicitly.

Large structured-document previews:

- execute outside the UI thread
- remain limited to the normal preview record count
- support cooperative cancellation
- are invalidated when the source or relevant profile configuration changes

This prevents obsolete preview work from blocking a later configuration request.

For Structured XML with an appropriate repeated-record path, preview processing can stop after the configured preview record limit rather than traversing unrelated records later in the document.

Preview behavior is evaluated for responsiveness and bounded usefulness rather than included in the full-import timing table.

## 6. Memory and file-size considerations

TraceScope does not publish a fixed maximum supported file size or record count.

### Streamed input does not mean constant application memory

Several line-oriented and XML paths process source data incrementally. This avoids requiring the complete source file to exist as one in-memory input buffer before parsing.

The resulting investigation itself is still retained in memory.

Memory can therefore grow with:

- normalized `InvestigationRecord` objects
- raw-source content
- source metadata
- dynamic custom attributes
- table/model state
- annotations
- derived analysis state
- other active investigation data

Streaming reduces one category of input-memory pressure; it does not make investigation memory usage constant.

### Structured JSON has different characteristics

Structured JSON uses complete-document parsing and therefore has different memory behavior from the streamed source paths.

Its practical memory requirements depend on both the structured source representation and the normalized investigation constructed from it.

### File size alone is not a sufficient limit

Two files of the same byte size can produce very different investigation workloads.

A file containing many small records may create substantially more model and object overhead than a file containing fewer large records. Likewise, records with many custom fields can require more retained state than simpler records.

Practical limits therefore depend on the combination of:

- file size
- record count
- record complexity
- selected source format
- import profile
- available system memory
- subsequent investigation workload

## 7. Interpreting the results

On the documented v1.0 test system, the measured scenarios covered:

- **90,000 to 220,000 records**
- approximately **20 MiB to 109 MiB** source files
- median end-to-end import times from **3.0 to 14.8 seconds**

These observations show that the documented workloads completed successfully and produced usable investigations on that system.

They do **not** establish:

- a maximum supported file size
- a maximum supported record count
- guaranteed import latency
- guaranteed parser throughput
- a fixed memory ceiling
- equivalent performance across source formats
- equivalent performance on different hardware or operating systems
- sustained live-follow ingestion capacity
- performance for arbitrary third-party source structures

The results are best understood as reproducible reference workloads for the v1.0 release.

Performance claims should remain tied to measured scenarios like these. Broader limits or throughput guarantees would require separate controlled benchmarking designed specifically to establish those claims.
