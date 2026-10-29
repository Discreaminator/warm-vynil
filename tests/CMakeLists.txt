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
