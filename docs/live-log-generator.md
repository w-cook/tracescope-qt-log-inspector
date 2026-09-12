# TraceScope Live-Log Generator

## Purpose

The TraceScope live-log generator is a small standalone test utility used to exercise Phase 15 live-file-following behavior under realistic, reproducible conditions. It provides both a command-line generator and a thin Qt Widgets launcher for convenient manual playback.

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

The core generator uses Qt Core for JSON handling, command-line parsing, timestamps, and file operations. A separate Qt Widgets launcher provides a thin graphical front end and invokes the command-line generator through `QProcess`.

The launcher does not duplicate scenario loading, renderer validation, playback, looping, or file-lifecycle behavior. The command-line generator remains the single source of truth for those responsibilities.

Neither executable depends on TraceScope investigation, workspace, filtering, comparison, or UI classes.

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

## Scenario Library

The repository includes a small permanent library of deterministic scenarios under `samples/live/`.

These scenarios are intended to serve two related purposes:

* provide repeatable manual-test inputs for Phase 15 live-file-following behavior
* provide realistic, presentation-quality data for screenshots and demonstrations of live following, filtering, analytics, navigation, and comparison behavior

Scenario content should therefore remain technically deterministic without looking like artificial test fixtures. Service names, event codes, entities, messages, and attributes should form a coherent fictional production story that a TraceScope user could reasonably imagine investigating.

The scenario library is intentionally small. Each scenario has a distinct role rather than attempting to make every scenario exercise every file lifecycle behavior or every output format.

| Scenario | Primary story and demonstration role | Lifecycle emphasis | Intended format families |
| -------- | ------------------------------------ | ------------------ | ------------------------ |
| `field-gateway-live-scenario.json` | A production field gateway moves from healthy telemetry delivery into rising upstream latency, retries, timeout pressure, queue growth, and recovery. Useful for live-follow, filtering, severity/event-code analysis, timeline activity, and investigation screenshots. | ordinary growth, waits, partial write, burst-like degradation, in-place truncation, recovery | JSON Lines, CSV, TSV, key-value / logfmt, generic regex text, Syslog RFC 5424, Syslog RFC 3164, Structured JSON, Structured XML, Windows Event XML |
| `web-access-live-scenario.json` | Representative production web traffic including health checks, page and asset requests, authenticated API activity, query traffic, 401/403/404 responses, a brief 503 failure burst, and recovery. Useful for access-log live-follow demonstrations. | ordinary growth, waits, partial write, short request/error bursts | IIS W3C, Apache Common, Apache Combined, Nginx Combined |
| `checkout-api-healthy-baseline-live-scenario.json` | Healthy checkout processing with inventory reservation, payment authorization, and order creation. Intended primarily as a clean baseline for live-comparison demonstrations. | ordinary growth and realistic service-to-service timing | application-oriented and operational formats |
| `checkout-api-payment-regression-live-scenario.json` | A related checkout session in which a release is followed by elevated payment-provider latency, retry behavior, timeout, checkout failure, circuit-breaker activity, and eventual recovery. Intended as the regression side of live-comparison screenshots and demonstrations. | ordinary growth, warning/error burst, partial write, recovery | application-oriented and operational formats |
| `warehouse-sync-deployment-rotation-live-scenario.json` | A warehouse synchronization worker is replaced during a rolling deployment, processes accumulated backlog in a catch-up burst, and continues under sustained load before returning to steady state. | same-path replacement, catch-up burst, partial write, multiple rotations, recovery | application-oriented and operational formats |

### Scenario Roles

The five scenarios are complementary.

`field-gateway-live-scenario.json` remains the primary truncation scenario. Its in-place truncation deliberately discards earlier active-file content before the gateway continues writing recovery records to the same path.

`warehouse-sync-deployment-rotation-live-scenario.json` is the primary replacement and rotation scenario. Records written before the replacement are intentionally discarded. After replacement, the scenario produces a catch-up burst and then rotates the active file twice, producing sequential `.1` and `.2` artifacts before continuing in the active file.

The two checkout scenarios form an intentional baseline/regression pair. They describe related versions of the same fictional production system so that a comparison view can show a meaningful contrast rather than two unrelated synthetic sessions.

`web-access-live-scenario.json` is intentionally access-log specific. It uses neutral HTTP attributes that can be rendered naturally into IIS W3C, Apache Common, Apache Combined, and Nginx Combined output without forcing application-domain severity or subsystem concepts into formats that do not naturally contain them.

### Scenario Library Coverage

Taken together, the scenario library exercises:

* ordinary append growth
* deterministic waits and semantic timing
* short record bursts
* partial physical writes
* in-place truncation
* same-path replacement
* single and multiple rotation
* recovery after degraded behavior
* healthy-versus-regressed comparison material
* realistic warning/error distributions
* multiple subsystems, event codes, and entities
* HTTP success, client-error, authorization-error, and server-error traffic

