qt_add_executable(TraceScopeLiveLogGenerator
    main.cpp
    LiveLogRecord.h
    LiveLogScenarioStep.h
    LiveLogScenario.h
    LiveLogScenarioLoader.h
    LiveLogScenarioLoader.cpp
    ILogRecordRenderer.h
    renderers/JsonLinesRenderer.h
    renderers/JsonLinesRenderer.cpp
    LiveLogScenarioPlayer.h
    LiveLogScenarioPlayer.cpp
)

target_link_libraries(TraceScopeLiveLogGenerator
    PRIVATE
        Qt6::Core
)