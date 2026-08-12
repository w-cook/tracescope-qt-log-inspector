qt_add_executable(ParserTests
    ParserTests.cpp
    ../src/parsing/JsonLineLogParser.h
    ../src/parsing/JsonLineLogParser.cpp
    ../src/compatibility/TelemetryEventAdapter.h
    ../src/compatibility/TelemetryEventAdapter.cpp
    ../src/domain/TelemetryEvent.h
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
    ../src/importing/ILogImporter.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/importing/JsonLinesImportConfig.h
    ../src/importing/JsonLinesImporter.h
    ../src/importing/JsonLinesImporter.cpp
    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp
)

target_link_libraries(ParserTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME ParserTests COMMAND ParserTests)

qt_add_executable(RecordSeverityTests
    RecordSeverityTests.cpp
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
)

target_link_libraries(RecordSeverityTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME RecordSeverityTests COMMAND RecordSeverityTests)

qt_add_executable(InvestigationRecordTests
    InvestigationRecordTests.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(InvestigationRecordTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationRecordTests
    COMMAND InvestigationRecordTests
)

qt_add_executable(ImportResultTests
    ImportResultTests.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(ImportResultTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportResultTests
    COMMAND ImportResultTests
)

qt_add_executable(FilterTests
    FilterTests.cpp
    ../src/filtering/TelemetryFilterCriteria.h
    ../src/filtering/TelemetryEventFilter.h
    ../src/filtering/TelemetryEventFilter.cpp
    ../src/domain/TelemetryEvent.h
)

target_link_libraries(FilterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME FilterTests COMMAND FilterTests)

qt_add_executable(IssueAnalyzerTests
    IssueAnalyzerTests.cpp
    ../src/analysis/TelemetryIssueGroup.h
    ../src/analysis/TelemetryIssueAnalyzer.h
    ../src/analysis/TelemetryIssueAnalyzer.cpp
    ../src/domain/TelemetryEvent.h
)

target_link_libraries(IssueAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME IssueAnalyzerTests COMMAND IssueAnalyzerTests)

qt_add_executable(CsvExporterTests
    CsvExporterTests.cpp
    ../src/exporting/InvestigationCsvExporter.h
    ../src/exporting/InvestigationCsvExporter.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(CsvExporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME CsvExporterTests COMMAND CsvExporterTests)

qt_add_executable(EventTimelineAnalyzerTests
    EventTimelineAnalyzerTests.cpp
    ../src/analysis/EventCountBucket.h
    ../src/analysis/EventTimelineAnalyzer.h
    ../src/analysis/EventTimelineAnalyzer.cpp
    ../src/domain/TelemetryEvent.h
)

target_link_libraries(EventTimelineAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME EventTimelineAnalyzerTests COMMAND EventTimelineAnalyzerTests)

qt_add_executable(ImporterRegistryTests
    ImporterRegistryTests.cpp
    ../src/importing/ILogImporter.h
    ../src/importing/ImporterRegistry.h
    ../src/importing/ImporterRegistry.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(ImporterRegistryTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImporterRegistryTests
    COMMAND ImporterRegistryTests
)

qt_add_executable(JsonLinesImporterTests
    JsonLinesImporterTests.cpp
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h
    ../src/importing/JsonLinesImporter.h
    ../src/importing/JsonLinesImporter.cpp
    ../src/importing/JsonLinesImportConfig.h
    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(JsonLinesImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME JsonLinesImporterTests
    COMMAND JsonLinesImporterTests
)

qt_add_executable(DelimitedTextImporterTests
    DelimitedTextImporterTests.cpp
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h
    ../src/importing/DelimitedTextImporter.h
    ../src/importing/DelimitedTextImporter.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(DelimitedTextImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME DelimitedTextImporterTests
    COMMAND DelimitedTextImporterTests
)

qt_add_executable(InvestigationTableModelTests
    InvestigationTableModelTests.cpp
    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(InvestigationTableModelTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationTableModelTests
    COMMAND InvestigationTableModelTests
)

qt_add_executable(InvestigationFilterProxyModelTests
    InvestigationFilterProxyModelTests.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp
    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationFilterProxyModelTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationFilterProxyModelTests
    COMMAND InvestigationFilterProxyModelTests
)

qt_add_executable(InvestigationControllerTests
    InvestigationControllerTests.cpp
    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp
    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationControllerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationControllerTests
    COMMAND InvestigationControllerTests
)

qt_add_executable(ImportProfileTests
    ImportProfileTests.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/domain/RecordSeverity.h
)

target_link_libraries(
    ImportProfileTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportProfileTests
    COMMAND ImportProfileTests
)

qt_add_executable(ImportProfileValidatorTests
    ImportProfileValidatorTests.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportProfileValidation.h
    ../src/importing/ImportProfileValidation.cpp
    ../src/importing/ImportProfileValidator.h
    ../src/importing/ImportProfileValidator.cpp
    ../src/domain/RecordSeverity.h
)

target_link_libraries(
    ImportProfileValidatorTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportProfileValidatorTests
    COMMAND ImportProfileValidatorTests
)

qt_add_executable(
    ImportProfileSerializationTests

    ImportProfileSerializationTests.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h

    ../src/importing/ImportProfileSerialization.h
    ../src/importing/ImportProfileSerialization.cpp

    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
)

target_link_libraries(
    ImportProfileSerializationTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportProfileSerializationTests
    COMMAND ImportProfileSerializationTests
)

qt_add_executable(ImportPreviewServiceTests
    ImportPreviewServiceTests.cpp

    ../src/importing/ImportPreviewResult.h
    ../src/importing/ImportPreviewService.h
    ../src/importing/ImportPreviewService.cpp

    ../src/importing/BuiltInImporterRegistry.h
    ../src/importing/BuiltInImporterRegistry.cpp
    ../src/importing/ImporterRegistry.h
    ../src/importing/ImporterRegistry.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportProfileValidation.h
    ../src/importing/ImportProfileValidation.cpp
    ../src/importing/ImportProfileValidator.h
    ../src/importing/ImportProfileValidator.cpp

    ../src/importing/JsonLinesImporter.h
    ../src/importing/JsonLinesImporter.cpp
    ../src/importing/JsonLinesImportConfig.h
    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp

    ../src/importing/DelimitedTextImporter.h
    ../src/importing/DelimitedTextImporter.cpp

    ../src/importing/RegexTextImporter.h
    ../src/importing/RegexTextImporter.cpp

    ../src/importing/KeyValueTextImporter.h
    ../src/importing/KeyValueTextImporter.cpp

    ../src/importing/ILogImporter.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp

    ../src/importing/StructuredJsonImportConfig.h
    ../src/importing/StructuredJsonImporter.h
    ../src/importing/StructuredJsonImporter.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(
    ImportPreviewServiceTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportPreviewServiceTests
    COMMAND ImportPreviewServiceTests
)

qt_add_executable(
    ImportFormatSuggestionServiceTests

    ImportFormatSuggestionServiceTests.cpp

    ../src/importing/ImportFormatSuggestion.h
    ../src/importing/ImportFormatSuggestionService.h
    ../src/importing/ImportFormatSuggestionService.cpp
)

target_link_libraries(
    ImportFormatSuggestionServiceTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME ImportFormatSuggestionServiceTests
    COMMAND ImportFormatSuggestionServiceTests
)

qt_add_executable(
    StructuredJsonImporterTests

    StructuredJsonImporterTests.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h
    ../src/importing/StructuredJsonImportConfig.h
    ../src/importing/StructuredJsonImporter.h
    ../src/importing/StructuredJsonImporter.cpp

    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp

    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(
    StructuredJsonImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME StructuredJsonImporterTests
    COMMAND StructuredJsonImporterTests
)

qt_add_executable(RegexTextImporterTests
    RegexTextImporterTests.cpp
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h
    ../src/importing/RegexTextImporter.h
    ../src/importing/RegexTextImporter.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(RegexTextImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME RegexTextImporterTests
    COMMAND RegexTextImporterTests
)

qt_add_executable(
    KeyValueTextImporterTests

    KeyValueTextImporterTests.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h

    ../src/importing/KeyValueTextImporter.h
    ../src/importing/KeyValueTextImporter.cpp

    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp

    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(
    KeyValueTextImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME KeyValueTextImporterTests
    COMMAND KeyValueTextImporterTests
)

qt_add_executable(
    SyslogImporterTests

    SyslogImporterTests.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h

    ../src/importing/SyslogImporter.h
    ../src/importing/SyslogImporter.cpp

    ../src/importing/JsonObjectRecordMapper.h
    ../src/importing/JsonObjectRecordMapper.cpp

    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordTimestamp.h
    ../src/domain/RecordTimestamp.cpp
    ../src/domain/RecordIdentity.h
    ../src/domain/RecordIdentity.cpp
)

target_link_libraries(
    SyslogImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME SyslogImporterTests
    COMMAND SyslogImporterTests
)