# TraceScope Architecture

TraceScope is a native, offline Qt/C++ desktop application for investigating file-based application, service, QA, engineering, and diagnostic logs. This document explains the main implementation boundaries and the decisions that connect importing, investigation, live following, persistence, comparisons, and reporting. It is intended for contributors and technical readers rather than as an end-user walkthrough.

**Documentation baseline:** This draft describes the Phase 16 source reviewed on `phase-16-final-polish` (commit `8011035`). The stable v1.0 code and any targeted post-freeze changes should be checked against it before release. This is an architecture overview, not a guarantee that every code path or edge case is described.

## Find the information you need

| I want to understand... | Go to |
| --- | --- |
| How the application is divided | [1. System overview](#1-system-overview) |
| The record type shared across formats | [2. Investigation data model](#2-investigation-data-model) |
| How source files become investigation records | [3. Import pipeline](#3-import-pipeline) |
| How filters, navigation, and analyses fit together | [4. Investigation and analysis](#4-investigation-and-analysis) |
| How new file content is followed without rebuilding the entire UI | [5. Live following and source identity](#5-live-following-and-source-identity) |
| How evidence survives source changes and workspace reopening | [6. Persistence and evidence continuity](#6-persistence-and-evidence-continuity) |
| Why comparisons and reports do not depend on later UI changes | [7. Comparison and reporting](#7-comparison-and-reporting) |
| Where to begin making a change | [8. Code map and extension points](#8-code-map-and-extension-points) |

## 1. System overview

TraceScope separates **source interpretation** from the **normalized investigation**, and separates that investigation from its **presentation and durable artifacts**. This matters because many input formats omit fields that other formats provide, and because a log file may change or disappear after its records are admitted into an investigation.

```text
Physical log file(s)                      Saved .tsinv snapshot
       |                                          |
Format detection / Import Configuration         restore
       |                                          |
Importer + profile -> ImportResult               |
       |                                          |
       +------------ InvestigationSession <--------+
                             |
                  InvestigationController
                   /                    \
        InvestigationTableModel   InvestigationFilterProxyModel
                   \                    /
                    InvestigationSessionView
                   /    |       |        \
          event table filters timeline review/analytics
                             |
                InvestigationWorkspace
                   /                  \
       comparison documents       snapshot/workspace save
                   \                  /
                 selected report/export models
                             |
                   CSV / offline HTML
```

This diagram is conceptual: the application also has source-family discovery, live-follow coordination, preference stores, detached document hosts, and separate immutable snapshot builders. It should not be interpreted as a literal sequence of calls.

- **Application and UI coordination:** `src/main.cpp`, `src/MainWindow.*`, and `src/ui/`. `MainWindow` coordinates file operations, document-level actions, imports, restoration, and reporting. Investigation-specific controls live largely under `src/ui/investigation/` and `src/ui/workspace/`.
- **Importing and domain:** `src/importing/`, `src/domain/`, `src/io/`, and `src/sources/` parse supported sources and normalize records while preserving raw and physical-source provenance.
- **Investigation and analysis:** `src/workspace/`, `src/controllers/`, `src/models/`, `src/filtering/`, `src/analysis/`, and `src/preferences/` own sessions, table/filter coordination, per-record annotations, analyses, and settings.
- **Continuity and output:** `src/live/`, `src/persistence/`, `src/workspace/`, and `src/exporting/` handle growing files, snapshot/workspace state, comparisons, and user-selected exports.
- **Test producer:** `tools/live-log-generator/` is a separate CLI program with a thin launcher. TraceScope reads only the files it produces; there is no application-to-generator integration.

TraceScope is a **local file-oriented desktop tool**, not a server, central log collector, or a live network ingestion service. See [Supported Formats](supported-formats.md) for format boundaries and [Live-Log Generator](live-log-generator.md) for the independent test utility.

## 2. Investigation data model

The central normalized record is [`InvestigationRecord`](../src/domain/InvestigationRecord.h). It carries a stable `recordId`; six *individually optional* canonical fields (timestamp, severity, subsystem, event code, entity ID, and message); a collection of source-specific `customAttributes`; original `rawSource`; and `RecordSourceMetadata` identifying where the record came from.

Optional fields are deliberate. For example, an access log may contain a request and status code without an intrinsic TraceScope severity or device identifier. Importers and profiles should not manufacture values merely to fill empty canonical columns.

A profile (`src/importing/ImportProfile.h`) identifies an importer, supplies source-to-canonical and named custom-field mappings, and can define severity aliases, timestamp parsing, and record-selection rules. Import profile files are **parsing configurations**, not saved investigations; see [Import Profiles](import-profiles.md).

`ImportResult` transfers the normalized records, processing counts, diagnostics, cancellation/truncation information, and relevant source-generation bookkeeping from importing into session creation or reload. Stable record identity and provenance are important to selection, finding annotations, snapshot retention, and source reconciliation. A record's source metadata and a logical source's identity are related but distinct concepts.

## 3. Import pipeline

### Configuration, selection, and preview

`ImportConfigurationDialog` uses `ImportFormatSuggestionService` to suggest an importer, optional built-in presets, and `ImportPreviewService` to sample the proposed interpretation. Suggestions are not proof that a profile is correct. A user may edit mappings, inspect the raw and normalized preview, load/save a JSON profile, and choose a rotated source family when applicable.

`ImportProfileValidator` checks configuration validity. Preview then uses the actual importer through `BuiltInImporterRegistry`, so it can expose content-dependent parsing failures that structural validation cannot detect. The normal preview is bounded; large structured-document previews require explicit background execution rather than forcing expensive work into the configuration UI.

### Importer boundary

[`ILogImporter`](../src/importing/ILogImporter.h) is the common import interface. `BuiltInImporterRegistry` registers the built-in JSON Lines, structured JSON, structured XML, regex text, key-value text, Syslog, IIS W3C, CSV, and TSV importers. Apache/Nginx and Windows Event XML recognition use profiles/presets over those importer families rather than unrelated parser architectures.

`JsonObjectRecordMapper` provides common object-to-record behavior for structured extracted values. Other importers normalize their native structures before or while applying a compatible profile. Every imported record should retain raw source text and source metadata appropriate to its format.

`SourceFamilyImportService` imports **each physical file independently in an ordered rotated family**, combines results and diagnostics, and propagates progress and cancellation. A rotated physical file is not a same-path source generation; these are separate concepts.

### Execution and memory

The normal large-file import flow is coordinated asynchronously from `MainWindow`, using Qt Concurrent and completion/progress handling so parsing need not block the Qt event loop. Streamed formats can report determinate progress and check for cancellation. Structured JSON is handled differently from the streamed line-based import paths; its complete-document processing and progress behavior should not be described as constant-memory streaming.

An imported investigation retains normalized records and analysis/presentation state in memory. Streamed source reading therefore does **not** imply fixed memory usage. Refer to the measured, environment-specific observations in [Performance Notes](performance.md); they are not published throughput or maximum-file-size guarantees.

## 4. Investigation and analysis

[`InvestigationSession`](../src/workspace/InvestigationSession.h) owns the investigation's profile, records through `InvestigationController`, source/backing metadata, import diagnostics, user annotations, and session-specific presentation preferences. [`InvestigationWorkspace`](../src/workspace/InvestigationWorkspace.h) owns multiple sessions and identifies the active session; it does not replace per-session data ownership.

`InvestigationController` coordinates an `InvestigationTableModel` source model with an `InvestigationFilterProxyModel`. The source model exposes canonical and discovered dynamic custom columns; the proxy handles sorting and filters. Selection and annotation actions should use the correct **proxy-to-source/record-ID mapping**, not assume the visible row number identifies the underlying record.

The filter proxy supports severity, subsystem, event code, entity, time range, dynamic custom fields, text search, finding-status filters, and bookmarks. A record ID anchors annotations in `InvestigationStateStore`, which stores bookmarks, notes, and finding classifications separately from the original log content.

Analysis lives in focused classes in `src/analysis/`, including timeline buckets, issue grouping, frequencies/trends, cadence, bursts, and session comparison. `InvestigationSessionView` composes the user-facing panels and coordinates filtering, navigation, selection, and derived-view refreshes. **An analysis result and the current Event Table need not always use identical populations**; use each feature's stated scope rather than assuming every number refers to currently visible rows. See [Investigating Logs](investigating-logs.md), [Findings](findings.md), and [Comparing Sessions](comparing-sessions.md).

The workspace presentation layer uses `WorkspaceDocument`, `WorkspaceDocumentHost`, investigation and comparison documents, and independent detached top-level windows. Document placement is separate from the investigation's data identity, allowing sessions/comparisons to move between window hosts without becoming separate investigations. Interface scaling and constrained-height section behavior are UI responsibilities rather than changes to normalized evidence.

## 5. Live following and source identity

Live following extends **the same session and table model used for static investigations**. It does not create a parallel live-only investigation representation.

[`LiveSessionFollowCoordinator`](../src/live/LiveSessionFollowCoordinator.h) owns a `LiveFileFollower` and format-specific incremental adapters and contexts. A timer polls the external file (default 250 ms); supported line-oriented formats frame complete records, while structured JSON/XML use their own incremental parsing contexts. Incomplete trailing content remains pending rather than being treated as a finished record. Structured XML live following requires a repeatable configured record path.

Completed records are admitted through `InvestigationSession::appendLiveImportResult` and the controller's `appendRecords` path. The table/proxy can display ordinary new rows incrementally; the implementation also accounts for new dynamic columns that require a model-column rebuild. Higher-cost derived panels are refreshed on a coalesced schedule rather than recalculated after each polling event (the reviewed UI has a roughly 750 ms coalescing interval).

Physical source continuity is explicit:

- **Rotation:** multiple independently imported physical files can belong to one logical source family. The active file remains the live-follow target.
- **Truncation/replacement:** new content at the active path can constitute a new physical **generation** of the same logical source. Generation changes must not be confused with separate rotated files.
- **Relocation:** a verified move can update an external `sourcePath` without changing its stable `logicalSourceKey`.

This distinction is especially important when evidence from earlier generations has been admitted and the current file no longer contains those bytes. See [Live Following](live-following.md) for user-visible operations, and [Live-Log Generator](live-log-generator.md) for reproducible partial-write, truncation, replacement, and rotation scenarios.

## 6. Persistence and evidence continuity

TraceScope distinguishes three [`InvestigationSessionBackingMode`](../src/workspace/InvestigationSessionBacking.h) states:

| Mode | Meaning |
| --- | --- |
| `SourceBacked` | An external source is the session's active backing. |
| `SnapshotBacked` | A durable TraceScope investigation snapshot supplies the saved normalized evidence independently of the current external file. |
| `Hybrid` | Both snapshot evidence and a verified external-source binding exist; reconstruction preserves the snapshot's admitted evidence while reconciling eligible source material. |

A `.tsinv` investigation snapshot captures the normalized evidence, the interpretation/profile that produced it, original import counts and diagnostics, and optionally source-continuity information. Its `sourceFidelity` distinguishes normalized evidence from a complete original-source capture; **a normalized-only snapshot is not a full replacement for original source bytes or arbitrary re-import**.

Workspace persistence is a related but broader operation. `WorkspaceSerializer` manages a versioned workspace manifest for investigations, comparisons, presentation/document layout and references. [`WorkspaceSavePackageService`](../src/workspace/WorkspaceSavePackageService.cpp) writes per-session snapshot sidecars **before** committing the manifest with `QSaveFile`; cleanup of superseded sidecars occurs only after the new manifest commits. Keep a saved workspace file and its associated snapshot directory together.

The source-recovery and transition code lives in `InvestigationSessionRestorationService`, `HybridInvestigationCandidateBuilder`, and `HybridInvestigationReconstructionService`. Source relocation/reconnection, ordinary reload, snapshot-only preservation, and **Use Source as Authoritative** are intentionally different choices. In particular, choosing source-authoritative behavior deliberately replaces the saved-evidence interpretation with current source content; it must not occur implicitly during normal Hybrid recovery.

See [Saving and Restoring](saving-and-restoring.md) for the exact user-facing implications and recovery choices.

## 7. Comparison and reporting

Comparison uses `InvestigationSessionComparisonAnalyzer` for deterministic analysis of selected baseline and comparison record sets. [`InvestigationComparisonSnapshotBuilder`](../src/workspace/InvestigationComparisonSnapshotBuilder.cpp) captures the **explicit Baseline → Comparison orientation** and derived results into a point-in-time comparison document. Later source-file changes, navigation or active filters should not silently redefine an existing captured comparison.

Export code in `src/exporting/` handles visible-record CSV, findings CSV, and offline HTML investigation reports. Report construction is split into configuration/selection, frozen report snapshot building, HTML rendering, and export; the renderer should consume captured report data rather than re-query mutable live session state during rendering. Captured comparison material can be included in a report without recalculating its original meaning.

For scope and user operations, see [Comparing Sessions](comparing-sessions.md) and [Reporting and Export](reporting-and-export.md).

## 8. Code map and extension points

| Change | Start by inspecting | Related verification |
| --- | --- | --- |
| New source format | `src/importing/ILogImporter.h`, built-in registry, profile presets, and format suggestions | Corresponding importer tests, sample/profile pair, preview and diagnostics |
| New mapping rule | `ImportProfile`, serializer, validator and `JsonObjectRecordMapper` or the importer-specific normalization | Serialization, validation, importer and preview tests |
| New table field/filter | `InvestigationTableModel`, `InvestigationFilterProxyModel`, `InvestigationController`, `InvestigationSessionView` | Model, proxy, controller, filter and UI tests |
| New analysis | Focused analyzer under `src/analysis/` and the owning investigation panel | Deterministic analyzer tests, filter/scope behavior |
| New live behavior | `LiveFileFollower`, coordinator, format adapters, session append and source identity | Framing/adapters/coordinator tests plus real-file generator scenarios |
| Persistence change | Backing model, snapshots, serializers, restoration, workspace save package | Versioned round-trips, reload/continuity and failure-path tests |
| New report section | Report selection/configuration, snapshot builder, HTML renderer | Immutable capture, escaped rendering and export tests |
| Workspace UI change | Document host, session/comparison documents, relevant presentation state | Workspace document/presentation tests and manual multi-window checks |

Changes that span live following, persistence, and source lifecycle deserve extra scrutiny: each can change which records remain authoritative after a physical file is replaced. For how to run and extend the automated suites, see [Testing](testing.md). For reproducible development setup, see [Building from Source](building-from-source.md).

---

**Release follow-up:** Recheck this document after the planned targeted development pass—particularly live-follow concurrency/presentation, session comparison scope, snapshot/recovery semantics, report capture, and UI document ownership. Do not turn prospective enhancements into claims about the current implementation.
