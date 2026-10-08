# Supported Log Formats in TraceScope

TraceScope imports supported file-based log formats into a common normalized record model used by investigation sessions. The selected **Format** determines how source records are read; an **import profile** determines how the values extracted from those records map to TraceScope's canonical and custom fields.

This reference describes the built-in importer families, recognized layouts, source requirements, bundled examples, and Live Following compatibility in TraceScope v1.0.

For the step-by-step import workflow, see [Importing Logs](importing-logs.md). For profile fields and mapping rules, see [Import Profiles](import-profiles.md).

**Having trouble identifying a format or source layout?** [Jump straight to Troubleshooting](#troubleshooting).

## Find the information you need

| I want to... | Go to |
| --- | --- |
| See every built-in importer | [1. Supported formats at a glance](#1-supported-formats-at-a-glance) |
| Understand automatic format suggestions | [2. Format suggestions](#2-format-suggestions) |
| Check the requirements of a specific format | [3. Format reference](#3-format-reference) |
| Find a matching sample and profile | [4. Bundled examples](#4-bundled-examples) |
| Check Live Following support | [5. Live Following compatibility](#5-live-following-compatibility) |
| Understand field and format limitations | [6. Field availability and limitations](#6-field-availability-and-limitations) |

## 1. Supported formats at a glance

TraceScope v1.0 contains **nine built-in importer families**.

Some familiar source layouts use a preset or profile over one of those importers rather than a separate parser.

| Format | Importer ID | Source model | Typical extensions |
| --- | --- | --- | --- |
| **JSON Lines** | `json-lines` | One JSON object per physical record line | `.jsonl`, `.ndjson`, `.log` |
| **Structured JSON** | `structured-json` | One JSON document containing an object or record collection | `.json` |
| **CSV** | `csv` | Header-based comma-separated records | `.csv` |
| **TSV** | `tsv` | Header-based tab-separated records | `.tsv` |
| **Key-Value / logfmt** | `key-value` | One line containing named `key=value` assignments | `.log`, `.txt` |
| **Syslog (RFC 5424 / RFC 3164)** | `syslog` | One supported Syslog message per line | `.log`, `.syslog` |
| **IIS W3C Extended Log** | `iis-w3c` | Header-driven IIS W3C text records | `.log` |
| **Structured XML** | `xml` | XML document root or selected record elements | `.xml` |
| **Regex Plain Text** | `regex-text` | One text record per line interpreted by a configured regular expression | `.log`, `.txt` |

### Recognized layouts built on these importers

The following layouts are supported through profiles or presets rather than separate importer families:

| Layout | Underlying importer |
| --- | --- |
| Apache Common Access Log | Regex Plain Text |
| Apache Combined Access Log | Regex Plain Text |
| Nginx Combined Access Log | Regex Plain Text |
| Windows Event XML | Structured XML |

Windows Event support applies to **XML-formatted events and event collections**. Native binary Windows `.evtx` files are not directly imported.

### TraceScope artifacts are not log formats

The following are application persistence formats rather than source-log import formats:

- `.tsinv` — a saved investigation session snapshot, containing captured records and supporting import/source information rather than the complete investigation session state
- `.tsw` — a saved workspace manifest, accompanied by a matching `.sessions/` folder containing managed investigation session snapshots

Open those using TraceScope's investigation/workspace commands rather than Import Configuration. See [Saving and Restoring](saving-and-restoring.md).

## 2. Format suggestions

When a source file is selected, TraceScope may show a **Likely format** and, where appropriate, a matching built-in preset.

Suggestions are advisory. The selected importer and profile remain the authority for how a source is interpreted.

### Extension-based suggestions

| Source indicator | Suggested family |
| --- | --- |
| `.jsonl`, `.ndjson` | JSON Lines |
| `.json` | Structured JSON |
| `.csv` | CSV |
| `.tsv` | TSV |
| `.xml` | Structured XML, with Windows Event XML recognition when applicable |
| `.syslog` | Syslog |

Extensions such as `.log` and `.txt` are not specific enough to identify one importer.

### Content-based suggestions

For generic text files, TraceScope can inspect a limited sample of the source content and recognize patterns such as:

- JSON objects on successive lines
- Key-Value / logfmt assignments
- RFC 5424 or RFC 3164 Syslog
- IIS W3C `#Fields:` declarations
- Apache Common access logs
- Apache/Nginx Combined access logs

Windows Event XML recognition inspects the XML structure rather than relying only on the `.xml` extension.

Format detection is intentionally conservative. A custom layout, mixed-format file, unusual header, or unrepresentative beginning of a file may not be recognized automatically.

Always verify:

- selected importer
- selected or loaded profile
- validation results
- source-record preview
- normalized preview

before relying on the import.

## 3. Format reference

### JSON Lines

**Importer ID:** `json-lines`

JSON Lines treats successive physical lines as independent records.

Each nonempty record line must contain a complete JSON object.

Representative record:

```json
{"timestamp":"2026-08-18T13:00:00.000Z","level":"INFO","subsystem":"ApiGateway","eventCode":"REQUEST_ACCEPTED","entityId":"ORD-7421","message":"Request accepted by order API"}
```

A JSON array spread across several lines is **not** JSON Lines; use Structured JSON instead.

Profiles can map top-level or nested object values. Dot-separated source paths such as:

```text
context.requestId
```

refer to values inside each record object.

Malformed lines produce import diagnostics rather than being silently converted into valid records.

Example:

- [`order-fulfillment-incident.jsonl`](../samples/order-fulfillment-incident.jsonl)
- [`order-fulfillment-incident-profile.json`](../samples/profiles/order-fulfillment-incident-profile.json)

### Structured JSON

**Importer ID:** `structured-json`

Structured JSON reads one JSON document.

With **Record path** empty:

- a root object becomes one source record
- a root array supplies multiple source records

Array elements selected as records must be JSON objects.

Representative source:

```json
[
  {
    "timestamp": "2026-08-11T10:00:08.120Z",
    "level": "INFO",
    "subsystem": "Gateway",
    "eventCode": "REQUEST_ACCEPTED",
    "entityId": "GW-01",
    "message": "Inbound request accepted"
  },
  {
    "timestamp": "2026-08-11T10:00:48.430Z",
    "level": "WARN",
    "subsystem": "Gateway",
    "eventCode": "RATE_LIMIT",
    "entityId": "GW-01",
    "message": "Client approaching rate limit"
  }
]
```

A configured dot-separated **Record path** begins at an object document root and may resolve to:

- one object
- an array of record objects

For example:

```text
data.records
```

can select the `records` value from:

```json
{
  "data": {
    "records": [
      { "message": "First" },
      { "message": "Second" }
    ]
  }
}
```

Field mappings are then resolved within each selected record.

Large Structured JSON sources use the structured-document preview behavior described in [Importing Logs](importing-logs.md); full imports parse the document as structured JSON rather than as independent physical lines.

Static import can also admit complete records from a supported structured JSON record array that is currently still open because another process is writing it.

Example profiles:

- [`structured-json-array-profile.json`](../samples/profiles/structured-json-array-profile.json)
- [`structured-json-nested-profile.json`](../samples/profiles/structured-json-nested-profile.json)

### CSV

**Importer ID:** `csv`

CSV requires a header row that names the source fields.

Representative source:

```text
timestamp,level,subsystem,eventCode,entityId,message
2026-08-11T09:00:05.120Z,INFO,Gateway,REQ_RECEIVED,GW-01,Request accepted
```

Profiles map header names rather than numeric column positions.

The CSV parser supports:

- comma delimiters
- quoted fields
- doubled quotation marks inside quoted values

Each physical line is treated as one record, even when fields are quoted. A quoted value containing an embedded newline is therefore not supported and may produce an import diagnostic.

Records with malformed quoting or inconsistent field counts produce diagnostics.

Example:

- [`service-session.csv`](../samples/service-session.csv)
- [`service-session-csv-profile.json`](../samples/profiles/service-session-csv-profile.json)

### TSV

**Importer ID:** `tsv`

TSV has the same record model as CSV but uses tab delimiters.

It requires a header row and maps fields by header name.

Representative source:

```text
event_time	priority	component	event_id	device_id	details
2026-08-11 10:15:03.115	NOTICE	Ingest	BATCH_OPENED	ING-01	Telemetry batch opened
```

Quoted-field handling follows the same delimited-record rules as CSV, including the limitation that one physical line represents one source record.

Example:

- [`telemetry-batch.tsv`](../samples/telemetry-batch.tsv)
- [`telemetry-batch-tsv-profile.json`](../samples/profiles/telemetry-batch-tsv-profile.json)

### Key-Value / logfmt

**Importer ID:** `key-value`

This importer reads one line-oriented record containing named assignments.

Representative record:

```text
timestamp=2026-08-12T08:20:18.427Z level=INFO subsystem=Orders eventCode=ORDER_RECEIVED entityId=ORD-5812 requestId=REQ-9201 message="Order received for processing"
```

Extracted keys become source fields available to the import profile.

Because `.log` is used by many unrelated formats, TraceScope relies on source content rather than the extension alone when suggesting Key-Value / logfmt.

Example:

- [`logfmt-service-session.log`](../samples/logfmt-service-session.log)
- [`logfmt-service-session-profile.json`](../samples/profiles/logfmt-service-session-profile.json)

The bundled Field Gateway sources also use this importer:

- [`field-gateway-known-good.log`](../samples/field-gateway-known-good.log)
- [`field-gateway-degraded.log`](../samples/field-gateway-degraded.log)
- [`field-gateway-support-session-profile.json`](../samples/profiles/field-gateway-support-session-profile.json)

### Syslog: RFC 5424 and RFC 3164

**Importer ID:** `syslog`

One Syslog record is read from each physical line.

The same importer recognizes supported RFC 5424 and RFC 3164 layouts.

TraceScope derives common values from the Syslog priority, including:

- facility code
- severity code
- Syslog severity name
- normalized TraceScope-compatible severity value

#### RFC 5424

Representative record:

```text
<134>1 2026-08-12T08:20:18.427Z api-01 orders-service 4120 ORDER_RECEIVED [request@32473 requestId="REQ-9201" customerRegion="us-east"] Order ORD-5812 received for processing
```

Supported parsed values include, where present:

- timestamp
- hostname
- application/subsystem name
- process ID
- message ID / event code
- structured data
- message

RFC 5424 structured-data parameters are available for profile mapping.

Example:

- [`syslog-rfc5424-session.log`](../samples/syslog-rfc5424-session.log)
- [`syslog-rfc5424-profile.json`](../samples/profiles/syslog-rfc5424-profile.json)

#### RFC 3164

Representative record:

```text
<134>Aug 12 08:20:18 api-01 orders-service[4120]: Order ORD-5812 received for processing
```

RFC 3164 does not carry an explicit year or timezone.

TraceScope:

1. infers the nearest valid year relative to the import reference date
2. interprets the resulting timestamp in the system's local timezone
3. records an informational import diagnostic indicating that inference occurred

That timestamp should therefore not be treated as equivalent to an independently recorded timestamp containing an explicit year and timezone.

RFC 3164 also contains less structured metadata than RFC 5424; an event code, for example, is not inherently available.

Example:

- [`syslog-rfc3164-session.log`](../samples/syslog-rfc3164-session.log)
- [`syslog-rfc3164-profile.json`](../samples/profiles/syslog-rfc3164-profile.json)

### IIS W3C Extended Log

**Importer ID:** `iis-w3c`

The IIS importer uses the active `#Fields:` directive to determine the names and order of the fields in subsequent W3C Extended Log records.

Representative source:

```text
#Fields: date time s-ip cs-method cs-uri-stem cs-uri-query s-port cs-username c-ip sc-status time-taken
2026-08-12 15:05:17 192.0.2.10 GET /api/orders/5812 - 443 analyst 198.51.100.31 200 57
```

The declared names become source fields available to the profile.

TraceScope also derives:

- a combined timestamp from `date` and `time`
- a message from available request method and request target fields

W3C comments and directives are not imported as investigation records.

A data line encountered before a valid field declaration cannot be interpreted reliably and produces a diagnostic.

HTTP response status is source-specific data; TraceScope does not automatically treat an HTTP status code as investigation severity.

Example:

- [`iis-w3c-access.log`](../samples/iis-w3c-access.log)
- [`iis-w3c-profile.json`](../samples/profiles/iis-w3c-profile.json)

### Structured XML

**Importer ID:** `xml`

Structured XML reads XML elements into source values that can be mapped by an import profile.

With **Record path** empty, the document root is treated as one source record.

A nonempty dot-separated Record path selects matching record elements within the document.

Representative record within a larger document:

```xml
<event sequence="1">
    <metadata>
        <timestamp>2026-08-13T12:00:00.000Z</timestamp>
        <level>INFO</level>
        <component>TestOrchestrator</component>
        <code>RUN_STARTED</code>
    </metadata>
    <context>
        <deviceId>RIG-ALPHA-07</deviceId>
    </context>
</event>
```

For that structure, a Record path can identify the repeated elements:

```text
session.events.event
```

XML attributes are represented using `@`-prefixed source names. The exact source-path rules and Windows Event normalization are documented in [Import Profiles](import-profiles.md).

Example:

- [`structured-engineering-session.xml`](../samples/structured-engineering-session.xml)
- [`structured-xml-engineering-session-profile.json`](../samples/profiles/structured-xml-engineering-session-profile.json)

#### Windows Event XML

Windows Event XML uses the Structured XML importer with Windows Event-specific presets and mappings.

Representative event fragment:

```xml
<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event">
    <System>
        <Provider Name="TraceScope-Sample-Engineering" />
        <EventID>4100</EventID>
        <Level>4</Level>
        <TimeCreated SystemTime="2026-08-13T12:00:00.000Z" />
    </System>
</Event>
```

TraceScope recognizes:

- a single rendered `<Event>` document in the Windows Event namespace
- collections containing Windows `<Event>` records

Typical mapped values can include:

- provider
- event ID
- level
- time created
- channel
- computer
- event data

Actual field availability varies by event.

TraceScope supports XML-formatted Windows Event data only. It does **not** directly parse native binary `.evtx` files.

Example:

- [`windows-event-engineering-session.xml`](../samples/windows-event-engineering-session.xml)
- [`windows-event-engineering-session-profile.json`](../samples/profiles/windows-event-engineering-session-profile.json)

### Regex Plain Text

**Importer ID:** `regex-text`

Regex Plain Text handles custom line-oriented logs through a configured Qt `QRegularExpression`.

Representative record:

```text
2026-08-11T13:02:03.447Z [WARN] [Orders] [SUPPLIER_SLOW] [ORD-1842] [REQ-4004] Supplier response exceeded 800 ms
```

A corresponding profile can use named capture groups for values such as timestamp, severity, subsystem, event code, entity, request ID, and message.

Conceptually:

```text
(?<timestamp>...)\s+\[(?<level>...)\]\s+\[(?<subsystem>...)\]\s+...
```

The configured expression must match the **entire source record**, not merely a substring.

A record that does not match produces an import diagnostic.

Example:

- [`general-application.log`](../samples/general-application.log)
- [`general-application-regex-profile.json`](../samples/profiles/general-application-regex-profile.json)

#### Apache Common Access Log

Apache Common uses the Regex Plain Text importer with a supplied preset.

Representative record:

```text
192.0.2.10 - - [12/Aug/2026:08:20:02 -0400] "GET / HTTP/1.1" 200 1256
```

Example:

- [`apache-common-access.log`](../samples/apache-common-access.log)
- [`apache-common-profile.json`](../samples/profiles/apache-common-profile.json)

#### Apache Combined Access Log

Apache Combined extends the common layout with referrer and user-agent fields.

Representative record:

```text
198.51.100.24 - analyst [12/Aug/2026:09:00:48 -0400] "GET /api/orders/5812 HTTP/1.1" 200 842 "https://portal.example.test/orders" "Mozilla/5.0"
```

Example:

- [`apache-combined-access.log`](../samples/apache-combined-access.log)
- [`apache-combined-profile.json`](../samples/profiles/apache-combined-profile.json)

#### Nginx Combined Access Log

The bundled Nginx Combined preset uses the corresponding line-oriented combined-access layout.

Representative record:

```text
203.0.113.41 - - [12/Aug/2026:10:12:44 -0400] "POST /api/orders HTTP/1.1" 201 428 "https://app.example.test/orders/new" "Mozilla/5.0"
```

Example:

- [`nginx-combined-access.log`](../samples/nginx-combined-access.log)
- [`nginx-combined-profile.json`](../samples/profiles/nginx-combined-profile.json)

The access-log presets expose useful HTTP request/response values without manufacturing canonical fields that the source does not contain.

For example, HTTP status remains source-specific data rather than automatically becoming TraceScope severity.

Custom server log layouts may require a custom regex profile instead of a built-in preset.

## 4. Bundled examples

The repository contains fictional sample sources and matching profiles for each major supported family and recognized layout.

| Format or layout | Sample | Profile |
| --- | --- | --- |
| JSON Lines | [`order-fulfillment-incident.jsonl`](../samples/order-fulfillment-incident.jsonl) | [`order-fulfillment-incident-profile.json`](../samples/profiles/order-fulfillment-incident-profile.json) |
| Structured JSON — root array | [`structured-json-array-session.json`](../samples/structured-json-array-session.json) | [`structured-json-array-profile.json`](../samples/profiles/structured-json-array-profile.json) |
| Structured JSON — nested records | [`structured-json-nested-session.json`](../samples/structured-json-nested-session.json) | [`structured-json-nested-profile.json`](../samples/profiles/structured-json-nested-profile.json) |
| CSV | [`service-session.csv`](../samples/service-session.csv) | [`service-session-csv-profile.json`](../samples/profiles/service-session-csv-profile.json) |
| TSV | [`telemetry-batch.tsv`](../samples/telemetry-batch.tsv) | [`telemetry-batch-tsv-profile.json`](../samples/profiles/telemetry-batch-tsv-profile.json) |
| Key-Value / logfmt | [`logfmt-service-session.log`](../samples/logfmt-service-session.log) | [`logfmt-service-session-profile.json`](../samples/profiles/logfmt-service-session-profile.json) |
| Field Gateway key-value pair | [`field-gateway-known-good.log`](../samples/field-gateway-known-good.log), [`field-gateway-degraded.log`](../samples/field-gateway-degraded.log) | [`field-gateway-support-session-profile.json`](../samples/profiles/field-gateway-support-session-profile.json) |
| Syslog RFC 5424 | [`syslog-rfc5424-session.log`](../samples/syslog-rfc5424-session.log) | [`syslog-rfc5424-profile.json`](../samples/profiles/syslog-rfc5424-profile.json) |
| Syslog RFC 3164 | [`syslog-rfc3164-session.log`](../samples/syslog-rfc3164-session.log) | [`syslog-rfc3164-profile.json`](../samples/profiles/syslog-rfc3164-profile.json) |
| IIS W3C | [`iis-w3c-access.log`](../samples/iis-w3c-access.log) | [`iis-w3c-profile.json`](../samples/profiles/iis-w3c-profile.json) |
| Structured XML | [`structured-engineering-session.xml`](../samples/structured-engineering-session.xml) | [`structured-xml-engineering-session-profile.json`](../samples/profiles/structured-xml-engineering-session-profile.json) |
| Windows Event XML | [`windows-event-engineering-session.xml`](../samples/windows-event-engineering-session.xml) | [`windows-event-engineering-session-profile.json`](../samples/profiles/windows-event-engineering-session-profile.json) |
| Regex plain text | [`general-application.log`](../samples/general-application.log) | [`general-application-regex-profile.json`](../samples/profiles/general-application-regex-profile.json) |
| Apache Common | [`apache-common-access.log`](../samples/apache-common-access.log) | [`apache-common-profile.json`](../samples/profiles/apache-common-profile.json) |
| Apache Combined | [`apache-combined-access.log`](../samples/apache-combined-access.log) | [`apache-combined-profile.json`](../samples/profiles/apache-combined-profile.json) |
| Nginx Combined | [`nginx-combined-access.log`](../samples/nginx-combined-access.log) | [`nginx-combined-profile.json`](../samples/profiles/nginx-combined-profile.json) |

A profile that works for one source does not automatically apply to every file using the same general serialization format.

For example, two JSON Lines applications may use entirely different names and structures for their timestamps, severities, identifiers, and messages.

Use the examples to understand the importer and profile structure, then verify mappings against the actual source being investigated.

## 5. Live Following compatibility

Static import support and Live Following compatibility are related but distinct.

All nine built-in importer families have a live-ingestion path when the physical source and configuration satisfy the requirements below.

| Importer family | Live Following requirements |
| --- | --- |
| **JSON Lines** | Line-oriented JSON objects. An incomplete final physical line remains pending until completed. |
| **CSV / TSV** | One physical line per record. Header state must be available for the active physical source. |
| **Key-Value / logfmt** | Line-oriented records using the configured profile. |
| **Syslog** | Supported RFC 5424 or RFC 3164 messages, one per line. |
| **IIS W3C** | Line-oriented source with valid active `#Fields:` context. |
| **Regex Plain Text** | One physical record per line matching the configured expression. |
| **Structured JSON** | A record **array** containing JSON objects: either the document root array or an array selected by Record path. |
| **Structured XML** | A **nonempty Record path** selecting repeated record elements in the growing document. |

### Structured JSON live sources

A Structured JSON root object can be imported statically, but it is not a repeated live-record container.

For Live Following, TraceScope must be able to locate an array of record objects.

For example, an actively written source may currently contain one of the following partial documents. The outer array or object has not closed yet, but the individual record objects shown are complete.

```json
[
  { "message": "First" },
  { "message": "Second" }
```

or an object containing a selected array:

```json
{
  "events": [
    { "message": "First" },
    { "message": "Second" }
```

while that array/document is still being written.

Complete record objects are admitted as they become available. An incomplete trailing object remains pending.

### Structured XML live sources

Live Structured XML requires a nonempty Record path selecting repeated elements.

For example:

```text
session.events.event
```

allows complete `<event>` elements to be admitted while their outer XML container remains open.

A static XML document whose root itself is the single record does not provide the repeated-record structure required for live XML following.

Windows Event XML collections use this same Structured XML live path. A growing collection of repeated `<Event>` records can be followed when the appropriate record path is configured.

### Physical source requirement

Live Following operates on a connected external file.

It is not:

- a network Syslog listener
- a socket or pipe consumer
- a database reader
- a direct application integration
- a live connection to a standalone `.tsinv` investigation session snapshot

An investigation session snapshot contains previously captured evidence; it is not a growing external log file. To follow additional records, the investigation session must have a suitable connected source.

For source replacement, truncation, rotation, and evidence-continuity behavior, see [Live Following](live-following.md) and [Saving and Restoring](saving-and-restoring.md).

## 6. Field availability and limitations

Support for a source serialization format does not imply that every TraceScope investigation field exists in that source.

### Canonical fields are optional

TraceScope's canonical fields are:

- Timestamp
- Severity
- Subsystem
- Event code
- Entity ID
- Message

A source may provide any subset of them.

Leave genuinely unavailable fields unmapped rather than inventing values.

Features that require an absent field may be unavailable or less informative.

### Custom fields retain source-specific information

Source-specific values belong naturally in custom attributes.

Examples include:

- HTTP status
- queue depth
- request ID
- firmware version
- client address
- process ID
- device-specific measurements

A custom field does not imply that the same-named field in an unrelated source has identical semantics.

### Timestamp quality depends on the source

Timestamp-based investigation depends on the actual timestamp information supplied by the source and its profile.

Check:

- format
- timezone
- parsing rule
- inferred values
- source clock assumptions

before comparing independently recorded systems.

RFC 3164's inferred year/local timezone behavior is a specific example of why timestamp provenance matters.

### Format recognition is not mapping validation

A correct format suggestion only identifies a likely parser family.

It does not prove that:

- field mappings are correct
- severity values are meaningful
- timestamps parse as intended
- nested record paths select the intended records
- a regex matches every source record
- custom fields mean the same thing across sources

Use profile validation, preview, and import diagnostics to verify those separate concerns.

### Unsupported native Windows Event files

TraceScope does not directly import binary `.evtx` files.

Windows Event data must be available in a supported XML representation before it can be handled by the Structured XML importer.

## Troubleshooting

| Situation | What to check |
| --- | --- |
| No likely-format suggestion appears, or the suggestion is wrong | Treat the suggestion as advisory. Generic `.log` and `.txt` files can represent many unrelated layouts; inspect representative records and select the importer that matches the actual source structure. See [Format suggestions](#2-format-suggestions) for more information. |
| A JSON source opens no records or is interpreted incorrectly | Confirm whether the source is **JSON Lines**—one complete JSON object per physical line—or **Structured JSON**—one JSON document containing an object or record collection. See [JSON Lines](#json-lines) and [Structured JSON](#structured-json) for related information. |
| CSV or TSV records are skipped or columns do not line up | Verify the selected importer, header row, delimiter, quoting, and physical record layout. Quoted fields spanning multiple physical lines are not supported. See [CSV](#csv) and [TSV](#tsv) for related information. |
| Syslog timestamps look different from the source time you expected | Check which Syslog layout is being parsed. RFC 3164 does not contain a year or timezone, so TraceScope must infer them as documented. See [Syslog: RFC 5424 and RFC 3164](#syslog-rfc-5424-and-rfc-3164) and [Timestamp quality depends on the source](#timestamp-quality-depends-on-the-source) for related information. |
| A Windows event file is rejected | Confirm that the source is XML-formatted Windows Event data. TraceScope does not directly import native binary `.evtx` files. See [Windows Event XML](#windows-event-xml) and [Unsupported native Windows Event files](#unsupported-native-windows-event-files) for more information. |
| Regex Plain Text skips records even though part of the line matches | The configured expression must match the **entire source record**, and the fields you want to map must be exposed through named capture groups. See [Regex Plain Text](#regex-plain-text) and [Import Profiles: Regex Plain Text](import-profiles.md#regex-plain-text) for related information. |
| A format imports successfully, but an expected canonical field is unavailable | Format support does not guarantee that the source contains every TraceScope canonical field. Check what the source actually supplies and leave genuinely absent concepts unmapped. See [Field availability and limitations](#6-field-availability-and-limitations) and [Import Profiles: The six canonical fields](import-profiles.md#the-six-canonical-fields) for related information. |
| A Structured JSON or XML file imports statically but cannot be followed live | Static import and Live Following have different source-shape requirements. Structured JSON needs a repeated record array for live use; Structured XML needs a nonempty Record path selecting repeated elements. See [Live Following compatibility](#5-live-following-compatibility) for more information. |

## Related documentation

- [Import Profiles](import-profiles.md) — importer IDs, record paths, canonical mappings, custom fields, severity aliases, timestamp rules, and saved profile JSON.
- [Importing Logs](importing-logs.md) — source selection, preview, mapping, validation, rotated source families, and import execution.
- [Live Following](live-following.md) — following growing files, pause/resume, source generations, truncation, replacement, and rotation.
- [Saving and Restoring](saving-and-restoring.md) — source-backed, snapshot-backed, and Hybrid evidence continuity.
- [Troubleshooting](troubleshooting.md) — import failures, unexpected mappings, live-source problems, and recovery guidance.
