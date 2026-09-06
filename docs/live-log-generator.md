# TraceScope Live-Log Generator

## Purpose

The TraceScope live-log generator is a small standalone command-line utility used to exercise Phase 15 live-file-following behavior under realistic, reproducible conditions.

The generator behaves like an external application writing logs to disk. TraceScope has no knowledge of or dependency on the generator; it observes only the resulting file.

The utility is intended as a permanent manual test artifact rather than a secondary application or generalized synthetic-data framework.

Its primary goals are to:

* produce deterministic, human-readable live-log scenarios
* render the same semantic scenario into multiple TraceScope-supported formats where practical
* exercise realistic file-writing behavior such as ordinary appends, bursts, idle periods, partial writes, truncation, replacement, and rotation
* make Phase 15 manual verification repeatable
* document the kinds of live-file conditions TraceScope has been tested against

## Architecture

The generator separates scenario meaning, playback behavior, and format serialization.

```text
Prewritten semantic scenario
          ↓
Generic scenario player
   timing / bursts / waits
 partial writes / truncate
       rotate / replace
          ↓
Runtime-selected renderer
 JSONL / CSV / logfmt / Syslog / ...
          ↓
Actual file on disk
          ↓
TraceScope Live Follow
```

TraceScope and the generator remain separate processes.

The generator may use Qt Core for JSON handling, command-line parsing, timestamps, and file operations, but it should not depend on TraceScope investigation, workspace, filtering, comparison, or UI classes.

## Semantic Record Model

Scenario records represent log meaning rather than preformatted source text.

The initial semantic record model contains:

* timestamp offset
* severity
* subsystem
* event code
* entity ID
* message
* custom attributes

Conceptually:

```cpp
struct LiveLogRecord
{
    qint64 timestampOffsetMs = 0;

    QString severity;
    QString subsystem;
    QString eventCode;
    QString entityId;
    QString message;

    QHash<QString, QVariant> attributes;
};
```

The generator should not reuse `InvestigationRecord` directly. `InvestigationRecord` represents TraceScope's normalized investigation domain and contains TraceScope-specific identity, raw-source, and source-metadata state that does not belong in an external log producer.

Custom attributes should initially be limited to values that serialize naturally across representative formats:

* string
* number
* boolean
* null

Nested object and array attributes are outside the initial generator scope because many supported line-oriented formats cannot represent them consistently.

## Time Model

Scenario playback time and semantic event time are separate concepts.

`timestampOffsetMs` defines the timestamp of a generated record relative to the beginning of the scenario.

Playback directives define when bytes are actually written to disk.

This allows playback speed to be accelerated or slowed without changing the logical timing represented by the incident itself.

For example, an incident representing five minutes of application activity may be played at `10x` speed in approximately thirty seconds while still producing timestamps spanning five minutes.

## Scenario Format

Scenarios use versioned JSON.

Example:

```json
{
  "schemaVersion": 1,
  "name": "Field Gateway Live Degradation",
  "description": "Deterministic gateway degradation and recovery scenario.",
  "steps": [
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 0,
        "severity": "INFO",
        "subsystem": "Gateway",
        "eventCode": "GW_START",
        "entityId": "gateway-17",
        "message": "Gateway startup completed.",
        "attributes": {
          "site": "north-yard",
          "firmware": "4.2.1"
        }
      }
    },
    {
      "type": "wait",
      "durationMs": 2000
    },
    {
      "type": "record",
      "record": {
        "timestampOffsetMs": 2000,
        "severity": "WARN",
        "subsystem": "Transport",
        "eventCode": "UPSTREAM_LATENCY",
        "entityId": "gateway-17",
        "message": "Upstream response latency exceeded expected range.",
        "attributes": {
          "latencyMs": 1840,
          "endpoint": "collector-a"
        }
      }
    }
  ]
}
```

### Scenario Step Types

The initial scenario language supports:

#### `record`

Writes one semantic log record through the selected format renderer.

A record may optionally request special write behavior.

Example normal record:

```json
{
  "type": "record",
  "record": {
    "timestampOffsetMs": 2000,
    "severity": "WARN",
    "subsystem": "Transport",
    "eventCode": "UPSTREAM_LATENCY",
    "entityId": "gateway-17",
    "message": "Upstream response latency exceeded expected range.",
    "attributes": {
      "latencyMs": 1840
    }
  }
}
```

