cmake_minimum_required(VERSION 3.21)

project(TraceScope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

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
)

target_link_libraries(TraceScope
    PRIVATE
        Qt6::Widgets
)

enable_testing()
add_subdirectory(tests)