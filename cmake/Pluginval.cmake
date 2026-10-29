# Optional local pluginval runner (SPEC §2). Downloads latest release when WG_RUN_PLUGINVAL=ON.
option(WG_RUN_PLUGINVAL "Add pluginval custom target when binary is found" OFF)

find_program(PLUGINVAL_EXECUTABLE pluginval
    HINTS
        "$ENV{LOCALAPPDATA}/pluginval"
        "$ENV{ProgramFiles}/pluginval"
        "${CMAKE_SOURCE_DIR}/tools/pluginval"
)

if(WG_RUN_PLUGINVAL AND PLUGINVAL_EXECUTABLE)
    add_custom_target(pluginval
        COMMAND "${PLUGINVAL_EXECUTABLE}"
            --strictness-level 5
            --validate-in-process
            --output-dir "${CMAKE_BINARY_DIR}/pluginval"
            "$<TARGET_FILE:Discreaminator_VST3>"
        DEPENDS Discreaminator_VST3
        COMMENT "Running pluginval strictness 5 on Discreaminator VST3"
        VERBATIM
    )
    message(STATUS "pluginval found: ${PLUGINVAL_EXECUTABLE}")
elseif(WG_RUN_PLUGINVAL)
    message(STATUS "pluginval not found on PATH; skip pluginval target")
endif()