The library is not a load-testing corpus and is not intended to generate large random datasets. Scenarios should remain readable enough that a developer can inspect the JSON and understand the complete incident story.

Renderer-specific tests remain responsible for validating serialization details. The scenario library provides realistic cross-format and lifecycle inputs for manual verification and demonstrations.

## Graphical Launcher

`TraceScopeLiveLogGeneratorLauncher` provides a small Qt Widgets interface for manually running the generator without entering command-line arguments.

The launcher exposes:

* scenario-file selection
* output-file selection
* output-format selection
* playback-speed selection
* optional looping
* selectable loop behavior: continuous append or output restart
* Play and Stop controls
* playback status
* captured generator standard output and error output

The launcher starts `TraceScopeLiveLogGenerator` as a child process through `QProcess`. The generator executable is expected to reside in the same application directory as the launcher.

Starting playback constructs the same arguments that may be supplied directly to the command-line interface. The launcher therefore does not introduce a separate playback path or alternate interpretation of scenarios.

While playback is active, scenario, output, format, speed, loop, and loop-behavior controls are disabled. The loop-behavior control is available only when looping is enabled. Normal completion restores the controls and reports that playback completed.

For looping scenarios, Stop intentionally terminates the child generator process and reports the stop as an expected user action rather than a playback failure. Closing the launcher while playback is active also terminates the child process so the generator is not left running independently.

The launcher is intentionally limited to playback convenience. It does not provide:

* scenario editing
* scenario history
* saved launcher presets
* generated-record counters
* advanced progress tracking
* renderer configuration
* TraceScope integration

Those features would add a second application surface without materially improving Phase 15 verification.

The command-line executable remains available for automated tests, direct invocation, and any workflow where a graphical launcher is unnecessary.

## Runtime Options

The command-line interface supports:

```text
TraceScopeLiveLogGenerator
    --scenario <path>
    --output <path>
    --format <format-id>
    [--speed <multiplier>]
    [--loop]
    [--loop-mode <append|restart>]
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

Continuously replays the scenario after each completed iteration.

Looping is optional. Finite execution remains the default so scenarios are deterministic and easy to verify.

By default, looping uses `append` behavior. The next scenario iteration continues writing to the same active physical output file without truncating or recreating it. Record-separator state and other active-file state therefore continue across the iteration boundary.

Each new iteration receives a fresh scenario start timestamp while retaining the same physical source unless the scenario itself performs an explicit truncate, replacement, or rotation step.

### `--loop-mode`

Selects the physical output behavior between loop iterations.

Supported values are:

* `append` — continues the next scenario iteration in the current active output file. This is the default and represents an application continuing to produce workload into the same live source.
* `restart` — finalizes the completed iteration where required by the renderer, recreates the active output file, and begins the next scenario iteration from a fresh source. This preserves the generator's original looping behavior for testing source restarts and whole-file rewrites.

`--loop-mode` defaults to `append`.

Explicit scenario lifecycle steps remain independent of the selected loop mode. A scenario may still truncate, replace, or rotate its active source during an iteration.

Continuous append looping:

```text
TraceScopeLiveLogGenerator
    --scenario samples/live/checkout-api-payment-regression-live-scenario.json
    --output live-test.jsonl
    --format jsonl
    --speed 0.5
    --loop
```

Explicit restart looping:

```text
TraceScopeLiveLogGenerator
    --scenario samples/live/checkout-api-payment-regression-live-scenario.json
    --output live-test.jsonl
    --format jsonl
    --speed 0.5
    --loop
    --loop-mode restart
```

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

Loop boundaries are also owned by the scenario player. In `append` mode, an ordinary iteration boundary does not finalize, truncate, replace, or reopen the active output file; playback simply continues into the same source. In `restart` mode, the completed iteration is finalized where required and the active output file is recreated before the next iteration begins.

This distinction allows ordinary looping to model continuous application workload while retaining the previous whole-file restart behavior as an explicit test mode.

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
| IIS W3C            | `iis-w3c`                | Access-log oriented             | Line-oriented append with header           | Yes                 | Yes, with header regeneration                      | Implemented      |
| Apache Common      | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| Apache Combined    | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
| Nginx Combined     | `regex-text` preset      | Access-log oriented             | Line-oriented append                       | Yes                 | Yes                                                | Implemented      |
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

The generator and scenario library are designed to support manual verification of:

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

The implemented renderer set, five-scenario library, and graphical launcher provide the planned source-family, lifecycle, and manual-playback inputs for this boundary. Remaining generator-side completion work should focus on the final documented manual verification pass rather than expanding the scenario language, adding speculative formats, or growing the launcher beyond its testing role.

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