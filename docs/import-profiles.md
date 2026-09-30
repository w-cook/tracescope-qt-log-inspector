# Import Profiles in TraceScope

An **import profile** tells TraceScope how to interpret records from a supported source. It identifies the importer, supplies format-specific parsing or record-selection configuration where needed, and maps extracted source values into TraceScope's canonical and custom investigation fields.

This reference describes the import-profile model and saved JSON schema used by TraceScope v1.0.

For the step-by-step import workflow, see [Importing Logs](importing-logs.md). To choose the correct importer and source shape first, see [Supported Formats](supported-formats.md).

**Having trouble with a profile or its mappings?** [Jump straight to Troubleshooting](#troubleshooting).

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Understand what a profile controls | [1. What an import profile controls](#1-what-an-import-profile-controls) |
| Look up profile settings and canonical fields | [2. Profile settings and canonical fields](#2-profile-settings-and-canonical-fields) |
| Understand source paths and format-specific configuration | [3. Source paths and format-specific configuration](#3-source-paths-and-format-specific-configuration) |
| Configure severity or timestamp interpretation | [4. Severity aliases and timestamp rules](#4-severity-aliases-and-timestamp-rules) |
| Understand explicit and preserved custom fields | [5. Custom fields and unmapped data](#5-custom-fields-and-unmapped-data) |
| Read or edit a saved profile | [6. Saved profile JSON reference](#6-saved-profile-json-reference) |
| Compare real profile examples | [7. Bundled profile examples](#7-bundled-profile-examples) |
| Understand validation, preview, and reuse | [8. Validation, preview, and reuse](#8-validation-preview-and-reuse) |

## 1. What an import profile controls

TraceScope separates two responsibilities:

1. the **importer** determines how source records are read
2. the **profile** determines how the values extracted by that importer are interpreted

A profile can define:

- the importer to use
- a Structured JSON or XML record path
- a Regex Plain Text pattern
- mappings for six canonical investigation fields
- named custom-field mappings
- severity aliases
- timestamp parsing rules
- whether additional unmapped source values are preserved

Profiles are reusable **configuration**, not investigation evidence.

A profile does not contain:

- source records
- the selected source-file path
- rotated-source selections
- bookmarks
- notes
- finding classifications
- saved comparisons
- workspace layout

Those belong to the source-selection, investigation, or persistence layers instead.

### One importer can use many profiles

A serialization format does not define one universal field schema.

For example, both of these applications could produce valid JSON Lines:

```json
{"time":"2026-09-30T12:00:00Z","level":"ERROR","msg":"Request failed"}
```

```json
{"observedAt":"2026-09-30T12:00:00Z","priority":"FAIL","details":{"message":"Request failed"}}
```

They use the same importer but require different mappings.

A successful format suggestion therefore identifies a likely **reader**, not a guaranteed profile.

## 2. Profile settings and canonical fields

### Settings at a glance

| Setting | Saved JSON property | Purpose |
| --- | --- | --- |
| **Name** | `name` | Human-readable profile name. Required and must not be blank. |
| **Format** | `importerId` | Identifies the built-in importer. Required. |
| **Record path** | `recordPath` | Selects records within Structured JSON or Structured XML. Optional for static imports; live structured sources have stricter requirements. |
| **Regular expression** | `regexPattern` | Defines named fields for Regex Plain Text. Required when `importerId` is `regex-text`. |
| **Canonical fields** | `canonicalFields` | Maps source values into the six standard investigation roles. |
| **Custom fields** | `customFields` | Creates explicitly named source-specific investigation attributes. |
| **Severity aliases** | `severityAliases` | Maps source severity labels into TraceScope severity values. |
| **Timestamp rules** | `timestampRules` | Defines how a mapped timestamp string is parsed. Rules are attempted in order. |
| **Preserve unmapped source fields** | `preserveUnmappedFields` | Retains additional extracted values as custom attributes. |
| **Schema version** | `schemaVersion` | Identifies the profile-file schema. v1.0 supports schema version `1`. |

### Built-in importer IDs

Saved profiles use the following importer IDs:

| Importer | `importerId` |
| --- | --- |
| JSON Lines | `json-lines` |
| Structured JSON | `structured-json` |
| CSV | `csv` |
| TSV | `tsv` |
| Key-Value / logfmt | `key-value` |
| Syslog | `syslog` |
| IIS W3C Extended Log | `iis-w3c` |
| Structured XML | `xml` |
| Regex Plain Text | `regex-text` |

Apache/Nginx access-log layouts use `regex-text`. Windows Event XML uses `xml`. They are presets over those importer families rather than separate importer IDs.

See [Supported Formats](supported-formats.md) for the corresponding source requirements.

### The six canonical fields

Canonical mappings give source-specific values a consistent investigation role.

| Canonical field | Saved JSON key | Typical source values |
| --- | --- | --- |
| Timestamp | `timestamp` | Time at which the event occurred |
| Severity | `severity` | Log level, priority, or normalized severity |
| Subsystem | `subsystem` | Service, component, provider, logger |
| Event code | `eventCode` | Application event code, event ID, Syslog message ID |
| Entity ID | `entityId` | Device, order, request, job, or other domain identifier |
| Message | `message` | Human-readable event text |

Every canonical mapping is individually optional.

If a source does not contain one of these concepts, leave the mapping empty instead of manufacturing a value.

For example:

```json
"canonicalFields": {
  "timestamp": "timestamp",
  "severity": "",
  "subsystem": "",
  "eventCode": "",
  "entityId": "",
  "message": "request"
}
```

is valid for a source that has a timestamp and request text but no meaningful severity or domain identifiers.

The saved `canonicalFields` object itself must always contain **all six string properties**, using `""` for unused mappings.

### Canonical values are text-oriented

Canonical mapping expects the resolved source value to be textual.

For JSON-like structured data, directly pointing a canonical field at a numeric, boolean, array, or object value does not automatically convert that value into a canonical string.

For example:

```json
{
  "status": 503
}
```

does not become canonical Severity merely because `"severity": "status"` is configured.

Such values are often better retained as custom fields unless the importer itself defines a textual normalized representation.

Importers such as Syslog, IIS W3C, and Structured XML can create or normalize source values before profile mapping; the profile then maps those extracted values in the usual way.

## 3. Source paths and format-specific configuration

Profiles use two related but different kinds of paths:

- **Record path** — identifies which object or element represents a record in a larger Structured JSON or XML document.
- **Field source path** — identifies a value inside each selected record.

Neither is a filesystem path.

### General source-path rules

Profile paths:

- are dot-separated
- must not contain empty path segments
- must not have leading or trailing whitespace
- use the exact extracted source-field names
- do not provide general JSONPath or XPath syntax

For example:

```text
context.requestId
```

means:

```text
context
  → requestId
```

It does not mean an arbitrary recursive search.

Path lookup is based on the names exposed by the selected importer. Match their case and spelling.

Array indexing, wildcard selectors, predicates, and general query expressions are not supported by profile field paths.

### Structured JSON

Structured JSON uses the profile's `recordPath` to select records from a complete JSON document.

With an empty Record path:

- a root object becomes one record
- a root array supplies multiple records

A nonempty path begins at an **object document root** and can resolve to:

- one object
- an array of record objects

Example source:

```json
{
  "data": {
    "records": [
      {
        "observedAt": "2026-08-11T10:00:08.120Z",
        "priority": "NOTICE",
        "service": {
          "name": "Gateway"
        },
        "details": {
          "message": "Inbound request accepted"
        }
      }
    ]
  }
}
```

Corresponding configuration:

```json
{
  "recordPath": "data.records",
  "canonicalFields": {
    "timestamp": "observedAt",
    "severity": "priority",
    "subsystem": "service.name",
    "eventCode": "",
    "entityId": "",
    "message": "details.message"
  }
}
```

`data.records` is resolved against the **outer document**.

`service.name` and `details.message` are then resolved against each **selected record**.

A record path cannot traverse through arbitrary array indexes. It follows named object members until reaching the selected object or record array.

For Live Following, Structured JSON is stricter: TraceScope must be able to locate a repeated **array of record objects**. A single object selected statically is not a live-record container. See [Live Following compatibility](supported-formats.md#5-live-following-compatibility).

### Structured XML

Structured XML also uses dot-separated element paths.

Example source:

```xml
<session>
    <events>
        <event sequence="1">
            <metadata>
                <timestamp>2026-08-13T12:00:00.000Z</timestamp>
                <level>INFO</level>
            </metadata>
            <context>
                <deviceId>RIG-ALPHA-07</deviceId>
            </context>
        </event>
    </events>
</session>
```

A profile can use:

```text
session.events.event
```

as the Record path.

Mappings inside each selected `<event>` can then use:

```text
metadata.timestamp
metadata.level
context.deviceId
```

#### XML attributes

Attributes are normalized using an `@` prefix.

For:

```xml
<event sequence="1">
```

the selected record exposes:

```text
@sequence
```

For a nested element:

```xml
<Provider Name="TraceScope-Sample-Engineering">
```

a profile can use:

```text
System.Provider.@Name
```

#### Element text

A simple XML element such as:

```xml
<message>Hello</message>
```

can be mapped through:

```text
message
```

When an element contains both attributes and text, TraceScope internally preserves its text under `#text`; canonical mapping can still address the element itself where the normalized value supplies that direct text.

#### Windows Event XML named data

Windows Event XML commonly contains elements such as:

```xml
<Data Name="DeviceId">RIG-ALPHA-07</Data>
```

TraceScope normalizes named event data under:

```text
EventData.NamedData
```

so the value can be addressed as:

```text
EventData.NamedData.DeviceId
```

This is TraceScope's normalized source representation, not XPath syntax.

With no Record path, a complete XML document root can be imported as one static record. Live Structured XML requires a **nonempty Record path selecting repeated elements**.

### CSV and TSV

Delimited importers expose header names as source paths.

Example:

```text
timestamp,level,deployment_ring
2026-08-11T09:00:05.120Z,INFO,blue
```

can use:

```text
timestamp
level
deployment_ring
```

as mappings.

Profiles refer to the header names, not numeric column positions.

### Key-Value / logfmt

Keys extracted from each assignment become source paths.

Example:

```text
level=INFO queueDepth=7 message="Batch uploaded"
```

exposes:

```text
level
queueDepth
message
```

### IIS W3C

IIS W3C exposes names from the active `#Fields:` declaration.

For example:

```text
#Fields: date time cs-method cs-uri-stem sc-status
```

provides fields including:

```text
cs-method
cs-uri-stem
sc-status
```

The importer additionally combines `date` and `time` into an extracted:

```text
timestamp
```

value and can generate:

```text
message
```

from the available request method and request target.

Profiles should map those derived fields rather than attempting to treat `date` and `time` as one profile path.

### Syslog

Syslog profiles map values produced by the Syslog parser.

Common extracted fields include:

```text
timestamp
hostname
subsystem
processId
eventCode
message
priority
facilityCode
severityCode
syslogSeverity
level
syslogFormat
```

RFC 5424 structured-data parameters can be addressed through nested paths such as:

```text
structuredDataFields.request@32473.requestId
```

Those paths refer to the parser's normalized representation, not literal byte positions in the original Syslog message.

RFC 3164 has different source limitations, including inferred year/timezone handling. See [Supported Formats](supported-formats.md#syslog-rfc-5424-and-rfc-3164).

### Regex Plain Text

Regex Plain Text uses `regexPattern` to define source fields.

The expression must use named capture groups.

Representative source:

```text
2026-08-11 10:15:03.115 [INFO] [worker-4] [Orders] Request accepted
```

A pattern can expose fields such as:

```text
timestamp
level
thread
logger
message
```

through named groups:

```regex
^(?<timestamp>...)\s+\[(?<level>...)\]\s+\[(?<thread>...)\]\s+\[(?<logger>...)\]\s+(?<message>.*)$
```

Those capture names then become field source paths.

The configured expression must be valid Qt `QRegularExpression` syntax and must match the **entire source record** for the record to import successfully.

A regular expression can therefore be syntactically valid while still being the wrong parser for the source.

## 4. Severity aliases and timestamp rules

These settings interpret values **after the importer has successfully extracted them**.

They cannot compensate for a missing field or incorrect source path.

### Severity aliases

TraceScope normalizes severity into six values:

- `TRACE`
- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`
- `CRITICAL`

The normal severity parser also recognizes common equivalents including:

- `INFORMATION` → `INFO`
- `WARNING` → `WARN`
- `FATAL` → `CRITICAL`

A profile's `severityAliases` object handles source-specific terminology beyond those built-in names.

For example:

```json
"severityAliases": {
  "CAUTION": "WARN",
  "FAIL": "ERROR",
  "NOTICE": "INFO"
}
```

combined with:

```json
"severity": "priority"
```

allows:

```text
priority=FAIL
```

to become canonical `ERROR`.

Alias source labels are matched case-insensitively.

Alias targets must resolve to a supported TraceScope severity. When TraceScope serializes the profile, targets are written using the normalized names:

```text
TRACE
DEBUG
INFO
WARN
ERROR
CRITICAL
```

Do not use aliases to create a severity when the source does not actually contain one.

### Timestamp rules

When a canonical timestamp path is configured, the profile requires at least one timestamp rule.

Supported rule types are:

| Saved `type` | Purpose | `format` |
| --- | --- | --- |
| `iso8601` | Parse a supported ISO 8601 timestamp | Not used |
| `qt-format` | Parse another fixed textual layout using a Qt date/time format | Required |

Example ISO rule:

```json
"timestampRules": [
  {
    "type": "iso8601"
  }
]
```

Example custom format:

```json
"timestampRules": [
  {
    "type": "qt-format",
    "format": "yyyy-MM-dd HH:mm:ss.zzz"
  }
]
```

For:

```text
2026-08-11 10:15:03.115
```

the second rule is appropriate.

When multiple rules are configured, TraceScope attempts them **in list order** and uses the first successful parse.

For example:

```json
"timestampRules": [
  {
    "type": "iso8601"
  },
  {
    "type": "qt-format",
    "format": "yyyy-MM-dd HH:mm:ss.zzz"
  }
]
```

can support records that legitimately use either representation.

A `qt-format` rule with no format is invalid.

An `iso8601` rule does not use `format`; if a format string is supplied anyway, validation reports a warning because it is ignored.

If no canonical timestamp mapping is configured, `timestampRules` may be an empty array.

Timestamp parsing establishes syntax. It does not establish that the source clock, timezone, or inferred date is semantically correct for comparison with another system.

## 5. Custom fields and unmapped data

Canonical fields provide common investigation roles. **Custom fields** preserve source-specific information without forcing it into those roles.

### Explicit custom mappings

Each custom mapping contains:

```json
{
  "name": "Deployment Ring",
  "sourcePath": "deployment_ring"
}
```

`name` is the investigation-facing attribute name.

`sourcePath` identifies the extracted source value.

Custom mapping names:

- must not be blank
- are unique ignoring case
- cannot conflict, ignoring case, with the canonical names:
  - `timestamp`
  - `severity`
  - `subsystem`
  - `eventCode`
  - `entityId`
  - `message`

The source path must be nonempty and structurally valid.

Different custom mappings may refer to source-specific concepts such as:

- request ID
- host
- region
- HTTP status
- queue depth
- process ID
- firmware
- device measurements

### Preserve unmapped source fields

When:

```json
"preserveUnmappedFields": true
```

TraceScope retains extracted values that are not already consumed by canonical or explicit custom mappings.

For structured object data, nested values are preserved using dotted source paths where applicable.

For example:

```json
{
  "context": {
    "traceId": "abc-123"
  }
}
```

can produce an unmapped attribute named:

```text
context.traceId
```

This is useful when exploring a source whose full schema is not yet known.

When:

```json
"preserveUnmappedFields": false
```

unmapped extracted values are not added as normalized custom attributes.

The original raw source remains preserved separately; disabling unmapped-field preservation does not erase the source record itself.

### Explicit mapping and preservation are different

Suppose the source contains:

```text
deployment_ring=blue
```

With preservation enabled, it may appear automatically using its source name:

```text
deployment_ring
```

An explicit custom mapping:

```json
{
  "name": "Deployment Ring",
  "sourcePath": "deployment_ring"
}
```

instead gives that value a stable investigation-facing name.

Explicit mappings are preferable for fields that are important to repeatable investigation and saved-profile reuse.

### Auto-detected custom fields

**New From Source** can create a starting profile and, for applicable importers, suggest custom mappings from source preview data.

This is discovery assistance rather than a complete schema scan.

The normal preview is limited to **50 processed records**. When a rotated source family is selected, that preview allowance is distributed across the selected physical sources.

Fields that occur only later in the source may therefore not be auto-detected.

For important values, inspect representative normal and failure records and add explicit mappings when needed.

## 6. Saved profile JSON reference

**Save Profile...** serializes the current profile as versioned JSON.

TraceScope v1.0 supports:

```json
"schemaVersion": 1
```

### Required top-level properties

The deserializer requires these properties with the indicated JSON types:

| Property | Type |
| --- | --- |
| `schemaVersion` | integer |
| `name` | string |
| `importerId` | string |
| `canonicalFields` | object containing all six required string properties |
| `customFields` | array |
| `severityAliases` | object |
| `timestampRules` | array |
| `preserveUnmappedFields` | boolean |

The serializer also writes:

- `recordPath` when it is nonempty
- `regexPattern` when it is nonempty

Those two properties are therefore optional in the saved representation.

### Complete example

The bundled Service Session CSV profile is:

```json
{
  "schemaVersion": 1,
  "name": "Service Session CSV",
  "importerId": "csv",
  "canonicalFields": {
    "timestamp": "timestamp",
    "severity": "level",
    "subsystem": "subsystem",
    "eventCode": "eventCode",
    "entityId": "entityId",
    "message": "message"
  },
  "customFields": [
    {
      "name": "Host",
      "sourcePath": "host"
    },
    {
      "name": "Region",
      "sourcePath": "region"
    },
    {
      "name": "Duration (ms)",
      "sourcePath": "duration_ms"
    },
    {
      "name": "Correlation ID",
      "sourcePath": "correlation_id"
    },
    {
      "name": "Deployment Ring",
      "sourcePath": "deployment_ring"
    }
  ],
  "severityAliases": {},
  "timestampRules": [
    {
      "type": "iso8601"
    }
  ],
  "preserveUnmappedFields": true
}
```

### `canonicalFields`

This object must contain all six string values:

```json
"canonicalFields": {
  "timestamp": "",
  "severity": "",
  "subsystem": "",
  "eventCode": "",
  "entityId": "",
  "message": ""
}
```

Empty strings represent intentionally unmapped canonical fields.

### `customFields`

Each array entry requires:

```json
{
  "name": "Display Name",
  "sourcePath": "source.field"
}
```

An empty `customFields` array is valid.

### `severityAliases`

This is a JSON object whose keys are source labels and whose values identify TraceScope severities.

Example:

```json
"severityAliases": {
  "FAIL": "ERROR",
  "NOTICE": "INFO"
}
```

An empty object is valid.

### `timestampRules`

Each entry requires a supported `type`.

ISO 8601:

```json
{
  "type": "iso8601"
}
```

Qt format:

```json
{
  "type": "qt-format",
  "format": "yyyy-MM-dd HH:mm:ss.zzz"
}
```

An empty array is valid only when no canonical timestamp path requires parsing.

### `recordPath`

When present:

```json
"recordPath": "data.records"
```

it is used by Structured JSON or Structured XML to select records.

The serializer omits the property when the value is empty.

### `regexPattern`

When present:

```json
"regexPattern": "^(?<timestamp>...) (?<message>.*)$"
```

it supplies the Regex Plain Text parser configuration.

`regex-text` profiles require a nonempty valid pattern.

The serializer omits the property when the value is empty.

### Loading a profile

Loading a saved profile proceeds through distinct checks:

1. the file must be readable
2. the contents must be valid JSON
3. the JSON must match the profile serialization structure
4. the resulting profile must pass configuration validation
5. its `importerId` must identify a built-in importer supported by the running TraceScope version

A file can therefore be valid JSON without being a valid TraceScope profile, and a structurally valid profile can still be unsuitable for a particular source.

Manual editing is supported by the readable JSON format, but **Save Profile...** is the safest way to produce the canonical serialized representation.

## 7. Bundled profile examples

The repository contains profiles that demonstrate different mapping problems rather than one universal profile pattern.

| Profile | Demonstrates |
| --- | --- |
| [`service-session-csv-profile.json`](../samples/profiles/service-session-csv-profile.json) | Header-based CSV mappings, ISO 8601 timestamp, explicitly named custom fields |
| [`telemetry-batch-tsv-profile.json`](../samples/profiles/telemetry-batch-tsv-profile.json) | TSV mappings, Qt timestamp format, source-specific severity aliases |
| [`structured-json-nested-profile.json`](../samples/profiles/structured-json-nested-profile.json) | Nested Structured JSON record selection and nested field paths |
| [`structured-xml-engineering-session-profile.json`](../samples/profiles/structured-xml-engineering-session-profile.json) | Repeated XML records, nested elements, selected-element attributes |
| [`windows-event-engineering-session-profile.json`](../samples/profiles/windows-event-engineering-session-profile.json) | Windows Event XML normalization, attributes, named EventData, numeric level aliases |
| [`general-application-regex-profile.json`](../samples/profiles/general-application-regex-profile.json) | Named Regex Plain Text captures mapped into canonical and custom fields |
| [`java-style-regex-profile.json`](../samples/profiles/java-style-regex-profile.json) | Regex captures plus a custom Qt timestamp format |
| [`syslog-rfc5424-profile.json`](../samples/profiles/syslog-rfc5424-profile.json) | Parser-provided Syslog fields and explicit RFC 5424 structured-data mappings |
| [`syslog-rfc3164-profile.json`](../samples/profiles/syslog-rfc3164-profile.json) | A source with intentionally unavailable canonical fields |
| [`iis-w3c-profile.json`](../samples/profiles/iis-w3c-profile.json) | Mapping importer-generated timestamp/message values and W3C source fields |

### Intentional absence is valid configuration

A profile does not need to fill every canonical field.

The RFC 5424 sample profile, for example, leaves Entity ID blank because the source does not define a universal application-domain entity identifier.

The RFC 3164 profile likewise leaves unsupported canonical concepts unmapped.

That is preferable to mapping an unrelated value merely to make the profile appear complete.

### Windows Event example

The bundled Windows Event profile demonstrates more specialized normalized paths:

```json
"canonicalFields": {
  "timestamp": "System.TimeCreated.@SystemTime",
  "severity": "System.Level",
  "subsystem": "System.Provider.@Name",
  "eventCode": "System.EventID",
  "entityId": "EventData.NamedData.DeviceId",
  "message": "RenderingInfo.Message"
}
```

and severity aliases:

```json
"severityAliases": {
  "1": "CRITICAL",
  "2": "ERROR",
  "3": "WARN",
  "4": "INFO",
  "5": "TRACE"
}
```

These mappings are appropriate to that Windows Event schema. They are not a claim that every Windows Event contains `DeviceId`, rendered message text, or the same application-specific data.

### RFC 5424 structured data

The bundled RFC 5424 profile explicitly maps fields such as:

```text
structuredDataFields.request@32473.requestId
structuredDataFields.payment@32473.provider
structuredDataFields.database@32473.elapsedMs
```

because its:

```json
"preserveUnmappedFields": false
```

setting means important structured-data values should be explicitly named if they are to become normalized custom attributes.

## 8. Validation, preview, and reuse

Profile validation and source preview answer different questions.

### Validation

Validation checks configuration rules including:

- supported schema version
- nonblank profile name
- nonblank importer ID
- valid Record path structure
- required and syntactically valid Regex Plain Text pattern
- valid canonical source paths
- nonblank custom-field names
- nonduplicate custom-field names, ignoring case
- no custom-field names conflicting with canonical names
- valid nonempty custom-field source paths
- nonblank and nonduplicate severity aliases
- valid severity targets
- required timestamp rules when Timestamp is mapped
- required `format` for `qt-format`
- ignored-format warning for `iso8601`

Validation establishes that the configuration is structurally acceptable.

It does **not** establish that the profile correctly describes the selected source.

### Preview

Preview exercises the configured importer against the selected source.

It can reveal problems that static profile validation cannot know, including:

- wrong field names
- wrong Record path
- unexpected source values
- timestamp parse failures
- unmapped severities
- malformed records
- regex records that do not match
- fields that appear only in some records

The normal preview processes up to **50 source records**.

For a selected rotated source family, that limit is shared across the ordered physical sources rather than being applied independently to every file.

Large Structured JSON and XML documents can require an explicit **Refresh Preview** so their bounded preview work runs in the background rather than repeatedly parsing an expensive document during configuration changes.

### Saving and reuse

**Save Profile...** writes the current configuration to JSON.

**Load Profile...** restores a saved profile after deserialization, validation, and importer-availability checks.

**Recent Profiles** provides shortcuts to previously used profile paths.

**New From Source** creates a new working configuration from the selected source's format suggestion and any applicable built-in preset or detected fields. It replaces the current working profile rather than modifying it incrementally.

Before reusing a profile with another source, confirm that the source still follows the same schema.

Changes such as:

- renamed CSV headers
- moved JSON fields
- changed XML structure
- new severity labels
- modified regex-oriented text layout
- altered timestamp format

can make a previously valid profile incorrect without making the saved profile file itself structurally invalid.

## Troubleshooting

| Situation | What to check |
| --- | --- |
| A saved profile will not load | Confirm that the file is valid JSON, uses supported schema version `1`, contains the required properties and JSON types, passes profile validation, and names a supported importer. See [Saved profile JSON reference](#6-saved-profile-json-reference) and [Loading a profile](#loading-a-profile) for related information. |
| A profile passes Validation, but mapped columns are blank or incorrect | Compare a representative **Selected Raw Source** record with the configured paths. Validation establishes that the configuration is structurally acceptable; it does not prove that the paths describe this source correctly. See [Source paths and format-specific configuration](#3-source-paths-and-format-specific-configuration) and [Validation, preview, and reuse](#8-validation-preview-and-reuse) for more information. |
| A numeric or boolean JSON value will not populate a canonical field | Canonical extraction is text-oriented. Numeric, boolean, array, and object values are not automatically converted into canonical strings. See [Canonical values are text-oriented](#canonical-values-are-text-oriented) for more information. |
| Nested JSON or XML values do not appear | Verify that field paths begin at the **selected record**, not the outer document. For XML attributes use `@`; for Windows Event named data use the normalized `EventData.NamedData` paths. See [Structured JSON](#structured-json) and [Structured XML](#structured-xml) for related information. |
| A regex profile imports too few records or puts values in the wrong fields | Test the expression against several representative records, confirm the intended named capture groups, and remember that the expression must match the complete record. See [Regex Plain Text](#regex-plain-text) for more information. |
| Severity is missing or events appear under the wrong severity | Check both the canonical Severity source path and any configured aliases. Aliases interpret an extracted value; they do not create severity when the source has none. See [Severity aliases](#severity-aliases) for more information. |
| Timestamps are unavailable, invalid, or unexpectedly shifted | Check the Timestamp source path, timestamp-rule type and order, Qt format string when applicable, and the source's timezone semantics. See [Timestamp rules](#timestamp-rules) and [Supported Formats: Timestamp quality depends on the source](supported-formats.md#timestamp-quality-depends-on-the-source) for related information. |
| A source-specific value exists in Raw Source but is not available as a custom attribute | Add an explicit custom mapping or enable **Preserve unmapped source fields** when appropriate. Raw-source preservation and normalized custom-field preservation are different. See [Custom fields and unmapped data](#5-custom-fields-and-unmapped-data) for more information. |
| **New From Source** did not detect a field that exists later in the file | Auto-detection uses bounded preview data rather than scanning the entire source schema. Add important late-appearing fields explicitly. See [Auto-detected custom fields](#auto-detected-custom-fields) and [Validation, preview, and reuse](#8-validation-preview-and-reuse) for related information. |
| A profile works for one file but not another with the same extension | Profiles describe source schemas, not file extensions. Compare actual headers, field names, nesting, severity vocabulary, and timestamp representation. See [One importer can use many profiles](#one-importer-can-use-many-profiles) and [Bundled profile examples](#7-bundled-profile-examples) for related information. |
| A Structured JSON or XML profile imports statically but cannot be used for Live Following | Live structured sources require repeatable record containers beyond ordinary static-profile validity. See [Structured JSON](#structured-json), [Structured XML](#structured-xml), and [Supported Formats: Live Following compatibility](supported-formats.md#5-live-following-compatibility) for related information. |

## Related documentation

- [Supported Formats](supported-formats.md) — importer families, expected source layouts, examples, and Live Following compatibility.
- [Importing Logs](importing-logs.md) — source selection, profile configuration, preview, rotated source families, and import execution.
- [Live Following](live-following.md) — additional source/profile requirements for growing files.
- [Saving and Restoring](saving-and-restoring.md) — distinction between reusable configuration, investigation evidence, and workspaces.
- [Troubleshooting](troubleshooting.md) — profile-loading failures, unexpected mappings, timestamp/severity problems, and import diagnostics.