Example partial write:

```json
{
  "type": "record",
  "write": {
    "mode": "partial",
    "splitFraction": 0.55,
    "holdMs": 1000
  },
  "record": {
    "timestampOffsetMs": 2200,
    "severity": "ERROR",
    "subsystem": "Transport",
    "eventCode": "UPSTREAM_TIMEOUT",
    "entityId": "gateway-17",
    "message": "Upstream request timed out."
  }
}
```

The renderer produces the complete serialized record first. The scenario player then controls how those bytes reach disk.

#### `wait`

Pauses playback before processing the next step.

```json
{
  "type": "wait",
  "durationMs": 3000
}
```

Wait duration is affected by the runtime playback-speed multiplier.

#### `truncate`

Truncates the currently active output file and then continues playback.

Formats that require a header or document prefix must restore whatever structural content is required before subsequent records are written.

#### `replace`

Replaces the active file with a newly created file at the same path.

This tests the case where an application recreates its current log rather than truncating the existing file in place.

#### `rotate`

Moves the current output file to a rotated filename and creates a new active file at the original output path.

This represents common file-rotation behavior while allowing TraceScope to continue observing the configured path.

No dedicated `burst` step is required. A burst is represented by several deterministic `record` steps separated by short waits.

## Runtime Options

The initial command-line interface should support:

```text
TraceScopeLiveLogGenerator
    --scenario <path>
    --output <path>
    --format <format-id>
    [--speed <multiplier>]
    [--loop]
```

Example:

```text
TraceScopeLiveLogGenerator
    --scenario samples/live/field-gateway-live-incident.json
    --output live-test.jsonl
    --format jsonl
    --speed 4
```

### `--scenario`

Path to the scenario JSON file.

### `--output`

Path to the file written by the generator.

### `--format`

Selects the runtime renderer.

### `--speed`

Playback-speed multiplier.

A value of:

* `1` means scenario playback time
* `2` means twice as fast
* `0.5` means half speed

Playback speed affects waits and partial-write hold periods. It does not alter semantic record timestamps.

### `--loop`

Restarts the scenario after it completes.

Looping is optional. Finite execution remains the default so scenarios are deterministic and easy to verify.

Additional controls such as starting from a particular step should be added only if Phase 15 testing demonstrates a concrete need.

## Renderer Boundary

Playback behavior and serialization must remain separate.

A renderer is responsible only for expressing a semantic record in a particular log format.

Conceptually:

```cpp
class ILogRecordRenderer
{
public:
    virtual ~ILogRecordRenderer() = default;

    virtual QByteArray initialContent() const = 0;

    virtual QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const = 0;
};
```

The scenario player owns:

* delays
* playback-speed scaling
* writing bytes
* flushing
* partial writes
* truncation
* replacement
* rotation
* looping

Renderers should not sleep, manipulate the output file, or know about scenario playback state.

## Format and Live-Behavior Capability Matrix

Static TraceScope format support does not imply that every source format is naturally suitable for append-style live following.

The generator and Phase 15 implementation should respect the following boundary.

