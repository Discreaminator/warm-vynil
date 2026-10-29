cmake_minimum_required(VERSION 3.25)

project(WarmVynil
    VERSION 0.1.0
    DESCRIPTION "WARM VYNIL — vinyl cutting / tube RIAA character (Warm Groove SPEC)"
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
    src/dsp/MacroMap.cpp
    src/dsp/os/Oversampler.cpp
    src/dsp/stages/MonoBass.cpp
    src/dsp/stages/RiaaDesign.cpp
    src/dsp/stages/Riaa.cpp
    src/dsp/stages/TubeLut.cpp
    src/dsp/stages/TubeStage.cpp
    src/dsp/stages/AutoGain.cpp
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

# CMake target WarmVynil (no spaces). Host/UI name comes from PRODUCT_NAME / PluginConfig.
juce_add_plugin(WarmVynil
    COMPANY_NAME "${WG_COMPANY_NAME}"
    COMPANY_COPYRIGHT "${WG_COPYRIGHT}"
    COMPANY_WEBSITE "https://github.com/discreaminator/warm-vynil"
    COMPANY_EMAIL "dev@discreaminator.local"
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
    TARGET WarmVynil
    CLAP_ID "${WG_CLAP_ID}"
    CLAP_FEATURES audio-effect distortion stereo mono
)

# Factory preset XML embedded for documentation / future BinaryData loaders (programs are coded in PresetManager).
juce_add_binary_data(WarmVynilFactoryData
    HEADER_NAME FactoryPresets.h
    NAMESPACE FactoryPresets
    SOURCES
        presets/factory/Default.xml
        presets/factory/LightWarm.xml
        presets/factory/ClassicVinyl.xml
        presets/factory/DeepTube.xml
)

set(WG_PLUGIN_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/plugin/ParameterLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/ParamBridge.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/StateSerializer.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PresetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginProcessor.cpp
)

target_sources(WarmVynil PRIVATE
    ${WG_PLUGIN_CORE_SOURCES}
    src/plugin/PluginEntry.cpp
)

target_include_directories(WarmVynil PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(WarmVynil
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(WarmVynil
    PRIVATE
        wg_dsp
        WarmVynilFactoryData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

wg_set_warnings(WarmVynil)

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

message(STATUS "${WG_PLUGIN_NAME} ${WG_PLUGIN_VERSION}")
message(STATUS "  Company: ${WG_COMPANY_NAME}  codes: ${WG_PLUGIN_MANUFACTURER_CODE}/${WG_PLUGIN_CODE}")
message(STATUS "  Formats: ${WG_FORMATS} + CLAP")
message(STATUS "  Bundle/CLAP ID: ${WG_BUNDLE_ID}")
message(STATUS "  ${WG_COPYRIGHT}")
