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

The scenario language supports:

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

The command-line interface supports:

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
    --scenario samples/live/field-gateway-live-scenario.json
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

A renderer is responsible for expressing semantic records and any format-specific file boundaries required to produce a realistic live source.

Conceptually:

```cpp
class ILogRecordRenderer
{
public:
    virtual ~ILogRecordRenderer() = default;

    virtual QByteArray initialContent() const = 0;

    virtual QByteArray recordSeparator() const
    {
        return {};
    }

    virtual QByteArray renderRecord(
        const LiveLogRecord &record,
        const QDateTime &scenarioStart
        ) const = 0;

    virtual QByteArray finalContent() const
    {
        return {};
    }
};
```

The four renderer boundaries have distinct purposes:

* `initialContent()` is written when an output file is first created and after a truncate, replacement, or rotation creates a fresh active file.
* `recordSeparator()` is written only between complete records. Most line-oriented renderers return no separator because their record output already contains its own line termination. Structured JSON uses this boundary for commas between array elements.
* `renderRecord()` serializes one semantic record.
* `finalContent()` closes any outer format container required for a normally completed file. It is written at normal playback completion and before a rotated file is preserved.

Renderers remain stateless. The scenario player tracks the number of completed records written to the current file so separator behavior resets correctly after truncation, replacement, and rotation.

The scenario player owns:

* delays
* playback-speed scaling
* writing bytes
* flushing
* partial writes
* record-separator placement
* truncation
* replacement
* rotation
* structured-file finalization
* looping

A partial record does not count as complete until its final bytes are written.

Truncation and replacement intentionally do not finalize the previous structured document before discarding it. These steps model abrupt source lifecycle behavior. Rotation does finalize the previous structured document before preserving it, so the rotated file is independently valid.

Renderers should not sleep, manipulate the output file, or know about scenario playback state.

## Format and Live-Behavior Capability Matrix

Phase 15 live following should support all current TraceScope source families when the configured import behavior identifies a usable repeatable record structure. The physical ingestion strategy differs by format; live support does not require every format to behave like a newline-delimited text file.

The generator should exercise both line-oriented append streams and structured open-container streams.

| Format             | TraceScope importer path | Semantic fidelity               | Live strategy                              | Partial-record test | Truncate / replacement / rotation                  | Generator status |
| ------------------ | ------------------------ | ------------------------------- | ------------------------------------------ | ------------------- | -------------------------------------------------- | ---------------- |
| JSON Lines         | `json-lines`             | Full                            | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| CSV                | `csv`                    | Full with fixed scenario schema | Line-oriented append with header           | Yes                 | Yes, with header regeneration                      | Implemented      |
| TSV                | `tsv`                    | Full with fixed scenario schema | Line-oriented append with header           | Yes                 | Yes, with header regeneration                      | Implemented      |
| key-value / logfmt | `key-value`              | Full                            | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| generic regex text | `regex-text`             | Full with matching profile      | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| Syslog RFC 5424    | `syslog-rfc5424`                 | High                            | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| Syslog RFC 3164    | `syslog-rfc3164`                 | Reduced                         | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| IIS W3C            | `iis-w3c`                | Access-log oriented             | Line-oriented append with header           | Yes                 | Yes, with header regeneration                      | Planned          |
| Apache Common      | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Planned          |
| Apache Combined    | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Planned          |
| Nginx Combined     | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Planned          |
| Structured JSON    | `structured-json`        | Full                            | Profiled open record-array container       | Yes                 | Yes, with fresh container / rewrite reconciliation | Implemented      |
| Structured XML     | `xml`                    | Full                            | Profiled open repeated-record container    | Yes                 | Yes, with fresh container / rewrite reconciliation | Implemented      |
| Windows Event XML  | `xml` preset             | High                            | Profiled open `Events`-style container     | Yes                 | Yes, with fresh container / rewrite reconciliation | Implemented      |

### Line-Oriented Sources

Line-oriented sources expose complete records through ordinary file growth.

An incomplete trailing record remains pending until its physical write finishes. Header-based formats regenerate their initial header content whenever a lifecycle event creates a fresh active file.

### Structured Open-Container Sources

Structured JSON, structured XML, and Windows Event XML are not excluded from true live following.

When a profile identifies a repeatable record structure, the producer may keep an outer document container open while complete child records are appended.

Conceptually:

```text
outer container
    complete record
    complete record
    partially written record   ← pending
EOF
```

Completed records are independently usable even though the trailing record and outer container are not yet complete.

For structured JSON, live append behavior requires a profiled repeatable record array. The generator writes the array/container prefix through `initialContent()`, inserts separators only between completed elements, writes each record independently, and closes the structure through `finalContent()` after normal completion.

For structured XML and Windows Event XML, live append behavior requires a profiled repeated-record element path. The generator writes the document/container prefix, appends complete record elements, and leaves the outer container intentionally open during playback.

Partial-write behavior applies to structured formats exactly as it does to line-oriented formats: the renderer first produces the complete serialized record, then the scenario player exposes only the configured first fraction of its bytes during the hold interval.

A normally completed structured playback must leave a valid standalone document. Rotation finalizes the old document before preserving it and starts a fresh container at the active path. Truncation and replacement intentionally model abrupt lifecycle events and therefore start a fresh container without first repairing the discarded document.

Structured sources may also be maintained by a producer that repeatedly rewrites, truncates, or replaces a formally closed document. Phase 15 should treat those cases through the corresponding file-lifecycle and reconciliation behavior rather than requiring byte-append semantics.

Appending an unrelated second complete JSON or XML document after an already closed document is not a supported live-source model.

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

* appended-record following for every supported line-oriented source family
* structured JSON record-array following
* structured XML repeated-record following
* Windows Event XML repeated-record following
* pause
* resume
* catch-up after resume
* incomplete trailing-line handling
* incomplete structured JSON record handling
* incomplete structured XML / Windows Event XML record handling
* still-open structured outer containers
* normal structured-stream finalization
* truncation handling
* file replacement
* file rotation
* fresh header or container creation after lifecycle events
* valid finalized structured files after rotation
* burst ingestion
* filtering while records arrive
* summary updates
* analytics updates
* live comparison behavior, if implemented

The generator itself does not verify TraceScope behavior. It creates known external conditions against which TraceScope can be observed and tested.

## Generator Completion Boundary

The live-log generator is not complete merely because a representative subset of renderers works.

Before Phase 15 implementation moves into TraceScope's live-follow ingestion and presentation behavior, the utility should provide reproducible generation for every currently supported source family using the appropriate live strategy.

Generator completion therefore requires representative coverage for:

* JSON Lines
* CSV
* TSV
* key-value / logfmt
* generic regex text
* Syslog RFC 5424
* Syslog RFC 3164
* IIS W3C
* Apache Common
* Apache Combined
* Nginx Combined
* Structured JSON
* Structured XML
* Windows Event XML

Across those formats, the generator must also exercise the lifecycle behaviors relevant to the format:

* ordinary record growth
* deterministic semantic timing independent of playback speed
* partial physical writes
* truncation
* replacement
* rotation
* header regeneration where required
* structured-container initialization, record separation, and normal finalization where required

TraceScope live-follow implementation should begin only after this generator-side coverage is complete and manually verifiable.

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