cmake_minimum_required(VERSION 3.25)

project(Discreaminator
    VERSION 0.1.0
    DESCRIPTION "Discreaminator — vinyl cutting / tube RIAA character (Warm Groove SPEC)"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Static CRT on MSVC (SPEC §11.1) — no redistributable dependency
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(WG_BUILD_TESTS "Build Catch2 unit tests" ON)
option(WG_COPY_PLUGIN_AFTER_BUILD "Copy plugins to system folders after build" OFF)
option(WG_BUILD_STANDALONE "Build Standalone app (off for M0; user prefers VST3+CLAP)" OFF)

include(cmake/PluginConfig.cmake)
include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

# --- JUCE ---
add_subdirectory(extern/JUCE EXCLUDE_FROM_ALL)

# --- clap-juce-extensions ---
add_subdirectory(extern/clap-juce-extensions EXCLUDE_FROM_ALL)

# --- DSP library (no JUCE) ---
add_library(wg_dsp STATIC
    src/dsp/Engine.cpp
)
target_include_directories(wg_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
wg_set_warnings(wg_dsp)

# --- Plugin formats ---
set(WG_FORMATS VST3)
if(APPLE AND WG_BUILD_AU)
    list(APPEND WG_FORMATS AU)
endif()
if(WG_BUILD_STANDALONE)
    list(APPEND WG_FORMATS Standalone)
endif()

juce_add_plugin(Discreaminator
    COMPANY_NAME "${WG_COMPANY_NAME}"
    COMPANY_WEBSITE "https://github.com/vynilaudio/discreaminator"
    COMPANY_EMAIL "dev@vynilaudio.local"
    BUNDLE_ID "${WG_BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE "${WG_PLUGIN_MANUFACTURER_CODE}"
    PLUGIN_CODE "${WG_PLUGIN_CODE}"
    FORMATS ${WG_FORMATS}
    PRODUCT_NAME "${WG_PLUGIN_NAME}"
    PLUGIN_NAME "${WG_PLUGIN_NAME}"
    DESCRIPTION "${WG_PLUGIN_DESCRIPTION}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${WG_COPY_PLUGIN_AFTER_BUILD}
    VST3_CATEGORIES ${WG_VST3_CATEGORIES}
    AU_MAIN_TYPE ${WG_AU_MAIN_TYPE}
    HARDENED_RUNTIME_ENABLED TRUE
)

clap_juce_extensions_plugin(
    TARGET Discreaminator
    CLAP_ID "${WG_CLAP_ID}"
    CLAP_FEATURES audio-effect distortion stereo mono
)

target_sources(Discreaminator PRIVATE
    src/plugin/PluginProcessor.cpp
    src/ui/PluginEditor.cpp
)

target_include_directories(Discreaminator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Discreaminator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(Discreaminator
    PRIVATE
        wg_dsp
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

wg_set_warnings(Discreaminator)

# --- Offline render tool ---
add_executable(wg_render
    tools/render/main.cpp
)
target_link_libraries(wg_render PRIVATE wg_dsp dr_wav)
wg_set_warnings(wg_render)

# --- Tests ---
if(WG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(cmake/Pluginval.cmake)

message(STATUS "Discreaminator ${WG_PLUGIN_VERSION}")
message(STATUS "  Company: ${WG_COMPANY_NAME}  codes: ${WG_PLUGIN_MANUFACTURER_CODE}/${WG_PLUGIN_CODE}")
message(STATUS "  Formats: ${WG_FORMATS} + CLAP")
message(STATUS "  Bundle/CLAP ID: ${WG_BUNDLE_ID}")
