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

qt_add_executable(InvestigationAnalyticsAnalyzerTests
    InvestigationAnalyticsAnalyzerTests.cpp
    ../src/analysis/InvestigationValueFrequency.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.cpp

    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp
    ../src/analysis/InvestigationValueTrendBucket.h

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(InvestigationAnalyticsAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationAnalyticsAnalyzerTests
    COMMAND InvestigationAnalyticsAnalyzerTests
)

qt_add_executable(InvestigationCadenceAnalyzerTests
    InvestigationCadenceAnalyzerTests.cpp
    ../src/analysis/InvestigationCadence.h
    ../src/analysis/InvestigationCadenceAnalyzer.h
    ../src/analysis/InvestigationCadenceAnalyzer.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(InvestigationCadenceAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationCadenceAnalyzerTests
    COMMAND InvestigationCadenceAnalyzerTests
)

qt_add_executable(InvestigationBurstAnalyzerTests
    InvestigationBurstAnalyzerTests.cpp
    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp
    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(InvestigationBurstAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationBurstAnalyzerTests
    COMMAND InvestigationBurstAnalyzerTests
)

qt_add_executable(
    InvestigationSessionComparisonAnalyzerTests

    InvestigationSessionComparisonAnalyzerTests.cpp

    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp

    ../src/analysis/InvestigationSessionComparison.h
    ../src/analysis/InvestigationSessionComparisonAnalyzer.h
    ../src/analysis/InvestigationSessionComparisonAnalyzer.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationSessionComparisonAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationSessionComparisonAnalyzerTests
    COMMAND InvestigationSessionComparisonAnalyzerTests
)

qt_add_executable(
    InvestigationComparisonSnapshotBuilderTests

    InvestigationComparisonSnapshotBuilderTests.cpp

    ../src/workspace/InvestigationComparisonSnapshot.h
    ../src/workspace/InvestigationComparisonSnapshotBuilder.h
    ../src/workspace/InvestigationComparisonSnapshotBuilder.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp

    ../src/analysis/InvestigationValueFrequency.h
    ../src/analysis/InvestigationSessionComparison.h
    ../src/analysis/InvestigationSessionComparisonAnalyzer.h
    ../src/analysis/InvestigationSessionComparisonAnalyzer.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportResult.h

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationComparisonSnapshotBuilderTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationComparisonSnapshotBuilderTests
    COMMAND InvestigationComparisonSnapshotBuilderTests
)

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

qt_add_executable(
    InvestigationFindingsCsvExporterTests

    InvestigationFindingsCsvExporterTests.cpp

    ../src/exporting/InvestigationFindingExport.h
    ../src/exporting/InvestigationFindingsCsvExporter.h
    ../src/exporting/InvestigationFindingsCsvExporter.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/InvestigationRecordState.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationFindingsCsvExporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationFindingsCsvExporterTests
    COMMAND InvestigationFindingsCsvExporterTests
)

qt_add_executable(
    InvestigationReportSessionSnapshotBuilderTests

    InvestigationReportSessionSnapshotBuilderTests.cpp

    ../src/exporting/InvestigationReportSnapshot.h
    ../src/exporting/InvestigationReportSessionSnapshotBuilder.h
    ../src/exporting/InvestigationReportSessionSnapshotBuilder.cpp

    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp

    ../src/analysis/EventCountBucket.h
    ../src/analysis/EventTimelineAnalyzer.h
    ../src/analysis/EventTimelineAnalyzer.cpp

    ../src/analysis/InvestigationValueFrequency.h
    ../src/analysis/InvestigationValueTrendBucket.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.cpp

    ../src/analysis/InvestigationCadence.h
    ../src/analysis/InvestigationCadenceAnalyzer.h
    ../src/analysis/InvestigationCadenceAnalyzer.cpp

    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp

    ../src/analysis/TelemetryIssueGroup.h
    ../src/analysis/TelemetryIssueAnalyzer.h
    ../src/analysis/TelemetryIssueAnalyzer.cpp

    ../src/analysis/InvestigationSessionComparison.h

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/InvestigationRecordState.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
)

target_link_libraries(
    InvestigationReportSessionSnapshotBuilderTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationReportSessionSnapshotBuilderTests
    COMMAND InvestigationReportSessionSnapshotBuilderTests
)

qt_add_executable(
    InvestigationReportSnapshotBuilderTests

    InvestigationReportSnapshotBuilderTests.cpp

    # report builders
    ../src/exporting/InvestigationReportConfiguration.h
    ../src/exporting/InvestigationReportSnapshot.h
    ../src/exporting/InvestigationReportSessionSnapshotBuilder.h
    ../src/exporting/InvestigationReportSessionSnapshotBuilder.cpp
    ../src/exporting/InvestigationReportSnapshotBuilder.h
    ../src/exporting/InvestigationReportSnapshotBuilder.cpp

    # analysis
    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp
    ../src/analysis/EventCountBucket.h
    ../src/analysis/EventTimelineAnalyzer.h
    ../src/analysis/EventTimelineAnalyzer.cpp
    ../src/analysis/InvestigationValueFrequency.h
    ../src/analysis/InvestigationValueTrendBucket.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.cpp
    ../src/analysis/InvestigationCadence.h
    ../src/analysis/InvestigationCadenceAnalyzer.h
    ../src/analysis/InvestigationCadenceAnalyzer.cpp
    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp
    ../src/analysis/TelemetryIssueGroup.h
    ../src/analysis/TelemetryIssueAnalyzer.h
    ../src/analysis/TelemetryIssueAnalyzer.cpp
    ../src/analysis/InvestigationSessionComparison.h

    # workspace
    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp
    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp
    ../src/workspace/InvestigationComparisonSnapshot.h

    # controller/models
    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp
    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    # domain
    ../src/domain/InvestigationRecord.h
    ../src/domain/InvestigationRecordState.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h

    # importing
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
)

target_link_libraries(
    InvestigationReportSnapshotBuilderTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationReportSnapshotBuilderTests
    COMMAND InvestigationReportSnapshotBuilderTests
)

qt_add_executable(
    InvestigationRecordExportFormatterTests

    InvestigationRecordExportFormatterTests.cpp

    ../src/exporting/InvestigationRecordExportFormatter.h
    ../src/exporting/InvestigationRecordExportFormatter.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationRecordExportFormatterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationRecordExportFormatterTests
    COMMAND InvestigationRecordExportFormatterTests
)

qt_add_executable(EventTimelineAnalyzerTests
    EventTimelineAnalyzerTests.cpp
    ../src/analysis/EventCountBucket.h
    ../src/analysis/EventTimelineAnalyzer.h
    ../src/analysis/EventTimelineAnalyzer.cpp
    ../src/domain/TelemetryEvent.h

    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp
)

target_link_libraries(EventTimelineAnalyzerTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME EventTimelineAnalyzerTests COMMAND EventTimelineAnalyzerTests)

qt_add_executable(AnalysisTimeBucketRangeTests
    AnalysisTimeBucketRangeTests.cpp
    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp
)

target_link_libraries(AnalysisTimeBucketRangeTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME AnalysisTimeBucketRangeTests
    COMMAND AnalysisTimeBucketRangeTests
)

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

    ../src/importing/BuiltInImportProfilePresets.h
    ../src/importing/BuiltInImportProfilePresets.cpp

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

    ../src/importing/SyslogImporter.h
    ../src/importing/SyslogImporter.cpp

    ../src/importing/IisW3cImporter.h
    ../src/importing/IisW3cImporter.cpp

    ../src/importing/ILogImporter.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp

    ../src/importing/StructuredJsonImportConfig.h
    ../src/importing/StructuredJsonImporter.h
    ../src/importing/StructuredJsonImporter.cpp

    ../src/importing/XmlImporter.h
    ../src/importing/XmlImporter.cpp

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

    ../src/importing/BuiltInImportProfilePresets.h
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

qt_add_executable(
    IisW3cImporterTests

    IisW3cImporterTests.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h

    ../src/importing/IisW3cImporter.h
    ../src/importing/IisW3cImporter.cpp

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
    IisW3cImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME IisW3cImporterTests
    COMMAND IisW3cImporterTests
)

qt_add_executable(
    XmlImporterTests

    XmlImporterTests.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ILogImporter.h

    ../src/importing/XmlImporter.h
    ../src/importing/XmlImporter.cpp

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
    XmlImporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME XmlImporterTests
    COMMAND XmlImporterTests
)

qt_add_executable(
    InvestigationStateStoreTests

    InvestigationStateStoreTests.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h
)

target_link_libraries(
    InvestigationStateStoreTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationStateStoreTests
    COMMAND InvestigationStateStoreTests
)

qt_add_executable(
    InvestigationSessionTests

    InvestigationSessionTests.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportResult.h

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h

    ../src/workspace/InvestigationSessionPersistence.h
    ../src/workspace/InvestigationSessionPersistence.cpp
)

target_link_libraries(
    InvestigationSessionTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationSessionTests
    COMMAND InvestigationSessionTests
)

qt_add_executable(
    InvestigationFindingExportSnapshotBuilderTests

    InvestigationFindingExportSnapshotBuilderTests.cpp

    ../src/exporting/InvestigationFindingExport.h
    ../src/exporting/InvestigationFindingExportSnapshotBuilder.h
    ../src/exporting/InvestigationFindingExportSnapshotBuilder.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportResult.h

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationFindingExportSnapshotBuilderTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationFindingExportSnapshotBuilderTests
    COMMAND InvestigationFindingExportSnapshotBuilderTests
)

qt_add_executable(
    InvestigationWorkspaceTests

    InvestigationWorkspaceTests.cpp

    ../src/workspace/InvestigationWorkspace.h
    ../src/workspace/InvestigationWorkspace.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationWorkspaceTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationWorkspaceTests
    COMMAND InvestigationWorkspaceTests
)

qt_add_executable(
    WorkspaceDocumentHostTests
    WorkspaceDocumentHostTests.cpp

    ../src/ui/workspace/DetachedWorkspaceDocumentWindow.h
    ../src/ui/workspace/DetachedWorkspaceDocumentWindow.cpp

    ../src/ui/workspace/WorkspaceDocument.h
    ../src/ui/workspace/WorkspaceDocument.cpp

    ../src/ui/workspace/WorkspaceDocumentHost.h
    ../src/ui/workspace/WorkspaceDocumentHost.cpp

    ../src/ui/workspace/WorkspaceTabBar.h
    ../src/ui/workspace/WorkspaceTabBar.cpp
)

target_link_libraries(
    WorkspaceDocumentHostTests
    PRIVATE
        Qt6::Test
        Qt6::Widgets
)

add_test(
    NAME WorkspaceDocumentHostTests
    COMMAND WorkspaceDocumentHostTests
)

qt_add_executable(
    RecentItemsStoreTests

    RecentItemsStoreTests.cpp

    ../src/preferences/RecentItemsStore.h
    ../src/preferences/RecentItemsStore.cpp
)

target_link_libraries(
    RecentItemsStoreTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME RecentItemsStoreTests
    COMMAND RecentItemsStoreTests
)

qt_add_executable(
    FilterPresetStoreTests

    FilterPresetStoreTests.cpp

    ../src/filtering/InvestigationFilterPreset.h

    ../src/preferences/FilterPresetStore.h
    ../src/preferences/FilterPresetStore.cpp
)

target_link_libraries(
    FilterPresetStoreTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME FilterPresetStoreTests
    COMMAND FilterPresetStoreTests
)

qt_add_executable(
    InvestigationComparisonDialogTests

    InvestigationComparisonDialogTests.cpp

    ../src/ui/InvestigationComparisonDialog.h
    ../src/ui/InvestigationComparisonDialog.cpp

    ../src/analysis/InvestigationCadence.h
    ../src/analysis/InvestigationCadenceAnalyzer.h
    ../src/analysis/InvestigationCadenceAnalyzer.cpp
    ../src/analysis/BurstDetectionSettings.h

    ../src/workspace/InvestigationWorkspace.h
    ../src/workspace/InvestigationWorkspace.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/domain/InvestigationRecordState.h

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp
    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/importing/ImportProfile.h
    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportDiagnostic.cpp
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
    ../src/domain/RecordSourceMetadata.h
)

target_link_libraries(
    InvestigationComparisonDialogTests
    PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
)

add_test(
    NAME InvestigationComparisonDialogTests
    COMMAND InvestigationComparisonDialogTests
)

qt_add_executable(WorkspaceSerializationTests
    WorkspaceSerializationTests.cpp
    ../src/workspace/WorkspacePersistenceState.h
    ../src/workspace/WorkspaceSerialization.h
    ../src/workspace/WorkspaceSerialization.cpp
    ../src/importing/ImportProfile.h
    ../src/importing/ImportProfileSerialization.h
    ../src/importing/ImportProfileSerialization.cpp
    ../src/importing/CanonicalFieldMappings.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp

    ../src/workspace/InvestigationComparisonPersistenceSerialization.h
    ../src/workspace/InvestigationComparisonPersistenceSerialization.cpp

    ../src/workspace/InvestigationPresentationState.h
    ../src/workspace/InvestigationPresentationStateSerialization.h
    ../src/workspace/InvestigationPresentationStateSerialization.cpp

    ../src/analysis/BurstDetectionSettings.h

    ../src/workspace/WorkspaceDocumentLayoutState.h
    ../src/workspace/WorkspaceDocumentLayoutSerialization.h
    ../src/workspace/WorkspaceDocumentLayoutSerialization.cpp
)

target_link_libraries(
    WorkspaceSerializationTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME WorkspaceSerializationTests
    COMMAND WorkspaceSerializationTests
)

qt_add_executable(
    InvestigationComparisonPersistenceTests
    InvestigationComparisonPersistenceTests.cpp

    ../src/workspace/InvestigationComparisonPersistence.h
    ../src/workspace/InvestigationComparisonPersistence.cpp

    ../src/workspace/InvestigationComparisonPersistenceSerialization.h
    ../src/workspace/InvestigationComparisonPersistenceSerialization.cpp

    ../src/workspace/InvestigationComparisonSnapshot.h
    ../src/workspace/WorkspacePersistenceState.h

    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp
)

target_link_libraries(
    InvestigationComparisonPersistenceTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(
    NAME InvestigationComparisonPersistenceTests
    COMMAND InvestigationComparisonPersistenceTests
)

qt_add_executable(
    InvestigationPresentationStateTests

    InvestigationPresentationStateTests.cpp

    ../src/workspace/InvestigationPresentationState.h

    ../src/ui/investigation/InvestigationEventPanel.h
    ../src/ui/investigation/InvestigationEventPanel.cpp

    ../src/ui/investigation/InvestigationEventDetailPanel.h
    ../src/ui/investigation/InvestigationEventDetailPanel.cpp

    ../src/workspace/InvestigationSession.h
    ../src/workspace/InvestigationSession.cpp

    ../src/workspace/InvestigationStateStore.h
    ../src/workspace/InvestigationStateStore.cpp

    ../src/controllers/InvestigationController.h
    ../src/controllers/InvestigationController.cpp

    ../src/models/InvestigationTableModel.h
    ../src/models/InvestigationTableModel.cpp

    ../src/models/InvestigationFilterProxyModel.h
    ../src/models/InvestigationFilterProxyModel.cpp

    ../src/importing/CanonicalFieldMappings.h
    ../src/importing/ImportDiagnostic.h
    ../src/importing/ImportProfile.h
    ../src/importing/ImportResult.h
    ../src/importing/ImportResult.cpp

    ../src/domain/InvestigationRecord.h
    ../src/domain/InvestigationRecordState.h
    ../src/domain/RecordSourceMetadata.h
    ../src/domain/RecordSeverity.h
    ../src/domain/RecordSeverity.cpp

    ../src/ui/investigation/InvestigationTimelinePanel.h
    ../src/ui/investigation/InvestigationTimelinePanel.cpp

    ../src/analysis/EventTimelineAnalyzer.h
    ../src/analysis/EventTimelineAnalyzer.cpp

    ../src/analysis/InvestigationAnalyticsAnalyzer.h
    ../src/analysis/InvestigationAnalyticsAnalyzer.cpp

    ../src/analysis/AnalysisTimeBucketRange.h
    ../src/analysis/AnalysisTimeBucketRange.cpp

    ../src/ui/investigation/InvestigationIssueSummaryPanel.h
    ../src/ui/investigation/InvestigationIssueSummaryPanel.cpp

    ../src/ui/investigation/InvestigationFindingsPanel.h
    ../src/ui/investigation/InvestigationFindingsPanel.cpp

    ../src/analysis/TelemetryIssueGroup.h
    ../src/analysis/TelemetryIssueAnalyzer.h
    ../src/analysis/TelemetryIssueAnalyzer.cpp

    ../src/ui/investigation/InvestigationAnalyticsPanel.h
    ../src/ui/investigation/InvestigationAnalyticsPanel.cpp

    ../src/ui/investigation/InvestigationReviewPanel.h
    ../src/ui/investigation/InvestigationReviewPanel.cpp

    ../src/analysis/InvestigationBurst.h
    ../src/analysis/InvestigationBurstAnalyzer.h
    ../src/analysis/InvestigationBurstAnalyzer.cpp

    ../src/analysis/InvestigationCadence.h
    ../src/analysis/InvestigationCadenceAnalyzer.h
    ../src/analysis/InvestigationCadenceAnalyzer.cpp

    ../src/analysis/BurstDetectionSettings.h
    ../src/analysis/InvestigationValueFrequency.h

    ../src/ui/workspace/WorkspaceDocument.h
    ../src/ui/workspace/WorkspaceDocument.cpp

    ../src/exporting/InvestigationRecordExportFormatter.h
    ../src/exporting/InvestigationRecordExportFormatter.cpp

    ../src/exporting/InvestigationFindingExport.h

    ../src/exporting/InvestigationFindingExportSnapshotBuilder.h
    ../src/exporting/InvestigationFindingExportSnapshotBuilder.cpp

    ../src/exporting/InvestigationFindingsCsvExporter.h
    ../src/exporting/InvestigationFindingsCsvExporter.cpp

    ../src/ui/workspace/InvestigationSessionView.h
    ../src/ui/workspace/InvestigationSessionView.cpp

    ../src/ui/investigation/InvestigationSessionSummaryPanel.h
    ../src/ui/investigation/InvestigationSessionSummaryPanel.cpp

    ../src/ui/investigation/InvestigationFilterPanel.h
    ../src/ui/investigation/InvestigationFilterPanel.cpp

    ../src/ui/MultiSelectFilterComboBox.h
    ../src/ui/MultiSelectFilterComboBox.cpp

    ../src/ui/CustomFieldFilterEditor.h
    ../src/ui/CustomFieldFilterEditor.cpp

    ../src/filtering/InvestigationFilterPreset.h

    ../src/preferences/FilterPresetStore.h
    ../src/preferences/FilterPresetStore.cpp

    ../src/exporting/InvestigationCsvExporter.h
    ../src/exporting/InvestigationCsvExporter.cpp
)

target_link_libraries(
    InvestigationPresentationStateTests
    PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
)

add_test(
    NAME InvestigationPresentationStateTests
    COMMAND InvestigationPresentationStateTests
)

set_tests_properties(
    InvestigationPresentationStateTests
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=minimal"
)