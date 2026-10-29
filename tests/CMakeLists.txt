add_executable(wg_tests
    test_smoke.cpp
    test_dsp_primitives.cpp
    test_riaa.cpp
    test_riaa_stage.cpp
    test_null.cpp
    test_thd.cpp
    test_aliasing.cpp
    test_determinism.cpp
    test_edge_cases.cpp
    test_nonlinear_shapes.cpp
    test_oversampler.cpp
)

target_include_directories(wg_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wg_tests PRIVATE
    wg_dsp
    Catch2::Catch2WithMain
    pocketfft
)

# Own warning set: /WX but silence pocketfft + benign test noise
if(MSVC)
    target_compile_options(wg_tests PRIVATE /W4 /permissive- /WX /wd4127 /wd4324 /wd4189 /wd4244)
else()
    target_compile_options(wg_tests PRIVATE -Wall -Wextra -Wpedantic -Wno-error=unused-parameter)
endif()

include(Catch)
catch_discover_tests(wg_tests)

# --- Plugin-level tests (JUCE + APVTS) — SPEC M2 T2/T6/T7/T11/T15 ---
add_executable(wg_plugin_tests
    test_plugin.cpp
    ${WG_PLUGIN_CORE_SOURCES}
    ${WG_UI_SOURCES}
)

target_include_directories(wg_plugin_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(wg_plugin_tests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(wg_plugin_tests PRIVATE
    wg_dsp
    WarmVynilFactoryData
    WarmVynilUiData
    Catch2::Catch2WithMain
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(wg_plugin_tests PRIVATE /W4 /permissive- /WX /wd4127 /wd4324 /wd4189 /wd4244)
else()
    target_compile_options(wg_plugin_tests PRIVATE -Wall -Wextra -Wpedantic -Wno-error=unused-parameter)
endif()

catch_discover_tests(wg_plugin_tests)
