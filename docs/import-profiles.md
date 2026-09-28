# Import Profiles in TraceScope

An **import profile** tells TraceScope how to interpret records from a particular kind of log. It identifies the importer, selects records within structured documents when necessary, and maps source values to the investigation fields and custom attributes you want to examine.

Use this reference when you need to understand a profile setting, build a mapping for an unfamiliar source, inspect a saved `.json` profile, or diagnose a configuration that produces unexpected results. For the step-by-step import workflow, see [Importing Logs](importing-logs.md). To choose a compatible importer first, see [Supported Formats](supported-formats.md).

**Having trouble with a profile or its mappings?** [Jump straight to Troubleshooting](#troubleshooting).

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Understand what a profile controls—and what it does not | [What an import profile controls](#1-what-an-import-profile-controls) |
| Look up a setting or canonical field | [Profile settings at a glance](#2-profile-settings-at-a-glance) |
| Understand nested paths, record selection, or regex captures | [Source paths and format-specific configuration](#3-source-paths-and-format-specific-configuration) |
| Configure severity aliases or timestamp parsing | [Severity aliases and timestamp rules](#4-severity-aliases-and-timestamp-rules) |
| Decide which custom fields to expose or preserve | [Custom fields and unmapped data](#5-custom-fields-and-unmapped-data) |
| Read or edit the saved JSON representation | [Saved profile JSON reference](#6-saved-profile-json-reference) |
| Examine profiles for real sample formats | [Examples from the bundled profiles](#7-examples-from-the-bundled-profiles) |
| Distinguish valid configuration from correct results | [Validation, preview, and reuse](#8-validation-preview-and-reuse) |
| Resolve an unexpected profile result | [Troubleshooting](#troubleshooting) |

## 1. What an import profile controls

TraceScope separates **reading a record** from **interpreting its fields**. The selected importer reads a supported source format; the profile determines how values extracted from that format are presented in the common investigation model.

A profile contains:

- An **importer ID** identifying the format reader, such as `json-lines`, `csv`, `syslog`, or `xml`.
- Optional **record-selection or parsing configuration**, such as a nested JSON/XML record path or a regular expression for plain-text logs.
- **Canonical field mappings** for the familiar investigation fields, where the source supplies them.
- **Custom field mappings** for source-specific details, plus options for severity aliases, timestamp parsing, and preservation of other extracted values.

Profiles are reusable **configurations**, not copies of log files. A saved profile does not contain the source records, selected file path, rotation-file selection, bookmarks, finding classifications, analyst notes, or workspace layout. Choosing which physical files belong to a rotated source family is a separate part of Import Configuration. See [Import a rotated source family](importing-logs.md#6-import-a-rotated-source-family) and [Saving and Restoring](saving-and-restoring.md) for those separate responsibilities.

**One format can require many profiles.** Two applications may both produce JSON Lines but use entirely different field names. Likewise, a profile that works for one XML structure is not automatically correct for another. File extensions and a successful format suggestion do not establish that the field mappings are right.

## 2. Profile settings at a glance

The settings below correspond to controls in **Import Configuration** and properties of the [saved JSON profile](#6-saved-profile-json-reference). Some options are useful only with particular importers.

| Setting | Saved JSON property | Purpose |
| --- | --- | --- |
| **Name** | `name` | Descriptive name for identifying and reusing a profile. Required. |
| **Format** | `importerId` | Chooses the built-in importer. Required and must identify an importer supported by this TraceScope version. |
| **Record path** | `recordPath` | Selects records within Structured JSON or Structured XML. Optional for static imports; the requirement for live structured files is more specific. |
| **Regular expression** | `regexPattern` | Defines how Regex Plain Text extracts named values from each line. Required for `regex-text`. |
| **Canonical field mappings** | `canonicalFields` | Associate up to six standard investigation fields with extracted source values. An individual mapping may be blank when the source lacks that field. |
| **Custom fields** | `customFields` | Give source-specific values explicitly named, reusable investigation columns. |
| **Severity aliases** | `severityAliases` | Translate nonstandard source severity labels into TraceScope severities. |
| **Timestamp rules** | `timestampRules` | Identify how a mapped timestamp string should be parsed; rules are tried in order. |
| **Preserve unmapped source fields** | `preserveUnmappedFields` | Keep additional extracted values as custom attributes even without explicit mappings. |
| **Schema version** | `schemaVersion` | Identifies the saved profile format. Current supported version: `1`. |

### The six canonical fields

A canonical field gives a source-specific value a consistent role within TraceScope. The **source path** is the field name or nested path extracted by the selected importer; it is not the display name of the canonical field.

| Canonical field | JSON key | Typical source values | If unavailable |
| --- | --- | --- | --- |
| Timestamp | `timestamp` | `timestamp`, `observedAt`, or a nested timestamp path | Leave blank; time-based views may have less information. |
| Severity | `severity` | `level`, `priority`, or a parsed format-specific severity | Leave blank rather than invent a severity. |
| Subsystem | `subsystem` | Service, component, provider, or logger name | Leave blank if the format has no such field. |
| Event code | `eventCode` | Named event ID, application event code, or Syslog message ID | Leave blank if not supplied. |
| Entity ID | `entityId` | Device, order, request, or other domain-specific entity identifier | Leave blank if not supplied. |
| Message | `message` | Human-readable message, description, or request text | Leave blank if not supplied. |

These fields are **optional individually**, but the saved JSON `canonicalFields` object must contain **all six keys**, using an empty string (`""`) for any unused mapping. TraceScope does not require every log format to supply every canonical field.

Canonical extraction expects text values. In particular, directly mapping a numeric or boolean value in a JSON object does not turn it into a canonical string automatically. For those source values, inspect the resulting preview and consider a custom field or an appropriate upstream representation instead of assuming the mapping worked. The XML and Syslog importers normalize certain native values into mapped text fields as part of their own parsing.

## 3. Source paths and format-specific configuration

There are two different kinds of path in a profile:

- **Record path** identifies *which objects or elements count as records* in a larger Structured JSON or XML document.
- **Field source paths** identify *values within each selected record*, whether for a canonical field or a named custom attribute.

A field path is not a filesystem path or a general JSONPath/XPath expression. In supported structured sources, TraceScope traverses object/element names separated by dots; it does not accept arbitrary array indices, wildcard selectors, or predicates in these mapping fields.

### Structured JSON: select records, then map their fields

Consider the bundled [nested Structured JSON sample](../samples/structured-json-nested-session.json). Its profile uses:

```json
{
  "importerId": "structured-json",
  "recordPath": "data.records",
  "canonicalFields": {
    "timestamp": "observedAt",
    "severity": "priority",
    "subsystem": "service.name",
    "eventCode": "event.code",
    "entityId": "resource.id",
    "message": "details.message"
  }
}
```

This excerpt illustrates record selection and mappings; it is **not** a complete import-profile file. `data.records` locates the array in the outer document. `service.name` and `details.message` are then resolved **inside each selected record**, not from the document root.

For an ordinary static Structured JSON import, an empty Record path permits a root object to become one record or a root array to provide multiple records. A nested array requires its actual record path. For **Live Following**, a repeated record array is required; a single standalone root-object record does not meet that live-ingestion requirement. See [Structured JSON](supported-formats.md#structured-json) and [Live Following compatibility](supported-formats.md#5-live-following-compatibility).

### Structured XML: elements, attributes, and named event data

Structured XML also uses a dot-separated record path. For example, the bundled [engineering XML profile](../samples/profiles/structured-xml-engineering-session-profile.json) selects `session.events.event`. Within each selected `event`, it can map:

- `metadata.timestamp` as the timestamp;
- `context.deviceId` as the entity ID;
- `@sequence` as a custom field taken from an attribute on the selected `event` element.

An `@` prefix identifies an XML attribute after TraceScope has normalized the selected XML record. Windows Event XML also has a useful normalization for named `<Data Name="...">` elements. For example, the [Windows Event XML profile](../samples/profiles/windows-event-engineering-session-profile.json) maps `System.Provider.@Name` to the subsystem, and `EventData.NamedData.DeviceId` to the domain-specific entity ID in that particular sample. Those names describe the normalized representation used by TraceScope; they are not XPath queries.

With no record path, a complete XML document root can be used as one record during static import. A **nonempty record path selecting repeated elements** is required for live XML following. See [Structured XML](supported-formats.md#structured-xml-including-windows-event-xml) for supported input shapes.

### CSV, TSV, key-value logs, IIS W3C, and Syslog

For formats that already expose named values, field paths generally name those extracted fields:

| Source | Example path | Meaning |
| --- | --- | --- |
| CSV or TSV | `deployment_ring` | A column name from the file's header row. |
| Key-Value / logfmt | `queueDepth` | A key extracted from the line's `key=value` assignments. |
| IIS W3C | `sc-status` | A name declared in the source's `#Fields:` header. |
| Syslog RFC 5424 | `structuredDataFields.request@32473.requestId` | A parameter in parsed Syslog structured data, under its specific SD-ID. |

The **IIS W3C** importer combines its source `date` and `time` columns into an extracted `timestamp` field. Use that resulting field for the canonical timestamp instead of trying to map the two input columns independently.

Syslog's parser also extracts format-specific fields before your profile runs. A supplied field path such as `priority` or `syslogSeverity` refers to the **parsed output**, not directly to arbitrary characters in the original line. RFC 3164 has no native year or timezone; TraceScope infers those during parsing. See [Syslog](supported-formats.md#syslog-rfc-5424-and-rfc-3164) for the important timestamp qualification.

### Regex Plain Text: named captures become source fields

A Regex Plain Text profile needs a valid **regular expression** with named capture groups. A group such as `(?<level>...)` extracts a value called `level`, which can then be mapped to the canonical severity field. Named captures also support custom fields, such as `thread` or `requestId`.

For instance, the bundled [Java-style regex profile](../samples/profiles/java-style-regex-profile.json) captures `timestamp`, `level`, `thread`, `logger`, and `message`. It maps `logger` to **Subsystem** and `thread` to a custom **Thread** column. An equivalent regex pattern without named captures would not provide those fields for the profile mappings.

Patterns should be tested against several representative records, not just one convenient line. Anchoring with `^` and `$` is useful when each physical line must match the expected whole-record structure. TraceScope uses Qt's regular-expression implementation; consult the [Qt QRegularExpression reference](https://doc.qt.io/qt-6/qregularexpression.html) for exact syntax. See [Configure an import profile](importing-logs.md#3-configure-an-import-profile) for the interactive workflow.

## 4. Severity aliases and timestamp rules

These settings change the interpretation of a **successfully extracted** source value. They cannot compensate for a wrong field path, an absent source field, or a parser that failed to identify the record.

### Severity aliases

TraceScope recognizes ordinary severity names such as `TRACE`, `DEBUG`, `INFO`, `WARN`/`WARNING`, `ERROR`, and `CRITICAL`/`FATAL`; `INFORMATION` is also accepted. A profile adds aliases when a source uses other terms.

For example, the audited [Telemetry Batch TSV profile](../samples/profiles/telemetry-batch-tsv-profile.json) uses:

```json
"severityAliases": {
  "CAUTION": "WARN",
  "FAIL": "ERROR",
  "FATAL": "CRITICAL",
  "NOTICE": "INFO"
}
```

The source field `priority` is mapped to canonical **Severity**. Its value `FAIL` is then interpreted as **ERROR**, while `NOTICE` becomes **INFO**. The alias labels are matched without regard to letter case, and the target must be a supported TraceScope severity.

**Choose aliases by meaning, not spelling alone.** A severity that looks unusual is not necessarily an error, and a misleading mapping can hide relevant records from warning/error analysis. If a source supplies no severity at all, leave its canonical Severity path blank rather than using aliases to manufacture one. An unrecognized nonempty severity value can produce an import diagnostic and leave that event's normalized severity unavailable.

### Timestamp rules

A mapped timestamp requires at least one rule explaining how to parse its text. Two rule types are supported:

| Rule type in JSON | UI name | Use it when |
| --- | --- | --- |
| `iso8601` | ISO 8601 | Source values use a supported ISO 8601 representation, such as `2026-08-11T10:00:08.120Z`. No `format` property is necessary. |
| `qt-format` | Qt Format | Source values follow another predictable text layout and need a Qt date/time format string. |

For example, the [Java-style sample profile](../samples/profiles/java-style-regex-profile.json) includes:

```json
"timestampRules": [
  {
    "type": "qt-format",
    "format": "yyyy-MM-dd HH:mm:ss.zzz"
  }
]
```

That rule corresponds to a source value such as `2026-08-11 10:15:03.115`. When you configure multiple rules, TraceScope attempts them **in the order listed** until a rule parses the extracted timestamp. A timestamp path pointing to the wrong source value will not be fixed by adding more rules.

Rules can be omitted only when no canonical timestamp path is mapped. A `qt-format` rule needs a nonempty `format`; an ISO 8601 rule does not use one. Supplying a format string to an ISO 8601 rule produces a validation **warning** because that string is ignored. Verify actual timezones as well as formatting before comparing independent sources, especially when a source timestamp has no explicit offset.

## 5. Custom fields and unmapped data

Canonical fields make unrelated source types investigable through common controls. **Custom fields** keep the details that are specific to a source without forcing them into an inappropriate standard role.

Each explicit custom-field mapping has a display `name` and a source `sourcePath`. For example, the corrected [Service Session CSV profile](../samples/profiles/service-session-csv-profile.json) includes:

```json
{
  "name": "Deployment Ring",
  "sourcePath": "deployment_ring"
}
```

A clear display name makes an attribute easier to recognize in the investigation table, details, and applicable export views. A custom field name must be nonempty, must not duplicate another custom name ignoring case, and must not conflict with a canonical field name such as `timestamp` or `severity`. Its source path must also be valid and nonempty.

### What Preserve unmapped source fields actually does

With **Preserve unmapped source fields** enabled (`"preserveUnmappedFields": true`), TraceScope retains additional extracted values as custom attributes even when you have not explicitly named them. For nested structured data, these may appear under paths such as `context.traceId`. This is useful while exploring an unfamiliar source or accommodating sparse fields that appear only in some records.

With preservation disabled (`false`), additional extracted values **not included in canonical or explicit custom mappings** are not promoted to investigation attributes. The record's original raw source remains available separately, but a value present only in the raw text is not automatically a filterable custom attribute.

The distinction matters especially with structured Syslog. The bundled RFC 5424 example uses explicit mappings for its relevant structured-data parameters because its profile has preservation disabled. A source can parse successfully while important structured values remain unavailable as normalized attributes if those mappings are missing.

**Preserving is not the same as explicitly mapping.** An automatically retained attribute may have a raw, technical source-path name rather than a reader-friendly column heading. Give important values explicit mappings when you want consistent naming across records and repeat imports. Do not disable preservation until you have checked that the fields you still need are accounted for.

### Automatically detected fields are a starting point

**New From Source** can suggest a format and, for appropriate formats, detect additional custom fields from a limited preview. This is a convenience, not an exhaustive schema scan. The standard preview is limited to **50 processed records**, and some structured or regex-based configurations require more deliberate setup. Rare fields that occur later in a file may be absent from the suggested list.

Compare representative normal, warning, and error records, not only the first few lines. If an important value appears under **Unmapped Custom Fields**, add an explicit mapping and verify it again. For a more detailed preview procedure, see [Verify the preview and validation](importing-logs.md#5-verify-the-preview-and-validation).

## 6. Saved profile JSON reference

**Save Profile...** writes a versioned JSON configuration that you can read, back up, or edit outside TraceScope. Current supported profiles use `"schemaVersion": 1`. A saved profile is portable as configuration, but it still needs a compatible source format and field schema to produce correct results.

The following is the audited **Service Session CSV** profile, including the explicitly mapped `deployment_ring` field. It also serves as a complete example of the required top-level JSON structure for a straightforward format:

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
    { "name": "Host", "sourcePath": "host" },
    { "name": "Region", "sourcePath": "region" },
    { "name": "Duration (ms)", "sourcePath": "duration_ms" },
    { "name": "Correlation ID", "sourcePath": "correlation_id" },
    { "name": "Deployment Ring", "sourcePath": "deployment_ring" }
  ],
  "severityAliases": {},
  "timestampRules": [
    { "type": "iso8601" }
  ],
  "preserveUnmappedFields": true
}
```

For more specialized profiles, the optional top-level `recordPath` and `regexPattern` properties store format-specific configuration. A serializer-written file includes these only when nonempty. Empty arrays are valid for `customFields` and, when no timestamp is mapped, `timestampRules`. The `severityAliases` property is an object, even when it has no entries. The saved profile must include its other required top-level properties with the appropriate JSON types.

**Editing a saved profile manually:** Keep it valid JSON, retain the complete six-key `canonicalFields` object, use supported importer IDs and severity targets, and keep the schema version unchanged unless TraceScope explicitly supports a newer one. Loading a profile checks its JSON structure, configuration validity, and importer availability; a syntactically correct JSON document is not necessarily a valid or useful import profile. When in doubt, edit through Import Configuration and use **Save Profile...** to generate the representation for you.

## 7. Examples from the bundled profiles

The repository includes sample files paired with profiles. These examples illustrate *different mapping decisions*, not interchangeable settings for every file with a matching extension.

| Sample and profile | What it demonstrates |
| --- | --- |
| [Service Session CSV](../samples/service-session.csv) · [profile](../samples/profiles/service-session-csv-profile.json) | Named header columns, a straightforward ISO 8601 timestamp, explicit custom mappings including Deployment Ring. |
| [Nested Structured JSON](../samples/structured-json-nested-session.json) · [profile](../samples/profiles/structured-json-nested-profile.json) | Separating the outer `data.records` record path from nested canonical and custom field paths; custom severity labels. |
| [Java-style application log](../samples/java-style-application.log) · [profile](../samples/profiles/java-style-regex-profile.json) | Named regex captures, logger-to-subsystem mapping, Thread as a custom field, and a Qt timestamp format. |
| [Engineering XML](../samples/structured-engineering-session.xml) · [profile](../samples/profiles/structured-xml-engineering-session-profile.json) | Repeated XML records, nested element paths, and the selected element's `@sequence` attribute. |
| [Windows Event XML](../samples/windows-event-engineering-session.xml) · [profile](../samples/profiles/windows-event-engineering-session-profile.json) | Normalized XML attributes, `EventData.NamedData` extraction, and numeric Windows level-to-severity aliases. |
| [Syslog RFC 5424](../samples/syslog-rfc5424-session.log) · [profile](../samples/profiles/syslog-rfc5424-profile.json) | Parser-provided fields and explicitly mapped structured-data parameters; no invented entity ID. |
| [Telemetry Batch TSV](../samples/telemetry-batch.tsv) · [profile](../samples/profiles/telemetry-batch-tsv-profile.json) | A custom Qt timestamp rule and source-specific severity aliases, including `FAIL` to `ERROR`. |

Some profiles are intentionally reusable across files that share a schema. For example, [Telemetry Session](../samples/profiles/telemetry-session.json) works with several bundled JSON Lines files; not every file contains all the optional fields listed in that profile. A mapping need not produce a value for every record to be useful.

Other examples show **intentional absences**. RFC 3164's [profile](../samples/profiles/syslog-rfc3164-profile.json) leaves event code and entity ID unmapped because the bundled source format supplies neither. Live-generator profiles may instead rely on unmapped-field preservation to accommodate multiple different scenarios. Neither case should be “fixed” by assigning unrelated values to the canonical fields.

For the full importer list, associated extensions, and supported live-record shapes, consult [Supported Formats](supported-formats.md).

## 8. Validation, preview, and reuse

TraceScope performs two complementary checks, and both matter:

- **Validation** asks whether the configuration is structurally acceptable: supported schema version, a name and importer, valid paths and regex syntax, unique custom-field names, valid severity aliases, and suitable timestamp rules. Validation errors prevent saving or importing; a warning can flag an ignored or questionable setting without making the profile invalid.
- **Source Record Preview** asks what happens when the *current source data* is interpreted using those settings. Check normalized columns, the **Unmapped Custom Fields** column, the selected **Raw Source**, record counts, and diagnostics. A structurally valid profile can still map the wrong field, misinterpret a value, or fail on unusual later records.

For ordinary sources, preview examines up to **50 processed records**, not the entire file. For large Structured JSON or XML sources, you may need **Refresh Preview** to perform the preview explicitly in the background. If you are investigating rotated files, the preview allowance is shared across the selected physical sources. See [Verify the preview and validation](importing-logs.md#5-verify-the-preview-and-validation).

When a configuration works, use **Save Profile...** to write it as a `.json` file. **Load Profile...** reuses a saved file, and **Recent Profiles** provides shortcuts to previously used profile paths. **New From Source** starts a replacement working configuration from the selected file; if you have edited your current profile, TraceScope asks before replacing it. Save anything you want to retain first. These actions are explained step by step in [Save, load, and reset profiles](importing-logs.md#4-save-load-and-reset-profiles).

**A final verification habit:** Before reusing a profile with another source, inspect at least one representative record of each important kind. A new application version, changed header, alternate XML namespace/structure, or newly introduced custom field can invalidate assumptions without making the profile file itself fail validation.

## Troubleshooting

| Situation | What to check |
| --- | --- |
| A saved profile will not load. | Confirm that it is valid JSON, uses the supported schema version, includes the required properties, and names a built-in importer. A correct `.json` extension alone is insufficient. See [Saved profile JSON reference](#6-saved-profile-json-reference) and [Profile settings at a glance](#2-profile-settings-at-a-glance) for more information. |
| A profile passes Validation, but imported columns are blank or incorrect. | Compare **Selected Raw Source** with the current mappings. Ensure **Record path** selects the intended records and field paths start at the selected record—not the outer document. See [Source paths and format-specific configuration](#3-source-paths-and-format-specific-configuration) and [Validation, preview, and reuse](#8-validation-preview-and-reuse) for more information. |
| Nested JSON or XML fields do not appear. | Verify the selected record structure and exact nested paths. For XML attributes, use `@`; for Windows named event data, check the normalized `EventData.NamedData` paths. See [Structured JSON](#structured-json-select-records-then-map-their-fields) and [Structured XML](#structured-xml-elements-attributes-and-named-event-data) for more information. |
| A regex profile parses too few records or puts text in the wrong field. | Test its named capture groups against normal and unusual lines; inspect skipped-record counts and raw preview records. A syntactically valid regex is not necessarily the right parser for the file. See [Regex Plain Text](#regex-plain-text-named-captures-become-source-fields) for more information. |
| A severity is missing, or error events appear with the wrong severity. | Check the canonical Severity source path **and** any alias applied to its value. Make sure aliases reflect the meaning of the source labels. See [Severity aliases](#severity-aliases) for more information. |
| Timestamps are unavailable or shifted unexpectedly. | Check the mapped timestamp value, rule type and order, custom format string, and whether the original source specifies a timezone. Remember that RFC 3164 requires year and timezone inference. See [Timestamp rules](#timestamp-rules) and [Syslog](#csv-tsv-key-value-logs-iis-w3c-and-syslog) for more information. |
| A source-specific field is in Raw Source but not available as a custom attribute. | Check whether it is explicitly mapped or **Preserve unmapped source fields** is enabled. For RFC 5424, inspect the structured-data path under the correct SD-ID; for rare fields, do not rely solely on early auto-detection. See [Custom fields and unmapped data](#5-custom-fields-and-unmapped-data) for more information. |
| A loaded profile is valid for one file but wrong for another with the same extension. | Compare actual headers or field names and use the correct importer/schema. Create or save a distinct profile when the sources differ. See [What an import profile controls](#1-what-an-import-profile-controls), [Examples from the bundled profiles](#7-examples-from-the-bundled-profiles), and [Supported Formats](supported-formats.md) for more information. |
| A structured file imports statically but cannot be followed live. | Confirm that Structured JSON supplies a repeatable record array or Structured XML has a nonempty path to repeated elements; check the connected external source separately. See [Structured JSON](#structured-json-select-records-then-map-their-fields), [Structured XML](#structured-xml-elements-attributes-and-named-event-data), and [Live Following](live-following.md) for more information. |

## Related documentation

- [Importing Logs](importing-logs.md) — the complete UI workflow for selecting files, configuring mappings, verifying a preview, and saving a profile.
- [Supported Formats](supported-formats.md) — available importers, expected source layouts, sample/profile pairs, and live-format limitations.
- [Live Following](live-following.md) — source and profile requirements for growing files.
- [Saving and Restoring](saving-and-restoring.md) — the difference between a reusable import profile, a saved workspace, and a standalone investigation snapshot.
