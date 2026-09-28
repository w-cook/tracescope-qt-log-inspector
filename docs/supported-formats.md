# Supported Log Formats in TraceScope

TraceScope imports local log files into a common investigation view. The **Format** selected in **Import Configuration** determines how records are read; an **import profile** determines which source values become timestamps, severities, messages, other familiar investigation fields, and source-specific attributes.

Use this reference to choose an importer and check its expected input. For the complete process of opening, previewing, and configuring a file, see [Importing Logs](importing-logs.md). If you are opening TraceScope for the first time, start with [Getting Started](getting-started.md).

**Having trouble identifying or importing a format?** [Jump straight to Troubleshooting](#troubleshooting).

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Identify an available format and its importer | [Supported formats at a glance](#1-supported-formats-at-a-glance) |
| Check how TraceScope suggests an importer | [Format suggestions and file extensions](#2-format-suggestions-and-file-extensions) |
| Understand what a particular importer expects | [Format-specific requirements](#3-format-specific-requirements) |
| Find a sample file and matching profile | [Bundled format examples](#4-bundled-format-examples) |
| Check which formats work with live files | [Live Following compatibility](#5-live-following-compatibility) |
| Understand what a valid import may still be missing | [Field availability and import limitations](#6-field-availability-and-import-limitations) |
| Fix a format-specific problem | [Troubleshooting](#troubleshooting) |

## 1. Supported formats at a glance

TraceScope includes nine built-in importers. Some familiar log layouts are recognized through **presets** for an existing importer rather than through separate parsers.

| Format in TraceScope | Importer ID | Expected source | Example |
| --- | --- | --- | --- |
| **JSON Lines** | `json-lines` | One JSON object per line; commonly `.jsonl` or `.ndjson` | [Order Fulfillment](../samples/order-fulfillment-incident.jsonl) |
| **Structured JSON** | `structured-json` | A JSON object, an array of record objects, or an object containing a selected record array; commonly `.json` | [Nested structured JSON](../samples/structured-json-nested-session.json) |
| **CSV** | `csv` | Comma-separated records with a header row; commonly `.csv` | [Service session CSV](../samples/service-session.csv) |
| **TSV** | `tsv` | Tab-separated records with a header row; commonly `.tsv` | [Telemetry batch TSV](../samples/telemetry-batch.tsv) |
| **Key-Value / logfmt** | `key-value` | Line-oriented `key=value` fields, including supported quoted values; often `.log` or `.txt` | [Field Gateway](../samples/field-gateway-known-good.log) |
| **Syslog (RFC 5424 / RFC 3164)** | `syslog` | Text records following either supported Syslog layout; commonly `.log` or `.syslog` | [RFC 5424 sample](../samples/syslog-rfc5424-session.log) |
| **IIS W3C Extended Log** | `iis-w3c` | IIS-style text with a `#Fields:` header; commonly `.log` | [IIS W3C sample](../samples/iis-w3c-access.log) |
| **Structured XML** | `xml` | XML with a document-root record or records selected by element path; commonly `.xml` | [Engineering session XML](../samples/structured-engineering-session.xml) |
| **Regex Plain Text** | `regex-text` | One text record per line, parsed using a configured regular expression with named captures; often `.log` or `.txt` | [General application log](../samples/general-application.log) |

**Note:** `.log` and `.txt` can contain several different log formats. See [Format suggestions and file extensions](#2-format-suggestions-and-file-extensions) for details.

**Recognized layouts, not extra importers:** Apache Common and Apache/Nginx Combined access logs use supplied **Regex Plain Text** presets. Windows Event XML uses **Structured XML** with an appropriate record path and field mappings. Native binary Windows `.evtx` files are **not** directly supported.

These are source-import formats. A saved TraceScope workspace (`.tsw`) or standalone investigation snapshot (`.tsinv`) is a separate TraceScope artifact, opened using its own application command; neither is a log-import format. See [Saving and Restoring](saving-and-restoring.md) for details.

## 2. Format suggestions and file extensions

When you select a file, **Likely format** offers a starting point. TraceScope uses recognizable extensions for several formats and can examine representative content for certain text formats.

| Source indicator | Typical suggestion |
| --- | --- |
| `.jsonl` or `.ndjson` | JSON Lines |
| `.json` | Structured JSON |
| `.csv` or `.tsv` | CSV or TSV |
| `.xml` | Structured XML; a recognizable Windows Event XML document or collection can also select a Windows Event preset |
| `.syslog` | Syslog |
| Suitable content in a generic text file | JSON Lines, Syslog, Key-Value / logfmt, IIS W3C, or a recognized Apache/Nginx access-log preset, depending on the content |

A `.log` or `.txt` extension does not identify a unique format. The suggestion process samples only part of the file and may not recognize custom or mixed-content logs. **Always inspect the selected Format, profile, Validation, and Source Record Preview** before importing. You can choose an importer or load a profile manually even when TraceScope suggests something else.

For the exact UI procedure, see [Import a log file](importing-logs.md#1-import-a-log-file) and [Verify the preview and validation](importing-logs.md#5-verify-the-preview-and-validation).

## 3. Format-specific requirements

### JSON Lines

Each nonempty record line should contain a complete JSON object. JSON Lines is useful for applications that write successive structured events without maintaining one large JSON document. It is not the same as a single JSON array spread across multiple lines.

A profile maps top-level or nested object fields using source paths, such as `context.requestId`. Malformed or unsuitable records may produce import diagnostics. For an initial example, use the [Order Fulfillment log](../samples/order-fulfillment-incident.jsonl) and [its matching profile](../samples/profiles/order-fulfillment-incident-profile.json).

### Structured JSON

Use Structured JSON when the file is one JSON document rather than independent line-delimited objects. With **Record path** left blank, TraceScope can import a root object as one record or a root array as multiple records. If an object contains the actual records deeper inside, supply a dot-separated **Record path** to select them.

For example, the [nested JSON sample](../samples/structured-json-nested-session.json) uses `data.records`. Its [profile](../samples/profiles/structured-json-nested-profile.json) demonstrates both selecting that array and mapping nested fields within each selected record.

For very large structured JSON files, TraceScope may require you to select **Refresh Preview** rather than automatically parsing a preview. Live Following has an additional repeated-record-array requirement explained [below](#5-live-following-compatibility).

### CSV and TSV

Both delimited importers expect a **header row**. Map the column names—not column positions—to canonical and custom investigation fields. CSV separates fields with commas; TSV separates them with tabs. Quoted values and escaped quotation marks are supported in individual delimited records.

These import paths treat each physical line as a record. Do not assume a CSV or TSV file with quoted fields spanning multiple physical lines can be imported correctly. Check the preview and diagnostics if data contains embedded line breaks, missing headers, inconsistent column counts, or unusual quoting.

Try the [service-session CSV](../samples/service-session.csv) with its [profile](../samples/profiles/service-session-csv-profile.json), or the [telemetry-batch TSV](../samples/telemetry-batch.tsv) with its [profile](../samples/profiles/telemetry-batch-tsv-profile.json).

### Key-Value / logfmt

This importer reads line-oriented records containing named assignments, such as `level=INFO` and `message="Batch uploaded"`. Values can contain supported quoted text. Use the extracted keys as source paths in the profile.

Because many unrelated applications use `.log`, format recognition for logfmt-like text depends on the content rather than a dedicated extension. The importer reports malformed assignments or other parsing problems through diagnostics. The [Field Gateway sample pair](#4-bundled-format-examples) demonstrates using one shared profile for two related log files.

### Syslog: RFC 5424 and RFC 3164

The Syslog importer recognizes the supported RFC 5424 and RFC 3164 line layouts and extracts available fields for profile mapping. The supplied profiles demonstrate timestamp, severity, subsystem, message, and Syslog-specific custom-field mappings. RFC 5424 also supplies event codes through its message ID. Neither bundled sample supplies an entity ID, and RFC 3164 does not supply event codes.

**Important timestamp distinction:** RFC 3164 timestamps have no year or timezone. TraceScope infers the nearest year relative to its import reference date and interprets the result using local time. It emits an informational diagnostic when this happens. An inferred time should not be treated as equivalent to an explicit, independently verified timestamp when comparing sources.

Use the [RFC 5424 sample](../samples/syslog-rfc5424-session.log) and [profile](../samples/profiles/syslog-rfc5424-profile.json), or the [RFC 3164 sample](../samples/syslog-rfc3164-session.log) and [profile](../samples/profiles/syslog-rfc3164-profile.json).

### IIS W3C Extended Log

Use this dedicated importer for IIS-style W3C access logs with a `#Fields:` declaration. It reads the declared field names, including names such as `cs-method` and `sc-status`, and makes them available for profile mapping. The importer combines source `date` and `time` fields into a timestamp value for mapping.

A log without the expected field declaration cannot be treated like an ordinary CSV simply because its data rows contain separated values. Request methods, response codes, and durations are meaningful custom fields, but an HTTP status code is **not automatically a TraceScope severity**. The supplied profile leaves unavailable canonical fields unmapped rather than inventing them.

See the [IIS sample](../samples/iis-w3c-access.log) and [profile](../samples/profiles/iis-w3c-profile.json).

### Structured XML, including Windows Event XML

The XML importer maps selected elements and attributes into source fields. Leave **Record path** empty when the document root is itself the record. For repeated elements within a container, supply their dot-separated element path.

The [engineering XML sample](../samples/structured-engineering-session.xml) uses `session.events.event` with its [matching profile](../samples/profiles/structured-xml-engineering-session-profile.json). For an XML collection of Windows Event records, the [Windows Event sample](../samples/windows-event-engineering-session.xml) uses `Events.Event` with its [matching profile](../samples/profiles/windows-event-engineering-session-profile.json). Windows Event mappings can use attribute paths such as `System.TimeCreated.@SystemTime`; levels can be converted using profile severity aliases.

TraceScope recognizes appropriate Windows Event XML structures and offers relevant presets. This supports **XML-formatted events and collections**, not direct import of binary `.evtx` files. The presence and meaning of individual Windows event fields can vary; for example, not every event contains a rendered message or application-specific entity ID.

For live XML, a nonempty record path selecting repeated elements is required. See [Live Following compatibility](#5-live-following-compatibility).

### Regex Plain Text, including web access-log presets

For custom line-oriented text, provide a **Qt `QRegularExpression`-compatible pattern with named capture groups**. Map the resulting capture names in the profile. A line that does not match your pattern cannot supply the expected fields; investigate skipped records and diagnostics rather than assuming a valid regular expression fits the entire file.

Start with the [general application sample](../samples/general-application.log) and its [regex profile](../samples/profiles/general-application-regex-profile.json). There are also supplied examples and presets for [Apache Common](../samples/apache-common-access.log), [Apache Combined](../samples/apache-combined-access.log), and [Nginx Combined](../samples/nginx-combined-access.log) layouts. These use the Regex Plain Text importer; they are not separate generic web-server parsers.

The built-in access-log presets map the HTTP request to **Message** and common request/response information to custom fields. They do not invent severity values for sources that do not provide them. Logs with customized access-log layouts may need a different regex profile. See [Import text using a regular expression](importing-logs.md#import-text-using-a-regular-expression) for configuration help.

## 4. Bundled format examples

These sample/profile pairs offer a practical way to verify your setup before configuring an unfamiliar source. They are fictional demonstration material.

| Format or layout | Sample source | Matching profile |
| --- | --- | --- |
| JSON Lines | [`order-fulfillment-incident.jsonl`](../samples/order-fulfillment-incident.jsonl) | [`order-fulfillment-incident-profile.json`](../samples/profiles/order-fulfillment-incident-profile.json) |
| Structured JSON, root array | [`structured-json-array-session.json`](../samples/structured-json-array-session.json) | [`structured-json-array-profile.json`](../samples/profiles/structured-json-array-profile.json) |
| Structured JSON, nested array | [`structured-json-nested-session.json`](../samples/structured-json-nested-session.json) | [`structured-json-nested-profile.json`](../samples/profiles/structured-json-nested-profile.json) |
| CSV | [`service-session.csv`](../samples/service-session.csv) | [`service-session-csv-profile.json`](../samples/profiles/service-session-csv-profile.json) |
| TSV | [`telemetry-batch.tsv`](../samples/telemetry-batch.tsv) | [`telemetry-batch-tsv-profile.json`](../samples/profiles/telemetry-batch-tsv-profile.json) |
| Key-Value / logfmt | [`field-gateway-known-good.log`](../samples/field-gateway-known-good.log) and [`field-gateway-degraded.log`](../samples/field-gateway-degraded.log) | [`field-gateway-support-session-profile.json`](../samples/profiles/field-gateway-support-session-profile.json) |
| RFC 5424 Syslog | [`syslog-rfc5424-session.log`](../samples/syslog-rfc5424-session.log) | [`syslog-rfc5424-profile.json`](../samples/profiles/syslog-rfc5424-profile.json) |
| RFC 3164 Syslog | [`syslog-rfc3164-session.log`](../samples/syslog-rfc3164-session.log) | [`syslog-rfc3164-profile.json`](../samples/profiles/syslog-rfc3164-profile.json) |
| IIS W3C | [`iis-w3c-access.log`](../samples/iis-w3c-access.log) | [`iis-w3c-profile.json`](../samples/profiles/iis-w3c-profile.json) |
| Structured XML | [`structured-engineering-session.xml`](../samples/structured-engineering-session.xml) | [`structured-xml-engineering-session-profile.json`](../samples/profiles/structured-xml-engineering-session-profile.json) |
| Windows Event XML collection | [`windows-event-engineering-session.xml`](../samples/windows-event-engineering-session.xml) | [`windows-event-engineering-session-profile.json`](../samples/profiles/windows-event-engineering-session-profile.json) |
| Regex plain text | [`general-application.log`](../samples/general-application.log) | [`general-application-regex-profile.json`](../samples/profiles/general-application-regex-profile.json) |
| Apache Common access log | [`apache-common-access.log`](../samples/apache-common-access.log) | [`apache-common-profile.json`](../samples/profiles/apache-common-profile.json) |
| Apache/Nginx Combined access log | [`nginx-combined-access.log`](../samples/nginx-combined-access.log) | [`nginx-combined-profile.json`](../samples/profiles/nginx-combined-profile.json) |

The same importer can support many different source schemas. A successful import with one sample profile does not mean the profile is correct for every file of that general format. Confirm field names, timestamps, optional values, and source-record preview against your own data.

## 5. Live Following compatibility

A successful static import and a followable growing file are related but different questions. All nine built-in importer families have live-ingestion paths, **subject to the source shape and configuration**. Live Following requires an accessible connected external source, not merely a saved standalone investigation snapshot.

| Importer family | Live Following requirement |
| --- | --- |
| JSON Lines | Complete JSON-object records are delimited by lines; incomplete trailing lines are held until complete. |
| CSV / TSV | Header-based, one physical line per record; the active file must retain the necessary header context. |
| Key-Value / logfmt, Syslog, Regex Plain Text | Line-oriented source with a matching import profile; incomplete trailing lines are held until complete. |
| IIS W3C | Line-oriented W3C source with its `#Fields:` header context. |
| Structured JSON | A repeatedly appended **array of record objects**: either the root array or an array selected by **Record path**. A one-off root object is useful for static import but is not a repeated live-record container. |
| Structured XML | A **nonempty Record path** selecting repeated record elements in a growing XML container. An isolated document-root record is not sufficient for live XML following. |

Live Following observes a growing **file**, not a Syslog network listener, a database, or an application process. New records join the existing investigation. Truncation, replacement, or rotation can affect which unread bytes remain available; plan preservation accordingly. See [Live Following](live-following.md) for operation, source changes, and evidence preservation.

## 6. Field availability and import limitations

A supported source format determines how TraceScope *reads* records. What you can investigate depends on which fields the source actually contains and how accurately the profile maps them.

- **Canonical fields are optional.** Timestamp, Severity, Subsystem, Event code, Entity ID, and Message need not all exist. Leave genuinely unavailable mappings blank. Views and comparisons that depend on absent data may be unavailable or less informative rather than showing an invented zero.
- **Custom fields preserve source-specific meaning.** Use them for attributes such as HTTP status, queue depth, firmware, device identifiers, or request context without pretending they are universally equivalent across unrelated sources.
- **Timestamp interpretation matters.** Verify timestamp rules, timezones, and inferred RFC 3164 dates before drawing conclusions about order, duration, or rates across independently recorded sources.
- **A recognized layout is not a guarantee of good mappings.** The preview samples only part of the source. Check representative records, including unusual and error conditions, and review available import diagnostics after importing.
- **Format conversion is not automatic.** TraceScope does not directly read binary Windows `.evtx` files; export events to a supported XML representation when that workflow is appropriate. Other custom source layouts may require a deliberately configured regex or a separate supported text/structured representation.

The separate **Import Profiles** reference will document the saved profile schema and individual configuration fields. For now, see [Configure an import profile](importing-logs.md#3-configure-an-import-profile).

## Troubleshooting

| Situation | What to check |
| --- | --- |
| TraceScope does not suggest a format for my `.log` file. | Extensions such as `.log` can contain many unrelated layouts. Inspect a few representative records, select the matching importer manually, or load a supplied profile. See [Format suggestions and file extensions](#2-format-suggestions-and-file-extensions) for more information. |
| My JSON file is treated as individual lines, or a JSON Lines file is treated as one document. | Distinguish independent JSON objects on successive lines from a single structured JSON object or array. Change the importer if necessary. See [JSON Lines](#json-lines) and [Structured JSON](#structured-json) for more information. |
| Structured JSON or XML imports no records, or the wrong records. | Confirm whether the document root is the record or whether you must set **Record path** to a nested array/element. Then check individual field mappings. See [Structured JSON](#structured-json), [Structured XML](#structured-xml-including-windows-event-xml), and [Importing Logs](importing-logs.md#select-records-inside-structured-json-or-xml) for more information. |
| CSV/TSV records are skipped or columns are wrong. | Verify the header row, selected delimiter, column names, and whether values contain unusual quoting or embedded newlines. See [CSV and TSV](#csv-and-tsv) for more information. |
| A web access log has no severity or event code. | The supported access-log presets intentionally do not manufacture fields the source does not contain. Use HTTP status as source-specific data unless you deliberately establish another mapping. See [Regex Plain Text, including web access-log presets](#regex-plain-text-including-web-access-log-presets) for more information. |
| RFC 3164 timestamps differ from expected times. | The source does not include a year or timezone. Check TraceScope's inference diagnostic and your actual collection context. See [Syslog: RFC 5424 and RFC 3164](#syslog-rfc-5424-and-rfc-3164) for more information. |
| I cannot import a Windows `.evtx` file. | TraceScope supports Windows **Event XML**, not native binary `.evtx` input. Obtain XML-formatted events and use appropriate mappings. See [Structured XML, including Windows Event XML](#structured-xml-including-windows-event-xml) for more information. |
| My imported structured document cannot be followed live. | Check whether the source is a repeated-record array (JSON) or has a configured repeating record path (XML), whether a connected external file still exists, and whether the producer is appending complete records. See [Live Following compatibility](#5-live-following-compatibility) and [Live Following](live-following.md) for more information. |

## Related documentation

- [Getting Started](getting-started.md) — import and investigate a provided JSON Lines sample without creating a profile.
- [Importing Logs](importing-logs.md) — configure a source, map fields, interpret preview/validation, and import rotated files.
- [Live Following](live-following.md) — observe supported growing files and preserve evidence when external sources change.
- [Saving and Restoring](saving-and-restoring.md) — understand the distinction between source log files, saved workspaces, and standalone investigation snapshots.
- **Import Profiles (planned)** — saved profile schema, source paths, field mappings, and reusable configuration.
