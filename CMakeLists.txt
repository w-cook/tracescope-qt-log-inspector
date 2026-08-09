cmake_minimum_required(VERSION 3.21)

project(TraceScope VERSION 0.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test Charts)

qt_standard_project_setup()

qt_add_executable(TraceScope
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/domain/TelemetryEvent.h
    src/parsing/JsonLineLogParser.h
    src/parsing/JsonLineLogParser.cpp
    src/filtering/TelemetryFilterCriteria.h
    src/filtering/TelemetryEventFilter.h
    src/filtering/TelemetryEventFilter.cpp
    src/analysis/TelemetryIssueGroup.h
    src/analysis/TelemetryIssueAnalyzer.h
    src/analysis/TelemetryIssueAnalyzer.cpp
    src/exporting/InvestigationCsvExporter.h
    src/exporting/InvestigationCsvExporter.cpp
    src/analysis/EventCountBucket.h
    src/analysis/EventTimelineAnalyzer.h
    src/analysis/EventTimelineAnalyzer.cpp
    src/domain/RecordSeverity.h
    src/domain/RecordSeverity.cpp
    src/domain/RecordSourceMetadata.h
    src/domain/RecordTimestamp.h
    src/domain/RecordTimestamp.cpp
    src/domain/RecordIdentity.h
    src/domain/RecordIdentity.cpp
    src/domain/InvestigationRecord.h
    src/importing/ImportDiagnostic.h
    src/importing/ImportDiagnostic.cpp
    src/importing/ImportResult.h
    src/importing/ImportResult.cpp
    src/importing/ILogImporter.h
    src/importing/ImporterRegistry.h
    src/importing/ImporterRegistry.cpp
    src/importing/JsonLinesImporter.h
    src/importing/JsonLinesImporter.cpp
    src/importing/JsonLinesImportConfig.h
    src/models/InvestigationTableModel.h
    src/models/InvestigationTableModel.cpp
    src/models/InvestigationFilterProxyModel.h
    src/models/InvestigationFilterProxyModel.cpp
    src/compatibility/TelemetryEventAdapter.h
    src/compatibility/TelemetryEventAdapter.cpp
    src/controllers/InvestigationController.h
    src/controllers/InvestigationController.cpp
    src/exporting/InvestigationCsvExporter.h
    src/exporting/InvestigationCsvExporter.h
    src/importing/CanonicalFieldMappings.h
    src/importing/ImportProfile.h
    src/importing/ImportProfileValidation.h
    src/importing/ImportProfileValidation.cpp
    src/importing/ImportProfileValidator.h
    src/importing/ImportProfileValidator.cpp
    src/importing/ImportProfileSerialization.h
    src/importing/ImportProfileSerialization.cpp
    src/importing/ImportPreviewResult.h
    src/importing/ImportPreviewService.h
    src/importing/ImportPreviewService.cpp
)

target_link_libraries(TraceScope
    PRIVATE
        Qt6::Widgets
        Qt6::Charts
)

enable_testing()
add_subdirectory(tests)