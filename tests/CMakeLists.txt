qt_add_executable(ParserTests
    ParserTests.cpp
    ../src/parsing/JsonLineLogParser.h
    ../src/parsing/JsonLineLogParser.cpp
    ../src/domain/TelemetryEvent.h
)

target_link_libraries(ParserTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME ParserTests COMMAND ParserTests)

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
    ../src/exporting/TelemetryCsvExporter.h
    ../src/exporting/TelemetryCsvExporter.cpp
    ../src/domain/TelemetryEvent.h
)

target_link_libraries(CsvExporterTests
    PRIVATE
        Qt6::Test
        Qt6::Core
)

add_test(NAME CsvExporterTests COMMAND CsvExporterTests)