| Format             | TraceScope importer path | Semantic fidelity               | Natural append stream | Partial-record test                    | Truncate                      | Replace / rotate              | Phase 15 role                |
| ------------------ | ------------------------ | ------------------------------- | --------------------- | -------------------------------------- | ----------------------------- | ----------------------------- | ---------------------------- |
| JSON Lines         | `json-lines`             | Full                            | Yes                   | Yes                                    | Yes                           | Yes                           | Primary                      |
| CSV                | `csv`                    | Full with fixed scenario schema | Yes                   | Yes                                    | Yes, with header regeneration | Yes, with header regeneration | Primary                      |
| TSV                | `tsv`                    | Full with fixed scenario schema | Yes                   | Yes                                    | Yes, with header regeneration | Yes, with header regeneration | Primary                      |
| key-value / logfmt | `key-value`              | Full                            | Yes                   | Yes                                    | Yes                           | Yes                           | Primary                      |
| generic regex text | `regex-text`             | Full with matching profile      | Yes                   | Yes                                    | Yes                           | Yes                           | Primary / secondary          |
| Syslog RFC 5424    | `syslog`                 | High                            | Yes                   | Yes                                    | Yes                           | Yes                           | Primary / secondary          |
| Syslog RFC 3164    | `syslog`                 | Reduced                         | Yes                   | Yes                                    | Yes                           | Yes                           | Secondary                    |
| IIS W3C            | `iis-w3c`                | Access-log oriented             | Yes                   | Yes                                    | Yes, with header regeneration | Yes, with header regeneration | Secondary                    |
| Apache Common      | `regex-text` preset      | Access-log oriented             | Yes                   | Yes                                    | Yes                           | Yes                           | Secondary                    |
| Apache Combined    | `regex-text` preset      | Access-log oriented             | Yes                   | Yes                                    | Yes                           | Yes                           | Secondary                    |
| Nginx Combined     | `regex-text` preset      | Access-log oriented             | Yes                   | Yes                                    | Yes                           | Yes                           | Secondary                    |
| Structured JSON    | `structured-json`        | Full                            | No                    | Not applicable as line-follow behavior | Whole-document rewrite        | Yes                           | Replacement/snapshot testing |
| Structured XML     | `xml`                    | Full                            | No                    | Not applicable as line-follow behavior | Whole-document rewrite        | Yes                           | Replacement/snapshot testing |
| Windows Event XML  | `xml` preset             | High                            | No                    | Not applicable as line-follow behavior | Whole-document rewrite        | Yes                           | Replacement/snapshot testing |

Structured JSON arrays and XML documents are not ordinary append streams. Appending arbitrary serialized records after the closing JSON array or XML root would make the document invalid.

Phase 15 should therefore not claim normal append-follow semantics for those formats unless a valid incremental representation is explicitly implemented.

For structured-document formats, replacement or whole-document rewrite behavior may still be useful for testing file-change detection.

## Format Fidelity

The scenario describes one semantic incident, but not every output format needs to preserve every canonical field.

Application-oriented formats such as JSON Lines, CSV, TSV, key-value, and configured regex text can preserve the complete semantic model.

Operational formats may intentionally expose a different subset.

Examples include:

* Apache/Nginx access logs, where HTTP request information is more natural than severity or subsystem
* IIS W3C logs, where HTTP fields are primary
* Syslog RFC 3164, which has less structured metadata than RFC 5424
* Windows Event XML, which does not provide a universal application-domain entity identifier

Renderers should preserve as much useful scenario meaning as the target format naturally supports without inventing misleading fields.

## Determinism

Scenarios are deterministic by default.

The generator should not randomly choose:

* severity
* incident timing
* failed entities
* event codes
* recovery behavior
* file disruptions

When a scenario describes a degradation followed by an error burst and recovery, the same sequence should occur on every run.

Runtime-generated values may include:

* absolute timestamps derived from the run start
* file rotation suffixes
* optional sequence values required by a target format

These values must not alter the semantic incident story.

## Phase 15 Verification Role

The generator should eventually support manual verification of:

* appended-record following
* pause
* resume
* catch-up after resume
* partial-line handling
* truncation handling
* file replacement
* file rotation
* burst ingestion
* filtering while records arrive
* summary updates
* analytics updates
* live comparison behavior, if implemented

The generator itself does not verify TraceScope behavior. It creates known external conditions against which TraceScope can be observed and tested.

## Stable Snapshot Boundaries

Live following must not weaken existing immutable behavior.

A live investigation session may continue changing as its source file grows.

However:

* an existing immutable comparison snapshot must remain unchanged
* an already-generated report must remain unchanged
* persisted immutable comparisons must retain their captured meaning

If Phase 15 introduces continuously updating live comparisons, those comparisons must remain explicitly distinct from immutable comparison snapshots and should be capable of being frozen into a stable snapshot for persistence or reporting.

## Scope Discipline

The live-log generator is not intended to become:

* a general synthetic-data platform
* a configurable log-design application
* a fuzzing framework
* a load-testing system
* a network log collector
* a monitoring service
* a TraceScope runtime dependency

Features should be added only when they materially improve reproducible Phase 15 verification.

The utility should remain small enough that its behavior can be easily understood and trusted.