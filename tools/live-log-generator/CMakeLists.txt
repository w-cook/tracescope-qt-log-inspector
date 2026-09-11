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
    renderers/DelimitedTextRenderer.h
    renderers/DelimitedTextRenderer.cpp
    LogRecordRendererFactory.h
    LogRecordRendererFactory.cpp
    renderers/KeyValueTextRenderer.h
    renderers/KeyValueTextRenderer.cpp
    renderers/StructuredJsonRenderer.h
    renderers/StructuredJsonRenderer.cpp
    renderers/StructuredXmlRenderer.h
    renderers/StructuredXmlRenderer.cpp
    renderers/WindowsEventXmlRenderer.h
    renderers/WindowsEventXmlRenderer.cpp
    renderers/SyslogRenderer.h
    renderers/SyslogRenderer.cpp
    renderers/RegexTextRenderer.h
    renderers/RegexTextRenderer.cpp
    renderers/IisW3cRenderer.h
    renderers/IisW3cRenderer.cpp
    renderers/AccessLogRenderer.h
    renderers/AccessLogRenderer.cpp

)

target_link_libraries(TraceScopeLiveLogGenerator
    PRIVATE
        Qt6::Core
)

qt_add_executable(
    TraceScopeLiveLogGeneratorLauncher

    launcher-main.cpp

    LiveLogGeneratorLauncherWindow.h
    LiveLogGeneratorLauncherWindow.cpp
)

target_link_libraries(
    TraceScopeLiveLogGeneratorLauncher
    PRIVATE
        Qt6::Widgets
)

add_dependencies(
    TraceScopeLiveLogGeneratorLauncher
    TraceScopeLiveLogGenerator
)

if(WIN32)
    # Locate windeployqt from the same Qt installation
    # used by the active CMake kit.
    get_target_property(
        _qmake_executable
        Qt6::qmake
        IMPORTED_LOCATION
    )

    get_filename_component(
        _qt_bin_dir
        "${_qmake_executable}"
        DIRECTORY
    )

    find_program(
        WINDEPLOYQT_EXECUTABLE
        windeployqt
        HINTS "${_qt_bin_dir}"
        REQUIRED
    )

    set(
        LIVE_LOG_GENERATOR_DEPLOY_DIR
        "${CMAKE_BINARY_DIR}/live-log-generator-portable"
    )

    # Explicit development deployment target.
    #
    # This is intentionally not part of the normal
    # ALL build because TraceScope development should
    # not pay the deployment cost on every compile.
    add_custom_target(
        TraceScopeLiveLogGeneratorPortable

        COMMAND
            "${CMAKE_COMMAND}"
            -E rm -rf
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}"

        COMMAND
            "${CMAKE_COMMAND}"
            -E make_directory
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}"

        COMMAND
            "${CMAKE_COMMAND}"
            -E copy_if_different
            "$<TARGET_FILE:TraceScopeLiveLogGenerator>"
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}"

        COMMAND
            "${CMAKE_COMMAND}"
            -E copy_if_different
            "$<TARGET_FILE:TraceScopeLiveLogGeneratorLauncher>"
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}"

        COMMAND
            "${WINDEPLOYQT_EXECUTABLE}"
            --compiler-runtime
            --no-translations
            --dir
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}"
            "${LIVE_LOG_GENERATOR_DEPLOY_DIR}/TraceScopeLiveLogGeneratorLauncher.exe"

        DEPENDS
            TraceScopeLiveLogGenerator
            TraceScopeLiveLogGeneratorLauncher

        COMMENT
            "Assembling portable TraceScope live-log generator"

        VERBATIM
    )
endif()