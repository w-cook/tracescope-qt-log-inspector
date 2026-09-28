# Testing TraceScope

This document describes TraceScope's automated test organization, continuous integration, and reproducible manual verification. It is intended for contributors changing the code or evaluating the engineering approach, not as an assertion that every behavior is automatically tested.

**Documentation baseline:** Phase 16 source on `phase-16-final-polish` at commit `8011035`. The reviewed `tests/CMakeLists.txt` declares **92 CTest test executables**; this is a count of registered test programs, **not** individual test cases, a code-coverage percentage, or a claim that this documentation run executed them. Revisit the test inventory after the planned targeted development pass.

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Understand what the automated tests cover | [1. Testing approach and coverage](#1-testing-approach-and-coverage) |
| Build or run the complete or selected suite | [2. Running automated tests](#2-running-automated-tests) |
| Understand the GitHub Actions gates | [3. Continuous integration](#3-continuous-integration) |
| Repeat realistic live-follow tests | [4. Manual scenario testing](#4-manual-scenario-testing) |
| Verify a release and interpret test claims | [5. Release verification](#5-release-verification) |
| Add appropriate tests with a change | [6. Test development guidelines](#6-test-development-guidelines) |

## 1. Testing approach and coverage

TraceScope uses **Qt Test / QTest**, CMake and CTest. Most automated tests exercise focused non-UI classes using controlled inputs. Selected tests also exercise Qt models, dialogs, document host behavior, or integration across multiple services. The source of truth for registered test programs is [`tests/CMakeLists.txt`](../tests/CMakeLists.txt).

Each test program is a separate executable registered with CTest. The following groups indicate the current areas of coverage; examples are representative, not an exhaustive inventory.

| Area | Representative test programs | What they help protect |
| --- | --- | --- |
| Domain, parsing, and profile rules | `InvestigationRecordTests`, `RecordSeverityTests`, `ImportProfileSerializationTests`, `ImportProfileValidatorTests` | Optional fields, severity conversion, schema round-trips, invalid configurations |
| Importers and preview | `JsonLinesImporterTests`, `StructuredJsonImporterTests`, `XmlImporterTests`, `SyslogImporterTests`, `IisW3cImporterTests`, `ImportPreviewServiceTests`, `SourceFamilyImportServiceTests` | Input structures, normalization, diagnostics, record limits, preview and ordered source families |
| Table, filters, and investigation logic | `InvestigationTableModelTests`, `InvestigationFilterProxyModelTests`, `InvestigationControllerTests`, `FilterPresetStoreTests` | Dynamic columns, model/proxy mapping, sorting, filters and saved presets |
| Analysis | `EventTimelineAnalyzerTests`, `InvestigationAnalyticsAnalyzerTests`, `InvestigationBurstAnalyzerTests`, `InvestigationCadenceAnalyzerTests`, `InvestigationSessionComparisonAnalyzerTests` | Deterministic event summaries, buckets, bursts, timing and comparison calculations |
| Findings and output | `InvestigationFindingExportSnapshotBuilderTests`, `InvestigationFindingsCsvExporterTests`, `InvestigationReportSnapshotBuilderTests`, `InvestigationReportHtmlRendererTests` | Selection, frozen report data, structured exports and safe HTML rendering |
| Workspace and continuity | `InvestigationSessionBackingTests`, `InvestigationSessionRestorationServiceTests`, `WorkspaceSavePackageServiceTests`, `WorkspaceDocumentHostTests` | Backing transitions, reconstruction/recovery, durable package construction and document behavior |
| Live following and physical sources | `LiveFileFollowerTests`, `LiveSessionFollowCoordinatorTests`, `LiveStructuredJsonImportAdapterTests`, `LiveStructuredXmlImportAdapterTests`, `SourcePhysicalIdentityTests` | Growth, record framing, generation changes, format adapters and continuity |
| Standalone generator | `LiveLogScenarioLoaderTests`, `LiveLogScenarioPlayerTests`, `LogRecordRendererFactoryTests`, individual renderer test programs | Deterministic scenario parsing/playback and output format behavior |

Concrete test examples visible in the reviewed tree include open structured JSON arrays with incomplete trailing records, structured XML and Windows Event XML growth, session-comparison snapshots remaining unchanged after live appends and reloads, and workspace package validation before writing. The section-resize-policy suite separately exercises constrained-height layout transitions; it does not replace visual testing on actual displays.

### Test boundaries

A green unit or component suite establishes the behaviors asserted by those tests, not complete correctness for every source file, display configuration or user workflow. In particular:

- Importing complex third-party logs requires representative source files and the actual matching profile. Passing a parser fixture cannot validate an unknown customer's source format.
- Live file behavior also depends on file-system timing, concurrent writes, partial records, rotation naming, file access and the user's operating system.
- Qt GUI behavior depends on font metrics, display scaling, accessibility, window geometry and focus transitions; some of these require hands-on checks.
- Package success depends on deployed runtime libraries and launching the **packaged artifact**, not merely running the development executable.

There is no published automated line/branch coverage percentage in this documentation. Do not interpret the number of CTest targets as such a metric.

## 2. Running automated tests

Build TraceScope using a supported Qt 6 desktop kit with **Qt Widgets, Qt Charts, Qt Concurrent and Qt Test**, CMake 3.21 or later, and a compatible C++17 compiler. Follow [Building from Source](building-from-source.md) first if the project has not been configured on this machine.

From the repository root after configuration:

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

To list registered tests or run a subset:

```sh
ctest --test-dir build -N
ctest --test-dir build -R 'LiveSessionFollowCoordinatorTests|LiveFileFollowerTests' --output-on-failure
ctest --test-dir build -R 'ImportProfile|Importer|ImportPreview' --output-on-failure
```

CTest's `-R` matches **registered test-executable names**. For an individual QTest function, consult that executable's QTest command-line support or run the program directly; CTest is primarily used for the complete registered suite.

**Headless/CI-compatible execution:** The GitHub workflow sets `QT_QPA_PLATFORM=offscreen` for its test step. If a GUI-related test fails solely because a desktop/display server is unavailable, try the same environment before assuming an application defect:

```sh
# Bash (Linux or a compatible shell)
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

```powershell
# PowerShell
$env:QT_QPA_PLATFORM = 'offscreen'
ctest --test-dir build --output-on-failure
```

Use a fresh build directory after switching compiler families, incompatible Qt installations or major configuration options. On Windows, building with the same MinGW toolchain as the selected Qt kit avoids mixing incompatible runtime/ABI versions.

When diagnosing a failure, preserve the failing test name, assertion output, platform, Qt version and exact source/branch revision. Then reproduce with that specific test before attempting the entire suite again.

## 3. Continuous integration

The reviewed workflow, [`.github/workflows/ci.yml`](../.github/workflows/ci.yml), runs on **push**, **pull request**, and **manual workflow dispatch**. It contains three jobs:

| Job | Environment | Current automated checks |
| --- | --- | --- |
| Windows build/test/package | GitHub-hosted Windows, Qt **6.10.3** with matching 64-bit MinGW | CMake/Ninja Release build, complete CTest suite, `windeployqt`, required-file verification, portable ZIP startup smoke test |
| Linux build/test/package | **Ubuntu 22.04**, GCC, Qt **6.10.3** | CMake Release build, complete CTest suite, pinned `linuxdeploy` tooling, AppDir/AppImage checks, offscreen packaged AppImage startup smoke test |
| Samples package | Ubuntu 22.04 | Required sample/profile file checks, neutral samples ZIP creation and archive-entry verification |

The build configuration in the reviewed workflow is **Release**. The Windows and Linux test jobs set `QT_QPA_PLATFORM=offscreen`. Packaging runs after the build/tests within each platform job. The platform jobs smoke-test the *extracted/deployed application* by starting it briefly; this checks startup and key packaging dependencies, **not** interactive behavior or extended investigation scenarios.

The workflow is currently versioned for the `v0.16.0` artifacts and must be updated and rerun for the final v1.0 candidate. CI results and uploaded workflow artifacts are build evidence, not a substitute for separately approving and publishing a GitHub Release. Consult the workflow itself before claiming a particular run passed; this document describes configured checks rather than reporting a live CI result.

## 4. Manual scenario testing

Some of the highest-value regression checks involve an external process changing a real file while TraceScope is reading it. For this purpose, the repository includes a separate deterministic **Live-Log Generator** (CLI and Qt Widgets launcher) under `tools/live-log-generator/`, with scenarios under `samples/live/`.

The generator's boundary is intentional: it produces ordinary physical files, and TraceScope has **no knowledge of the generating process**. Test the real file-follow path rather than a direct injection hook.

A focused manual session should include:

1. **Baseline import:** Open a representative sample with the matching bundled profile. Confirm a recognizable first/last record, mapped dynamic fields, and correct import counts.
2. **Follow and incomplete records:** Start live following on the generator's output; check that complete records appear and that a deliberately partial trailing record is not admitted prematurely.
3. **Pause/resume/stop:** Verify control state and the expected treatment of content added while paused or between follow sessions.
4. **Physical lifecycle:** Exercise the scenario's supported truncation, same-path replacement or rotation case. Inspect source generations, rotated-family membership and record provenance separately.
5. **Continuity:** Save an investigation snapshot/workspace when evidence matters. Where relevant, reopen after a source changes or is unavailable, and choose explicit source-reconnection/authority actions deliberately.
6. **Presentation/output:** Verify filtering, timeline, findings, annotations, comparisons and HTML/CSV export on the resulting investigation, especially after longer runs.

Use the scenario suited to the question rather than combining unrelated failure modes into a single ambiguous test:

| Scenario | Intended focus |
| --- | --- |
| `field-gateway-live-scenario.json` | Rising latency, retries, partial records, in-place truncation and recovery |
| `warehouse-sync-deployment-rotation-live-scenario.json` | Replacement, catch-up traffic, and multiple rotations |
| `checkout-api-healthy-baseline-live-scenario.json` and `checkout-api-payment-regression-live-scenario.json` | Meaningful baseline/regression comparison, including error bursts |
| `web-access-live-scenario.json` | Web-access renderers and corresponding access-log investigation |

The generator supports multiple renderer families, but not every format can preserve every semantic scenario field. Use its documented capability matrix and the matching profile; do not assume a format like RFC 3164 or Apache access logs carries every field present in the source scenario. See [Live-Log Generator](live-log-generator.md) and [Supported Formats](supported-formats.md).

**Safety of test material:** Generate into a disposable directory, not a real diagnostic source that must be preserved. Keep saved `.tsinv` and workspace sidecars when reproducing source-data continuity behavior.

## 5. Release verification

Before approving a new version, run the automated suite **and** check behavior that CI does not fully establish:

- Confirm a clean build and complete CTest result on the intended release revision; inspect Windows and Linux job results individually.
- Extract and launch the actual Windows portable ZIP and Linux AppImage; verify bundled sample/profile files and at least one representative import on each target platform when available.
- Exercise saved workspace and snapshot recovery when an external file is unavailable or has changed. Verify an existing captured comparison and a report generated after restoration.
- Run representative manual live-follow playback, including partial writes, a source generation change, and a rotated source family.
- Check high-DPI/fractional-scale and constrained-window layouts in a real desktop session; offscreen CI cannot establish their visual correctness.
- Update release artifact versions, screenshots, performance claims and this document only after evidence from the actual release candidate.

Current measured performance observations and test-machine details are recorded in [Performance Notes](performance.md). Preserve the original test conditions if adding new measurements; do not silently present historical runs as a new-version benchmark.

## 6. Test development guidelines

1. **Put correctness rules in testable classes where practical.** A focused domain/importer/analyzer/persistence test is generally easier to make deterministic than one that drives an entire main window.
2. **Test boundaries as well as happy paths.** Useful cases include missing canonical fields, malformed and partial input, duplicate values, cancellation, source rotation/replacement, incompatible snapshots, and failed saves.
3. **Preserve provenance and immutable-artifact semantics.** For reload and live-follow changes, test existing annotations and earlier admitted evidence. For comparison/report changes, check that previously captured output does not silently change after source/session mutations.
4. **Pair each new format or generator renderer with tests and a readable sample.** Keep parsing, profile mapping, preview, runtime following and renderer behavior separate where they express different contracts.
5. **Check platform/UI boundaries manually.** Record precise reproduction steps and the intended display/toolchain configuration for visual or timing issues; add deterministic policy/model tests for logic extracted from UI code.
6. **Register new suites in `tests/CMakeLists.txt`.** Verify the relevant named test locally, then run the complete suite before the user's normal PR/merge workflow.

For architectural ownership and starting points, see [Architecture](architecture.md). For local development prerequisites, see [Building from Source](building-from-source.md).

---

**After the targeted development pass:** Re-evaluate this guide's test inventory and CI/version references; add focused regression tests for whichever deferred fixes are actually implemented. Planned changes are not current test coverage.
