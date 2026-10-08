# Testing TraceScope

This document describes TraceScope's automated test organization, continuous integration, manual scenario testing, and release verification. It is intended for contributors changing the code or evaluating how behavior is verified, not as an assertion that every possible source file, display environment, or user workflow is automatically tested.

The current test configuration in [`tests/CMakeLists.txt`](../tests/CMakeLists.txt) registers **92 CTest test programs**. This is a count of registered executables, not individual QTest functions, a code-coverage percentage, or a quality score.

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Understand what the automated tests cover | [1. Testing approach and coverage](#1-testing-approach-and-coverage) |
| Build or run the complete or selected suite | [2. Running automated tests](#2-running-automated-tests) |
| Understand the GitHub Actions gates | [3. Continuous integration](#3-continuous-integration) |
| Repeat realistic live-follow tests | [4. Manual scenario testing](#4-manual-scenario-testing) |
| Understand release-level verification | [5. Release verification](#5-release-verification) |
| Add appropriate tests with a change | [6. Test development guidelines](#6-test-development-guidelines) |

## 1. Testing approach and coverage

TraceScope uses **Qt Test / QTest**, CMake, and CTest. Most automated tests exercise focused domain, importer, analysis, persistence, or policy classes using deterministic inputs. Other suites exercise Qt models, dialogs, document hosts, source lifecycle behavior, or integration across several services.

Each registered CTest test program corresponds to a separate executable and may contain multiple QTest test functions or cases. [`tests/CMakeLists.txt`](../tests/CMakeLists.txt) is the source of truth for the current automated inventory.

The following groups describe the major areas covered by the suite. Test names are representative rather than exhaustive.

| Area | Representative test programs | What they help protect |
| --- | --- | --- |
| Domain, parsing, and profile rules | `InvestigationRecordTests`, `RecordSeverityTests`, `ImportProfileTests`, `ImportProfileSerializationTests`, `ImportProfileValidatorTests` | Optional normalized fields, severity handling, profile structure, validation, and schema round trips |
| Importers, preview, and source access | `JsonLinesImporterTests`, `StructuredJsonImporterTests`, `XmlImporterTests`, `SyslogImporterTests`, `IisW3cImporterTests`, `ImportPreviewServiceTests`, `SourceFamilyImportServiceTests`, `SharedReadFileTests` | Format normalization, diagnostics, bounded preview, rotated source families, and passive access to producer-owned files |
| Table, filters, navigation, and layout policy | `InvestigationTableModelTests`, `InvestigationFilterProxyModelTests`, `InvestigationControllerTests`, `FilterPresetStoreTests`, `InvestigationSectionResizePolicyTests`, `InvestigationPresentationStateTests` | Dynamic columns, source/proxy mapping, filters, navigation state, presets, constrained-height behavior, presentation-state persistence, scale-aware column restoration, timeline positioning, and analytics drilldowns |
| Analysis and comparison | `EventTimelineAnalyzerTests`, `InvestigationAnalyticsAnalyzerTests`, `InvestigationBurstAnalyzerTests`, `InvestigationCadenceAnalyzerTests`, `InvestigationSessionComparisonAnalyzerTests`, `InvestigationComparisonSnapshotBuilderTests`, `InvestigationComparisonPersistenceTests` | Timeline aggregation, issue metrics, cadence and bursts, independently time-scoped comparisons, immutable capture, and persistence |
| Findings, reporting, and export | `InvestigationFindingExportSnapshotBuilderTests`, `InvestigationFindingsCsvExporterTests`, `InvestigationReportSelectionModelTests`, `InvestigationReportSessionSnapshotBuilderTests`, `InvestigationReportSnapshotBuilderTests`, `InvestigationReportHtmlRendererTests` | Findings selection, report capture boundaries, frozen analytical state, structured export, and escaped/self-contained HTML rendering |
| Workspace and evidence continuity | `InvestigationSessionBackingTests`, `InvestigationSessionBackingPersistenceSerializationTests`, `InvestigationSessionRestorationServiceTests`, `InvestigationSessionSnapshotSerializationTests`, `WorkspaceSerializationTests`, `WorkspaceSavePackageServiceTests`, `WorkspaceWorkingArtifactStoreTests`, `WorkspaceDocumentHostTests` | Backing modes, versioned persistence, recovery and reconciliation, committed save-package durability, temporary working-artifact isolation and cleanup, document ownership, and cross-window movement |
| Live following and physical-source identity | `LiveFileFollowerTests`, `LiveSessionFollowCoordinatorTests`, `LiveLineImportAdapterTests`, `LiveStructuredJsonImportAdapterTests`, `LiveStructuredXmlImportAdapterTests`, `LiveLineSourceContextTests`, `SourcePhysicalIdentityTests` | Appends, partial records, generation changes, format-specific incremental parsing, source continuity, and safe observation of changing files |
| Standalone Live-Log Generator | `LiveLogScenarioLoaderTests`, `LiveLogScenarioPlayerTests`, `LogRecordRendererFactoryTests`, and renderer-specific suites | Deterministic scenario parsing/playback and source-format rendering used for real-file live-follow verification |

Several v1.0 behaviors deliberately have tests at more than one layer. For example:

- `SharedReadFileTests` verify that TraceScope's read handle does not prevent an external writer from continuing to write and, where the platform supports the operation, remove or rename the source while the TraceScope handle remains open.
- comparison tests cover complete investigation session capture, independently selected time ranges, invalid or empty ranges, persistence, and the rule that an existing comparison document does not change after later live additions to its source investigation sessions.
- `WorkspaceDocumentHostTests` verify document identity and ownership, detachment and redocking, close-request handling, movement to another visible peer window without discarding documents, selection preservation, and workspace-layout round trips.
- `InvestigationSectionResizePolicyTests` exercise automatic shrink/growth and collapse/recovery rules independently from the widget hierarchy.
- `InvestigationPresentationStateTests` exercise selected presentation and interaction contracts, including timeline Follow Newest positioning, analytics frequency drilldowns, and event-table column-width restoration across interface scales. These tests complement rather than replace visual regression testing on real displays.

These focused tests complement rather than replace end-to-end and visual verification.

### Test boundaries

A green automated suite establishes the behaviors asserted by those tests. It does not imply complete correctness for every external input or environment.

Important boundaries include:

- **Source formats:** parser and profile fixtures cannot validate an arbitrary third-party log whose actual structure has not been inspected.
- **Concurrent files:** live behavior depends on producer timing, partial writes, truncation, replacement, rotation, filesystem semantics, and operating-system file sharing. Deterministic source-lifecycle tests reduce risk but do not reproduce every external producer.
- **GUI presentation:** font metrics, native window frames, fractional DPI, accessibility settings, monitor geometry, and focus behavior require real desktop verification in addition to policy/model tests.
- **Packaging:** building the development executable does not establish that a deployed ZIP, AppImage, or Live-Log Generator package contains every required runtime dependency or starts correctly.
- **Performance:** functional tests do not establish throughput, latency, or maximum supported file size. Measured observations belong in [Performance Notes](performance.md).

TraceScope does not publish a line- or branch-coverage percentage. The number of registered CTest programs should not be interpreted as one.

## 2. Running automated tests

Build TraceScope using a supported Qt 6 desktop kit with **Qt Widgets, Qt Charts, Qt Concurrent, and Qt Test**, CMake 3.21 or later, and a compatible C++17 compiler. See [Building from Source](building-from-source.md) for the complete development setup.

From the repository root after configuration:

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

To list the registered programs or run a subset:

```sh
ctest --test-dir build -N
ctest --test-dir build -R 'LiveSessionFollowCoordinatorTests|LiveFileFollowerTests' --output-on-failure
ctest --test-dir build -R 'ImportProfile|Importer|ImportPreview' --output-on-failure
ctest --test-dir build -R 'InvestigationComparison|ReportSnapshot|ReportHtml' --output-on-failure
```

CTest's `-R` option matches **registered test-program names**. A single registered executable may contain several QTest functions. To target an individual QTest function, run that test executable directly using QTest's command-line selection support.

### Offscreen execution

The GitHub Actions jobs run the test suite with:

```text
QT_QPA_PLATFORM=offscreen
```

The same setting is useful when running GUI-dependent test programs in an environment without a normal display server.

```sh
# Bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

```powershell
# PowerShell
$env:QT_QPA_PLATFORM = 'offscreen'
ctest --test-dir build --output-on-failure
```

Offscreen execution verifies program logic that depends on Qt GUI classes; it does not substitute for manual visual testing of native windows, display scaling, or constrained layouts.

Use a fresh build directory after switching compiler families, incompatible Qt installations, or major CMake configuration options. On Windows, the compiler must remain ABI-compatible with the selected Qt kit.

When diagnosing a failure, preserve the registered test name, failing QTest function where available, assertion output, operating system, Qt/toolchain version, and source revision. Reproduce the focused failure before rerunning the entire suite.

## 3. Continuous integration

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs on **push**, **pull request**, and **manual workflow dispatch**. The v1.0 workflow builds and verifies the application on Windows and Linux and produces the release-oriented application, Live-Log Generator, and sample artifacts.

The CI build type is **Release**, and the automated platform builds use Qt **6.10.3**.

| Area | Environment | Automated checks and artifacts |
| --- | --- | --- |
| Windows application | GitHub-hosted Windows with matching 64-bit MinGW and Qt 6.10.3 | CMake/Ninja Release build, complete CTest suite with the offscreen Qt platform, `windeployqt`, required-file checks, portable application ZIP creation, extraction, and startup smoke test |
| Linux application | Ubuntu 22.04 with GCC and Qt 6.10.3 | CMake Release build, complete CTest suite with the offscreen Qt platform, pinned `linuxdeploy` tooling, AppDir/AppImage verification, and packaged AppImage startup smoke test |
| Live-Log Generator | Windows and Linux release packaging | Separate convenience packages containing the standalone generator and launcher, required runtime files, and package/startup verification |
| Samples | Ubuntu 22.04 | Required source/profile validation, complete samples archive creation, archive-content checks, and source-versus-package file-count verification |

The Windows and Linux application jobs execute the complete CTest registration for the configured build. A failure in a registered test prevents that platform job from proceeding as a successful release build.

Packaging checks deliberately run against **deployed artifacts**, not only the executable in the build tree. The application smoke tests extract or launch the produced package and verify that the process can start with its deployed runtime dependencies. These are startup checks, not substitutes for interactive investigation testing.

The Live-Log Generator is packaged separately from TraceScope because it is an independent test and demonstration utility rather than part of the application runtime. TraceScope communicates with it only through the physical files the generator writes.

Release artifacts use the v1.0 version identity rather than earlier prerelease package names.

GitHub Actions establishes reproducible build/test/package gates. It does not by itself establish every manual acceptance criterion described below.

## 4. Manual scenario testing

Some of TraceScope's most important behaviors depend on an independent process modifying a real file while TraceScope observes it. The repository therefore includes the standalone deterministic **Live-Log Generator** under `tools/live-log-generator/`, with reusable scenarios under `samples/live/`.

The generator's separation from TraceScope is intentional:

```text
scenario → generator → physical file → TraceScope
```

TraceScope has no direct integration with the generator and cannot distinguish generator output from a file written by another application.

A representative manual live-follow session covers the following areas.

1. **Baseline import**  
   Open a representative source with its matching profile and confirm recognizable records, expected mapped fields, source provenance, and import counts.

2. **Passive source access**  
   Keep TraceScope attached to the source while the generator continues writing. Exercise source lifecycle operations supported by the scenario—append, rename/rotation, truncation, or replacement—and confirm TraceScope does not require the producer to stop or surrender ownership of its file.

3. **Incomplete records**  
   Verify that a deliberately partial trailing record or structured fragment is not admitted until the source content completes it.

4. **Pause, resume, and stop**  
   Confirm the expected control state and treatment of content written while following is paused or between follow sessions.

5. **Physical lifecycle and provenance**  
   Exercise truncation, same-path replacement, and rotated source families as separate concepts. Verify source-generation changes and physical-file provenance rather than treating rotation as a generation change.

6. **Evidence continuity**  
   Save investigation session snapshots and workspaces where appropriate. Verify recovery when external sources are unavailable, **SnapshotBacked** and **Hybrid** investigation session behavior, verified source reconnection, and explicit source-authority transitions without silently discarding preserved evidence.

7. **Investigation and comparison behavior**  
   Exercise filtering, findings, annotations, timeline/analytics, and comparison creation after the live run. For time-scoped comparison documents, verify that each side retains its independently captured time range and that subsequently admitted live records do not change the existing comparison results. Also verify contextual filtering from the event table, analytics frequency drilldowns, Follow Newest behavior in both the event table and timeline, and preservation of manual column widths across interface-scale changes.

8. **Reporting and export**  
   Produce CSV and HTML output from the resulting investigation. Confirm that a generated report represents its captured state rather than continuing to change with the live session.

Use the scenario that isolates the behavior under test rather than combining unrelated source-lifecycle changes into one ambiguous run.

| Scenario | Intended focus |
| --- | --- |
| `field-gateway-live-scenario.json` | Rising latency, retries, partial records, in-place truncation, and recovery |
| `warehouse-sync-deployment-rotation-live-scenario.json` | Replacement, catch-up traffic, and multiple rotations |
| `checkout-api-healthy-baseline-live-scenario.json` and `checkout-api-payment-regression-live-scenario.json` | Baseline/regression comparison, including error bursts |
| `web-access-live-scenario.json` | Web-access rendering and access-log investigation |

The generator supports multiple renderer families, but not every output format can represent every semantic field in a scenario. Use the documented renderer capabilities and matching TraceScope profiles rather than assuming that formats such as RFC 3164 or access logs preserve the same normalized fields as JSON or XML.

See [Live-Log Generator](live-log-generator.md), [Live Following](live-following.md), and [Supported Formats](supported-formats.md).

Generate test output into a disposable directory rather than an important diagnostic source. Preserve the corresponding `.tsinv`, `.tsw`, and managed session snapshots when the purpose of a reproduction is evidence continuity.

## 5. Release verification

The v1.0 release combines automated CI gates with manual acceptance checks that are difficult or inappropriate to reduce to headless tests.

The v1.0 release verification checklist includes:

- a clean Release build and complete CTest run on the v1.0 revision on both Windows and Linux CI environments
- successful creation and startup smoke testing of the Windows portable package and Linux AppImage
- successful creation and verification of the separate Windows and Linux Live-Log Generator convenience packages
- verification of the packaged sample/profile collection and at least one representative import from the deployed application
- manual live-follow playback covering partial writes, producer-safe shared reading, a same-path generation change, and a rotated source family
- save/reopen and missing-source recovery for workspace and standalone snapshot evidence
- SnapshotBacked/Hybrid continuity and an explicit return to source authority
- complete-session and independently time-scoped comparison capture, including confirmation that captured comparisons remain unchanged after later session mutations
- HTML report generation from restored/captured state and offline review of the resulting document
- multi-window document movement, window-close protection, document identity and selection preservation, and workspace restoration across peer windows
- real-desktop checks for native/fractional DPI behavior, Interface Scale changes, constrained-height section behavior, focus/navigation, and the final major UI surfaces
- review of the packaged application rather than only the development build

The release workflow distinguishes between **configured checks** and **evidence from a particular run**. When documenting or diagnosing a release, identify the source revision and the CI/manual verification that actually produced the result rather than inferring success merely from the existence of the workflow.

Measured performance observations and their test environment are documented separately in [Performance Notes](performance.md). Release verification uses fresh measurements where a v1.0 performance claim depends on them rather than treating earlier development measurements as automatically representative.

## 6. Test development guidelines

1. **Put deterministic correctness rules in focused classes where practical.**  
   Import parsing, source identity, analysis, persistence, comparison capture, and layout policy are easier to verify reliably when their rules are not buried inside a complete window interaction.

2. **Test boundaries and failure behavior, not only successful inputs.**  
   Relevant cases include missing canonical fields, malformed and partial source data, cancellation, empty selections, invalid comparison ranges, source truncation/replacement, incompatible persistence, failed saves, and unavailable external sources.

3. **Treat producer-owned source access as a contract.**  
   Code that reads external logs should preserve the passive shared-read boundary. Tests should verify that TraceScope does not unnecessarily prevent the producer from continuing normal write, rotation, rename, replacement, or removal behavior supported by the platform.

4. **Preserve evidence-authority semantics.**  
   Changes to reload, live following, restoration, or source reconciliation should test which evidence remains authoritative and ensure that earlier admitted or saved evidence is not discarded as an accidental side effect.

5. **Preserve immutable-artifact semantics.**  
   Comparison tests should verify captured scope and analysis after later session changes. Report tests should verify that rendering consumes frozen report data rather than mutable session state.

6. **Pair new source formats and generator renderers with readable examples.**  
   Keep parsing, profile mapping, preview, incremental/live behavior, and generator rendering separately testable where they express different contracts.

7. **Test workspace ownership separately from window appearance.**  
   Document ownership, transfer, workspace-layout persistence, and presentation-state rules should be tested independently where practical. Deterministic host/layout and presentation-state tests should cover document identity, cross-window lifecycle, section resize policies, scale-related state restoration, and interaction behavior. Native frame rendering, fractional DPI, visual spacing, accessibility, and focus still require manual desktop regression checks.

8. **Register new suites with CTest and run both focused and complete verification.**  
   Add the executable and `add_test` registration in `tests/CMakeLists.txt`, run the affected program while developing, and run the complete suite before merging.

For subsystem ownership and extension points, see [Architecture](architecture.md). For compiler, Qt, CMake, and local configuration details, see [Building from Source](building-from-source.md).
