cmake_minimum_required(VERSION 3.21)

project(TraceScope VERSION 0.1.0 LANGUAGES CXX)

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
    src/exporting/TelemetryCsvExporter.h
    src/exporting/TelemetryCsvExporter.cpp
    src/analysis/EventCountBucket.h
    src/analysis/EventTimelineAnalyzer.h
    src/analysis/EventTimelineAnalyzer.cpp
)

target_link_libraries(TraceScope
    PRIVATE
        Qt6::Widgets
        Qt6::Charts
)

enable_testing()
add_subdirectory(tests)