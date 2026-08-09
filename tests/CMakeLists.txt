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