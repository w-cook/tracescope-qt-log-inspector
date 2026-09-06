qt_add_executable(TraceScopeLiveLogGenerator
    main.cpp
    LiveLogRecord.h
    LiveLogScenarioStep.h
    LiveLogScenario.h
    LiveLogScenarioLoader.h
    LiveLogScenarioLoader.cpp
)

target_link_libraries(TraceScopeLiveLogGenerator
    PRIVATE
        Qt6::Core
)